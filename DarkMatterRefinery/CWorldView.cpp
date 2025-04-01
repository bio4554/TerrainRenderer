#include "pch.h"
#include "CWorldView.h"

#include "CCellSettingsDialog.h"
#include "CEngineManager.h"
#include "DMEAppMessages.h"
#include "DMEUtilities.h"
#include "DMEViewRegistry.h"

IMPLEMENT_DYNCREATE(CWorldView, CView)

BEGIN_MESSAGE_MAP(CWorldView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_WORLD_TAB, OnTcnSelchangeTab)
	ON_MESSAGE(WM_DME_CELL_CHANGED, RefreshCellList)
	ON_NOTIFY(NM_DBLCLK, IDC_WORLD_CELL_LIST, CellListClicked)
END_MESSAGE_MAP()

CWorldView::CWorldView()
{
	GViews.SetWorldView(this);
}

CWorldView::~CWorldView()
{

}

int CWorldView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!_tabCtrl.Create(WS_CHILD | WS_VISIBLE | TCS_TABS | TCS_HOTTRACK, CRect(0, 0, 0, 0), this, IDC_WORLD_TAB))
	{
		TRACE0("Failed to create tab control\n");
		return -1;
	}

	//SetWindowTheme(_tabCtrl.GetSafeHwnd(), L"", L"");

	TCITEM tie;
	tie.mask = TCIF_TEXT;
	wchar_t textureTabName[20] = L"Objects";
	wchar_t meshTabName[20] = L"Cells";
	tie.pszText = textureTabName;
	_tabCtrl.InsertItem(0, &tie);
	tie.pszText = meshTabName;
	_tabCtrl.InsertItem(1, &tie);

	if (!_listCtrlObjects.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, CRect(0, 0, 0, 0), this, IDC_WORLD_OBJ_LIST))
	{
		TRACE0("Failed to create list control 1\n");
		return -1;
	}
	_listCtrlObjects.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	_listCtrlObjects.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
	_listCtrlObjects.InsertColumn(1, _T("Extent"), LVCFMT_RIGHT, 100);

	// Create second list control
	if (!_listCtrlCells.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, CRect(0, 0, 0, 0), this, IDC_WORLD_CELL_LIST))
	{
		TRACE0("Failed to create list control 2\n");
		return -1;
	}
	_listCtrlCells.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	_listCtrlCells.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 150);
	_listCtrlCells.InsertColumn(1, _T("Name"), LVCFMT_RIGHT, 100);
	_listCtrlCells.InsertColumn(2, _T("Position"), LVCFMT_RIGHT, 100);

	_listCtrlObjects.ShowWindow(SW_SHOW);
	_listCtrlCells.ShowWindow(SW_HIDE);

	/*SetWindowTheme(_listCtrlTexture.GetSafeHwnd(), L"", L"");
	SetWindowTheme(_listCtrlMesh.GetSafeHwnd(), L"", L"");*/

	RefreshObjectList();
	RefreshCellList();

	return 0;
}

void CWorldView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CRect rect;
	GetClientRect(rect);

	int tabHeight = 30;
	_tabCtrl.MoveWindow(0, 0, rect.Width(), tabHeight);

	CRect listRect(0, tabHeight, rect.Width(), rect.Height() - tabHeight);
	_listCtrlObjects.MoveWindow(listRect);
	_listCtrlCells.MoveWindow(listRect);
}

void CWorldView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(rect);

	int tabHeight = 30;
	_tabCtrl.MoveWindow(0, 0, cx, tabHeight);

	CRect listRect(0, tabHeight, cx, cy - tabHeight);
	_listCtrlObjects.MoveWindow(listRect);
	_listCtrlCells.MoveWindow(listRect);
}

void CWorldView::OnDraw(CDC* pDC)
{

}

LRESULT CWorldView::RefreshCellList(WPARAM wParam, LPARAM lParam)
{
	_listCtrlCells.DeleteAllItems();

	auto cells = GlobalEngineManager->GetCellData();

	for (size_t i = 0; i < cells.size(); i++)
	{
		auto& cell = cells[i];
		auto index = _listCtrlCells.InsertItem(static_cast<int>(i), dme::ToCString(std::to_string(cell.id)));
		_listCtrlCells.SetItemText(index, 1, dme::ToCString(cell.name));
		_listCtrlCells.SetItemText(index, 2, dme::ToCString(cell.position));
	}

	return 0;
}

LRESULT CWorldView::RefreshObjectList(WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

void CWorldView::OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult)
{
	int sel = _tabCtrl.GetCurSel();
	if (sel == 0)
	{
		_listCtrlObjects.ShowWindow(SW_SHOW);
		_listCtrlCells.ShowWindow(SW_HIDE);
	}
	else if (sel == 1)
	{
		_listCtrlObjects.ShowWindow(SW_HIDE);
		_listCtrlCells.ShowWindow(SW_SHOW);
	}
	*pResult = 0;
}

void CWorldView::CellListClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nItem = pNMItemActivate->iItem;

	if (nItem != -1) // Ensure a valid item was clicked
	{
		CString strItemText = _listCtrlCells.GetItemText(nItem, 0);

		auto dialog = new CCellSettingsDialog(static_cast<uint32_t>(nItem), this);
		if (dialog->Create(IDD_DIALOG_CELL_SETTINGS, this))
		{
			dialog->ShowWindow(SW_SHOW);
		}
		else
		{
			delete dialog;
		}
	}

	*pResult = 0;
}

