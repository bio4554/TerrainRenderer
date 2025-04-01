// ReSharper disable CppClangTidyGoogleRuntimeOperator
#pragma once
#include <mutex>

#include "DM3DResource.h"
#include "DM3DShader.h"
#include <unordered_map>

namespace dm3d
{
	struct PipelineCacheDesc
	{
		Shader* vs = nullptr;
		Shader* gs = nullptr;
		Shader* ps = nullptr;
		Shader* ms = nullptr;
		Shader* as = nullptr;
		Shader* hs = nullptr;
		Shader* ds = nullptr;
		ID3D12RootSignature* rootSig = nullptr;
		D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE;
		BOOL depthEnable = FALSE;
		BOOL stencilEnable = FALSE;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		std::vector<Image*> renderTargets;
		uint32_t numRenderTargets = 0;
		DXGI_FORMAT depthFormat;
		D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
		BOOL depthClipEnable = TRUE;
	};

	class PipelineStateObjectCache
	{
	public:
		PipelineStateObjectCache(ID3D12Device10* device);

		ID3D12PipelineState* get_state(PipelineCacheDesc& desc);

	private:
		template <typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultArg = InnerStructType>
		class alignas(void*) StreamSubObject
		{
		private:
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE pssType;
			InnerStructType pssInner;
		public:
			StreamSubObject() noexcept : pssType(Type), pssInner(DefaultArg()) {}
			StreamSubObject(InnerStructType const& i) noexcept : pssType(Type), pssInner(i) {}
			StreamSubObject& operator=(InnerStructType const& i) noexcept { pssType = Type; pssInner = i; return *this; }
			operator InnerStructType const& () const noexcept { return pssInner; }
			operator InnerStructType& () noexcept { return pssInner; }
			InnerStructType* operator&() noexcept { return &pssInner; }
			InnerStructType const* operator&() const noexcept { return &pssInner; }
		};


		void hash_pointer_combine(size_t& seed, const void* p) const;
		void hash_cull_combine(size_t& seed, const D3D12_CULL_MODE mode) const;
		void hash_int_combine(size_t& seed, const int i) const;
		void hash_top_combine(size_t& seed, const D3D12_PRIMITIVE_TOPOLOGY_TYPE top) const;
		void hash_uint_combine(size_t& seed, const uint32_t i) const;
		void hash_fill_combine(size_t& seed, const D3D12_FILL_MODE mode) const;
		void hash_format_combine(size_t& seed, const DXGI_FORMAT format) const;


		size_t hash_state(const PipelineCacheDesc& desc) const;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> create_state(PipelineCacheDesc& desc) const;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> create_mesh_state(PipelineCacheDesc& desc) const;

		ID3D12Device10* _device;

		std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> _stateCache;

		std::hash<const void*> _pointerHasher;
		std::hash<D3D12_CULL_MODE> _cullModeHasher;
		std::hash<int> _intHasher;
		std::hash<D3D12_PRIMITIVE_TOPOLOGY_TYPE> _topologyHasher;
		std::hash<uint32_t> _uintHasher;
		std::hash<D3D12_FILL_MODE> _fillHasher;
		std::hash<DXGI_FORMAT> _formatHasher;

		std::mutex _lock;
	};
}
