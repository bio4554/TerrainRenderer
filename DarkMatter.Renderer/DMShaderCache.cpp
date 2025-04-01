#include "pch.h"
#include "DMShaderCache.h"

#include "DMUtilities.h"

namespace dm::renderer
{
	ShaderCache::ShaderCache(dm3d::Context* context)
	{
		_context = context;
	}

	std::shared_ptr<dm3d::Shader> ShaderCache::GetShader(const std::string& name, const dm3d::ShaderStage stage)
	{
		if (_shaderCache.contains(name))
		{
			return _shaderCache[name];
		}

		auto shader = CreateShader(name, stage);

		_shaderCache[name] = shader;

		return shader;
	}

	std::shared_ptr<dm3d::Shader> ShaderCache::CreateShader(const std::string& name, const dm3d::ShaderStage stage) const
	{
		auto data = core::utility::ReadBinaryFile(name);

		assert(data.size() != 0);

		auto shader = _context->create_shader(data.data(), data.size(), stage);

		return shader;
	}


}
