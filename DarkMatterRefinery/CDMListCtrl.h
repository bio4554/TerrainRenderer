#pragma once

#include <afxwin.h>

class CDMListCtrl : public CListCtrl
{
protected:
	void PreSubclassWindow() override;
};