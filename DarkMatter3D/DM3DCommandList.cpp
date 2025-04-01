#include "pch.h"
#include "DM3DCommandList.h"

#include <utility>

#include "DM3DEnumTranslator.h"
#include "DM3DInternalUtilities.h"

namespace dm3d
{
	CommandList::CommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> commandList, const Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, PipelineStateObjectCache* psoCache) : _bindingManager(rootSignature, psoCache)
	{
		_commandList = std::move(commandList);
	}

	void CommandList::try_defer_transition(const std::shared_ptr<Resource>& resource, ResourceState desiredState)
	{
		_transitions.emplace_back(resource, desiredState);
	}

	void CommandList::set_vertex(const std::shared_ptr<Shader>& shader)
	{
		_bindingManager.bind_vertex_shader(shader.get());
	}

	void CommandList::set_pixel(const std::shared_ptr<Shader>& shader)
	{
		_bindingManager.bind_pixel_shader(shader.get());
	}

	void CommandList::set_amp(const std::shared_ptr<Shader>& shader)
	{
		_bindingManager.bind_amp_shader(shader.get());
	}

	void CommandList::set_mesh(const std::shared_ptr<Shader>& shader)
	{
		_bindingManager.bind_mesh_shader(shader.get());
	}

	void CommandList::set_geo(const std::shared_ptr<Shader>& shader)
	{
		_bindingManager.bind_geo_shader(shader.get());
	}

	void CommandList::set_depth_clip(const bool enabled)
	{
		_bindingManager.set_depth_clip(enabled);
	}

	void CommandList::set_depth_test_enabled(const bool enabled)
	{
		_bindingManager.set_enable_depth(enabled);
	}

	void CommandList::set_cull_mode(const CullMode mode)
	{
		_bindingManager.set_cull_mode(D3D12_Translator::cull_mode(mode));
	}

	void CommandList::set_draw_extent(const Extent2D extent)
	{
		_bindingManager.set_draw_extent(extent);
	}

	void CommandList::set_fill_mode(const FillMode mode)
	{
		_bindingManager.set_fill_mode(D3D12_Translator::fill_mode(mode));
	}

	void CommandList::set_input_primitive(const InputTopologyType type)
	{
		_bindingManager.set_input_topology(D3D12_Translator::input_topology(type));
	}

	void CommandList::set_num_render_targets(const uint32_t count)
	{
		_bindingManager.set_num_rt(count);
	}

	void CommandList::set_primitive(const TopologyType type)
	{
		_bindingManager.set_topology(D3D12_Translator::topology(type));
	}

	void CommandList::set_stencil_test_enabled(const bool enabled)
	{
		_bindingManager.set_enable_stencil(enabled);
	}

	void CommandList::clear_depth(const float value, const std::shared_ptr<Image>& depth) const
	{
		assert(depth->get_pointer_d3d12_dsv() != nullptr);

		_commandList->ClearDepthStencilView(depth->get_d3d12_dsv(), D3D12_CLEAR_FLAG_DEPTH, value, 0, 0, nullptr);
	}

	void CommandList::clear_image(float rgba[4], const Image* image) const
	{
		assert(image->_rtvDescriptor.has_value());

		_commandList->ClearRenderTargetView(image->get_d3d12_rtv(), rgba, 0, nullptr);
	}

	void CommandList::bind_depth_target(const std::shared_ptr<Image>& target)
	{
		_resourceLocks.push(target);
		_bindingManager.bind_depth_target(target);
	}

	void CommandList::bind_index_buffer(const std::shared_ptr<IndexBuffer>& buffer)
	{
		_resourceLocks.push(buffer);
		_bindingManager.bind_index_buffer(buffer);
	}

	void CommandList::bind_render_target(const uint16_t slot, const std::shared_ptr<Image>& target)
	{
		_resourceLocks.push(target);
		_bindingManager.bind_render_target(slot, target);
	}

	

	void CommandList::draw_indexed(const uint32_t indexCount, const uint32_t firstIndex)
	{
		pre_draw();

		_commandList->DrawIndexedInstanced(indexCount, 1, firstIndex, 0, 0);
	}

	void CommandList::draw_instanced(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t startVertexLocation, const uint32_t startInstanceLocation)
	{
		pre_draw();

		_commandList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void CommandList::dispatch_mesh(const uint32_t x, const uint32_t y, const uint32_t z)
	{
		pre_draw();

		_commandList->DispatchMesh(x, y, z);
	}

	void CommandList::pre_draw()
	{
		_bindingManager.bind_pso_if_invalid(_commandList.Get());
		_bindingManager.bind_shader_resources(_commandList.Get());
	}

	void CommandList::resolve_transitions(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const
	{
		for (auto& transition : _transitions)
		{
			auto desiredState = D3D12_Translator::resource_state(transition.second);
			auto& resource = transition.first;

			if (resource->_currentState != desiredState)
			{
				D3D12_RESOURCE_BARRIER barrier = {};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrier.Transition.pResource = resource->get_d3d12_resource();
				barrier.Transition.StateBefore = resource->_currentState;
				barrier.Transition.StateAfter = desiredState;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				commandList->ResourceBarrier(1, &barrier);

				resource->_currentState = desiredState;
			}
		}
	}

}
