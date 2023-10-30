// FileNewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "FileNewDlg.h"
#include "ModuleV2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CFileNewDlg dialog

CFileNewDlg::CFileNewDlg(CWnd* pParent)
	: CDialog(CFileNewDlg::IDD, pParent)
{
	m_moduleName = MODULE_DEFAULT_NAME;
	m_moduleAuthor = MODULE_DEFAULT_AUTHOR;
	m_moduleCopyright = MODULE_DEFAULT_COPYRIGHT;
	m_subtuneName = MODULE_DEFAULT_SUBTUNE_NAME;
	m_songLength = MODULE_DEFAULT_SONG_LENGTH;
	m_patternLength = MODULE_DEFAULT_PATTERN_LENGTH;
	m_songSpeed = MODULE_DEFAULT_SONG_SPEED;
	m_instrumentSpeed = MODULE_DEFAULT_INSTRUMENT_SPEED;
	m_channelCount = MODULE_DEFAULT_SOUNDCHIP_COUNT;
}

void CFileNewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_MODULE_NAME, m_moduleName);
	DDV_MaxChars(pDX, m_moduleName, MODULE_SONG_NAME_MAX);
	DDX_Text(pDX, IDC_MODULE_AUTHOR, m_moduleAuthor);
	DDV_MaxChars(pDX, m_moduleAuthor, MODULE_AUTHOR_NAME_MAX);
	DDX_Text(pDX, IDC_MODULE_COPYRIGHT, m_moduleCopyright);
	DDV_MaxChars(pDX, m_moduleCopyright, MODULE_COPYRIGHT_INFO_MAX);
	DDX_Text(pDX, IDC_MODULE_SUBTUNE_NAME, m_subtuneName);
	DDV_MaxChars(pDX, m_subtuneName, SUBTUNE_NAME_MAX);
	DDX_Text(pDX, IDC_MODULE_SONG_LENGTH, m_songLength);
	DDV_MinMaxInt(pDX, m_songLength, 1, SONGLINE_COUNT);
	DDX_Text(pDX, IDC_MODULE_PATTERN_LENGTH, m_patternLength);
	DDV_MinMaxInt(pDX, m_patternLength, 1, ROW_COUNT);
	DDX_Text(pDX, IDC_MODULE_SONG_SPEED, m_songSpeed);
	DDV_MinMaxInt(pDX, m_songSpeed, 1, SONG_SPEED_MAX);
	DDX_Text(pDX, IDC_MODULE_INSTRUMENT_SPEED, m_instrumentSpeed);
	DDV_MinMaxInt(pDX, m_instrumentSpeed, 1, INSTRUMENT_SPEED_MAX);
	DDX_Control(pDX, IDC_MODULE_SOUNDCHIP_COUNT, m_soundchipCount);
}

BEGIN_MESSAGE_MAP(CFileNewDlg, CDialog)
	// Insert messages here
END_MESSAGE_MAP()

BOOL CFileNewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_soundchipCount.AddString("Single POKEY (4 Channels)");
	m_soundchipCount.AddString("Dual POKEY (8 Channels)");
	m_soundchipCount.AddString("Triple POKEY (12 Channels)");
	m_soundchipCount.AddString("Quad POKEY (16 Channels)");
	m_soundchipCount.SetCurSel(m_channelCount);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CFileNewDlg message handlers

void CFileNewDlg::OnOK() 
{
	// Set the actual Channel Count based on the Soundchip Count chosen
	m_channelCount = (m_soundchipCount.GetCurSel() + 1) * POKEY_CHANNEL_COUNT;

	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CChangeMaxtracklenDlg dialog

/*
CChangeMaxtracklenDlg::CChangeMaxtracklenDlg(CWnd* pParent) //=NULL)
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
*/
/////////////////////////////////////////////////////////////////////////////
// CChangeMaxtracklenDlg message handlers
