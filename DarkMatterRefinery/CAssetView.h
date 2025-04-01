#pragma once
#include <afxwin.h>
#include <afxcmn.h>  // For CListCtrl

#include "CDMListCtrl.h"

class CAssetView : public CView
{
protected:
	CAssetView();
	DECLARE_DYNCREATE(CAssetView)

public:
	virtual ~CAssetView();

protected:
	CTabCtrl _tabCtrl;
	CDMListCtrl _listCtrlTexture;
	CDMListCtrl _listCtrlMesh;

	virtual void OnInitialUpdate() override;
	virtual void OnDraw(CDC* pDC) override;

	LRESULT RefreshTextureList(WPARAM wParam = 0, LPARAM lParam = 0);
	void RefreshMeshList();

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

#define IDC_ASSET_TAB 1001