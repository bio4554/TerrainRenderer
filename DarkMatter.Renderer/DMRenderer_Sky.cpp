#include "pch.h"
#include "DMRenderer.h"

namespace dm::renderer
{
	void Renderer::RenderSky(model::WorldModel* pWorld)
	{
		auto cmd = _context->allocate_command_list();

		auto vertexShader = _shaderCache->GetShader("FullQuadVertexShader.cso", dm3d::ShaderStage::Vertex);
		auto pixelShader = _shaderCache->GetShader("SkyPixelShader.cso", dm3d::ShaderStage::Pixel);

		// Get a reference to the current back buffer
		auto backBuffer = _context->get_back_buffer();

		// Transition used resources to the correct states
		cmd->try_defer_transition(backBuffer, dm3d::ResourceState::RenderTarget);

		// Setup GPU state, including render target
		cmd->bind_render_target(0, backBuffer);
		cmd->set_num_render_targets(1);
		cmd->set_fill_mode(dm3d::Solid);
		cmd->set_input_primitive(dm3d::TriangleList);
		cmd->set_primitive(dm3d::Triangle);
		cmd->set_cull_mode(dm3d::CullNone);
		cmd->set_depth_test_enabled(false);
		cmd->set_draw_extent(_context->get_current_draw_extent());

		cmd->set_vertex(vertexShader);
		cmd->set_pixel(pixelShader);

		cmd->draw_instanced(3, 1, 0, 0);

		_context->submit_list(std::move(cmd));
	}
}