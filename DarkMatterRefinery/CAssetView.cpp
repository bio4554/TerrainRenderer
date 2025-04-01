#include "pch.h"
#include "CAssetView.h"

#include "CEngineManager.h"
#include "DMEAppMessages.h"
#include "DMEUtilities.h"
#include "DMEViewRegistry.h"

IMPLEMENT_DYNCREATE(CAssetView, CView)

BEGIN_MESSAGE_MAP(CAssetView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_ASSET_TAB, OnTcnSelchangeTab)
	ON_MESSAGE(WM_DME_TEXTURE_ASSET_CHANGED, RefreshTextureList)
END_MESSAGE_MAP()

CAssetView::CAssetView()
{
	GViews.SetAssetView(this);
}

CAssetView::~CAssetView()
{
	
}

int CAssetView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!_tabCtrl.Create(WS_CHILD | WS_VISIBLE | TCS_TABS | TCS_HOTTRACK, CRect(0, 0, 0, 0), this, IDC_ASSET_TAB))
	{
		TRACE0("Failed to create tab control\n");
		return -1;
	}

	//SetWindowTheme(_tabCtrl.GetSafeHwnd(), L"", L"");

	TCITEM tie;
	tie.mask = TCIF_TEXT;
	wchar_t textureTabName[20] = L"Textures";
	wchar_t meshTabName[20] = L"Meshes";
	tie.pszText = textureTabName;
	_tabCtrl.InsertItem(0, &tie);
	tie.pszText = meshTabName;
	_tabCtrl.InsertItem(1, &tie);

	if (!_listCtrlTexture.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, CRect(0, 0, 0, 0), this, 1234))
	{
		TRACE0("Failed to create list control 1\n");
		return -1;
	}
	_listCtrlTexture.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	_listCtrlTexture.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
	_listCtrlTexture.InsertColumn(1, _T("Extent"), LVCFMT_RIGHT, 100);

	// Create second list control
	if (!_listCtrlMesh.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, CRect(0, 0, 0, 0), this, 1235))
	{
		TRACE0("Failed to create list control 2\n");
		return -1;
	}
	_listCtrlMesh.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	_listCtrlMesh.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
	_listCtrlMesh.InsertColumn(1, _T("Value"), LVCFMT_RIGHT, 100);

	_listCtrlTexture.ShowWindow(SW_SHOW);
	_listCtrlMesh.ShowWindow(SW_HIDE);

	/*SetWindowTheme(_listCtrlTexture.GetSafeHwnd(), L"", L"");
	SetWindowTheme(_listCtrlMesh.GetSafeHwnd(), L"", L"");*/

	RefreshTextureList();

	return 0;
}

void CAssetView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CRect rect;
	GetClientRect(rect);

	int tabHeight = 30;
	_tabCtrl.MoveWindow(0, 0, rect.Width(), tabHeight);

	CRect listRect(0, tabHeight, rect.Width(), rect.Height() - tabHeight);
	_listCtrlMesh.MoveWindow(listRect);
	_listCtrlTexture.MoveWindow(listRect);
}

void CAssetView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(rect);

	int tabHeight = 30;
	_tabCtrl.MoveWindow(0, 0, cx, tabHeight);

	CRect listRect(0, tabHeight, cx, cy - tabHeight);
	_listCtrlMesh.MoveWindow(listRect);
	_listCtrlTexture.MoveWindow(listRect);
}

void CAssetView::OnDraw(CDC* pDC)
{
	
}

LRESULT CAssetView::RefreshTextureList(WPARAM wParam, LPARAM lParam)
{
	_listCtrlTexture.DeleteAllItems();

	auto textures = GlobalEngineManager->GetTextureAssets();

	for (size_t i = 0; i < textures.size(); i++)
	{
		auto& texture = textures[i];
		auto index = _listCtrlTexture.InsertItem(static_cast<int>(i), dme::ToCString(texture.name));
		_listCtrlTexture.SetItemText(index, 1, dme::ToCString(texture.extent));
	}

	return 0;
}

void CAssetView::OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult)
{
	int sel = _tabCtrl.GetCurSel();
	if (sel == 0)
	{
		_listCtrlTexture.ShowWindow(SW_SHOW);
		_listCtrlMesh.ShowWindow(SW_HIDE);
	}
	else if (sel == 1)
	{
		_listCtrlTexture.ShowWindow(SW_HIDE);
		_listCtrlMesh.ShowWindow(SW_SHOW);
	}
	*pResult = 0;
}


