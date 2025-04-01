#include "pch.h"
#include <format>
#include "imgui.h"
#include "DMEditor.h"
#include "DMEditorTerrain.h"
#include "DMInputSystem.h"


namespace dm::editor
{
	void TerrainEditor::RenderUI()
	{
		ImGui::Begin("Terrain editor");
		if (ImGui::RadioButton("None", _mode == TerrainEditorMode::None))
		{
			_mode = TerrainEditorMode::None;
		}
		if (ImGui::RadioButton("Raise/Lower", _mode == TerrainEditorMode::Raise))
		{
			_mode = TerrainEditorMode::Raise;
		}
		ImGui::End();

		auto activeCamera = std::dynamic_pointer_cast<core::Camera>(
			_editor->GetWorld()->globalObjectStore[_editor->GetWorld()->activeCamera]);
	}

	void TerrainEditor::Tick()
	{
		if (_mode == TerrainEditorMode::None)
		{
			return;
		}

		switch (_mode)
		{
		case TerrainEditorMode::None:
			break;
		case TerrainEditorMode::Raise:
			HandleRaiseLowerTool();
			break;
		}
	}

	void TerrainEditor::HandleRaiseLowerTool()
	{
		auto& heightMap = _editor->GetWorld()->terrainHeightMap;
		heightMap.ClearOverlay({0, 0, 0, 0});
		heightMap.overlayDirty = true;

		auto heightMapPointOpt = GetHeightMapPoint(
			std::dynamic_pointer_cast<core::Camera>(
				_editor->GetWorld()->globalObjectStore[_editor->GetWorld()->activeCamera]).get());

		if (heightMapPointOpt.has_value() == false)
		{
			return;
		}

		auto heightMapPoint = heightMapPointOpt.value();
		auto heightMapWidth = heightMap.GetWidth();
		auto& overlayData = heightMap.GetOverlayData();
		int radius = 20;

		auto affectedPoints = GetAffectedIndices(heightMapPoint, radius, static_cast<int32_t>(heightMapWidth));

		for (const auto& affectedPoint : affectedPoints)
		{
			overlayData[affectedPoint.y][affectedPoint.x] = { 255, 0, 0, 1 };
		}
	}


	void TerrainEditor::SetSelectedCell(model::Cell* pCell)
	{
		_selectedCell = pCell;
	}

	std::optional<glm::ivec2> TerrainEditor::GetHeightMapPoint(core::Camera* camera) const
	{
		auto heightMap = _editor->GetWorld()->terrainHeightMap;
		auto mousePos = core::InputSystem::inputState.mousePosLocal;
		auto screenSize = _editor->GetScreenSize();

		HeightMapRay ray;
		{
			auto mouseNdcX = 2.f * mousePos.x / screenSize.width - 1.f;
			auto mouseNdcY = 1.f - 2.f * mousePos.y / screenSize.height;
			auto clipCoord = glm::vec4(mouseNdcX, mouseNdcY, 0.f, 1.f);
			auto invProj = glm::inverse(camera->GetProjectionMatrix());
			auto eyeCoord = invProj * clipCoord;
			eyeCoord.z = -1.f;
			eyeCoord.w = 0.f;
			auto invView = glm::inverse(camera->GetViewMatrix());
			auto worldRay = invView * eyeCoord;
			ray.origin = camera->position;
			ray.direction = glm::normalize(glm::vec3(worldRay));
		}


		float t = 0.0f;
		constexpr float stepSize = 2.5f;
		constexpr float tMax = 5120.f;
		bool hit = false;

		uint64_t rayMarchIterations = 0;

		while (t < tMax)
		{
			rayMarchIterations++;
			glm::vec3 pos = ray.origin + ray.direction * t;
			float terrainHeight = heightMap.GetHeight(pos.x, pos.z);

			if (pos.y < terrainHeight)
			{
				hit = true;
				break;
			}
			t += stepSize;
		}

		if (!hit)
			return std::optional<glm::ivec2>();

		float tLow = t - stepSize;
		float tHigh = t;
		constexpr int maxIterations = 10;

		for (int i = 0; i < maxIterations; ++i)
		{
			float tMid = (tLow + tHigh) * 0.5f;
			glm::vec3 posMid = ray.origin + ray.direction * tMid;
			float terrainHeight = heightMap.GetHeight(posMid.x, posMid.z);

			if (posMid.y < terrainHeight)
				tHigh = tMid;
			else
				tLow = tMid;
		}

		glm::vec3 hitWorldPos = ray.origin + ray.direction * tHigh;

		float u = hitWorldPos.x / 5120.0f;
		float v = hitWorldPos.z / 5120.0f;

		int texX = static_cast<int>(std::round(u * (1024 - 1)));
		int texZ = static_cast<int>(std::round(v * (1024 - 1)));

		auto outHitTexCoord = glm::ivec2(texX, texZ);

		ImGui::Begin("Ray cast terrain stats");
		ImGui::Text(std::format("Iterations: {}", rayMarchIterations).c_str());
		ImGui::Text(std::format("X: {}, Y: {}", outHitTexCoord.x, outHitTexCoord.y).c_str());
		ImGui::Text(std::format("Raw: X: {}, Y: {}", u * 1023, v * 1023).c_str());
		ImGui::End();

		if (outHitTexCoord.x >= 0 && outHitTexCoord.x < heightMap.GetWidth() && outHitTexCoord.y >= 0 &&
			outHitTexCoord.y < heightMap.GetWidth())
		{
			return glm::ivec2{outHitTexCoord.x, outHitTexCoord.y};
		}
		return std::optional<glm::ivec2>();
	}

	std::vector<glm::ivec2> TerrainEditor::GetAffectedIndices(glm::ivec2 point, int32_t radius, int32_t max)
	{
		std::vector<glm::ivec2> output;

		for (int x = point.x - radius; x <= point.x + radius; x++)
		{
			for (int y = point.y - radius; y <= point.y + radius; y++)
			{
				if (x < 0 || x >= max || y < 0 || y >= max)
					continue;
				int dx = x - point.x;
				int dy = y - point.y;

				if (dx * dx + dy * dy <= radius * radius)
					output.push_back({ x,y });
			}
		}

		return output;
	}

}
