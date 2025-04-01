#pragma once
#include <vector>
#include <wrl/client.h>

#include "concurrentqueue.h"
#include "D3D12MemAlloc.h"
#include "DM3DDescriptorRingBuffer.h"



namespace dm3d
{
	using Microsoft::WRL::ComPtr;
	struct CpuDescriptorHandle
	{
		CpuDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE h)
		{
			handle = h;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle;
	};

	struct GpuDescriptorHandle
	{
		GpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE h)
		{
			handle = h;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE handle;
	};

	template<uint32_t max_size>
	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, std::wstring name);
		~DescriptorAllocator();

		D3D12_CPU_DESCRIPTOR_HANDLE allocate_cpu();
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> allocate_gpu();
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> allocate_cpu(uint32_t count);
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> allocate_gpu(uint32_t count);
		ID3D12DescriptorHeap* get_heap()
		{
			return _heap.Get();
		}
		UINT get_increment_size() const
		{
			return _descriptorSize;
		}
	private:

		ID3D12Device2* _device;
		D3D12_DESCRIPTOR_HEAP_TYPE _type;
		ComPtr<ID3D12DescriptorHeap> _heap;
		DescriptorRingBuffer<max_size> _ringBuffer;
		UINT _descriptorSize;
	};

	template<uint32_t max_size>
	inline DescriptorAllocator<max_size>::DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, std::wstring name)
	{
		_device = device;
		_type = type;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = max_size;
		heapDesc.Flags = flags;
		heapDesc.Type = _type;

		auto hr = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));
		if (FAILED(hr))
		{
			throw std::runtime_error("failed to create desc heap");
		}

		_heap->SetName(name.c_str());

		_descriptorSize = _device->GetDescriptorHandleIncrementSize(_type);
	}

	template<uint32_t max_size>
	inline DescriptorAllocator<max_size>::~DescriptorAllocator()
	{
		_heap->Release();
	}

	template<uint32_t max_size>
	inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator<max_size>::allocate_cpu()
	{
		DescriptorRange range = _ringBuffer.allocate(1);

		auto start = _heap->GetCPUDescriptorHandleForHeapStart();

		start.ptr += range.start * _descriptorSize;

		return start;
	}

	template<uint32_t max_size>
	inline std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DescriptorAllocator<max_size>::allocate_gpu()
	{
		DescriptorRange range = _ringBuffer.allocate(1);

		auto gpuStart = _heap->GetGPUDescriptorHandleForHeapStart();
		auto cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();


		gpuStart.ptr = gpuStart.ptr + (range.start * _descriptorSize);
		cpuStart.ptr = cpuStart.ptr + (range.start * _descriptorSize);

		return { cpuStart, gpuStart };
	}

	template<uint32_t max_size>
	inline std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DescriptorAllocator<max_size>::allocate_cpu(uint32_t count)
	{
		DescriptorRange range = _ringBuffer.allocate(count);

		auto start = _heap->GetCPUDescriptorHandleForHeapStart();

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> res(range.size);

		start.ptr = start.ptr + (range.start * _descriptorSize);

		for (int i = 0; i < range.size; i++)
		{
			auto desc = start;
			desc.ptr += i * _descriptorSize;
			res.push_back(desc);
		}

		return res;
	}

	template<uint32_t max_size>
	inline std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> DescriptorAllocator<max_size>::allocate_gpu(uint32_t count)
	{
		DescriptorRange range = _ringBuffer.allocate(count);

		auto gpuStart = _heap->GetGPUDescriptorHandleForHeapStart();
		auto cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();

		std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE>> res(range.size);

		gpuStart.ptr = gpuStart.ptr + (range.start * _descriptorSize);
		cpuStart.ptr = cpuStart.ptr + (range.start * _descriptorSize);


		return { cpuStart, gpuStart };
	}

	struct DarkDescriptorPair
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		uint32_t idx;
	};

	template<uint32_t max_size>
	class DarkDescriptorAllocator
	{
	public:
		DarkDescriptorAllocator();
		~DarkDescriptorAllocator() = default;

		void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
		void reset();

		DarkDescriptorPair allocate();
		std::vector<DarkDescriptorPair> allocate(uint32_t count);

		void free(DarkDescriptorPair descriptor);

		uint32_t get_size() { return _descriptorSize; }

		uint32_t get_used()
		{
			return max_size - static_cast<uint32_t>(_freeIndices.size_approx());
		}

		uint32_t get_capacity()
		{
			return max_size;
		}

		ID3D12DescriptorHeap* get_heap()
		{
			return _heap.Get();
		}

	private:
		ComPtr<ID3D12DescriptorHeap> _heap;
		moodycamel::ConcurrentQueue<size_t> _freeIndices;
		uint32_t _descriptorSize;
		D3D12_DESCRIPTOR_HEAP_TYPE _type;
		bool _shaderVisible;
		D3D12_GPU_DESCRIPTOR_HANDLE _gpuStart;
	};

	template<uint32_t max_size>
	class GpuDescriptorAllocator
	{
	public:
		GpuDescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::wstring name);
		~GpuDescriptorAllocator() = default;

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> allocate();
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> allocate(uint32_t count);
		uint32_t end_frame();
		ID3D12DescriptorHeap* get_heap()
		{
			return _allocator->get_heap();
		}
	private:
		DescriptorAllocator<max_size>* _allocator;
		uint32_t _frameCount = 0;
	};

	template<uint32_t max_size>
	inline GpuDescriptorAllocator<max_size>::GpuDescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::wstring name)
	{
		_allocator = new DescriptorAllocator<max_size>(device, type, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, name);
	}

	template<uint32_t max_size>
	inline std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GpuDescriptorAllocator<max_size>::allocate()
	{
		_frameCount++;
		return _allocator->allocate_gpu();
	}

	template<uint32_t max_size>
	inline std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GpuDescriptorAllocator<max_size>::allocate(uint32_t count)
	{
		_frameCount += count;
		return _allocator->allocate_gpu(count);
	}

	template<uint32_t max_size>
	inline uint32_t GpuDescriptorAllocator<max_size>::end_frame()
	{
		auto current = _frameCount;
		_frameCount = 0;

		return current;
	}

	template<uint32_t max_size>
	inline DarkDescriptorAllocator<max_size>::DarkDescriptorAllocator()
	{
		reset();
	}

	template<uint32_t max_size>
	inline void DarkDescriptorAllocator<max_size>::init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
	{
		_type = type;
		_shaderVisible = shaderVisible;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = max_size;
		heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));
		if (FAILED(hr))
		{
			throw std::runtime_error("failed to create dark descriptor allocator");
		}

		_descriptorSize = device->GetDescriptorHandleIncrementSize(type);

		if (_shaderVisible)
		{
			_gpuStart = _heap->GetGPUDescriptorHandleForHeapStart();
		}

		if (_freeIndices.size_approx() == 0)
		{
			for (uint32_t i = 0; i < max_size; i++)
			{
				_freeIndices.enqueue(i);
			}
		}

		assert(_freeIndices.size_approx() == max_size);
	}

	template<uint32_t max_size>
	inline void DarkDescriptorAllocator<max_size>::reset()
	{
		_descriptorSize = 0;
		_heap = nullptr;
		_type = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}

	template<uint32_t max_size>
	inline DarkDescriptorPair DarkDescriptorAllocator<max_size>::allocate()
	{
		size_t index;
		//VOID_ERROR(_freeIndices.try_dequeue(index), "Failed to allocate descriptor");

		if (!_freeIndices.try_dequeue(index))
		{
			throw std::runtime_error("Failed to allocate descriptor");
		}


		assert(index < max_size);

		DarkDescriptorPair pair = {};

		auto descriptor = _heap->GetCPUDescriptorHandleForHeapStart();
		descriptor.ptr += static_cast<size_t>(index) * static_cast<size_t>(_descriptorSize);

		pair.cpuHandle = descriptor;
		pair.idx = static_cast<uint32_t>(index);

		if (_shaderVisible)
		{
			auto gpuDesc = _gpuStart;
			gpuDesc.ptr += index * static_cast<size_t>(_descriptorSize);
			pair.gpuHandle = gpuDesc;
		}

		return pair;
	}

	template<uint32_t max_size>
	inline std::vector<DarkDescriptorPair> DarkDescriptorAllocator<max_size>::allocate(uint32_t count)
	{
		std::vector<DarkDescriptorPair> result;

		for (uint32_t i = 0; i < count; i++)
		{
			result.push_back(allocate());
		}

		return result;
	}

	template<uint32_t max_size>
	inline void DarkDescriptorAllocator<max_size>::free(DarkDescriptorPair descriptor)
	{
		if (descriptor.cpuHandle.ptr != 0)
		{
			const size_t heapStart = _heap->GetCPUDescriptorHandleForHeapStart().ptr;

			const size_t offset = descriptor.cpuHandle.ptr - heapStart;

			const size_t index = offset / static_cast<size_t>(_descriptorSize);

			if (!_freeIndices.enqueue(index))
				throw std::runtime_error("failed to free descriptor");
		}
	}

}