
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.h"
#include "framework.h"
#include "DarkMatterRefinery.h"

#include "MainFrm.h"

#include "CAssetView.h"
#include "CBlankView.h"
#include "CEngineView.h"
#include "CImportPngDialog.h"
#include "CWorldView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_IMPORT_PNG, &CMainFrame::OnImportPng)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() noexcept
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (_mainSplitterWnd.GetSafeHwnd() != nullptr)
	{
		// Calculate row heights for the main splitter
		int nTopHeight = (cy * 3) / 4;     // 75% for top row (nested splitter)
		int nBottomHeight = cy - nTopHeight; // 25% for bottom row

		_mainSplitterWnd.SetRowInfo(0, nTopHeight, 0);
		_mainSplitterWnd.SetRowInfo(1, nBottomHeight, 0);
		_mainSplitterWnd.RecalcLayout();

		int nColWidth = cx / 6;
		_topSplitterWnd.SetColumnInfo(0, nColWidth, 0);
		_topSplitterWnd.SetColumnInfo(1, nColWidth * 4, 0);
		_topSplitterWnd.SetColumnInfo(2, nColWidth, 0);
		_topSplitterWnd.RecalcLayout();
	}
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//// create a view to occupy the client area of the frame
	//if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, nullptr))
	//{
	//	TRACE0("Failed to create view window\n");
	//	return -1;
	//}

	if (!_toolBarWnd.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS) || !_toolBarWnd.LoadToolBar(IDR_MAINTOOLBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}

	_toolBarWnd.EnableDocking(CBRS_ALIGN_TOP);
	EnableDocking(CBRS_ALIGN_TOP);
	DockControlBar(&_toolBarWnd);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	_mainSplitterWnd.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (_mainSplitterWnd.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if (!_mainSplitterWnd.CreateStatic(this, 2, 1))
	{
		TRACE("Failed to create main splitter\n");
		return FALSE;
	}

	if (!_topSplitterWnd.CreateStatic(&_mainSplitterWnd, 1, 3))
	{
		TRACE("Failed to create top splitter\n");
		return FALSE;
	}

	// Create views in the top splitter.
	if (!_topSplitterWnd.CreateView(0, 0, RUNTIME_CLASS(CWorldView), CSize(200, 400), pContext) ||
		!_topSplitterWnd.CreateView(0, 1, RUNTIME_CLASS(CEngineView), CSize(400, 400), pContext) ||
		!_topSplitterWnd.CreateView(0, 2, RUNTIME_CLASS(CAssetView), CSize(200, 400), pContext))
	{
		TRACE("Failed to create top splitter views\n");
		return FALSE;
	}

	// Create the bottom view in the main splitter.
	if (!_mainSplitterWnd.CreateView(1, 0, RUNTIME_CLASS(CBlankView), CSize(600, 150), pContext))
	{
		TRACE("Failed to create bottom view\n");
		return FALSE;
	}

	return TRUE;
}

void CMainFrame::OnImportPng()
{
	auto dialog = new CImportPngDialog(this);
	if (dialog->Create(IDD_DIALOG_IMPORT_PNG, this))
	{
		dialog->ShowWindow(SW_SHOW);
	}
	else
	{
		delete dialog;
	}
}
