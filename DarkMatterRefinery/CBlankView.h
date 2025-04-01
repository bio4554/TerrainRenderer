#pragma once

#include <afxwin.h>

class CBlankView : public CView
{
protected:
    CBlankView();
    DECLARE_DYNCREATE(CBlankView)
public:
    virtual void OnDraw(CDC* pDC) override;
protected:
    DECLARE_MESSAGE_MAP()
};