#include "pch.h"
#include "CDMListCtrl.h"

void CDMListCtrl::PreSubclassWindow()
{
    CListCtrl::PreSubclassWindow();

    // Ensure the control has a 3D look
    ModifyStyleEx(0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);

    // Disable visual theming for the ListView control
    SetWindowTheme(GetSafeHwnd(), L"", L"");

    CHeaderCtrl* pHeader = GetHeaderCtrl();
    if (pHeader != nullptr)
    {
        pHeader->ModifyStyle(0, HDS_BUTTONS | HDS_HORZ);
        SetWindowTheme(pHeader->GetSafeHwnd(), L"", L"");
    }
}
