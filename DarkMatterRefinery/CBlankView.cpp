#include "pch.h"
#include "CBlankView.h"

IMPLEMENT_DYNCREATE(CBlankView, CView)

BEGIN_MESSAGE_MAP(CBlankView, CView)
END_MESSAGE_MAP()

CBlankView::CBlankView()
{

}

void CBlankView::OnDraw(CDC* pDC)
{
	pDC->TextOutW(10, 10, _T("CBlankView"));
}