// ExportDlgs.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "ExportDlgs.h"
#include "r_music.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpSAPDlg dialog


CExpSAPDlg::CExpSAPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpSAPDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpSAPDlg)
	m_author = _T("");
	m_date = _T("");
	m_name = _T("");
	m_subsongs = _T("");
	//}}AFX_DATA_INIT
}


void CExpSAPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpSAPDlg)
	DDX_Text(pDX, IDC_AUTHOR, m_author);
	DDX_Text(pDX, IDC_DATE, m_date);
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Text(pDX, IDC_SUBSONGS, m_subsongs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpSAPDlg, CDialog)
	//{{AFX_MSG_MAP(CExpSAPDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpSAPDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CExpRMTDlg dialog


CExpRMTDlg::CExpRMTDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpRMTDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpRMTDlg)
	//}}AFX_DATA_INIT
}


void CExpRMTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpRMTDlg)
	DDX_Control(pDX, IDC_GLOBALVOLUMEFADE, m_c_gvf);
	DDX_Control(pDX, IDC_NOSTARTINGSONGLINE, m_c_nos);
	DDX_Control(pDX, IDC_WARNING, m_c_warning);
	DDX_Control(pDX, IDC_SFX, m_c_sfx);
	DDX_Control(pDX, IDC_RMTFEAT, m_c_rmtfeat);
	DDX_Control(pDX, IDC_ADDR, m_c_addr);
	DDX_Control(pDX, IDC_INFO, m_c_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpRMTDlg, CDialog)
	//{{AFX_MSG_MAP(CExpRMTDlg)
	ON_EN_CHANGE(IDC_ADDR, OnChangeAddr)
	ON_BN_CLICKED(IDC_COPYTOCLIPBOARD, OnCopytoclipboard)
	ON_BN_CLICKED(IDC_SFX, OnSfx)
	ON_BN_CLICKED(IDC_NOSTARTINGSONGLINE, OnNostartingsongline)
	ON_BN_CLICKED(IDC_GLOBALVOLUMEFADE, OnGlobalvolumefade)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpRMTDlg message handlers

BOOL CExpRMTDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_c_sfx.SetCheck(m_sfx);
	m_c_gvf.SetCheck(m_gvf);
	m_c_nos.SetCheck(m_nos);
	
	CString s;
	s.Format("%04X",m_adr);
	m_c_addr.SetWindowText(s);

	ChangeParams();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpRMTDlg::ChangeParams()
{
	CString s;
	char *ch;
	m_c_addr.GetWindowText(s);
	int adr = strtoul(s,&ch,16);

	m_sfx=m_c_sfx.GetCheck();
	if (!m_sfx)
	{
		//m_c_rmtfeat.SetWindowText(m_rmtfeat);
		if (adr>0x10000-m_len) adr = 0x10000-m_len;
		m_adr = adr;
		s.Format("=>  $%04X - $%04X , length $%04X (%u bytes)",m_adr,m_adr+m_len-1,m_len,m_len);
		m_c_warning.SetWindowText("Warning:\nThis output file doesn't contain any unused or empty tracks and instruments, song name and names of all instruments.");
	}
	else
	{
		//m_c_rmtfeat.SetWindowText(m_rmtfeat2);
		if (adr>0x10000-m_len2) adr = 0x10000-m_len2;
		m_adr = adr;
		s.Format("=>  $%04X - $%04X , length $%04X (%u bytes)",m_adr,m_adr+m_len2-1,m_len2,m_len2);
		m_c_warning.SetWindowText("Warning:\nThis output file doesn't contain song name and names of all instruments.");
	}
	m_c_info.SetWindowText(s);

	m_gvf=m_c_gvf.GetCheck();
	m_nos=m_c_nos.GetCheck();

	BYTE *instrsav = (m_sfx)? m_instrsaved2 : m_instrsaved;
	BYTE *tracksav = (m_sfx)? m_tracksaved2 : m_tracksaved;
	m_song->ComposeRMTFEATstring(s,m_filename,instrsav,tracksav,m_sfx,m_gvf,m_nos);
	m_c_rmtfeat.SetWindowText(s);
}

void CExpRMTDlg::OnSfx() 
{
	ChangeParams();
}

void CExpRMTDlg::OnGlobalvolumefade() 
{
	ChangeParams();
}

void CExpRMTDlg::OnNostartingsongline() 
{
	ChangeParams();
}

void CExpRMTDlg::OnChangeAddr() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	ChangeParams();
}

void CExpRMTDlg::OnCopytoclipboard() 
{
	// TODO: Add your control notification handler code here
	m_c_rmtfeat.SetFocus();
	m_c_rmtfeat.SetSel(0,-1);
	m_c_rmtfeat.Copy();
}

/////////////////////////////////////////////////////////////////////////////
// CExpMSXDlg dialog


CExpMSXDlg::CExpMSXDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpMSXDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpMSXDlg)
	m_meter = FALSE;
	m_msx_shuffle = FALSE;
	m_region_auto = FALSE;
	m_speedinfo = _T("");
	//}}AFX_DATA_INIT
}


void CExpMSXDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpMSXDlg)
	DDX_Control(pDX, IDC_SCROLLBAR1, m_scroll1);
	DDX_Control(pDX, IDC_COLORNUMBER, m_colorinfotext);
	DDX_Control(pDX, IDC_CHECK2, m_checkkeytest);
	DDX_Control(pDX, IDC_PREV, m_prev);
	DDX_Control(pDX, IDC_EDIT, m_edit);
	DDX_Check(pDX, IDC_RASTERLINE, m_meter);
	DDX_Check(pDX, IDC_MSXSHUFFLE, m_msx_shuffle);
	DDX_Check(pDX, IDC_MSXREGIONAUTO, m_region_auto);
	DDX_Text(pDX, IDC_SPEEDINFO, m_speedinfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpMSXDlg, CDialog)
	//{{AFX_MSG_MAP(CExpMSXDlg)
	ON_EN_CHANGE(IDC_EDIT, OnChangeTxtedit)
	ON_BN_CLICKED(IDC_CHECK2, OnCheck2)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_RASTERLINE, OnRasterline)
	ON_BN_CLICKED(IDC_MSXSHUFFLE, OnRasterline)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpMSXDlg message handlers

BOOL g_msxcheck = 1;
BOOL g_msx_shuffle = 1;
BOOL g_region_auto = 1;
int g_msxcol = 6;

BOOL CExpMSXDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_font.CreatePointFont(80,"Fixedsys");
	m_prev.SetFont(&m_font);

	((CButton*)GetDlgItem(IDC_RASTERLINE))->SetCheck(g_msxcheck);

	((CButton*)GetDlgItem(IDC_MSXSHUFFLE))->SetCheck(g_msx_shuffle);

	((CButton*)GetDlgItem(IDC_MSXREGIONAUTO))->SetCheck(g_region_auto);

	m_scroll1.SetScrollRange(1,127);
	OnHScroll(SB_THUMBPOSITION,g_msxcol/2,&m_scroll1);

	// TODO: Add extra initialization here
	m_edit.SetWindowText(m_txt);
	ChangeParams();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpMSXDlg::ChangeParams()
{
	//
	CString s,l,d,d4th,d5th;
	m_edit.GetWindowText(s);
	int i,from=0,line=0;
	while(from<s.GetLength() && line<5)
	{
		i = s.Find("\n",from);
		if (i>=0)
		{
			int len = i-from;
			if (len>40) len=40;
			l = s.Mid(from,len)+"\x0d\x0a"; 
			d+=l;
			if (line!=4) d4th+=l;	//without line 5
			if (line!=3) d5th+=l;	//without line 4
			from = i+1;
			line++;
		}
		else
		{
			l=s.Mid(from,40);
			d+=l;
			if (line!=4) d4th+=l;	//without line 5
			if (line!=3) d5th+=l;	//without line 4
			break;
		}
	}

	BOOL shift=((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck();

	m_prev.SetWindowText((shift)? d5th : d4th);
	m_txt = d;

	BOOL meter = ((CButton*)GetDlgItem(IDC_RASTERLINE))->GetCheck();

	m_scroll1.EnableWindow(meter);
	m_colorinfotext.EnableWindow(meter);

	GetDlgItem(IDC_MSXSHUFFLE)->EnableWindow(meter);	//disable the checkbox

}

void CExpMSXDlg::OnChangeTxtedit() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	ChangeParams();	
}

void CExpMSXDlg::OnCheck2() 
{
	//shiftkey test
	ChangeParams();
}

void CExpMSXDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (pScrollBar==&m_scroll1)
	{
		int c=g_msxcol/2;
		if (nSBCode==SB_LINELEFT) c--;
		else
		if (nSBCode==SB_LINERIGHT) c++;
		else
		if (nSBCode==SB_PAGELEFT) c+=8;
		else
		if (nSBCode==SB_PAGERIGHT) c-=8;
		else
		if (nSBCode==SB_THUMBPOSITION || nSBCode==SB_THUMBTRACK) c=nPos;

		if (c>127) c=127;
		else
		if (c<1) c=1;

		g_msxcol=c*2;
		m_scroll1.SetScrollPos(c);
		CString s;
	
		const char *bar[]={"Gray","Rust","Orange","Red-orange","Pink","Purple","Cobalt blue","Blue",
			"Medium blue","Dark blue","Blue-grey","Olive green","Medium green","Dark green","Orange-green","Brown"};
		
		//NTSC colour names may or may not be correct yet
		//const char *bar[]={"Black","Rust","Red-orange","Dark-orange","Red","Lavender","Cobalt blue","Ultramarine",
		//	"Medium blue","Dark blue","Blue-grey","Olive green","Medium green","Dark green","Orange-green","Orange"};		

		s.Format("%i = %s %i",g_msxcol,bar[g_msxcol/16],g_msxcol%16);
		m_colorinfotext.SetWindowText((LPCTSTR)s);
	}
	else
		CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CExpMSXDlg::OnOK() 
{
	m_metercolor=g_msxcol;	
	g_msx_shuffle=((CButton*)GetDlgItem(IDC_MSXSHUFFLE))->GetCheck();
	g_region_auto=((CButton*)GetDlgItem(IDC_MSXREGIONAUTO))->GetCheck();
	g_msxcheck=((CButton*)GetDlgItem(IDC_RASTERLINE))->GetCheck();
	CDialog::OnOK();
}

void CExpMSXDlg::OnRasterline() 
{
	ChangeParams();	
}
/////////////////////////////////////////////////////////////////////////////
// CExpASMDlg dialog


CExpASMDlg::CExpASMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpASMDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpASMDlg)
	m_labelsprefix = _T("");
	//}}AFX_DATA_INIT
}


void CExpASMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpASMDlg)
	DDX_Text(pDX, IDC_LABELSPREFIX, m_labelsprefix);
	DDV_MaxChars(pDX, m_labelsprefix, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpASMDlg, CDialog)
	//{{AFX_MSG_MAP(CExpASMDlg)
	ON_BN_CLICKED(IDC_RADIO1, OnRadio)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio)
	ON_BN_CLICKED(IDC_RADIO3, OnRadio)
	ON_BN_CLICKED(IDC_RADIO4, OnRadio)
	ON_BN_CLICKED(IDC_RADIO5, OnRadio)
	ON_BN_CLICKED(IDC_RADIO6, OnRadio)
	ON_BN_CLICKED(IDC_RADIO7, OnRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpASMDlg message handlers

BOOL CExpASMDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_RADIO3))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_RADIO5))->SetCheck(1);

	CString s;
	s.Format("Note indexes $00-$%02X",NOTESNUM-1);
	((CWnd*)GetDlgItem(IDC_RADIO3))->SetWindowText(s);

	OnRadio();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpASMDlg::OnOK() 
{
	// TODO: Add extra validation here
	int i,r[8];
	for(i=1; i<=7; i++) r[i]=((CButton*)GetDlgItem(IDC_RADIO1-1+i))->GetCheck();
	m_type=r[1]+r[2]*2;
	m_notes=r[3]+r[4]*2;
	m_durations=r[5]+r[6]*2+r[7]*3;	
	CDialog::OnOK();
}

void CExpASMDlg::OnRadio() 
{
	//int i,r[8];
	//for(i=1; i<=7; i++) r[i]=((CButton*)GetDlgItem(IDC_RADIO1-1+i))->GetCheck();
	//m_check_merge.EnableWindow(r[2] && (r[6] || r[7]));
}
