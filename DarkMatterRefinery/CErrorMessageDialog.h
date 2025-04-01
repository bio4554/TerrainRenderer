#pragma once
#include <afxwin.h>

#include "resource.h"

class CErrorMessageDialog : public CDialogEx
{
public:
	CErrorMessageDialog(std::string message, CWnd* pParent = nullptr);

	enum { IDD = IDD_DIALOG_ERROR_MESSAGE };

protected:
	virtual void PostNcDestroy() override;
	DECLARE_MESSAGE_MAP()

protected:
};
