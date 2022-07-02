// FileNewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "FileNewDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileNewDlg dialog


CFileNewDlg::CFileNewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFileNewDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileNewDlg)
	m_maxtracklen = 64;
	m_combotype = 1;		//stereo 8 tracks
	//}}AFX_DATA_INIT
}


void CFileNewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileNewDlg)
	DDX_Text(pDX, IDC_MAXTRACKLEN, m_maxtracklen);
	DDV_MinMaxInt(pDX, m_maxtracklen, 1, 256);
	DDX_CBIndex(pDX, IDC_COMBOTYPE, m_combotype);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileNewDlg, CDialog)
	//{{AFX_MSG_MAP(CFileNewDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileNewDlg message handlers


void CFileNewDlg::OnOK() 
{
	CEdit* ed = (CEdit*)GetDlgItem(IDC_MAXTRACKLEN);
	CString s;
	ed->GetWindowText(s);
	int mtl = atoi((LPCTSTR)s);
	if (mtl>64 && mtl<=TRACKLEN)
	{
		int r=MessageBox("Warning:\nMaximal length of tracks is greater than 64.\nRMT Atari internal module format allows 256 bytes max. for each track data,\nso you may not use too many events in such longer tracks.\nOne event (note or speed command) spends 2 bytes approximately.\n\nIf a problem appears, you will see a warning during the file output function.\nOk?","New RMT module - Warning",MB_YESNO | MB_ICONQUESTION);
		if (r!=IDYES) return;
	}
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CChangeMaxtracklenDlg dialog


CChangeMaxtracklenDlg::CChangeMaxtracklenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChangeMaxtracklenDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChangeMaxtracklenDlg)
	m_info = _T("");
	m_maxtracklen = 0;
	//}}AFX_DATA_INIT
}


void CChangeMaxtracklenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangeMaxtracklenDlg)
	DDX_Text(pDX, IDC_INFO, m_info);
	DDX_Text(pDX, IDC_MAXTRACKLEN, m_maxtracklen);
	DDV_MinMaxInt(pDX, m_maxtracklen, 1, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChangeMaxtracklenDlg, CDialog)
	//{{AFX_MSG_MAP(CChangeMaxtracklenDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangeMaxtracklenDlg message handlers
