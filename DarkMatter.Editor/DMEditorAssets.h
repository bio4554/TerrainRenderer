#pragma once
#include "DMAssetRegistry.h"

namespace dm::editor
{
	class Editor;

	class AssetEditor
	{
	public:
		AssetEditor(Editor* pEditor);

		void RenderUI();
	private:
		void RenderSelectedTexture();

		Editor* _editor;
		uint32_t _selectedTextureId = 0;
	};
}
