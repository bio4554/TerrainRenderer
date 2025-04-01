#include "pch.h"
#include "DMEditorDialogImportTexture.h"

#include <filesystem>

#include "DMEditor.h"
#include "imgui.h"

#include <format>

#include "DMTextureTools.h"
#include "DMUtilities.h"
#include "ImGuiFileDialog.h"

namespace dm::editor
{
	void DialogImportTexture::Render()
	{
		ImGui::Begin(_name.c_str());
		if (ImGui::BeginTabBar("ImportTextureTabBar"))
		{
			if (ImGui::BeginTabItem("Raw"))
			{
				_rawImport = true;
				ImGui::Checkbox("Use File Name", &_useFileName);
				if (!_useFileName)
				{
					ImGui::InputText("Texture Name", _nameCharBuffer.data(), 128);
					_textureName = std::string(_nameCharBuffer.data());
				}
				ImGui::Text(std::format("Path: {}", _importPath).c_str());
				if (ImGui::Button("Open File"))
				{
					IGFD::FileDialogConfig config;
					config.path = ".";
					_fileDialog.OpenDialog(_name, "Choose image", ".png", config);
				}
				if (_fileDialog.Display(_name))
				{
					if (_fileDialog.IsOk()) { // action if OK
						std::string filePathName = _fileDialog.GetFilePathName();
						std::string filePath = _fileDialog.GetCurrentPath();
						_importPath = filePathName;
					}

					// close
					_fileDialog.Close();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("DDS"))
			{
				_rawImport = false;
				ImGui::InputText("Relative Path", _ddsPathBuffer.data(), 512);
				_importPath = std::string(_ddsPathBuffer.data());
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		
		if (_importPath != "" && ImGui::Button("OK"))
		{
			ProcessFile();
			Close();
		}
		if (ImGui::Button("Cancel"))
		{
			Close();
		}
		ImGui::End();
	}

	std::string DialogImportTexture::GetDialogName()
	{
		auto id = GetId();
		return std::format("Import Texture##{}", id);
	}

	void DialogImportTexture::ProcessFile()
	{
		try 
		{
			if (_rawImport) 
			{

				auto rawData = core::utility::ReadBinaryFile(_importPath);

				if (_useFileName)
				{
					_textureName = std::filesystem::path(_importPath).stem().string();
				}

				auto textureOpt = asset::TextureTools::ConvertToDDS(rawData.data(), rawData.size(), _textureName, _editor->GetFileSystem(), true, 9);
				if (textureOpt.has_value() == false)
				{
					return;
				}

				auto texture = textureOpt.value();
				auto registry = _editor->GetAssetRegistry();
				auto newId = registry->Allocate();
				texture.SetId(newId);
				registry->Register(texture);
			}
			else
			{
				ProcessDDS();
			}
		}
		catch (std::runtime_error& e)
		{
			_log.error(std::format("Texture import failed with exception: {}", e.what()));
		}
		catch (...)
		{
			_log.error("Texture import failed with unhandled exception.");
		}
	}

	void DialogImportTexture::ProcessDDS()
	{
		try
		{
			auto fileSystem = _editor->GetFileSystem();
			if (!fileSystem->FileExists(_importPath))
			{
				_log.error("Failed to import DDS, invalid relative path: " + _importPath);
				return;
			}

			auto textureOpt = asset::TextureTools::LoadDDS(_importPath, fileSystem);

			if (!textureOpt.has_value())
			{
				_log.error("Failed to import DDS, DDS load failed: " + _importPath);
				return;
			}

			auto texture = textureOpt.value();
			auto registry = _editor->GetAssetRegistry();
			auto newId = registry->Allocate();
			texture.SetId(newId);
			registry->Register(texture);
		}
		catch (std::runtime_error& e)
		{
			_log.error(std::format("Texture import failed with exception: {}", e.what()));
		}
		catch (...)
		{
			_log.error("Texture import failed with unhandled exception.");
		}
	}

}
