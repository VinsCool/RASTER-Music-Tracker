// importdlgs.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "importdlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImportModDlg dialog


CImportModDlg::CImportModDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportModDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportModDlg)
	m_info = _T("");
	m_check1 = FALSE;
	m_check2 = FALSE;
	m_check3 = FALSE;
	m_check4 = FALSE;
	m_check5 = FALSE;
	m_check6 = FALSE;
	m_check7 = FALSE;
	m_check8 = FALSE;
	//}}AFX_DATA_INIT
}


void CImportModDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportModDlg)
	DDX_Text(pDX, IDC_INFO, m_info);
	DDX_Check(pDX, IDC_CHECK1, m_check1);
	DDX_Check(pDX, IDC_CHECK2, m_check2);
	DDX_Check(pDX, IDC_CHECK3, m_check3);
	DDX_Check(pDX, IDC_CHECK4, m_check4);
	DDX_Check(pDX, IDC_CHECK5, m_check5);
	DDX_Check(pDX, IDC_CHECK6, m_check6);
	DDX_Check(pDX, IDC_CHECK7, m_check7);
	DDX_Check(pDX, IDC_CHECK8, m_check8);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportModDlg, CDialog)
	//{{AFX_MSG_MAP(CImportModDlg)
	ON_BN_CLICKED(IDC_CHECK2, OnCheck2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportModDlg message handlers

BOOL CImportModDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_RADIO1)->SetWindowText((LPCTSTR)m_txtradio1);
	GetDlgItem(IDC_RADIO2)->SetWindowText((LPCTSTR)m_txtradio2);
	((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK6))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CImportModDlg::OnOK() 
{
	// TODO: Add extra validation here
	if ( ((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck() ) 
		m_txtradio1="";
	else
		m_txtradio2="";
	
	CDialog::OnOK();
}

void CImportModDlg::OnCheck2() 
{
	int chbx2=((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck();
	if (!chbx2)
	{
		((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_CHECK3))->EnableWindow(0);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK3))->EnableWindow(1);
		((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(1);
	}
}
/////////////////////////////////////////////////////////////////////////////
// CImportModFinishedDlg dialog


CImportModFinishedDlg::CImportModFinishedDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportModFinishedDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportModFinishedDlg)
	m_info = _T("");
	//}}AFX_DATA_INIT
}


void CImportModFinishedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportModFinishedDlg)
	DDX_Control(pDX, IDC_INFO2, m_info2);
	DDX_Control(pDX, IDOK, m_okbutt);
	DDX_Control(pDX, IDC_CHECK1, m_check1);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportModFinishedDlg, CDialog)
	//{{AFX_MSG_MAP(CImportModFinishedDlg)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportModFinishedDlg message handlers

BOOL g_importmodyesokok=0;

BOOL CImportModFinishedDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_check1.SetCheck(g_importmodyesokok);
	m_okbutt.EnableWindow(g_importmodyesokok);
	m_info2.SetWindowText("Please, now you have to look over all the instuments and set up proper envelopes distortions (drums, basses, etc.), also you must correct instruments tunings according to the tuning of original samples and improve them (chords, effects, noises, etc.).");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CImportModFinishedDlg::OnCheck1() 
{
	g_importmodyesokok=m_check1.GetCheck();
	m_okbutt.EnableWindow(g_importmodyesokok);
}
/////////////////////////////////////////////////////////////////////////////
// CImportTmcDlg dialog


CImportTmcDlg::CImportTmcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportTmcDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportTmcDlg)
	m_check1 = FALSE;
	m_check6 = FALSE;
	m_check7 = FALSE;
	m_info = _T("");
	//}}AFX_DATA_INIT
}


void CImportTmcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportTmcDlg)
	DDX_Check(pDX, IDC_CHECK1, m_check1);
	DDX_Check(pDX, IDC_CHECK6, m_check6);
	DDX_Check(pDX, IDC_CHECK7, m_check7);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportTmcDlg, CDialog)
	//{{AFX_MSG_MAP(CImportTmcDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportTmcDlg message handlers

BOOL CImportTmcDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK6))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CImportTmcFinishedDlg dialog


CImportTmcFinishedDlg::CImportTmcFinishedDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportTmcFinishedDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportTmcFinishedDlg)
	m_info = _T("");
	//}}AFX_DATA_INIT
}


void CImportTmcFinishedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportTmcFinishedDlg)
	DDX_Control(pDX, IDC_INFO2, m_info2);
	DDX_Control(pDX, IDOK, m_okbutt);
	DDX_Control(pDX, IDC_CHECK1, m_check1);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportTmcFinishedDlg, CDialog)
	//{{AFX_MSG_MAP(CImportTmcFinishedDlg)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportTmcFinishedDlg message handlers

BOOL g_importtmcyesokok=0;

BOOL CImportTmcFinishedDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_check1.SetCheck(g_importtmcyesokok);
	m_okbutt.EnableWindow(g_importtmcyesokok);
	m_info2.SetWindowText("Please, now you have to look over all the instuments and whole song and check if it's all right, otherwise you must correct it manually (some special instrument effects aren't converted automatically). Also some stereo and AUDCTL events may be wrong, because of different stereo and AUDCTL conception in TMC and RMT.");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CImportTmcFinishedDlg::OnCheck1() 
{
	g_importtmcyesokok=m_check1.GetCheck();
	m_okbutt.EnableWindow(g_importtmcyesokok);
}
/////////////////////////////////////////////////////////////////////////////
// CTracksLoadDlg dialog


CTracksLoadDlg::CTracksLoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTracksLoadDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTracksLoadDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTracksLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTracksLoadDlg)
	DDX_Control(pDX, IDC_TEXT1, m_text1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTracksLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CTracksLoadDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTracksLoadDlg message handlers

BOOL CTracksLoadDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_radio=0;
	CButton* radio1=(CButton*)GetDlgItem(IDC_RADIO1);

	CString s;
	s.Format("There are %u tracks in TXT file.",m_tracknum);
	m_text1.SetWindowText(s);
	s.Format("Load tracks to $%02X-$%02X.",m_trackfrom,m_trackfrom+m_tracknum-1);
	radio1->SetWindowText(s);
	radio1->SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTracksLoadDlg::OnOK() 
{
	CButton* radio1=(CButton*)GetDlgItem(IDC_RADIO1);
	m_radio = (radio1->GetCheck())? 0:1;
	CDialog::OnOK();
}

