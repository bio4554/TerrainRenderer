#include "pch.h"

#include "DMEditorAssets.h"

#include <format>
#include <ranges>

#include "DMEditor.h"
#include "DMEditorDialogImportTexture.h"
#include "imgui.h"

namespace dm::editor
{
	AssetEditor::AssetEditor(Editor* pEditor)
	{
		_editor = pEditor;
	}

	void AssetEditor::RenderUI()
	{
		ImGui::Begin("Assets");
		ImGui::BeginTabBar("AssetsTabs");
		if (ImGui::BeginTabItem("Textures"))
		{
			auto& textures = _editor->GetAssetRegistry()->GetTextures();
			if (ImGui::Button("Import Texture"))
			{
				_editor->OpenDialog<DialogImportTexture>();
			}
			if (ImGui::BeginListBox("TexturesListBox")) {
				for (const auto& texture : textures | std::views::values)
				{
					if (ImGui::Selectable(texture.GetPath().c_str(), texture.GetId() == _selectedTextureId))
					{
						_selectedTextureId = texture.GetId();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();

		if (_editor->GetAssetRegistry()->GetTextures().size() > 0)
		{
			RenderSelectedTexture();
		}

		ImGui::End();
	}

	void AssetEditor::RenderSelectedTexture()
	{
		auto texture = _editor->GetAssetRegistry()->GetTexture(_selectedTextureId);

		ImGui::Text(std::format("Texture ID: {}", texture.GetId()).c_str());
		ImGui::Text(std::format("Path: {}", texture.GetPath()).c_str());
		ImGui::Text(std::format("Size: {}", texture.GetSize()).c_str());
		ImGui::Text(std::format("Format: {}", (int)texture.GetFormat()).c_str());
		ImGui::Text(std::format("Extent: {}x{}x{}", texture.GetExtent().width, texture.GetExtent().height, texture.GetExtent().depth).c_str());
	}

}
