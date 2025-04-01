#include "pch.h"
#include "DM3DShader.h"

#include "DM3DContext.h"

namespace dm3d
{
	Shader::~Shader()
	{
		if (_context != nullptr)
		{
			_context->release_shader_internal(this);
		}
	}

}
