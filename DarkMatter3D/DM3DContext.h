#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <wrl.h>

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <memory>
#include <queue>
#include <string>
#include "SDL3/SDL.h"

#include "D3D12MemAlloc.h"
#include "DM3DCommandList.h"
#include "DM3DDescriptorAllocator.h"
#include "DM3DEnums.h"
#include "DM3DFencePool.h"
#include "DM3DResource.h"
#include "DM3DShader.h"
#include "DM3DTypes.h"
#include "DM3DPipelineStateCache.h"

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

using Microsoft::WRL::ComPtr;

namespace dm3d
{
	enum class QueueType
	{
		Main,
		Immediate
	};

	class Context
	{
	public:
		Context(Extent2D windowExtent, SDL_Window* windowHandle, bool useDebug = false);
		~Context();

		// handle window changes
		void rebuild_swapchain(Extent2D newExtent);

		// frame present
		void present();

		// resource creation
		std::shared_ptr<Buffer> create_buffer(size_t size, bool dynamic = false, std::string name = "");
		std::shared_ptr<dm3d::Image> create_image(Extent3D size, ImageFormat format, ResourceFlags flags = ResourceFlags::None, ResourceState initialState = ResourceState::ShaderRead, std::string name = "");
		std::shared_ptr<dm3d::Shader> create_shader(void* data, size_t size, ShaderStage stage);
		std::shared_ptr<dm3d::IndexBuffer> create_index_buffer(uint32_t* pIndices, size_t count, std::string name = "");

		// view creation
		void register_render_target_view(std::shared_ptr<Image> target);
		void register_depth_stencil_view(std::shared_ptr<Image> target);
		void register_image_view(std::shared_ptr<Image> image);
		void register_resource_view(std::shared_ptr<Buffer> buffer);
		void register_resource_view(std::shared_ptr<Buffer> buffer, size_t numElements, size_t elementSize);
		void register_constant_view(std::shared_ptr<Buffer> buffer);

		// buffer copy
		void copy_image(void* data, std::shared_ptr<Image> image);
		void copy_buffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst);

		// free stuff
		void release_shader_internal(Shader* shader);
		void release_resource_internal(Resource* resource);
		void alloc_resource_internal();

		// command lists
		std::unique_ptr<CommandList> allocate_command_list();
		void submit_list(std::unique_ptr<CommandList> list, QueueType queueType = QueueType::Main);

		// utility
		std::shared_ptr<Image> get_back_buffer();
		uint32_t get_current_frame_index();
		Extent2D get_current_draw_extent();
		uint8_t get_frame_index();
		DarkMatter3DUsageStats get_usage_stats();
		void release_on_delete(std::shared_ptr<Resource> resource);
		void wait_for_idle();
		std::shared_ptr<Image> load_dds(void* data, size_t size, std::string name = "");
	private:
		struct PerFrameData
		{
			//moodycamel::ConcurrentQueue<ComPtr<ID3D12CommandAllocator>> allocatorFreeList;
			//moodycamel::ConcurrentQueue<std::pair<ComPtr<ID3D12CommandAllocator>, ComPtr<ID3D12GraphicsCommandList6>>> allocatorUsedList;
			std::vector<ComPtr<ID3D12CommandAllocator>> allocatorCache;
			size_t allocatorIdx = 0L;
			std::queue<std::shared_ptr<Resource>> resourceLocks; // guarantee that resources live for at least one frame.
			std::mutex queueLock;
		};

		ComPtr<IDXGIAdapter4> get_adapter(bool useDebug);
		void init_device_and_resources(bool useDebug);
		void build_swapchain(Extent2D windowExtent);
		void free_descriptors(const Resource* resource) const;
		void submit_list_main(ComPtr<ID3D12GraphicsCommandList6> list) const;
		void submit_list_immediate(ComPtr<ID3D12GraphicsCommandList6> list);
		ComPtr<ID3D12GraphicsCommandList6> allocate_raw_command_list();
		void transition_resource(ID3D12GraphicsCommandList6* cmd, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
		void try_transition_resource(ID3D12GraphicsCommandList6* cmd, Resource* resource, D3D12_RESOURCE_STATES desiredState);
		void wait_for_fence_value(ID3D12Fence* fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
		uint64_t signal_fence(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, std::atomic<uint64_t>& fenceValue);
		void reset_allocators(uint32_t frameIdx);
		void init_root_signature();

		static constexpr uint8_t _numFrames = 2;

		// track all created resources
		std::vector<Resource*> _createdResources;

		HWND _windowHandle;
		Extent2D _windowExtent;

		// d3d12 crap
		static constexpr D3D_FEATURE_LEVEL _desiredFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		ComPtr<IDXGIFactory6> _dxgiFactory;
		ComPtr<ID3D12Device10> _device;
		ComPtr<IDXGIAdapter4> _adapter;
		D3D12MA::Allocator* _allocator = nullptr;
		ComPtr<ID3D12CommandQueue> _directCommandQueue;
		ComPtr<ID3D12CommandQueue> _directUploadQueue;
		ComPtr<ID3D12CommandQueue> _copyCommandQueue;
		ComPtr<ID3D12Fence> _fenceDirect;
		ComPtr<ID3D12Fence> _fenceCopy;
		ComPtr<ID3D12Fence> _immediateFence;
		HANDLE _fenceEvent = nullptr;
		FencePool _fencePool;
		ComPtr<IDXGISwapChain4> _swapChain;
		ComPtr<ID3D12RootSignature> _rootSignature;

		// managed
		std::shared_ptr<Image> _backBuffers[_numFrames];
		PerFrameData _perFrameData[_numFrames];
		uint64_t _currentFrame = 0;
		std::atomic<uint64_t> _fenceValue = 0;
		std::atomic<uint64_t> _immediateFenceValue = 0;
		std::atomic<uint32_t> _totalAllocators = 0;
		std::atomic<uint32_t> _totalResourcesAllocated = 0;
		std::atomic<int> _frameResourcesAllocated[_numFrames] = { 0, 0 };
		uint64_t _frameFenceValues[_numFrames] = {};
		std::mutex _submitLock;
		std::mutex _allocatorLock;
		moodycamel::ConcurrentQueue<std::shared_ptr<Resource>> _releaseOnDelete;

		// descriptor stuff
		static constexpr uint32_t _maxCpuBufferDesc = 100000;
		static constexpr uint32_t _maxCpuSamplerDesc = 1000;
		static constexpr uint32_t _maxCpuRtvDesc = 1000;
		static constexpr uint32_t _maxCpuDsvDesc = 1000;
		DarkDescriptorAllocator<_maxCpuSamplerDesc>* _cpuSamplerDescAllocator = nullptr;
		DarkDescriptorAllocator<_maxCpuRtvDesc>* _cpuRtvDescAllocator = nullptr;
		DarkDescriptorAllocator<_maxCpuDsvDesc>* _cpuDsvDescAllocator = nullptr;
		std::unique_ptr<DarkDescriptorAllocator<1'000'000>> _gpuMainSrvDescHeap = nullptr;
		std::unique_ptr<GpuDescriptorAllocator<100>> _gpuMainSamplerDescHeap = nullptr;

		// state management
		std::unique_ptr<PipelineStateObjectCache> _psoCache = nullptr;

		// imgui
		std::unique_ptr<DarkDescriptorAllocator<1>> _imguiDescAllocator = nullptr;

	public:
		// resource creation helpers
		template<typename T>
		std::shared_ptr<dm3d::Buffer> build_constant(T& data, bool dynamic)
		{
			auto buffer = create_buffer(sizeof(T), dynamic);
			register_constant_view(buffer);

			if (dynamic)
			{
				auto pBuffer = buffer->map();
				memcpy(pBuffer, &data, sizeof(T));
				buffer->unmap();
			}
			else
			{
				auto tempBuffer = create_buffer(sizeof(T), true);
				auto pBuffer = tempBuffer->map();
				memcpy(pBuffer, &data, sizeof(T));
				tempBuffer->unmap();
				copy_buffer(tempBuffer, buffer);
			}

			return buffer;
		}

		template<typename T>
		std::shared_ptr<dm3d::Buffer> build_resource_table(T& data)
		{
			if (sizeof(T) > (32))
			{
				throw std::runtime_error("Resource table must be less than 32 bytes");
			}

			auto buffer = create_buffer(sizeof(T), true);
			register_constant_view(buffer);
			
			auto pBuffer = buffer->map();
			memcpy(pBuffer, &data, sizeof(T));
			buffer->unmap();

			return buffer;
		}

		template<typename T>
		std::shared_ptr<dm3d::Buffer> build_structured(std::vector<T>& data, bool dynamic)
		{
			return build_structured(data.data(), data.size(), dynamic);
		}

		template<typename T>
		std::shared_ptr<dm3d::Buffer> build_structured(T* data, size_t count, bool dynamic)
		{
			auto buffer = create_buffer(sizeof(T) * count, dynamic);
			register_resource_view(buffer, count, sizeof(T));

			if (dynamic)
			{
				auto pBuffer = buffer->map();
				memcpy(pBuffer, data, sizeof(T) * count);
				buffer->unmap();
			}
			else
			{
				auto tempBuffer = create_buffer(sizeof(T) * count, true);
				auto pBuffer = tempBuffer->map();
				memcpy(pBuffer, data, sizeof(T) * count);
				tempBuffer->unmap();
				copy_buffer(tempBuffer, buffer);
			}

			return buffer;
		}
	};
}