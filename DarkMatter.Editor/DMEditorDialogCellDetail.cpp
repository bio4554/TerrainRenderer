#include "pch.h"
#include "DMEditorDialogCellDetail.h"

#include "imgui.h"

namespace dm::editor
{
	void DialogCellDetail::Render()
	{
		/*ImGui::Begin(std::format("Cell Detail##{}", GetId()).c_str());
		if (ImGui::BeginTabBar("CellDetailTabBar"))
		{
			if (ImGui::BeginTabItem("Texture Splats"))
			{
				int splat1, splat2, splat3, splat4;
				splat1 = static_cast<int>(_cell->GetTerrainTexture(0));
				splat2 = static_cast<int>(_cell->GetTerrainTexture(1));
				splat3 = static_cast<int>(_cell->GetTerrainTexture(2));
				splat4 = static_cast<int>(_cell->GetTerrainTexture(3));

				ImGui::InputInt("Texture #1", &splat1);
				ImGui::InputInt("Texture #2", &splat2);
				ImGui::InputInt("Texture #3", &splat3);
				ImGui::InputInt("Texture #4", &splat4);

				if (splat1 != static_cast<int>(_cell->GetTerrainTexture(0)))
				{
					_cell->SetTerrainTexture(splat1, 0);
				}
				if (splat2 != static_cast<int>(_cell->GetTerrainTexture(1)))
				{
					_cell->SetTerrainTexture(splat2, 1);
				}
				if (splat3 != static_cast<int>(_cell->GetTerrainTexture(2)))
				{
					_cell->SetTerrainTexture(splat3, 2);
				}
				if (splat4 != static_cast<int>(_cell->GetTerrainTexture(3)))
				{
					_cell->SetTerrainTexture(splat4, 3);
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();*/
	}

}
