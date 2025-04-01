#pragma once
#include "DM3DConstants.h"
#include "DM3DTypes.h"
#include "DM3DPipelineStateCache.h"

namespace dm3d
{
	enum PSODirtyFlags
	{
		Clean = 0,
		VertexShader = 1,
		PixelShader = 2,
		GeoShader = 4,
		RootConstant = 8,
		SamplerBuffers = 16,
		RenderTargets = 32,
		DepthTest = 64,
		Stencil = 128,
		Topology = 256,
		CullModeState = 512,
		IndexBufferDirty = 1024,
		InputPrimitive = 2048,
		DrawExtent = 4096,
		FillModeState = 8192,
		ConstantBuffers = 16384,
		DepthBuffers = 32768,
		MeshShader = 65536,
		AmpShader = 131072
	};

#define ANY_SHADER_DIRTY (VertexShader | GeoShader | PixelShader | MeshShader | AmpShader)

#define ANY_STATE_DIRTY (VertexShader | GeoShader | PixelShader | MeshShader | AmpShader | RenderTargets | DepthTest | Stencil | Topology | CullModeState | InputPrimitive | FillModeState | DepthBuffers | DrawExtent)

#define ANY_RESOURCE_DIRTY (RootConstant | SamplerBuffers | IndexBufferDirty | ConstantBuffers)

	class ResourceBindingManager
	{
	public:
		ResourceBindingManager(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature, PipelineStateObjectCache* psoCache);

		void bind_vertex_shader(Shader* shader);
		void bind_geo_shader(Shader* shader);
		void bind_pixel_shader(Shader* shader);
		void bind_mesh_shader(Shader* shader);
		void bind_amp_shader(Shader* shader);
		void bind_render_target(uint16_t slot, std::shared_ptr<Image> target);
		void bind_depth_target(std::shared_ptr<Image> target);
		void bind_index_buffer(std::shared_ptr<IndexBuffer> buffer);

		void set_enable_depth(bool enabled);
		void set_enable_stencil(bool enabled);
		void set_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
		void set_input_topology(D3D_PRIMITIVE_TOPOLOGY type);
		void set_num_rt(uint32_t num);
		void set_cull_mode(D3D12_CULL_MODE mode);
		void set_draw_extent(Extent2D extent);
		void set_fill_mode(D3D12_FILL_MODE mode);
		void set_depth_clip(bool enabled);

		void bind_shader_resources(ID3D12GraphicsCommandList* cmd);
		void bind_pso_if_invalid(ID3D12GraphicsCommandList* cmd, bool onlyRoot = false);

		template<typename T>
		void bind_resource_table(uint8_t slot, T& table)
		{
			auto dataSize = sizeof(T);

			if (slot > constants::NumResourceTables)
			{
				throw std::runtime_error("Resource table slot is greater than constants::NumResourceTables");
			}

			if (dataSize % 4 != 0)
			{
				throw std::runtime_error("Resource table size in bytes must be a multiple of 4");
			}

			_boundResourceTables[slot].pData = &table;
			_boundResourceTables[slot].numConstants = static_cast<uint32_t>(dataSize) / 4u;
			_rootConstantDirty[slot] = true;
		}
	private:
		struct BoundResourceTable
		{
			void* pData;
			uint32_t numConstants;
		};

		bool is_dirty(uint32_t flag) const
		{
			return (_dirtyFlags & flag) == flag;
		}

		static void bind_view_scissor(ID3D12GraphicsCommandList* cmd, Extent2D viewport, Extent2D scissor);

		static constexpr uint16_t _maxRtvs = 8;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
		PipelineStateObjectCache* _psoCache;
		PipelineCacheDesc _stateDesc;
		uint32_t _dirtyFlags = 0;
		bool _rootConstantDirty[constants::NumResourceTables];

		std::shared_ptr<Image> _boundRtvs[_maxRtvs];
		std::shared_ptr<Image> _boundDsv;
		std::shared_ptr<IndexBuffer> _boundIndexBuffer;
		D3D_PRIMITIVE_TOPOLOGY _boundInputTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		BoundResourceTable _boundResourceTables[constants::NumResourceTables];
		Extent2D _viewport;
	};
}
