#pragma once
#include <afxwin.h>

#include "resource.h"

class CImportPngDialog : public CDialogEx
{
public:
	CImportPngDialog(CWnd* pParent = nullptr);

	enum {IDD = IDD_DIALOG_IMPORT_PNG};

protected:
	virtual void OnOK() override;
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual void PostNcDestroy() override;
	DECLARE_MESSAGE_MAP()

protected:
	CString _fileName;
	CString _filePath;

	afx_msg void OnBnClickedBrowse();
};
