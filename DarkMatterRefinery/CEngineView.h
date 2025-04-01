#pragma once
#include <afxwin.h>
#include <memory>

#include "DMEngine.h"

class CEngineView : public CView
{
protected:
    CEngineView();
    ~CEngineView();
    DECLARE_DYNCREATE(CEngineView)
public:
    virtual void OnDraw(CDC* pDC) override;
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
private:
    
};
