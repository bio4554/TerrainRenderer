#pragma once
#include <memory>
#include <string>
#include <DM3DShader.h>

namespace dm::renderer
{
	class ShaderCache
	{
	public:
		ShaderCache(dm3d::Context* context);

		std::shared_ptr<dm3d::Shader> GetShader(const std::string& name, dm3d::ShaderStage stage);

	private:
		std::shared_ptr<dm3d::Shader> CreateShader(const std::string& name, dm3d::ShaderStage stage) const;

		std::unordered_map<std::string, std::shared_ptr<dm3d::Shader>> _shaderCache;
		dm3d::Context* _context;
	};
}