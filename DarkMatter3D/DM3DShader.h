#pragma once
#include <wrl/client.h>

#include <utility>

#include "DM3DEnums.h"

namespace dm3d
{
	class Context;

	class Shader
	{
	public:
		Shader(Microsoft::WRL::ComPtr<ID3DBlob> blob, const ShaderStage stage, Context* context)
		{
			_blob = std::move(blob);
			_stage = stage;
			_context = context;
		}

		~Shader();

		[[nodiscard]] ID3DBlob* get_d3d12_blob() const
		{
			return _blob.Get();
		}

	private:
		Microsoft::WRL::ComPtr<ID3DBlob> _blob;
		ShaderStage _stage;
		Context* _context;
	};
}
