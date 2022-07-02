// MainFrm.cpp : implementation of the CMainFrame class
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

extern BOOL g_closeapplication;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_CBN_SELCHANGE(IDC_COMBO_LINESAFTER, OnSelchangeComboLinesAfter)
	ON_CBN_KILLFOCUS(IDC_COMBO_LINESAFTER,OnSelendok)
	ON_COMMAND(1,OnSelendok)	//pri stlaceni Enteru v ComboBoxu
	ON_COMMAND(2,OnSelendok)	//pri stlaceni ESCape v ComboBoxu
	ON_CBN_CLOSEUP(IDC_COMBO_LINESAFTER,OnSelendok)	//pri closeupu ComboBoxu
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS
/*
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
*/
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this)/*, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP 
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) */||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create main toolbar\n");
		return -1;      // fail to create
	}

	CRect rect;

	int index=m_wndToolBar.CommandToIndex(ID_BUTTONCOMBO1);
	const int combow=40;
	m_wndToolBar.SetButtonInfo(index, ID_BUTTONCOMBO1, TBBS_SEPARATOR, combow);
    m_wndToolBar.GetItemRect(index, &rect);
	rect.bottom+=300;	//velikost rozkliknuteho comboboxu
	if (!m_c_linesafter.Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,rect, &m_wndToolBar, IDC_COMBO_LINESAFTER))
		return -1;

	//m_c_linesafter.ShowWindow(SW_SHOW);

	char ss[2];
	ss[1]=0;
	for(char i='0'; i<='8'; i++)	//0..8
	{
		ss[0]=i;
		m_c_linesafter.AddString(ss);
	}

	m_c_linesafter.SetCurSel(1);


	/*
	if (!m_ToolBarPlay.CreateEx(this,TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_ToolBarPlay.LoadToolBar(IDR_TOOLBARPLAY))
	{
		TRACE0("Failed to create toolbar2\n");
		return -1;      // fail to create
	}
	//*/

	/*
	if (!m_ToolBarChannels.CreateEx(this)/*,TBSTYLE_FLAT, WS_CHILD  | WS_VISIBLE | CBRS_TOP 
		| CBRS_GRIPPER | CBRS_TOOLTIPS  | CBRS_FLYBY | CBRS_SIZE_DYNAMIC )* / ||
		!m_ToolBarChannels.LoadToolBar(IDR_TOOLBARCHANNELS))
	{
		TRACE0("Failed to create toolbar channels\n");
		return -1;      // fail to create
	}
	//*/

	if (!m_ToolBarBlock.CreateEx(this)/*,TBSTYLE_FLAT, WS_CHILD  | WS_VISIBLE | CBRS_TOP 
		| CBRS_GRIPPER | CBRS_TOOLTIPS  | CBRS_FLYBY | CBRS_SIZE_DYNAMIC )*/ ||
		!m_ToolBarBlock.LoadToolBar(IDR_TOOLBARBLOCK))
	{
		TRACE0("Failed to create toolbar block\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) 
		|| !m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)) )
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}


	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar) ||
		!m_wndReBar.AddBar(&m_ToolBarBlock))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	m_ToolBarBlock.SetBarStyle(m_ToolBarBlock.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
/*	m_wndToolBar.EnableDocking( CBRS_ALIGN_ANY );
	//m_ToolBarPlay.EnableDocking(CBRS_ALIGN_ANY);
	m_ToolBarBlock.EnableDocking( CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY); //CBRS_FLOAT_MULTI);	//CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar); //,AFX_IDW_DOCKBAR_TOP,(LPCRECT)CRect(0,0,600,32));
	DockControlBar(&m_ToolBarBlock); //,AFX_IDW_DOCKBAR_TOP,(LPCRECT)CRect(620,0,800,32));

	//DockControlBar(&m_ToolBarPlay);
	//m_ToolBarBlock.SetWindowPos(0,600,0,0,0,SWP_NOSIZE | SWP_SHOWWINDOW);
	//RepositionBars( IDR_MAINFRAME, IDR_TOOLBARBLOCK, IDR_TOOLBARBLOCK); //UINT nIDLeftOver, UINT nFlag = CWnd::reposDefault, LPRECT lpRectParam = NULL, LPCRECT lpRectClient = NULL, BOOL bStretch = TRUE );
	//RecalcLayout();
*/
	g_viewblocktoolbar	= 1;
	ShowControlBar((CControlBar*)&m_ToolBarBlock,g_viewblocktoolbar,0); //nebude zobrazen

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{

	//==== Restore main window position

	CWinApp* app = AfxGetApp();
	int s, t, b, r, l;

	// only restore if there is a previously saved position
	if ( -1 != (s = app->GetProfileInt("Frame", "Status",   -1)) &&
		-1 != (t = app->GetProfileInt("Frame", "Top",      -1)) &&
		-1 != (l = app->GetProfileInt("Frame", "Left",     -1)) &&
		-1 != (b = app->GetProfileInt("Frame", "Bottom",   -1)) &&
		-1 != (r = app->GetProfileInt("Frame", "Right",    -1))
		) {

			// restore the window's status
			app->m_nCmdShow = s;

			// restore the window's width and height
			cs.cx = r - l;
			cs.cy = b - t;

			// the following correction is needed when the taskbar is
			// at the left or top and it is not "auto-hidden"
			RECT workArea;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
			l += workArea.left;
			t += workArea.top;

			// make sure the window is not completely out of sight
			int max_x = GetSystemMetrics(SM_CXSCREEN) -
				GetSystemMetrics(SM_CXICON);
			int max_y = GetSystemMetrics(SM_CYSCREEN) -
				GetSystemMetrics(SM_CYICON);
			cs.x = min(l, max_x);
			cs.y = min(t, max_y);
	}

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style = WS_OVERLAPPED | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX | WS_SIZEBOX | WS_MAXIMIZEBOX;

	return TRUE;
}

void CMainFrame::OnSelchangeComboLinesAfter()
{
	g_linesafter = m_c_linesafter.GetCurSel();
}


/////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnClose() 
{
	//CRmtView* fw = (CRmtView*)(AfxGetApp()->GetMainWnd());
	//if (fw) fw->m_song.Stop(); <--vyhodi kopyta
/*
	//
	g_rmtexit=1;		//kvuli nevypinani NumLocku v SetFocusu
						//tj. kdyz se vrati Focus z Exit dialogu RMTcku, ktere se pak zavira

	//int r = IDYES; //default
	//kdyz doslo ke zmenam, zepta se:
	//if (g_changes) r = MessageBox("Exit from RMT?\n\n(Warning: All unsaved changes will lost.)","Exit",MB_YESNO | MB_ICONQUESTION);
	//
	//if (r == IDYES)
	//XXXXXXXXXXXXXXXXX;

	CRmtView* fw = (CRmtView*)(AfxGetApp()->GetMainWnd());

	if ( !fw || !(fw->m_song.WarnUnsavedChanges()) )
	{
		CFrameWnd::OnClose();
	}
	else
	{	//pokud nepotvrdil Exit
		g_rmtexit=0;
		//a ted navic musi udelat vypnuti NumLocku, ktere se v KillFocusu neprovedlo
		//kvuli nastavenemu g_rmtexit;
		if (fw) fw->SetNumLock(0);
	}
	//CFrameWnd::OnClose();
*/

	// Save main window position
	CWinApp* app = AfxGetApp();
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	app->WriteProfileInt("Frame", "Status", wp.showCmd);
	app->WriteProfileInt("Frame", "Top",    wp.rcNormalPosition.top);
	app->WriteProfileInt("Frame", "Left",   wp.rcNormalPosition.left);
	app->WriteProfileInt("Frame", "Bottom", wp.rcNormalPosition.bottom);
	app->WriteProfileInt("Frame", "Right",  wp.rcNormalPosition.right);

	if (g_closeapplication)
	{
		CFrameWnd::OnClose();
	}
	else
	{
		//posle si zpravu ze chce ukoncit
		CRmtView* fw = (CRmtView*)(AfxGetApp()->GetMainWnd());
		fw->PostMessage(WM_COMMAND,ID_WANTEXIT,0);
	}
}

/*
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	long msg = pMsg->message;
	if (msg==WM_KEYDOWN || msg==WM_KEYUP)
	{
		if (pMsg->wParam == VK_F10) pMsg->wParam=VK_F9;
	}
	return CFrameWnd::PreTranslateMessage(pMsg);
}
*/

void CMainFrame::OnSelendok()
{
	// TODO: Add your message handler code here and/or call default
	//if (nChar==VK_TAB || nChar==13 /*VK_ENTER*/ || nChar==VK_ESCAPE )
	{
		//zrusi focus u ComboBoxu a da ho hlavnimu oknu
		CRmtView* fw = (CRmtView*)(AfxGetApp()->GetMainWnd());
		if (fw) fw->SetFocus();
	}
}

void CMainFrame::OnGetMinMaxInfo( MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 800;
	lpMMI->ptMinTrackSize.y = 400;
}
