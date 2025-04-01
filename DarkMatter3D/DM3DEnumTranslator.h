#pragma once

#include <stdexcept>

#include "DM3DEnums.h"

namespace dm3d
{
	namespace D3D12_Translator
	{
		inline DXGI_FORMAT dsv_type_map(DXGI_FORMAT imageFormat)
		{
			switch (imageFormat)
			{
			case DXGI_FORMAT_R32_TYPELESS:
				return DXGI_FORMAT_D32_FLOAT;
			default:
				return imageFormat;
			}
		}

		inline DXGI_FORMAT srv_type_map(DXGI_FORMAT imageFormat)
		{
			switch (imageFormat)
			{
			case DXGI_FORMAT_R32_TYPELESS:
				return DXGI_FORMAT_R32_FLOAT;
			default:
				return imageFormat;
			}
		}

		inline DXGI_FORMAT format(ImageFormat format)
		{
			switch (format)
			{
			case R8G8B8A8_UNORM:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case R32G32B32A32_FLOAT:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case R16G16B16A16_FLOAT:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case R32_TYPELESS:
				return DXGI_FORMAT_R32_TYPELESS;
			case D32_FLOAT:
				return DXGI_FORMAT_D32_FLOAT;
			case R32_UINT:
				return DXGI_FORMAT_R32_UINT;
			case R32_FLOAT:
				return DXGI_FORMAT_R32_FLOAT;
			}

			throw std::runtime_error("out of range");
		}

		inline D3D12_RESOURCE_FLAGS resource_flags(ResourceFlags flags)
		{
			D3D12_RESOURCE_FLAGS res = D3D12_RESOURCE_FLAG_NONE;

			if ((flags & ResourceFlags::DepthStencil) == DepthStencil)
			{
				res |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			}

			if ((flags & ResourceFlags::RenderTarget) == RenderTarget)
			{
				res |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			}

			if ((flags & ResourceFlags::UnorderedAccess) == UnorderedAccess)
			{
				res |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			return res;
		}

		inline D3D12_RESOURCE_STATES resource_state(ResourceState state)
		{
			switch (state)
			{
			case ResourceState::Unknown:
				return D3D12_RESOURCE_STATE_COMMON;
			case ResourceState::ShaderRead:
				return D3D12_RESOURCE_STATE_GENERIC_READ;
			case ResourceState::DepthWrite:
				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			case ResourceState::RenderTarget:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case ResourceState::CopySrc:
				return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case ResourceState::CopyDst:
				return D3D12_RESOURCE_STATE_COPY_DEST;
			case ResourceState::Present:
				return D3D12_RESOURCE_STATE_PRESENT;
			}

			throw std::runtime_error("out of range");
		}

		inline uint32_t format_stride(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return 4;
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				return 16;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
				return 8;
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_R32_FLOAT:
				return 4;
			default:
				throw std::runtime_error("out of range");
			}
		}

		inline D3D12_PRIMITIVE_TOPOLOGY_TYPE topology(TopologyType type)
		{
			switch (type)
			{
			case Triangle:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			case Line:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case Patch:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			}

			throw std::runtime_error("out of range");
		}

		inline D3D_PRIMITIVE_TOPOLOGY input_topology(InputTopologyType type)
		{
			switch (type)
			{
			case TriangleList:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case LineList:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case PatchList3:
				return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			case PatchList4:
				return D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
			}

			throw std::runtime_error("out of range");
		}

		inline D3D12_FILL_MODE fill_mode(FillMode mode)
		{
			switch (mode)
			{
			case Solid:
				return D3D12_FILL_MODE_SOLID;
			case Wireframe:
				return D3D12_FILL_MODE_WIREFRAME;
			}

			throw std::runtime_error("out of range");
		}

		inline D3D12_CULL_MODE cull_mode(CullMode mode)
		{
			switch (mode)
			{
			case CullBack:
				return D3D12_CULL_MODE_BACK;
			case CullFront:
				return D3D12_CULL_MODE_FRONT;
			case CullNone:
				return D3D12_CULL_MODE_NONE;
			}

			throw std::runtime_error("out of range");
		}
	}
}
