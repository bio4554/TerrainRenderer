#include "pch.h"
#include "CImportPngDialog.h"

#include <filesystem>

#include "CEngineManager.h"
#include "DMEUtilities.h"

BEGIN_MESSAGE_MAP(CImportPngDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_PNG_PICK, &CImportPngDialog::OnBnClickedBrowse)
END_MESSAGE_MAP()

CImportPngDialog::CImportPngDialog(CWnd* pParent) : CDialogEx(IDD_DIALOG_IMPORT_PNG, pParent)
{
	
}

void CImportPngDialog::OnOK()
{
	UpdateData(TRUE);

	auto filePath = dme::ToString(_filePath);
	auto fileName = dme::ToString(_fileName);

	try 
	{
		GlobalEngineManager->ImportTexture(filePath, fileName);
	}
	catch (dm::core::filesystem::exceptions::AlreadyExists& e)
	{
		this->MessageBoxW(dme::ToCString(std::format("File already exists: {}", e.name)), _T("Error"), MB_ICONERROR | MB_OK);
		return;
	}

	DestroyWindow();
}

void CImportPngDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT_PNG_NAME, _fileName);
}

void CImportPngDialog::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;
}

void CImportPngDialog::OnBnClickedBrowse()
{
	CFileDialog fileDialog(TRUE);
	if (fileDialog.DoModal() == IDOK)
	{
		CString chosenFilePath = fileDialog.GetPathName();

		_filePath = chosenFilePath;

		auto onlyFileName = std::filesystem::path(dme::ToString(_filePath)).filename().string();

		auto pButton = GetDlgItem(IDC_BUTTON_PNG_PICK);
		if (pButton)
		{
			pButton->SetWindowTextW(dme::ToCString(onlyFileName));
		}
	}
}
