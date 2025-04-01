#pragma once
#include <afxwin.h>
#include <afxcmn.h>  // For CListCtrl

#include "CDMListCtrl.h"

class CWorldView : public CView
{
protected:
	CWorldView();
	DECLARE_DYNCREATE(CWorldView)

public:
	virtual ~CWorldView();

protected:
	CTabCtrl _tabCtrl;
	CDMListCtrl _listCtrlCells;
	CDMListCtrl _listCtrlObjects;

	virtual void OnInitialUpdate() override;
	virtual void OnDraw(CDC* pDC) override;

	LRESULT RefreshCellList(WPARAM wParam = 0, LPARAM lParam = 0);
	LRESULT RefreshObjectList(WPARAM wParam = 0, LPARAM lParam = 0);
	void CellListClicked(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

#define IDC_WORLD_TAB 1002
#define IDC_WORLD_CELL_LIST 1003
#define IDC_WORLD_OBJ_LIST 1004