
// DarkMatterRefinery.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "DarkMatterRefinery.h"

#include "CEngineManager.h"
#include "DMGlobalSettings.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDarkMatterRefineryApp

BEGIN_MESSAGE_MAP(CDarkMatterRefineryApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CDarkMatterRefineryApp::OnAppAbout)
END_MESSAGE_MAP()


// CDarkMatterRefineryApp construction

CDarkMatterRefineryApp::CDarkMatterRefineryApp() noexcept
{
	ConfigureGlobalSettings();

	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("DarkMatterRefinery.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	GlobalEngineManager = new CEngineManager();
}

// The one and only CDarkMatterRefineryApp object

CDarkMatterRefineryApp theApp;


// CDarkMatterRefineryApp initialization

BOOL CDarkMatterRefineryApp::InitInstance()
{
	CWinApp::InitInstance();

	//Enable3dControls();

	// Enable control container support if needed.
	AfxEnableControlContainer();

	// Create and load the main frame window.
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame || !pFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pFrame;
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	return TRUE;
}

int CDarkMatterRefineryApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	delete GlobalEngineManager;
	return CWinApp::ExitInstance();
}

// CDarkMatterRefineryApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CDarkMatterRefineryApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CDarkMatterRefineryApp message handlers


void CDarkMatterRefineryApp::ConfigureGlobalSettings()
{
	dm::core::GSettings.DebugDirectX = false;
	dm::core::GSettings.IgnoreQuitEvents = true;
}

