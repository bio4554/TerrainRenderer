
// DarkMatterRefinery.h : main header file for the DarkMatterRefinery application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CDarkMatterRefineryApp:
// See DarkMatterRefinery.cpp for the implementation of this class
//

class CDarkMatterRefineryApp : public CWinApp
{
public:
	CDarkMatterRefineryApp() noexcept;

private:
	void ConfigureGlobalSettings();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CDarkMatterRefineryApp theApp;
