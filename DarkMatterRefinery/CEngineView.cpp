#include "pch.h"
#include "CEngineView.h"

#include "CEngineManager.h"
#include "DMEViewRegistry.h"
#include "DMInputSystem.h"
#include "DMRealFileSystem.h"
#include "EditorCamera.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"

IMPLEMENT_DYNCREATE(CEngineView, CView)

BEGIN_MESSAGE_MAP(CEngineView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CEngineView::CEngineView()
{
	GViews.SetEngineView(this);
}

CEngineView::~CEngineView()
{
	
}

void CEngineView::OnClose()
{
	
}

void CEngineView::OnDestroy()
{
	GlobalEngineManager->Shutdown();
}


void CEngineView::OnDraw(CDC* pDC)
{
	//pDC->TextOutW(10, 10, _T("Object List"));
}

void CEngineView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	GlobalEngineManager->Resize(cx, cy);
}


int CEngineView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	GlobalEngineManager->Init(this->GetSafeHwnd());

	return 0;
}
