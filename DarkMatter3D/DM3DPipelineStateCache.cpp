#include "pch.h"
#include "DM3DPipelineStateCache.h"

#include "DM3DEnumTranslator.h"

namespace dm3d
{
	PipelineStateObjectCache::PipelineStateObjectCache(ID3D12Device10* device)
	{
		_device = device;
		_stateCache = std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>>();
	}

	ID3D12PipelineState* PipelineStateObjectCache::get_state(PipelineCacheDesc& desc)
	{
		auto stateHash = hash_state(desc);
		Microsoft::WRL::ComPtr<ID3D12PipelineState> newState;
		{
			std::unique_lock lock{ _lock };
			auto cached = _stateCache.find(stateHash);
			if (cached != _stateCache.end())
			{
				return cached->second.Get();
			}

			// need to build pso

			// if we have a mesh shader attached, we need to build a special pipeline
			if (desc.ms != nullptr)
			{
				newState = create_mesh_state(desc);
			}
			else
			{
				newState = create_state(desc);
			}

			_stateCache[stateHash] = newState;
		}
		return newState.Get();
	}

	void PipelineStateObjectCache::hash_pointer_combine(size_t& seed, const void* p) const
	{
		seed ^= _pointerHasher(p) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_cull_combine(size_t& seed, const D3D12_CULL_MODE mode) const
	{
		seed ^= _cullModeHasher(mode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_int_combine(size_t& seed, const int i) const
	{
		seed ^= _intHasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_top_combine(size_t& seed, const D3D12_PRIMITIVE_TOPOLOGY_TYPE top) const
	{
		seed ^= _topologyHasher(top) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_uint_combine(size_t& seed, const uint32_t i) const
	{
		seed ^= _uintHasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_fill_combine(size_t& seed, const D3D12_FILL_MODE mode) const
	{
		seed ^= _fillHasher(mode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	void PipelineStateObjectCache::hash_format_combine(size_t& seed, const DXGI_FORMAT format) const
	{
		seed ^= _formatHasher(format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	size_t PipelineStateObjectCache::hash_state(const PipelineCacheDesc& desc) const
	{
		size_t seed = 0;

		hash_pointer_combine(seed, desc.vs);
		hash_pointer_combine(seed, desc.gs);
		hash_pointer_combine(seed, desc.ps);
		hash_pointer_combine(seed, desc.ms);
		hash_pointer_combine(seed, desc.as);
		hash_pointer_combine(seed, desc.hs);
		hash_pointer_combine(seed, desc.ds);
		hash_pointer_combine(seed, desc.rootSig);

		hash_cull_combine(seed, desc.cullMode);
		hash_int_combine(seed, desc.depthEnable);
		hash_int_combine(seed, desc.depthClipEnable);
		hash_int_combine(seed, desc.stencilEnable);
		hash_top_combine(seed, desc.topologyType);

		for (const auto& rt : desc.renderTargets)
		{
			hash_pointer_combine(seed, rt);
		}

		hash_uint_combine(seed, desc.numRenderTargets);

		hash_format_combine(seed, desc.depthFormat);
		//hash_pointer_combine(seed, desc.depthImage);

		hash_fill_combine(seed, desc.fillMode);

		return seed;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObjectCache::create_state(PipelineCacheDesc& desc) const
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = desc.rootSig;
		psoDesc.VS = desc.vs == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.vs->get_d3d12_blob()->GetBufferPointer(), desc.vs->get_d3d12_blob()->GetBufferSize() };
		psoDesc.GS = desc.gs == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.gs->get_d3d12_blob()->GetBufferPointer(), desc.gs->get_d3d12_blob()->GetBufferSize() };
		psoDesc.PS = desc.ps == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.ps->get_d3d12_blob()->GetBufferPointer(), desc.ps->get_d3d12_blob()->GetBufferSize() };
		psoDesc.HS = desc.hs == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.hs->get_d3d12_blob()->GetBufferPointer(), desc.hs->get_d3d12_blob()->GetBufferSize() };
		psoDesc.DS = desc.ds == nullptr ? D3D12_SHADER_BYTECODE() : D3D12_SHADER_BYTECODE{ desc.ds->get_d3d12_blob()->GetBufferPointer(), desc.ds->get_d3d12_blob()->GetBufferSize() };

		psoDesc.RasterizerState = {};
		psoDesc.RasterizerState.FillMode = desc.fillMode;
		psoDesc.RasterizerState.CullMode = desc.cullMode;
		psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = desc.depthClipEnable;
		psoDesc.RasterizerState.MultisampleEnable = FALSE;
		psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		psoDesc.BlendState = {};
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		constexpr D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (auto& i : psoDesc.BlendState.RenderTarget)
			i = defaultRenderTargetBlendDesc;

		psoDesc.DepthStencilState = {};

		psoDesc.DepthStencilState.DepthEnable = desc.depthEnable;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.StencilEnable = desc.stencilEnable;
		psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		constexpr D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
		psoDesc.DepthStencilState.BackFace = defaultStencilOp;

		if (desc.depthEnable)
		{
			psoDesc.DSVFormat = D3D12_Translator::dsv_type_map(desc.depthFormat);
		}

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = desc.topologyType;
		psoDesc.NumRenderTargets = desc.numRenderTargets;

		for (uint32_t i = 0; i < desc.numRenderTargets; i++)
		{
			psoDesc.RTVFormats[i] = desc.renderTargets[i]->get_d3d12_format();
		}

		psoDesc.SampleDesc.Count = 1;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		HRESULT hr = _device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
		if (FAILED(hr))
		{
			throw std::runtime_error("failed to create pso");
		}

		return pso;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObjectCache::create_mesh_state(PipelineCacheDesc& desc) const
	{
		struct MeshPipelineStateStream
		{
			StreamSubObject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> RS;

			StreamSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS> AS;

			StreamSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS> MS;

			StreamSubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> PS;

			StreamSubObject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER> Rasterizer;

			StreamSubObject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND> Blend;

			StreamSubObject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL> DepthStencil;

			StreamSubObject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> DepthFormat;

			StreamSubObject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> RTV;
		};

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		MeshPipelineStateStream stateStream;

		stateStream.RS = desc.rootSig;

		stateStream.MS = D3D12_SHADER_BYTECODE{ desc.ms->get_d3d12_blob()->GetBufferPointer(), desc.ms->get_d3d12_blob()->GetBufferSize() };

		stateStream.PS = D3D12_SHADER_BYTECODE{ desc.ps->get_d3d12_blob()->GetBufferPointer(), desc.ps->get_d3d12_blob()->GetBufferSize() };

		stateStream.AS = desc.as != nullptr ? D3D12_SHADER_BYTECODE{ desc.as->get_d3d12_blob()->GetBufferPointer(), desc.as->get_d3d12_blob()->GetBufferSize() } : D3D12_SHADER_BYTECODE();

		D3D12_RASTERIZER_DESC rasterizerState = {};
		rasterizerState.FillMode = desc.fillMode;
		rasterizerState.CullMode = desc.cullMode;
		rasterizerState.FrontCounterClockwise = FALSE;
		rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerState.DepthClipEnable = desc.depthClipEnable;
		rasterizerState.MultisampleEnable = FALSE;
		rasterizerState.AntialiasedLineEnable = FALSE;
		rasterizerState.ForcedSampleCount = 0;
		rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		stateStream.Rasterizer = rasterizerState;

		D3D12_BLEND_DESC blendState = {};
		blendState.AlphaToCoverageEnable = FALSE;
		blendState.IndependentBlendEnable = FALSE;
		constexpr D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (auto& i : blendState.RenderTarget)
			i = defaultRenderTargetBlendDesc;
		stateStream.Blend = blendState;

		D3D12_DEPTH_STENCIL_DESC depthStencilState = {};

		depthStencilState.DepthEnable = desc.depthEnable;
		depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilState.StencilEnable = desc.stencilEnable;
		depthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		constexpr D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilState.FrontFace = defaultStencilOp;
		depthStencilState.BackFace = defaultStencilOp;
		
		stateStream.DepthStencil = depthStencilState;

		if (desc.depthEnable)
		{
			DXGI_FORMAT dsvFormat = D3D12_Translator::dsv_type_map(desc.depthFormat);
			stateStream.DepthFormat = dsvFormat;
		}

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = desc.numRenderTargets;

		for (uint32_t i = 0; i < desc.numRenderTargets; i++)
		{
			rtvFormats.RTFormats[i] = desc.renderTargets[i]->get_d3d12_format();
		}

		stateStream.RTV = rtvFormats;

		streamDesc.pPipelineStateSubobjectStream = &stateStream;
		streamDesc.SizeInBytes = sizeof(MeshPipelineStateStream);

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		if (HRESULT hr = _device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso)); FAILED(hr))
		{
			throw std::runtime_error("failed to create pso");
		}

		return pso;
	}
}
