// Rmt.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Rmt.h"

#include "MainFrm.h"
#include "RmtDoc.h"
#include "RmtView.h"
#include <afxadv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRmtApp

BEGIN_MESSAGE_MAP(CRmtApp, CWinApp)
	//{{AFX_MSG_MAP(CRmtApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRmtApp construction

CRmtApp::CRmtApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRmtApp object

CRmtApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRmtApp initialization

BOOL CRmtApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	CoInitialize(NULL);

#ifdef _AFXDLL
//	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings(5);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CRmtDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CRmtView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	// Inicializace nahodneho cisla
	srand( (unsigned)time( NULL ) );

	//PostMessage(m_pMainWnd->m_hWnd,WM_COMMAND,ID_APP_ABOUT,0); //aby se pak nejdriv vyvolal about dialog

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_rmtversion;
	CString	m_rmtauthor;
	CString	m_credits;
	CString	m_about6502;
	CString	m_aboutpokey;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_rmtversion = _T("");
	m_rmtauthor = _T("");
	m_credits = _T("");
	m_about6502 = _T("");
	m_aboutpokey = _T("");
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_RMTVERSION, m_rmtversion);
	DDX_Text(pDX, IDC_RMTAUTHOR, m_rmtauthor);
	DDX_Text(pDX, IDC_CREDITS, m_credits);
	DDX_Text(pDX, IDC_ABOUT6502, m_about6502);
	DDX_Text(pDX, IDC_ABOUTPOKEY, m_aboutpokey);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CRmtApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.m_rmtversion.LoadString(IDS_RMTVERSION);
	aboutDlg.m_rmtauthor.LoadString(IDS_RMTAUTHOR);

	aboutDlg.m_credits="Bob!k/C.P.U., JirkaS/C.P.U., PG, Fandal, Atari800 emulator developer team, Fox/Taquart, Jaskier/Taquart, Tatqoo/Taquart, Sack/Cosine, Grayscale music band, LiSU, Miker, Dely";
	aboutDlg.m_credits.Replace("\n","\x0d\x0a");
	aboutDlg.m_aboutpokey=g_aboutpokey;
	aboutDlg.m_aboutpokey.Replace("\n","\x0d\x0a");
	aboutDlg.m_about6502=g_about6502;
	aboutDlg.m_about6502.Replace("\n","\x0d\x0a");

	aboutDlg.DoModal();
}

CString & CRmtApp::GetRecentFile(int i)
{
	return (*m_pRecentFileList)[i];
}

/////////////////////////////////////////////////////////////////////////////
// CRmtApp message handlers


/*
BOOL CRmtApp::SaveAllModified() 
{
	// TODO: Add your specialized code here and/or call the base class
	MessageBox(g_hwnd,"SaveAllModified()","MSG",IDOK);
	
	return CWinApp::SaveAllModified();
}
*/
