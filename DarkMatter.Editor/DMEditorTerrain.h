#pragma once
#include "DMCamera.h"
#include "DMCell.h"
#include "DMLogger.h"

namespace dm::editor
{
	class Editor;

	class TerrainEditor
	{
	public:
		TerrainEditor(Editor* pEditor)
		{
			_editor = pEditor;
		}

		void RenderUI();
		void Tick();
		void SetSelectedCell(model::Cell* pCell);
	private:
		struct TerrainQuad
		{
			glm::vec3 positions[4];
		};

		struct HeightMapRay
		{
			glm::vec3 origin;
			glm::vec3 direction;
		};

		enum class TerrainEditorMode
		{
			None,
			Raise
		};

		std::optional<glm::ivec2> GetHeightMapPoint(core::Camera* camera) const;
		std::vector<glm::ivec2> GetAffectedIndices(glm::ivec2 point, int32_t radius, int32_t max);
		void HandleRaiseLowerTool();

		model::Cell* _selectedCell = nullptr;

		Editor* _editor;
		TerrainEditorMode _mode = TerrainEditorMode::None;
		LoggerContext _log = LoggerContext("TerrainEditor");
	};
}
