#include "pch.h"
#include "DM3DResourceBindingManager.h"

#include <cassert>

namespace dm3d
{
	ResourceBindingManager::ResourceBindingManager(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, PipelineStateObjectCache* psoCache)
	{
		_rootSignature = rootSignature;
		_psoCache = psoCache;
		_dirtyFlags = ANY_STATE_DIRTY | ANY_RESOURCE_DIRTY;
		_stateDesc = {};
		_stateDesc.rootSig = _rootSignature.Get();
		for (uint32_t i = 0; i < constants::NumResourceTables; i++)
		{
			_boundResourceTables[i] = {};
			_rootConstantDirty[i] = false;
		}
	}

	void ResourceBindingManager::bind_vertex_shader(Shader* shader)
	{
		if (_stateDesc.vs == shader)
		{
			return;
		}

		_stateDesc.vs = shader;
		_dirtyFlags |= VertexShader;
	}

	void ResourceBindingManager::bind_geo_shader(Shader* shader)
	{
		if (_stateDesc.gs == shader)
		{
			return;
		}

		_stateDesc.gs = shader;
		_dirtyFlags |= GeoShader;
	}

	void ResourceBindingManager::bind_pixel_shader(Shader* shader)
	{
		if (_stateDesc.ps == shader)
		{
			return;
		}

		_stateDesc.ps = shader;
		_dirtyFlags |= PixelShader;
	}

	void ResourceBindingManager::bind_mesh_shader(Shader* shader)
	{
		if (_stateDesc.ms == shader)
		{
			return;
		}

		_stateDesc.ms = shader;
		_dirtyFlags |= MeshShader;
	}

	void ResourceBindingManager::bind_amp_shader(Shader* shader)
	{
		if (_stateDesc.as == shader)
		{
			return;
		}

		_stateDesc.as = shader;
		_dirtyFlags |= AmpShader;
	}

	void ResourceBindingManager::bind_render_target(uint16_t slot, std::shared_ptr<Image> target)
	{
		assert(slot < _maxRtvs);

		_dirtyFlags |= RenderTargets;
		_boundRtvs[slot] = target;
	}

	void ResourceBindingManager::bind_depth_target(std::shared_ptr<Image> target)
	{
		_dirtyFlags |= DepthBuffers;

		_boundDsv = target;
		_stateDesc.depthFormat = target->get_d3d12_format();
	}

	void ResourceBindingManager::bind_index_buffer(std::shared_ptr<IndexBuffer> buffer)
	{
		_boundIndexBuffer = buffer;
		_dirtyFlags |= IndexBufferDirty;
	}

	

	void ResourceBindingManager::set_enable_depth(bool enabled)
	{
		_stateDesc.depthEnable = enabled ? 1 : 0;
		_dirtyFlags |= DepthTest;
	}

	void ResourceBindingManager::set_enable_stencil(bool enabled)
	{
		_stateDesc.stencilEnable = enabled ? 1 : 0;
		_dirtyFlags |= Stencil;
	}

	void ResourceBindingManager::set_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
	{
		_stateDesc.topologyType = type;

		_dirtyFlags |= PSODirtyFlags::Topology;
	}

	void ResourceBindingManager::set_input_topology(D3D_PRIMITIVE_TOPOLOGY type)
	{
		_boundInputTopology = type;

		_dirtyFlags |= PSODirtyFlags::InputPrimitive;
	}

	void ResourceBindingManager::set_num_rt(uint32_t num)
	{
		_stateDesc.numRenderTargets = num;

		if (_stateDesc.renderTargets.size() < num)
		{
			_stateDesc.renderTargets.resize(num);
		}

		for (uint32_t i = 0; i < num; i++)
		{
			_stateDesc.renderTargets[i] = _boundRtvs[i].get();
		}

		_dirtyFlags |= PSODirtyFlags::RenderTargets;
	}

	void ResourceBindingManager::set_cull_mode(D3D12_CULL_MODE mode)
	{
		_stateDesc.cullMode = mode;

		_dirtyFlags |= PSODirtyFlags::CullModeState;
	}

	void ResourceBindingManager::set_draw_extent(Extent2D extent)
	{
		_viewport = extent;

		_dirtyFlags |= PSODirtyFlags::DrawExtent;
	}

	void ResourceBindingManager::set_fill_mode(D3D12_FILL_MODE mode)
	{
		_stateDesc.fillMode = mode;

		_dirtyFlags |= PSODirtyFlags::FillModeState;
	}

	void ResourceBindingManager::set_depth_clip(bool enabled)
	{
		_stateDesc.depthClipEnable = enabled ? TRUE : FALSE;

		_dirtyFlags |= PSODirtyFlags::DepthTest;
	}

	void ResourceBindingManager::bind_shader_resources(ID3D12GraphicsCommandList* cmd)
	{
		if (is_dirty(DrawExtent))
		{
			bind_view_scissor(cmd, _viewport, _viewport);

			_dirtyFlags ^= DrawExtent;
		}

		if (is_dirty(RenderTargets))
		{
			// todo maybe validate resource state?

			auto dsv = _boundDsv;

			auto rtvDescriptors = std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(_stateDesc.numRenderTargets);

			for (uint32_t i = 0; i < _stateDesc.numRenderTargets; i++)
			{
				rtvDescriptors[i] = _boundRtvs[i]->get_d3d12_rtv();
			}

			cmd->OMSetRenderTargets(_stateDesc.numRenderTargets, rtvDescriptors.data(), FALSE, dsv != nullptr ? dsv->get_pointer_d3d12_dsv() : nullptr);

			_dirtyFlags ^= RenderTargets;
		}

		if (is_dirty(IndexBufferDirty))
		{
			if (_boundIndexBuffer != nullptr)
			{
				cmd->IASetIndexBuffer(_boundIndexBuffer->get_pointer_view());
			}

			_dirtyFlags ^= IndexBufferDirty;
		}

		if (is_dirty(InputPrimitive))
		{
			cmd->IASetPrimitiveTopology(_boundInputTopology);

			_dirtyFlags ^= InputPrimitive;
		}

		for (uint32_t i = 0; i < constants::NumResourceTables; i++)
		{
			if (_rootConstantDirty[i])
			{
				auto [pData, dataSize] = _boundResourceTables[i];
				cmd->SetGraphicsRoot32BitConstants(i, dataSize, pData, 0);

				_rootConstantDirty[i] = false;
			}
		}
	}

	void ResourceBindingManager::bind_view_scissor(ID3D12GraphicsCommandList* cmd, Extent2D viewport, Extent2D scissor)
	{
		D3D12_VIEWPORT vp = {};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(viewport.width);
		vp.Height = static_cast<float>(viewport.height);
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;
		cmd->RSSetViewports(1, &vp);

		D3D12_RECT scissorRect = {};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = static_cast<LONG>(scissor.width);
		scissorRect.bottom = static_cast<LONG>(scissor.height);

		cmd->RSSetScissorRects(1, &scissorRect);
	}

	void ResourceBindingManager::bind_pso_if_invalid(ID3D12GraphicsCommandList* cmd, bool onlyRoot)
	{
		if ((_dirtyFlags & ANY_STATE_DIRTY) != 0)
		{
			if (!onlyRoot)
			{
				auto pso = _psoCache->get_state(_stateDesc);
				cmd->SetPipelineState(pso);
			}
			cmd->SetGraphicsRootSignature(_rootSignature.Get());

			auto rtvDirty = _dirtyFlags & PSODirtyFlags::RenderTargets;
			auto inputTopDirty = _dirtyFlags & PSODirtyFlags::InputPrimitive;
			auto viewportDirty = _dirtyFlags & PSODirtyFlags::DrawExtent;
			auto depthDirty = _dirtyFlags & PSODirtyFlags::DepthBuffers;
			auto constantDirty = _dirtyFlags & RootConstant;

			_dirtyFlags ^= ANY_STATE_DIRTY;

			// OR with this so that the resource binding stage can know if RTVs need to be bound.
			_dirtyFlags |= rtvDirty;

			// same with input topology
			_dirtyFlags |= inputTopDirty;

			// also viewport
			_dirtyFlags |= viewportDirty;

			// also depth...
			_dirtyFlags |= depthDirty;

			_dirtyFlags |= constantDirty;
		}
	}

}
