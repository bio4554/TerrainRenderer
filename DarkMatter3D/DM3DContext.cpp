#include "pch.h"
#include "DM3DContext.h"

#include <stdexcept>

#include "D3D12MemAlloc.h"
#include "DDSTextureLoader12.h"
#include "DM3DEnumTranslator.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_sdl3.h"
#include "DM3DInternalUtilities.h"

namespace dm3d
{
	Context::Context(const Extent2D windowExtent, SDL_Window* windowHandle, const bool useDebug)
	{
		_windowHandle = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(windowHandle), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));;
		_windowExtent = windowExtent;

		if (useDebug)
		{
			ComPtr<ID3D12Debug> debugInterface;
			check_result(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
			debugInterface->EnableDebugLayer();
		}

		_adapter = get_adapter(useDebug);

		if (_adapter == nullptr)
			throw std::runtime_error("failed to find valid GPU");

		init_device_and_resources(useDebug);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui::StyleColorsDark();
		ImGui_ImplSDL3_InitForD3D(windowHandle);

		ImGui_ImplDX12_InitInfo dx12initInfo = {};
		dx12initInfo.Device = _device.Get();
		dx12initInfo.CommandQueue = _directCommandQueue.Get();
		dx12initInfo.NumFramesInFlight = _numFrames;
		dx12initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		dx12initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
		dx12initInfo.SrvDescriptorHeap = _imguiDescAllocator->get_heap();
		dx12initInfo.SrvDescriptorAllocFn = [&](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
			{
				auto desc = _imguiDescAllocator->allocate();
				*out_cpu_handle = desc.cpuHandle;
				*out_gpu_handle = desc.gpuHandle;
			};

		dx12initInfo.SrvDescriptorFreeFn = [&](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
			{
				_imguiDescAllocator->free({ .cpuHandle = cpu_handle, .gpuHandle = gpu_handle, .idx = 0 });
			};

		ImGui_ImplDX12_Init(&dx12initInfo);

		ImGui_ImplSDL3_NewFrame();
		ImGui_ImplDX12_NewFrame();
		ImGui::NewFrame();

		_psoCache = std::make_unique<PipelineStateObjectCache>(_device.Get());
	}

	Context::~Context()
	{
		for (int i = 0; i < _numFrames; i++)
		{
			_frameFenceValues[i] = signal_fence(_directCommandQueue.Get(), _fenceDirect.Get(), _fenceValue);

			wait_for_fence_value(_fenceDirect.Get(), _frameFenceValues[i], _fenceEvent);
		}

		while (_releaseOnDelete.size_approx() != 0)
		{
			std::shared_ptr<Resource> temp;
			_releaseOnDelete.try_dequeue(temp);
		}

		for (uint32_t i = 0; i < _numFrames; i++)
		{
			reset_allocators(i);
			_perFrameData[i].allocatorCache = std::vector<ComPtr<ID3D12CommandAllocator>>();
			_perFrameData[i].resourceLocks = std::queue<std::shared_ptr<Resource>>();
		}

		_psoCache.reset();

		_allocator->Release();
		_dxgiFactory->Release();
	}

	void Context::rebuild_swapchain(const Extent2D newExtent)
	{
		_windowExtent = newExtent;

		if (!_swapChain)
		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = _numFrames;
			swapChainDesc.Width = static_cast<UINT>(_windowExtent.width);
			swapChainDesc.Height = static_cast<UINT>(_windowExtent.height);
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			ComPtr<IDXGISwapChain1> tempSwapChain;
			check_result(_dxgiFactory->CreateSwapChainForHwnd(_directCommandQueue.Get(), _windowHandle, &swapChainDesc, nullptr,
				nullptr, &tempSwapChain));

			check_result(tempSwapChain.As(&_swapChain));

			check_result(_dxgiFactory->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER));
		}
		else
		{
			// wait for all frames in flight to be done
			for (unsigned long long& _frameFenceValue : _frameFenceValues)
			{
				// Schedule a Signal command in the GPU queue.
				UINT64 fenceValue = _frameFenceValue;
				if (SUCCEEDED(_directCommandQueue->Signal(_fenceDirect.Get(), fenceValue)))
				{
					// Wait until the Signal has been processed.
					if (SUCCEEDED(_fenceDirect->SetEventOnCompletion(fenceValue, _fenceEvent)))
					{
						std::ignore = WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);

						// Increment the fence value for the current frame.
						_frameFenceValue++;
					}
				}
			}

			for (auto& buffer : _backBuffers)
			{
				buffer->get_d3d12_resource()->Release();
				free_descriptors(buffer.get());
			}

			auto hresult = _swapChain->ResizeBuffers(_numFrames, _windowExtent.width, _windowExtent.height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

			if (FAILED(hresult))
			{
				throw std::runtime_error("Failed HRESULT on swapchain resize");
			}

		}

		for (uint32_t i = 0; i < _numFrames; i++)
		{
			ID3D12Resource* backBuffer;
			check_result(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			if (_backBuffers[i] != nullptr)
			{
				free_descriptors(_backBuffers[i].get());
			}

			_backBuffers[i] = std::make_shared<Image>(_windowExtent.width, _windowExtent.height, 1, nullptr, backBuffer,
				DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_DESC(), nullptr, i == 0 ? "BackBuffer0" : "BackBuffer1");
			_backBuffers[i]->_currentState = D3D12_RESOURCE_STATE_COMMON;
			_backBuffers[i]->_d3d12Resource->SetName(i == 0 ? L"BackBuffer0" : L"BackBuffer1");

			register_render_target_view(_backBuffers[i]);
		}
	}

	ComPtr<IDXGIAdapter4> Context::get_adapter(const bool useDebug)
	{
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;

		if (useDebug)
		{
			createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		check_result(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;


		ComPtr<IDXGIFactory6> factory6;
		check_result(dxgiFactory.As(&factory6));

		HRESULT hr;

		if (factory6)
			_dxgiFactory = factory6;

		for (UINT testAdapterIndex = 0; ; ++testAdapterIndex)
		{
			ComPtr<IDXGIAdapter1> testAdapter;

			if (factory6)
			{
				ComPtr<IDXGIAdapter> baseAdapter;
				hr = factory6->EnumAdapterByGpuPreference(testAdapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
					IID_PPV_ARGS(&baseAdapter));
				if (SUCCEEDED(hr))
				{
					check_result(baseAdapter.As(&testAdapter));
				}
			}
			else
			{
				throw std::runtime_error("need dxgi 6");
			}

			if (FAILED(hr))
				break;

			DXGI_ADAPTER_DESC1 testDesc;
			if (!check_result_safe(testAdapter->GetDesc1(&testDesc)))
			{
				continue;
			}

			if (SUCCEEDED(D3D12CreateDevice(testAdapter.Get(), _desiredFeatureLevel, _uuidof(ID3D12Device), nullptr)))
			{
				auto wstr = std::wstring(testDesc.Description);
				//_log.information(fmt::format("found device {} with {}mb memory", std::string(wstr.begin(), wstr.end()), memSize));

				if (!dxgiAdapter1)
				{
					dxgiAdapter1 = std::move(testAdapter);
				}
			}
		}


		if (dxgiAdapter1)
		{
			dxgiAdapter1.As(&dxgiAdapter4);
		}

		return dxgiAdapter4;
	}

	void Context::init_device_and_resources(const bool useDebug)
	{
		ComPtr<ID3D12Device2> device;
		check_result(D3D12CreateDevice(_adapter.Get(), _desiredFeatureLevel, IID_PPV_ARGS(&device)));

		check_result(device->QueryInterface(IID_PPV_ARGS(&_device)));

		if (useDebug)
		{
			ComPtr<ID3D12InfoQueue> pInfoQueue;
			if (SUCCEEDED(_device.As(&pInfoQueue)))
			{
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
			}
		}

		// init D3D12MA
		{
			D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
			allocatorDesc.pDevice = _device.Get();
			allocatorDesc.pAdapter = _adapter.Get();
			allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
			allocatorDesc.PreferredBlockSize = 0;

			if (FAILED(D3D12MA::CreateAllocator(&allocatorDesc, &_allocator)))
			{
				throw std::runtime_error("failed to init D3D12MA");
			}
		}

		// init queues
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_directCommandQueue)));
			_directCommandQueue->SetName(L"Direct Queue"); // don't care if this fails

			check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_directUploadQueue)));
			_directUploadQueue->SetName(L"Direct Queue"); // don't care if this fails

			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

			check_result(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_copyCommandQueue)));
			_copyCommandQueue->SetName(L"Copy Queue");

			check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fenceCopy)));
			_fenceCopy->SetName(L"fenceCopy");

			check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_immediateFence)));

			check_result(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fenceDirect)));
			_fenceDirect->SetName(L"fenceDirect");

			_fenceEvent = CreateEventA(nullptr, FALSE, FALSE, "fenceEvent");
		}

		// init descriptors
		{
			_cpuRtvDescAllocator = new DarkDescriptorAllocator<_maxCpuRtvDesc>();
			_cpuSamplerDescAllocator = new DarkDescriptorAllocator<_maxCpuSamplerDesc>();
			_cpuDsvDescAllocator = new DarkDescriptorAllocator<_maxCpuDsvDesc>();

			_cpuRtvDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
			_cpuSamplerDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, false);
			_cpuDsvDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

			_gpuMainSrvDescHeap = std::make_unique<DarkDescriptorAllocator<1'000'000>>();
			_gpuMainSrvDescHeap->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
			_gpuMainSamplerDescHeap = std::make_unique<GpuDescriptorAllocator<100>>(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, L"Main GPU desc heap (sampler)");

			_imguiDescAllocator = std::make_unique<DarkDescriptorAllocator<1>>();
			_imguiDescAllocator->init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
			/*D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			check_result(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_imguiDescHeap)));*/
		}

		init_root_signature();

		build_swapchain(_windowExtent);
	}

	void Context::build_swapchain(Extent2D windowExtent)
	{
		if (!_swapChain)
		{
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = _numFrames;
			swapChainDesc.Width = static_cast<UINT>(windowExtent.width);
			swapChainDesc.Height = static_cast<UINT>(windowExtent.height);
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			ComPtr<IDXGISwapChain1> tempSwapChain;
			check_result(_dxgiFactory->CreateSwapChainForHwnd(_directCommandQueue.Get(), _windowHandle, &swapChainDesc, nullptr,
				nullptr, &tempSwapChain));

			check_result(tempSwapChain.As(&_swapChain));

			_dxgiFactory->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER);
		}
		else
		{
			throw std::runtime_error("can't build swapchain, already exists");
		}

		for (uint32_t i = 0; i < _numFrames; i++)
		{
			ID3D12Resource* backBuffer;
			check_result(_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			if (_backBuffers[i] != nullptr)
			{
				free_descriptors(_backBuffers[i].get());
			}

			_backBuffers[i] = std::make_shared<Image>(windowExtent.width, windowExtent.height, 1, nullptr, backBuffer,
				DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_DESC(), nullptr, i == 0 ? "BackBuffer0" : "BackBuffer1");
			_backBuffers[i]->_currentState = D3D12_RESOURCE_STATE_COMMON;
			_backBuffers[i]->_d3d12Resource->SetName(i == 0 ? L"BackBuffer0" : L"BackBuffer1");

			register_render_target_view(_backBuffers[i]);
		}
	}

	void Context::free_descriptors(const Resource* resource) const
	{
		if (resource->_srvDescriptor.has_value())
			_gpuMainSrvDescHeap->free(resource->_srvDescriptor.value());

		if (resource->_rtvDescriptor.has_value())
			_cpuRtvDescAllocator->free(resource->_rtvDescriptor.value());

		if (resource->_dsvDescriptor.has_value())
			_cpuDsvDescAllocator->free(resource->_dsvDescriptor.value());

		if (resource->_cbvDescriptor.has_value())
			_gpuMainSrvDescHeap->free(resource->_cbvDescriptor.value());
	}

	void Context::submit_list_main(ComPtr<ID3D12GraphicsCommandList6> list) const
	{
		check_result(list->Close());

		ID3D12CommandList* ppCommandLists[] = { list.Get() };

		_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void Context::present()
	{
		auto backBuffer = get_back_buffer();

		{
			auto imguiList = allocate_command_list();
			imguiList->try_defer_transition(backBuffer, ResourceState::RenderTarget);
			//float clearColor[4] = { 0.f,0.f,0.f,0.f };
			//imguiList->_commandList->ClearRenderTargetView(backBuffer->get_d3d12_rtv(), clearColor, 0, nullptr);
			auto rtv = backBuffer->get_d3d12_rtv();
			imguiList->_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
			auto imguiHeap = _imguiDescAllocator->get_heap();
			imguiList->_commandList->SetDescriptorHeaps(1, &imguiHeap);
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), imguiList->_commandList.Get());
			submit_list(std::move(imguiList));
		}

		if (backBuffer->_currentState != D3D12_RESOURCE_STATE_PRESENT)
		{
			auto tempList = allocate_raw_command_list();
			transition_resource(tempList.Get(), backBuffer->get_d3d12_resource(), backBuffer->_currentState, D3D12_RESOURCE_STATE_PRESENT);
			backBuffer->_currentState = D3D12_RESOURCE_STATE_PRESENT;
			submit_list_main(tempList);
		}

		UINT syncInterval = 1;
		UINT presentFlags = 0;

		check_result(_swapChain->Present(syncInterval, presentFlags));
		_frameFenceValues[get_current_frame_index()] = signal_fence(_directCommandQueue.Get(), _fenceDirect.Get(), _fenceValue);

		wait_for_fence_value(_fenceDirect.Get(), _frameFenceValues[get_current_frame_index()], _fenceEvent);

		reset_allocators(get_current_frame_index());

		auto frameIdx = get_current_frame_index();
		_frameResourcesAllocated[frameIdx].store(0);

		// empty the locks
		{
			std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);
			auto& resourceLocks = _perFrameData[get_current_frame_index()].resourceLocks;
			std::shared_ptr<Resource> resource;
			while (!resourceLocks.empty())
			{
				resourceLocks.pop();
			}
		}

		ImGui_ImplSDL3_NewFrame();
		ImGui_ImplDX12_NewFrame();
		ImGui::NewFrame();
	}

	std::shared_ptr<Buffer> Context::create_buffer(const size_t size, const bool dynamic, std::string name)
	{
		auto heapType = dynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

		D3D12MA::Allocation* srvAllocation;
		ID3D12Resource* srvResource;

		// align all buffer sizes to 256
		auto alignedSize = (size + 255) & ~255;

		D3D12_RESOURCE_DESC srvDesc = {};
		srvDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		srvDesc.Alignment = 0;
		srvDesc.Width = alignedSize;
		srvDesc.Height = 1;
		srvDesc.DepthOrArraySize = 1;
		srvDesc.MipLevels = 1;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.SampleDesc.Count = 1;
		srvDesc.SampleDesc.Quality = 0;
		srvDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		srvDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC srvAllocDesc = {};
		srvAllocDesc.HeapType = heapType;

		HRESULT hr = _allocator->CreateResource(&srvAllocDesc, &srvDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
			&srvAllocation, IID_PPV_ARGS(&srvResource));

		if (FAILED(hr))
		{
			auto reason = _device->GetDeviceRemovedReason();
			throw std::runtime_error("failed to allocate buffer");
		}

		//auto res = std::make_shared<Buffer>(srvAllocation, srvResource, alignedSize, heapType, this);
		auto res = std::make_shared<Buffer>(alignedSize, srvAllocation, srvResource, this, srvDesc, heapType, name);
		res->_currentState = D3D12_RESOURCE_STATE_COMMON;
		res->_resourceDesc = srvDesc;

		{
			std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);
			_perFrameData[get_current_frame_index()].resourceLocks.push(res);
		}

		return res;
	}

	std::shared_ptr<dm3d::Image> Context::create_image(Extent3D size, const ImageFormat format, const ResourceFlags flags, const ResourceState initialState, std::string name)
	{
		// create texture resource

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Alignment = 0;
		textureDesc.Width = static_cast<UINT64>(size.width);
		textureDesc.Height = static_cast<UINT>(size.height);
		textureDesc.DepthOrArraySize = static_cast<UINT16>(size.depth);
		textureDesc.MipLevels = 1;
		textureDesc.Format = D3D12_Translator::format(format);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_Translator::resource_flags(flags);

		D3D12MA::Allocation* textureAllocation;
		ID3D12Resource* textureResource;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_CLEAR_VALUE* clearValue = nullptr;

		D3D12_CLEAR_VALUE clearValDesc = {};

		if ((flags & ResourceFlags::DepthStencil) == DepthStencil)
		{
			clearValDesc.Format = D3D12_Translator::dsv_type_map(D3D12_Translator::format(format));
			clearValDesc.DepthStencil.Depth = 1.f;
			clearValDesc.DepthStencil.Stencil = 0;

			clearValue = &clearValDesc;
		}
		else if ((flags & ResourceFlags::RenderTarget) == RenderTarget)
		{
			clearValDesc.Format = D3D12_Translator::srv_type_map(D3D12_Translator::format(format));
			clearValDesc.Color[0] = 0.f;
			clearValDesc.Color[1] = 0.f;
			clearValDesc.Color[2] = 0.f;
			clearValDesc.Color[3] = 0.f;

			clearValue = &clearValDesc;
		}

		HRESULT hr = _allocator->CreateResource(&allocDesc, &textureDesc, D3D12_Translator::resource_state(initialState),
			clearValue, &textureAllocation, IID_PPV_ARGS(&textureResource));

		if (FAILED(hr))
		{
			throw std::runtime_error("failed to create texture resource");
		}

		auto response = std::make_shared<Image>(size.width, size.height, size.depth, textureAllocation, textureResource, D3D12_Translator::format(format), textureDesc, this, name);
		response->_currentState = D3D12_Translator::resource_state(initialState);
		response->_resourceDesc = textureDesc;

		{
			std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);
			_perFrameData[get_current_frame_index()].resourceLocks.push(response);
		}

		return response;
	}

	std::shared_ptr<dm3d::Shader> Context::create_shader(void* data, const size_t size, ShaderStage stage)
	{
		ComPtr<ID3DBlob> shaderBlob;

		check_result(D3DCreateBlob(size, &shaderBlob));
		memcpy(shaderBlob->GetBufferPointer(), data, size);

		return std::make_shared<Shader>(shaderBlob, stage, this);
	}

	std::shared_ptr<dm3d::IndexBuffer> Context::create_index_buffer(uint32_t* pIndices, size_t count, std::string name)
	{
		auto bufferSize = sizeof(uint32_t) * count;

		D3D12MA::Allocation* indexBufferAllocation;
		ID3D12Resource* indexBuffer;

		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = bufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC allocDesc = {};
		allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		check_result(_allocator->CreateResource(&allocDesc, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
			&indexBufferAllocation, IID_PPV_ARGS(&indexBuffer)));

		D3D12MA::Allocation* uploadBufferAllocation;
		ID3D12Resource* uploadBuffer;

		D3D12_RESOURCE_DESC uploadBufferDesc = {};
		uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadBufferDesc.Alignment = 0;
		uploadBufferDesc.Width = bufferSize;
		uploadBufferDesc.Height = 1;
		uploadBufferDesc.DepthOrArraySize = 1;
		uploadBufferDesc.MipLevels = 1;
		uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		uploadBufferDesc.SampleDesc.Count = 1;
		uploadBufferDesc.SampleDesc.Quality = 0;
		uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
		uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		check_result(_allocator->CreateResource(&uploadAllocDesc, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, &uploadBufferAllocation, IID_PPV_ARGS(&uploadBuffer)));

		void* mappedData = nullptr;
		check_result(uploadBuffer->Map(0, nullptr, &mappedData));
		memcpy(mappedData, pIndices, bufferSize);
		uploadBuffer->Unmap(0, nullptr);

		auto tempList = allocate_raw_command_list();

		tempList->CopyBufferRegion(indexBuffer, 0, uploadBuffer, 0, bufferSize);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = indexBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		tempList->ResourceBarrier(1, &barrier);

		submit_list_immediate(tempList);

		uploadBufferAllocation->Release();

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;

		auto res = std::make_shared<IndexBuffer>(indexBufferAllocation, indexBufferView, indexBuffer, this, name);

		{
			std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);
			_perFrameData[get_current_frame_index()].resourceLocks.push(res);
		}

		return res;
	}

	void Context::register_render_target_view(std::shared_ptr<Image> target)
	{
		if (target->get_depth() < 2)
		{
			auto rtv = _cpuRtvDescAllocator->allocate();

			_device->CreateRenderTargetView(target->get_d3d12_resource(), nullptr, rtv.cpuHandle);

			target->_rtvDescriptor = rtv;
		}
		else
		{
			auto rtv = _cpuRtvDescAllocator->allocate();

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = target->get_d3d12_format();
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.ArraySize = target->get_depth();
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;

			_device->CreateRenderTargetView(target->get_d3d12_resource(), &rtvDesc, rtv.cpuHandle);

			target->_rtvDescriptor = rtv;
		}
	}

	void Context::register_depth_stencil_view(std::shared_ptr<Image> target)
	{
		if (target->get_depth() < 2)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = D3D12_Translator::dsv_type_map(target->get_d3d12_format());
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2D.MipSlice = 0;

			auto dsvHandle = _cpuDsvDescAllocator->allocate();

			_device->CreateDepthStencilView(target->get_d3d12_resource(), &dsvDesc, dsvHandle.cpuHandle);

			target->_dsvDescriptor = dsvHandle;
		}
		else
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = D3D12_Translator::dsv_type_map(target->get_d3d12_format());
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.ArraySize = target->get_depth();
			dsvDesc.Texture2DArray.FirstArraySlice = 0;

			auto dsv = _cpuDsvDescAllocator->allocate();

			_device->CreateDepthStencilView(target->get_d3d12_resource(), &dsvDesc, dsv.cpuHandle);

			target->_dsvDescriptor = dsv;
		}
	}

	void Context::register_image_view(std::shared_ptr<Image> image)
	{
		auto& desc = image->_resourceDesc;

		if (image->get_depth() < 2)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = D3D12_Translator::srv_type_map(desc.Format);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc.MipLevels;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

			auto srv = _gpuMainSrvDescHeap->allocate();

			_device->CreateShaderResourceView(image->get_d3d12_resource(), &srvDesc, srv.cpuHandle);

			image->_srvDescriptor = srv;
		}
		else
		{
			// create srv array

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = D3D12_Translator::srv_type_map(desc.Format);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			auto srv = _gpuMainSrvDescHeap->allocate();

			_device->CreateShaderResourceView(image->get_d3d12_resource(), &srvDesc, srv.cpuHandle);

			image->_srvDescriptor = srv;
		}
	}

	void Context::register_constant_view(std::shared_ptr<Buffer> buffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = buffer->get_d3d12_resource()->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(buffer->get_size());

		auto cbvHandle = _gpuMainSrvDescHeap->allocate();
		_device->CreateConstantBufferView(&cbvDesc, cbvHandle.cpuHandle);

		buffer->_cbvDescriptor = cbvHandle;
	}

	void Context::copy_image(void* data, std::shared_ptr<Image> image)
	{
		const UINT64 uploadBufferSize = get_required_intermediate_size(image->get_d3d12_resource(), 0, 1);

		D3D12_RESOURCE_DESC uploadBufferDesc = {};
		uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		uploadBufferDesc.Alignment = 0;
		uploadBufferDesc.Width = uploadBufferSize;
		uploadBufferDesc.Height = 1;
		uploadBufferDesc.DepthOrArraySize = 1;
		uploadBufferDesc.MipLevels = 1;
		uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		uploadBufferDesc.SampleDesc.Count = 1;
		uploadBufferDesc.SampleDesc.Quality = 0;
		uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::Allocation* uploadAllocation;
		ID3D12Resource* uploadBuffer;

		D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
		uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		HRESULT hr = _allocator->CreateResource(&uploadAllocDesc, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, &uploadAllocation, IID_PPV_ARGS(&uploadBuffer));

		check_result(hr);

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = data;
		textureData.RowPitch = image->get_width() * D3D12_Translator::format_stride(image->get_d3d12_format());
		textureData.SlicePitch = textureData.RowPitch * image->get_height();

		auto tempList = allocate_raw_command_list();

		auto oldState = image->_currentState;

		if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			transition_resource(tempList.Get(), image->get_d3d12_resource(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);
		}

		update_subresources(tempList.Get(), image->get_d3d12_resource(), uploadBuffer, 0, 0, 1, &textureData);

		transition_resource(tempList.Get(), image->get_d3d12_resource(), D3D12_RESOURCE_STATE_COPY_DEST, oldState);

		submit_list_immediate(tempList);

		uploadBuffer->Release();
		uploadAllocation->Release();
	}

	void Context::copy_buffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst)
	{
		auto tempList = allocate_raw_command_list();

		try_transition_resource(tempList.Get(), src.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		try_transition_resource(tempList.Get(), dst.get(), D3D12_RESOURCE_STATE_COPY_DEST);

		tempList->CopyBufferRegion(dst->get_d3d12_resource(), 0, src->get_d3d12_resource(), 0, src->get_size());

		submit_list_immediate(tempList);
	}

	void Context::register_resource_view(std::shared_ptr<Buffer> buffer)
	{
		auto& desc = buffer->_resourceDesc;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescView = {};
		srvDescView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescView.Format = DXGI_FORMAT_UNKNOWN;
		srvDescView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDescView.Buffer.FirstElement = 0;
		srvDescView.Buffer.NumElements = 1;
		srvDescView.Buffer.StructureByteStride = static_cast<UINT>(desc.Width);
		srvDescView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		auto srv = _gpuMainSrvDescHeap->allocate();

		_device->CreateShaderResourceView(buffer->get_d3d12_resource(), &srvDescView, srv.cpuHandle);

		buffer->_srvDescriptor = srv;
	}

	void Context::register_resource_view(std::shared_ptr<Buffer> buffer, size_t numElements, size_t elementSize)
	{
		auto& desc = buffer->_resourceDesc;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescView = {};
		srvDescView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescView.Format = DXGI_FORMAT_UNKNOWN;
		srvDescView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDescView.Buffer.FirstElement = 0;
		srvDescView.Buffer.NumElements = static_cast<UINT>(numElements);
		srvDescView.Buffer.StructureByteStride = static_cast<UINT>(elementSize);
		srvDescView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		auto srv = _gpuMainSrvDescHeap->allocate();

		_device->CreateShaderResourceView(buffer->get_d3d12_resource(), &srvDescView, srv.cpuHandle);

		buffer->_srvDescriptor = srv;
	}

	void Context::release_shader_internal(Shader* shader)
	{
		shader->get_d3d12_blob()->Release();
	}

	void Context::release_resource_internal(Resource* resource)
	{
		free_descriptors(resource);
		resource->get_d3d12_resource()->Release();
		auto allocation = resource->get_d3d12_allocation();
		if (allocation != nullptr)
			allocation->Release();
		_totalResourcesAllocated.fetch_sub(1U);
		_frameResourcesAllocated[get_current_frame_index() == 1 ? 0 : 1].fetch_sub(1);

		//std::cout << "DM3D Freed resource: " << (uint64_t)resource << "\n";
	}

	void Context::alloc_resource_internal()
	{
		_totalResourcesAllocated.fetch_add(1U);
		_frameResourcesAllocated[get_current_frame_index() == 1 ? 0 : 1].fetch_add(1);
	}

	std::unique_ptr<CommandList> Context::allocate_command_list()
	{
		auto commandList = allocate_raw_command_list();

		auto renderList = std::make_unique<CommandList>(commandList, _rootSignature, _psoCache.get());

		return renderList;
	}

	void Context::submit_list(std::unique_ptr<CommandList> list, const QueueType queueType)
	{
		std::unique_lock lock(_submitLock);

		{
			std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);

			while (!list->_resourceLocks.empty())
			{
				auto resource = list->_resourceLocks.front();
				_perFrameData[get_current_frame_index()].resourceLocks.push(resource);
				list->_resourceLocks.pop();
			}
		}

		if (!list->_transitions.empty())
		{
			auto transList = allocate_raw_command_list();

			list->resolve_transitions(transList);

			if (queueType == QueueType::Immediate)
			{
				submit_list_immediate(transList);
			}
			else
			{
				submit_list_main(transList);
			}
		}

		if (queueType == QueueType::Immediate)
		{
			submit_list_immediate(list->_commandList);
			return;
		}

		submit_list_main(list->_commandList);
	}

	std::shared_ptr<Image> Context::get_back_buffer()
	{
		return _backBuffers[get_current_frame_index()];
	}

	uint32_t Context::get_current_frame_index()
	{
		return _swapChain->GetCurrentBackBufferIndex();
	}

	Extent2D Context::get_current_draw_extent()
	{
		return _windowExtent;
	}

	uint8_t Context::get_frame_index()
	{
		return static_cast<uint8_t>(_swapChain->GetCurrentBackBufferIndex());
	}

	DarkMatter3DUsageStats Context::get_usage_stats()
	{
		D3D12MA::Budget budget;
		_allocator->GetBudget(&budget, nullptr);

		auto stats = DarkMatter3DUsageStats();
		stats.availableMemory = budget.BudgetBytes;
		stats.usedMemory = budget.UsageBytes;
		stats.allocators = _totalAllocators.load();
		stats.usedAllocators = 0;
		stats.allocatedResources = _totalResourcesAllocated.load();
		stats.frameAllocatedResources = _frameResourcesAllocated[get_current_frame_index()].load();

		for (int i = 0; i < _numFrames; i++)
		{
			stats.usedAllocators += static_cast<uint32_t>(_perFrameData[i].allocatorIdx);
		}

		return stats;
	}

	void Context::release_on_delete(std::shared_ptr<Resource> resource)
	{
		while (!_releaseOnDelete.enqueue(resource))
		{}
	}

	void Context::wait_for_idle()
	{
		for (int i = 0; i < _numFrames; i++)
		{
			_frameFenceValues[i] = signal_fence(_directCommandQueue.Get(), _fenceDirect.Get(), _fenceValue);

			wait_for_fence_value(_fenceDirect.Get(), _frameFenceValues[i], _fenceEvent);
		}
	}

	std::shared_ptr<Image> Context::load_dds(void* data, size_t size, std::string name)
	{
		std::shared_ptr<Image> response;
		ID3D12Resource* pTexture;
		std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;

		check_result(DirectX::LoadDDSTextureFromMemory(_device.Get(), reinterpret_cast<uint8_t*>(data), size, &pTexture, subresourceData));

		auto texDesc = pTexture->GetDesc();

		{
			D3D12MA::Allocation* textureAllocation;
			ID3D12Resource* textureResource;

			D3D12MA::ALLOCATION_DESC allocDesc = {};
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

			HRESULT hr = _allocator->CreateResource(&allocDesc, &texDesc, D3D12_Translator::resource_state(ResourceState::ShaderRead),
				nullptr, &textureAllocation, IID_PPV_ARGS(&textureResource));

			if (FAILED(hr))
			{
				throw std::runtime_error("failed to create texture resource");
			}

			response = std::make_shared<Image>(static_cast<uint32_t>(texDesc.Width), static_cast<uint32_t>(texDesc.Height), static_cast<uint32_t>(texDesc.DepthOrArraySize), textureAllocation, textureResource, texDesc.Format, texDesc, this, name);
			response->_currentState = D3D12_Translator::resource_state(ResourceState::ShaderRead);
			response->_resourceDesc = texDesc;

			{
				std::unique_lock qLock(_perFrameData[get_current_frame_index()].queueLock);
				_perFrameData[get_current_frame_index()].resourceLocks.push(response);
			}
		}
		D3D12MA::Allocation* uploadAllocation;
		ID3D12Resource* uploadBuffer;
		// copy DDS data to intermediate texture
		{
			const UINT64 uploadBufferSize = get_required_intermediate_size(pTexture, 0,
				static_cast<UINT>(subresourceData.size()));

			D3D12_RESOURCE_DESC uploadBufferDesc = {};
			uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			uploadBufferDesc.Alignment = 0;
			uploadBufferDesc.Width = uploadBufferSize;
			uploadBufferDesc.Height = 1;
			uploadBufferDesc.DepthOrArraySize = 1;
			uploadBufferDesc.MipLevels = 1;
			uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			uploadBufferDesc.SampleDesc.Count = 1;
			uploadBufferDesc.SampleDesc.Quality = 0;
			uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
			uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

			HRESULT hr = _allocator->CreateResource(&uploadAllocDesc, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, &uploadAllocation, IID_PPV_ARGS(&uploadBuffer));

			check_result(hr);
		}

		{
			auto tempList = allocate_raw_command_list();

			auto oldState = response->_currentState;

			if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
			{
				transition_resource(tempList.Get(), response->get_d3d12_resource(), oldState, D3D12_RESOURCE_STATE_COPY_DEST);
			}

			update_subresources(tempList.Get(), response->get_d3d12_resource(), uploadBuffer, 0, 0, static_cast<UINT>(subresourceData.size()), subresourceData.data());

			transition_resource(tempList.Get(), response->get_d3d12_resource(), D3D12_RESOURCE_STATE_COPY_DEST, oldState);

			submit_list_immediate(tempList);
		}

		uploadBuffer->Release();
		uploadAllocation->Release();
		pTexture->Release();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(texDesc.MipLevels);
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

		auto srv = _gpuMainSrvDescHeap->allocate();

		_device->CreateShaderResourceView(response->get_d3d12_resource(), &srvDesc, srv.cpuHandle);

		response->_srvDescriptor = srv;

		return response;
	}

	void Context::submit_list_immediate(ComPtr<ID3D12GraphicsCommandList6> list)
	{
		check_result(list->Close());

		ID3D12CommandList* ppCommandLists[] = { list.Get() };

		_directUploadQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		auto newFenceValue = ++_immediateFenceValue;

		check_result(_directUploadQueue->Signal(_immediateFence.Get(), newFenceValue));

		if (_immediateFence->GetCompletedValue() < newFenceValue)
		{
			auto event = _fencePool.get_event();
			check_result(_immediateFence->SetEventOnCompletion(newFenceValue, event));

			WaitForSingleObject(event, INFINITE);
			_fencePool.free(event);
		}
	}

	ComPtr<ID3D12GraphicsCommandList6> Context::allocate_raw_command_list()
	{
		std::unique_lock lock(_allocatorLock);
		ComPtr<ID3D12CommandAllocator> allocator;
		auto& frameData = _perFrameData[get_current_frame_index()];
		if ((frameData.allocatorCache.size() == 0) || (frameData.allocatorIdx > frameData.allocatorCache.size() - 1))
		{
			// need to create one
			check_result(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
			frameData.allocatorCache.push_back(allocator);
		}
		else
		{
			allocator = frameData.allocatorCache[frameData.allocatorIdx];
			check_result(allocator->Reset());
		}

		frameData.allocatorIdx++;

		ComPtr<ID3D12GraphicsCommandList6> commandList;

		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

		/*if (!_perFrameData[get_current_frame_index()].allocatorUsedList.enqueue({allocator, commandList}))
		{
			throw std::runtime_error("failed to add command allocator to used list");
		}*/

		ID3D12DescriptorHeap* ppHeaps[] = { _gpuMainSrvDescHeap->get_heap(), _gpuMainSamplerDescHeap->get_heap() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		return commandList;
	}

	void Context::transition_resource(ID3D12GraphicsCommandList6* cmd, ID3D12Resource* resource, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		cmd->ResourceBarrier(1, &barrier);
	}

	void Context::try_transition_resource(ID3D12GraphicsCommandList6* cmd, Resource* resource, const D3D12_RESOURCE_STATES desiredState)
	{
		if (resource->_currentState != desiredState)
		{
			transition_resource(cmd, resource->get_d3d12_resource(), resource->_currentState, desiredState);
			resource->_currentState = desiredState;
		}
	}

	void Context::wait_for_fence_value(ID3D12Fence* fence, const uint64_t fenceValue, const HANDLE fenceEvent, const std::chrono::milliseconds duration)
	{
		if (fence->GetCompletedValue() < fenceValue)
		{
			check_result(fence->SetEventOnCompletion(fenceValue, fenceEvent));
			::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
		}
	}

	uint64_t Context::signal_fence(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, std::atomic<uint64_t>& fenceValue)
	{
		auto fenceValueForSignal = ++fenceValue;
		check_result(commandQueue->Signal(fence, fenceValueForSignal));

		return fenceValueForSignal;
	}

	void Context::reset_allocators(const uint32_t frameIdx)
	{
		std::unique_lock lock(_allocatorLock);
		_perFrameData[frameIdx].allocatorIdx = 0L;
		//auto currentSize = _perFrameData[frameIdx].allocatorUsedList.size_approx();
		//while (_perFrameData[frameIdx].allocatorUsedList.size_approx() != 0)
		//{
		//	std::pair<ComPtr<ID3D12CommandAllocator>, ComPtr<ID3D12GraphicsCommandList6>> allocatorPair;
		//	if (_perFrameData[frameIdx].allocatorUsedList.try_dequeue(allocatorPair))
		//	{
		//		/*allocatorPair.first->Reset();

		//		allocatorPair.second->Reset(allocatorPair.first.Get(), nullptr);*/

		//		_perFrameData[frameIdx].allocatorFreeList.enqueue(allocatorPair.first);
		//	}
		//}
	}

	void Context::init_root_signature()
	{
		D3D12_STATIC_SAMPLER_DESC staticSampler = {};
		staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias = 0.0f;
		staticSampler.MaxAnisotropy = 1;
		staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD = 0.0f;
		staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister = 0; // s0
		staticSampler.RegisterSpace = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC staticPcfSampler = {};
		staticPcfSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // Use linear filtering with comparison
		staticPcfSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;               // Clamp to avoid wrapping
		staticPcfSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticPcfSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticPcfSampler.MipLODBias = 0.0f;
		staticPcfSampler.MaxAnisotropy = 1;
		staticPcfSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;         // Comparison function for shadow mapping
		staticPcfSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticPcfSampler.MinLOD = 0.0f;
		staticPcfSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticPcfSampler.ShaderRegister = 1; // s0
		staticPcfSampler.RegisterSpace = 0;
		staticPcfSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC staticPointSampler = {};
		staticPointSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticPointSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticPointSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticPointSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticPointSampler.MipLODBias = 0.0f;
		staticPointSampler.MaxAnisotropy = 1;
		staticPointSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticPointSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticPointSampler.MinLOD = 0.0f;
		staticPointSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticPointSampler.ShaderRegister = 2; // s0
		staticPointSampler.RegisterSpace = 0;
		staticPointSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC staticHeightSampler = {};
		staticHeightSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticHeightSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		staticHeightSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		staticHeightSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		staticHeightSampler.MipLODBias = 0.0f;
		staticHeightSampler.MaxAnisotropy = 1;
		staticHeightSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		staticHeightSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticHeightSampler.MinLOD = 0.0f;
		staticHeightSampler.MaxLOD = D3D12_FLOAT32_MAX;
		staticHeightSampler.ShaderRegister = 3; // s0
		staticHeightSampler.RegisterSpace = 0;
		staticHeightSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC staticSamplers[4];
		staticSamplers[0] = staticSampler;
		staticSamplers[1] = staticPcfSampler;
		staticSamplers[2] = staticPointSampler;
		staticSamplers[3] = staticHeightSampler;

		D3D12_ROOT_PARAMETER1 rootParams[constants::NumResourceTables] = {};

		for (uint32_t i = 0; i < constants::NumResourceTables; i++)
		{
			rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParams[i].Constants.ShaderRegister = i;
			rootParams[i].Constants.RegisterSpace = 0;
			rootParams[i].Constants.Num32BitValues = 8;
		}

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc;
		rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootDesc.Desc_1_1.NumParameters = _countof(rootParams);
		rootDesc.Desc_1_1.pParameters = rootParams;
		rootDesc.Desc_1_1.NumStaticSamplers = _countof(staticSamplers);
		rootDesc.Desc_1_1.pStaticSamplers = staticSamplers;
		rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
			| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		check_result(D3D12SerializeVersionedRootSignature(&rootDesc, &signature, &error));

		check_result(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
	}
}
