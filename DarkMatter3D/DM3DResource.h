#pragma once
#include <optional>
#include <stdexcept>

#include "DM3DDescriptorAllocator.h"

namespace dm3d
{
	class Resource
	{
		friend class Context;
		friend class CommandList;

	public:
		Resource(D3D12MA::Allocation* alloc, ID3D12Resource* resource, dm3d::Context* context, const D3D12_RESOURCE_DESC& resourceDesc, const std::string& name);

		~Resource();

		uint32_t get_structured_index() const
		{
			if (!_srvDescriptor.has_value())
			{
				throw std::runtime_error("gpu srv handle not valid");
			}

			return _srvDescriptor.value().idx;
		}

		uint32_t get_constant_index() const
		{
			if (!_cbvDescriptor.has_value())
			{
				throw std::runtime_error("gpu cbv handle not valid");
			}

			return _cbvDescriptor.value().idx;
		}

	protected:
		ID3D12Resource* get_d3d12_resource() const
		{
			return _d3d12Resource;
		}

		D3D12MA::Allocation* get_d3d12_allocation() const
		{
			return _allocation;
		}

		D3D12MA::Allocation* _allocation;
		std::optional<DarkDescriptorPair> _srvDescriptor;
		std::optional<DarkDescriptorPair> _rtvDescriptor;
		std::optional<DarkDescriptorPair> _dsvDescriptor;
		std::optional<DarkDescriptorPair> _cbvDescriptor;
		ID3D12Resource* _d3d12Resource;
		D3D12_RESOURCE_DESC _resourceDesc;
		D3D12_RESOURCE_STATES _currentState;
		Context* _context;
		std::string _name;
	};

	class Buffer : public Resource
	{
	public:
		Buffer(size_t size, D3D12MA::Allocation* alloc, ID3D12Resource* resource, Context* context, D3D12_RESOURCE_DESC resourceDesc, D3D12_HEAP_TYPE heapType, std::string name) : Resource(alloc, resource, context, resourceDesc, name)
		{
			_size = size;
			_heapType = heapType;
		}

		size_t get_size() const
		{
			return _size;
		}

		void* map()
		{
			if (_pData != nullptr)
				return _pData;

			assert(_heapType == D3D12_HEAP_TYPE_UPLOAD);

			_d3d12Resource->Map(0, nullptr, &_pData);

			return _pData;
		}

		void unmap()
		{
			assert(_pData != nullptr);

			_d3d12Resource->Unmap(0, nullptr);

			_pData = nullptr;
		}

	protected:
		size_t _size;
		void* _pData = nullptr;
		D3D12_HEAP_TYPE _heapType;
	};

	class Image : public Resource
	{
	public:
		Image(uint32_t width, uint32_t height, uint32_t depth, D3D12MA::Allocation* allocation, ID3D12Resource* resource, DXGI_FORMAT format, D3D12_RESOURCE_DESC resourceDesc, Context* context, std::string name) : Resource(allocation, resource, context, resourceDesc, name)
		{
			_width = width;
			_height = height;
			_depth = depth;
			_format = format;
		}

		uint32_t get_width() const
		{
			return _width;
		}

		uint32_t get_height() const
		{
			return _height;
		}

		uint32_t get_depth() const
		{
			return _depth;
		}

		DXGI_FORMAT get_d3d12_format() const
		{
			return _format;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_rtv() const
		{
			if (!_rtvDescriptor.has_value())
			{
				throw std::runtime_error("image didn't have an rtv handle");
			}

			return _rtvDescriptor.value().cpuHandle;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* get_pointer_d3d12_dsv() const
		{
			if (!_dsvDescriptor.has_value())
			{
				throw std::runtime_error("image didn't have an rtv handle");
			}

			return &_dsvDescriptor.value().cpuHandle;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE get_d3d12_dsv() const
		{
			if (!_dsvDescriptor.has_value())
			{
				throw std::runtime_error("image didn't have an rtv handle");
			}

			return _dsvDescriptor.value().cpuHandle;
		}

	private:
		DXGI_FORMAT _format;
	protected:
		uint32_t _width;
		uint32_t _height;
		uint32_t _depth;
	};

	class IndexBuffer : public Resource
	{
	public:
		IndexBuffer(D3D12MA::Allocation* alloc, D3D12_INDEX_BUFFER_VIEW view, ID3D12Resource* resource, Context* context, std::string name) : Resource(alloc, resource, context, D3D12_RESOURCE_DESC(), name)
		{
			_view = view;
		}

		const D3D12_INDEX_BUFFER_VIEW* get_pointer_view() const
		{
			return &_view;
		}

	private:
		D3D12_INDEX_BUFFER_VIEW _view;
	};
}
