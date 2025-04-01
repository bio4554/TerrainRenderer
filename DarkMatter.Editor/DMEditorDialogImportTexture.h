#pragma once

#include <atomic>
#include <string>

#include "DMEditorDialog.h"
#include "DMLogger.h"
#include "ImGuiFileDialog.h"

namespace dm::editor
{
	class DialogImportTexture : public Dialog
	{
	public:
		DialogImportTexture(Editor* editor) : Dialog(editor)
		{
			_name = GetDialogName();
			_nameCharBuffer = std::vector<char>(128);
			_nameCharBuffer[0] = '\0';
			_ddsPathBuffer = std::vector<char>(512);
			_ddsPathBuffer[0] = '\0';
		}

		void Render() override;

	private:
		std::string GetDialogName();

		void ProcessFile();
		void ProcessDDS();

		LoggerContext _log = LoggerContext("DialogImportTexture");
		IGFD::FileDialog _fileDialog;
		std::string _importPath;
		std::string _textureName;
		std::string _name;
		std::vector<char> _nameCharBuffer;
		std::vector<char> _ddsPathBuffer;
		bool _useFileName = false;
		bool _rawImport = true;
	};
}
