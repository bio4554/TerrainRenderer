#pragma once
#include <memory>
#include <queue>
#include <vector>
#include <wrl/client.h>

#include "DM3DEnums.h"
#include "DM3DResource.h"
#include "DM3DShader.h"
#include "DM3DTypes.h"
#include "DM3DPipelineStateCache.h"
#include "DM3DResourceBindingManager.h"

namespace dm3d
{
	class CommandList
	{
		friend class Context;
	public:
		CommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> commandList, const Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature, PipelineStateObjectCache* psoCache);

		void try_defer_transition(const std::shared_ptr<Resource>& resource, ResourceState desiredState);

		// shader binding
		void set_vertex(const std::shared_ptr<Shader>& shader);
		void set_pixel(const std::shared_ptr<Shader>& shader);
		void set_amp(const std::shared_ptr<Shader>& shader);
		void set_mesh(const std::shared_ptr<Shader>& shader);
		void set_geo(const std::shared_ptr<Shader>& shader);

		// set state
		void set_depth_clip(bool enabled);
		void set_depth_test_enabled(bool enabled);
		void set_cull_mode(CullMode mode);
		void set_draw_extent(Extent2D extent);
		void set_fill_mode(FillMode mode);
		void set_input_primitive(InputTopologyType type);
		void set_num_render_targets(uint32_t count);
		void set_primitive(TopologyType type);
		void set_stencil_test_enabled(bool enabled);

		// clear images
		void clear_depth(float value, const std::shared_ptr<Image>& depth) const;
		void clear_image(float rgba[4], const Image* image) const;

		// resource binding
		void bind_depth_target(const std::shared_ptr<Image>& target);
		void bind_index_buffer(const std::shared_ptr<IndexBuffer>& buffer);
		void bind_render_target(uint16_t slot, const std::shared_ptr<Image>& target);

		// drawing
		void draw_indexed(uint32_t indexCount, uint32_t firstIndex);
		void draw_instanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);
		void dispatch_mesh(uint32_t x, uint32_t y, uint32_t z);

		template<typename T>
		void set_resource_table(const uint32_t slot, T& buffer)
		{
			_bindingManager.bind_resource_table(static_cast<uint8_t>(slot), buffer);
		}
	private:
		void pre_draw();
		void resolve_transitions(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> _commandList;
		std::vector<std::pair<std::shared_ptr<Resource>, ResourceState>> _transitions;
		ResourceBindingManager _bindingManager;
		std::queue<std::shared_ptr<dm3d::Resource>> _resourceLocks;
	};
}
