// Rmt.cpp : Defines the class behaviors for the application.
// originally made by Raster, 2002-2009
// reworked by VinsCool, 2021-2022
//

#include "stdafx.h"
#include "Rmt.h"
#include "MainFrm.h"
#include "RmtDoc.h"
#include "RmtView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Some information for the about box is supplied by components outside this file
extern CString g_aboutpokey;
extern CString g_about6502;
extern CString g_driverversion;	//used to display the RMT Driver "tracker.obx" version number

extern CSong g_Song;

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

#ifdef _AFXDLL
//	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("RASTER Music Tracker"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	CoInitialize(NULL);
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

	// Initialization of a random number
	srand( (unsigned)time( NULL ) );

	g_Song.ClearSong(8);

	//PostMessage(m_pMainWnd->m_hWnd,WM_COMMAND,ID_APP_ABOUT,0); //so that the dialogue can be triggered first

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
	CString m_driverversion;
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
	m_driverversion = _T("");
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
	DDX_Text(pDX, IDC_DRIVERVERSION, m_driverversion);
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
	aboutDlg.m_driverversion = g_driverversion;
	aboutDlg.m_credits =
		"Bob!k/C.P.U., JirkaS/C.P.U., PG, Fandal, Atari800 emulator developer team, Fox/Taquart, Jaskier/Taquart, Tatqoo/Taquart, Sack/Cosine, Grayscale music band, LiSU, Miker, Dely\n\n"
		"RASTER Music Tracker is now open source! Contributions are welcome, and appreciated, for making this program better in any way!\n\n"
		"Special thanks to Bob!k and Fandal for originally granting me the permission to access the original sources, and ultimately agree to allow me setting up a Github repository.\n\n"
		"Also thanks to Mathy for the original email communication, this is where this adventure took a new and unexpected twist!\n\n"
		"I also want to thank Ivop for helping me figure out how to use git, and how to set up the repository from the original sources, where I have then added my own contributions for the last months.\n\n"
		"--------------------------------------------------------------------------------------\n\n"
		"Additional credits since version 1.31.0:\n\n"
		"New features, bugfixes and improvements by VinsCool\n\n"
		"POKEY Tuning Calculations programming by VinsCool, with helpful advices from synthpopalooza and OPNA2608\n\n"
		"SAP-R Dumper and VUPlayer programming by VinsCool\n\n"
		"LZSS compression programming by DMSC, C++ port by VinsCool\n\n"
		"Unrolled LZSS music driver by Rensoupp, with few changes and new features by VinsCool\n\n"
		"New Bitmap graphics, ideas and beta testing by PG\n\n"
		"Ideas, features suggestions and inspiration by PG, Enderdude, Spring, Ivop, Tatqoo, Miker\n\n" 
		"Spiteful inspiration by Rensoupp, Emkay, and anyone who challenged me to try doing things believed impossible or outside of my abilities ;)\n\n"
		"Special thanks to everyone from The Chiptune Café, Atari Age, and GBAtemp who motivated me to work harder on the revival of RMT!\n\n"
		"If you think anyone was forgotten or credits were incorrectly attributed, feel free to pester VinsCool about it :D\n\n"
		"And now, time to make some Atari 8-bit music! Thanks to all dedicated fans for keeping the scene alive!";

	aboutDlg.m_credits.Replace("\n","\x0d\x0a");
	aboutDlg.m_aboutpokey=g_aboutpokey;
	aboutDlg.m_aboutpokey.Replace("\n","\x0d\x0a");
	aboutDlg.m_about6502=g_about6502;
	aboutDlg.m_about6502.Replace("\n","\x0d\x0a");

	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CRmtApp message handlers

