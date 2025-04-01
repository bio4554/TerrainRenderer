#include "pch.h"
#include "CErrorMessageDialog.h"

BEGIN_MESSAGE_MAP(CErrorMessageDialog, CDialogEx)
	
END_MESSAGE_MAP()

CErrorMessageDialog::CErrorMessageDialog(std::string message, CWnd* pParent) : CDialogEx(IDD_DIALOG_ERROR_MESSAGE, pParent)
{

}

void CErrorMessageDialog::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;
}
