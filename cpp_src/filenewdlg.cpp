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
	m_maxTrackLength = 64;
	m_comboMonoOrStereo = 1;		// 0 = mono 4 tracks, 1 = stereo 8 tracks
	//}}AFX_DATA_INIT
}


void CFileNewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileNewDlg)
	DDX_Text(pDX, IDC_MAXTRACKLEN, m_maxTrackLength);
	DDV_MinMaxInt(pDX, m_maxTrackLength, 1, 256);
	DDX_CBIndex(pDX, IDC_COMBOTYPE, m_comboMonoOrStereo);
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
	// Set the track length.
	// If its more than 64
	CEdit* ed = (CEdit*)GetDlgItem(IDC_MAXTRACKLEN);
	CString s;
	ed->GetWindowText(s);
	int mtl = atoi((LPCTSTR)s);
	if (mtl > 64 && mtl <= TRACKLEN)
	{
		int r = MessageBox("Warning:\nLength of tracks is greater than 64.\nRMT's internal module format allows for a maximum of\n256 bytes for each track. It is not recommended to use\na large number of events in long tracks.\nEach track event (note or speed command) uses about 2 bytes.\n\nWhen saving the RMT file it will report any problems with it.\n\nOk?", "New RMT module - Warning", MB_YESNO | MB_ICONQUESTION);
		if (r != IDYES) return;
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
