// ExportDlgs.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "Song.h"
#include "ExportDlgs.h"
#include "General.h"


extern CSong			g_Song;

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

CExportStrippedRMTDialog::CExportStrippedRMTDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CExportStrippedRMTDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExpRMTDlg)
	//}}AFX_DATA_INIT
}

void CExportStrippedRMTDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpRMTDlg)
	DDX_Control(pDX, IDC_COMBO_ASM_FORMAT, m_cmbAsmFormat);
	DDX_Control(pDX, IDC_GLOBALVOLUMEFADE, m_c_gvf);
	DDX_Control(pDX, IDC_NOSTARTINGSONGLINE, m_c_nos);
	DDX_Control(pDX, IDC_WARNING, m_c_warning);
	DDX_Control(pDX, IDC_SFX, m_ctrlWithSfx);
	DDX_Control(pDX, IDC_RMTFEAT, m_c_rmtfeat);
	DDX_Control(pDX, IDC_ADDR, m_ctrlExportAddress);
	DDX_Control(pDX, IDC_INFO, m_c_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExportStrippedRMTDialog, CDialog)
	//{{AFX_MSG_MAP(CExpRMTDlg)
	ON_EN_CHANGE(IDC_ADDR, OnChangeAddr)
	ON_BN_CLICKED(IDC_COPYTOCLIPBOARD, OnCopytoclipboard)
	ON_BN_CLICKED(IDC_SFX, OnSfx)
	ON_BN_CLICKED(IDC_NOSTARTINGSONGLINE, OnNostartingsongline)
	ON_BN_CLICKED(IDC_GLOBALVOLUMEFADE, OnGlobalvolumefade)
	ON_CBN_SELCHANGE(IDC_COMBO_ASM_FORMAT, OnCbnSelchangeComboAsmFormat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpRMTDlg message handlers

BOOL CExportStrippedRMTDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Transfer the starting values to the MFC controls
	m_ctrlWithSfx.SetCheck(m_sfxSupport);
	m_c_gvf.SetCheck(m_globalVolumeFade);
	m_c_nos.SetCheck(m_noStartingSongLine);
	
	CString s;
	s.Format("%04X", m_exportAddr);
	m_ctrlExportAddress.SetWindowText(s);

	// Setup the assembler formats
	int idx = m_cmbAsmFormat.AddString("Atasm");
	m_cmbAsmFormat.SetItemData(idx, ASSEMBLER_FORMAT_ATASM);
	idx = m_cmbAsmFormat.AddString("Xasm");
	m_cmbAsmFormat.SetItemData(idx, ASSEMBLER_FORMAT_XASM);

	if (m_assemblerFormat < 0) m_assemblerFormat = 0;
	if (m_assemblerFormat > 1) m_assemblerFormat = 1;
	m_cmbAsmFormat.SetCurSel(m_assemblerFormat);

	ChangeParams();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExportStrippedRMTDialog::ChangeParams()
{
	// Transfer all the variables from MFC

	CString s;
	char *ptrToEndInString;
	m_ctrlExportAddress.GetWindowText(s);
	int adr = strtoul(s, &ptrToEndInString, 16);		// Parse the HEX address

	int idx = m_cmbAsmFormat.GetCurSel();
	m_assemblerFormat = m_cmbAsmFormat.GetItemData(idx);

	m_sfxSupport = m_ctrlWithSfx.GetCheck();
	if (!m_sfxSupport)
	{
		if (adr > 0x10000 - m_moduleLengthForStrippedRMT)
			adr = 0x10000 - m_moduleLengthForStrippedRMT;
		m_exportAddr = adr;
		s.Format("=>  $%04X - $%04X , length $%04X (%u bytes)",m_exportAddr,m_exportAddr+m_moduleLengthForStrippedRMT-1,m_moduleLengthForStrippedRMT,m_moduleLengthForStrippedRMT);
		m_c_warning.SetWindowText("Warning:\nThis output file doesn't contain any unused or empty tracks and instruments, song name and names of all instruments.");
	}
	else
	{
		if (adr > 0x10000 - m_moduleLengthForSFX) adr = 0x10000 - m_moduleLengthForSFX;
		m_exportAddr = adr;
		s.Format("=>  $%04X - $%04X , length $%04X (%u bytes)", m_exportAddr, m_exportAddr + m_moduleLengthForSFX - 1, m_moduleLengthForSFX, m_moduleLengthForSFX);
		m_c_warning.SetWindowText("Warning:\nThis output file doesn't contain song name and names of all instruments.");
	}
	m_c_info.SetWindowText(s);

	m_globalVolumeFade=m_c_gvf.GetCheck();
	m_noStartingSongLine=m_c_nos.GetCheck();

	BYTE *instrsav = (m_sfxSupport)? m_savedInstrFlagsForSFX : m_savedInstrFlagsForStrippedRMT;
	BYTE *tracksav = (m_sfxSupport)? m_savedTracksFlagsForSFX : m_savedTracksFlagsForStrippedRMT;
	m_song->ComposeRMTFEATstring(s, m_filename, instrsav, tracksav, m_sfxSupport, m_globalVolumeFade, m_noStartingSongLine, m_assemblerFormat);
	m_c_rmtfeat.SetWindowText(s);
}

void CExportStrippedRMTDialog::OnSfx() 
{
	ChangeParams();
}

void CExportStrippedRMTDialog::OnGlobalvolumefade() 
{
	ChangeParams();
}

void CExportStrippedRMTDialog::OnNostartingsongline() 
{
	ChangeParams();
}

void CExportStrippedRMTDialog::OnChangeAddr() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	ChangeParams();
}

void CExportStrippedRMTDialog::OnCopytoclipboard() 
{
	// TODO: Add your control notification handler code here
	m_c_rmtfeat.SetFocus();
	m_c_rmtfeat.SetSel(0,-1);
	m_c_rmtfeat.Copy();
}


void CExportStrippedRMTDialog::OnCbnSelchangeComboAsmFormat()
{
	ChangeParams();
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
// CExportAsmDlg dialog


CExportAsmDlg::CExportAsmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExportAsmDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExportAsmDlg)
	m_prefixForAllAsmLabels = _T("");
	//}}AFX_DATA_INIT
}

void CExportAsmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportAsmDlg)
	DDX_Text(pDX, IDC_LABELSPREFIX, m_prefixForAllAsmLabels);
	DDV_MaxChars(pDX, m_prefixForAllAsmLabels, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExportAsmDlg, CDialog)
	//{{AFX_MSG_MAP(CExportAsmDlg)
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
// CExportAsmDlg message handlers

BOOL CExportAsmDlg::OnInitDialog() 
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

void CExportAsmDlg::OnOK() 
{
	int radioState[8];
	for (int i = 1; i <= 7; i++)
	{
		radioState[i] = ((CButton*)GetDlgItem(IDC_RADIO1 - 1 + i))->GetCheck();
	}
	m_exportType = radioState[1] + radioState[2] * 2;
	m_notesIndexOrFreq = radioState[3] + radioState[4] * 2;
	m_durationsType = radioState[5] + radioState[6] * 2 + radioState[7] * 3;
	CDialog::OnOK();
}

void CExportAsmDlg::OnRadio() 
{
	//int i,r[8];
	//for(i=1; i<=7; i++) r[i]=((CButton*)GetDlgItem(IDC_RADIO1-1+i))->GetCheck();
	//m_check_merge.EnableWindow(r[2] && (r[6] || r[7]));
}


/////////////////////////////////////////////////////////////////////////////
// CExportRelocatableAsmForRmtPlayer dialog

CExportRelocatableAsmForRmtPlayer::CExportRelocatableAsmForRmtPlayer(CWnd* pParent /*=NULL*/)
	: CDialog(CExportRelocatableAsmForRmtPlayer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExportRelocatableAsmForRmtPlayer)
	//}}AFX_DATA_INIT
}

void CExportRelocatableAsmForRmtPlayer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportRelocatableAsmForRmtPlayer)S
	DDX_Control(pDX, IDC_RMT_MODULE_ASM_LABEL, m_editRmtModuleAsmLabel);
	DDX_Control(pDX, IDC_CHK_RELOCATE_TRACKS, m_chkWantRelocatableTracks);
	DDX_Control(pDX, IDC_EDIT_TRACKS_LABEL, m_editTracksLabel);
	DDX_Control(pDX, IDC_CHK_RELOCATE_SONG, m_chkWantRelocatableSongLines);
	DDX_Control(pDX, IDC_EDIT_SONG_LINES_LABEL, m_editSongLinesLabel);
	DDX_Control(pDX, IDC_CHK_RELOCATE_INSTRUMENTS, m_chkWantRelocatableInstruments);
	DDX_Control(pDX, IDC_EDIT_INSTRUMENTS_LABEL, m_editInstrumentsLabel);

	DDX_Control(pDX, IDC_GLOBALVOLUMEFADE, m_c_gvf);
	DDX_Control(pDX, IDC_NOSTARTINGSONGLINE, m_c_nos);
	DDX_Control(pDX, IDC_RASM_SFX, m_ctrlWithSfx);
	DDX_Control(pDX, IDC_RMTFEAT, m_c_rmtfeat);
	DDX_Control(pDX, IDC_INFO, m_c_info);
	DDX_Control(pDX, IDC_COMBO_ASM_FORMAT, m_cmbAsmFormat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExportRelocatableAsmForRmtPlayer, CDialog)
	//{{AFX_MSG_MAP(CExportRelocatableAsmForRmtPlayer)
	ON_BN_CLICKED(IDC_CHK_RELOCATE_TRACKS, OnChangeCheckbox)
	ON_BN_CLICKED(IDC_CHK_RELOCATE_SONG, OnChangeCheckbox)
	ON_BN_CLICKED(IDC_CHK_RELOCATE_INSTRUMENTS, OnChangeCheckbox)
	ON_EN_CHANGE(IDC_EDIT_TRACKS_LABEL, OnChangeLabel)
	ON_EN_CHANGE(IDC_EDIT_SONG_LINES_LABEL, OnChangeLabel)
	ON_EN_CHANGE(IDC_EDIT_INSTRUMENTS_LABEL, OnChangeLabel)
	ON_BN_CLICKED(IDC_RASM_SFX, OnChangeCheckbox)
	ON_BN_CLICKED(IDC_NOSTARTINGSONGLINE, OnChangeCheckbox)
	ON_BN_CLICKED(IDC_GLOBALVOLUMEFADE, OnChangeCheckbox)
	ON_CBN_SELCHANGE(IDC_COMBO_ASM_FORMAT, OnCbnSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportRelocatableAsmForRmtPlayer message handlers

BOOL CExportRelocatableAsmForRmtPlayer::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_InitPhase = 1;			// Tell the DDX handler to ignore all changes until everything is setup

	// Transfer the starting values to the MFC controls
	CString str;

	// Set the default song labels
	if (m_strAsmLabelForStartOfSong.IsEmpty())
		m_strAsmLabelForStartOfSong = "RMT_SONG_DATA";
	m_editRmtModuleAsmLabel.SetWindowText(m_strAsmLabelForStartOfSong);

	if (m_strAsmTracksLabel.IsEmpty())
		m_strAsmTracksLabel = "RMT_SONG_TRACKS";
	m_editTracksLabel.SetWindowText(m_strAsmTracksLabel);
	m_chkWantRelocatableTracks.SetCheck(m_wantRelocatableTracks);

	if (m_strAsmSongLinesLabel.IsEmpty())
		m_strAsmSongLinesLabel = "RMT_SONG_LINES";
	m_editSongLinesLabel.SetWindowText(m_strAsmSongLinesLabel);
	m_chkWantRelocatableSongLines.SetCheck(m_wantRelocatableSongLines);

	if (m_strAsmInstrumentsLabel.IsEmpty())
		m_strAsmInstrumentsLabel = "RMT_INSTRUMENT_DATA";
	m_editInstrumentsLabel.SetWindowText(m_strAsmInstrumentsLabel);
	m_chkWantRelocatableInstruments.SetCheck(m_wantRelocatableInstruments);

	// Setup the assembler formats
	int idx = m_cmbAsmFormat.AddString("Atasm");
	m_cmbAsmFormat.SetItemData(idx, ASSEMBLER_FORMAT_ATASM);
	idx = m_cmbAsmFormat.AddString("Xasm");
	m_cmbAsmFormat.SetItemData(idx, ASSEMBLER_FORMAT_XASM);

	if (m_assemblerFormat < 0) m_assemblerFormat = 0;
	if (m_assemblerFormat > 1) m_assemblerFormat = 1;
	m_cmbAsmFormat.SetCurSel(m_assemblerFormat);

	//
	m_ctrlWithSfx.SetCheck(m_sfxSupport);
	m_c_gvf.SetCheck(m_globalVolumeFade);
	m_c_nos.SetCheck(m_noStartingSongLine);

	m_InitPhase = 0;

	ChangeParams();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportRelocatableAsmForRmtPlayer::ChangeParams()
{
	if (m_InitPhase)
		return;

	CString s;
	m_wantRelocatableTracks = m_chkWantRelocatableTracks.GetCheck();
	m_wantRelocatableSongLines = m_chkWantRelocatableSongLines.GetCheck();
	m_wantRelocatableInstruments = m_chkWantRelocatableInstruments.GetCheck();

	m_editTracksLabel.EnableWindow(m_wantRelocatableTracks);
	m_editSongLinesLabel.EnableWindow(m_wantRelocatableSongLines);
	m_editInstrumentsLabel.EnableWindow(m_wantRelocatableInstruments);

	m_editRmtModuleAsmLabel.GetWindowText(s);
	m_strAsmLabelForStartOfSong = s;

	m_editTracksLabel.GetWindowText(s);
	m_strAsmTracksLabel = s;

	m_editSongLinesLabel.GetWindowText(s);
	m_strAsmSongLinesLabel = s;

	m_editInstrumentsLabel.GetWindowText(s);
	m_strAsmInstrumentsLabel = s;

	// With or without SFX and unused tracks
	m_sfxSupport = m_ctrlWithSfx.GetCheck();
	int len = m_sfxSupport
		? (m_exportDescWithSFX->firstByteAfterModule - m_exportDescWithSFX->targetAddrOfModule) 
		: (m_exportDescStripped->firstByteAfterModule - m_exportDescStripped->targetAddrOfModule);
	s.Format("Length $%04X (%u bytes)", len, len);
	m_c_info.SetWindowText(s);

	int idx = m_cmbAsmFormat.GetCurSel();
	m_assemblerFormat = m_cmbAsmFormat.GetItemData(idx);

	m_globalVolumeFade = m_c_gvf.GetCheck();
	m_noStartingSongLine = m_c_nos.GetCheck();

	BYTE* instrsav = (m_sfxSupport) ? m_exportDescWithSFX->instrumentSavedFlags : m_exportDescStripped->trackSavedFlags;
	BYTE* tracksav = (m_sfxSupport) ? m_exportDescWithSFX->trackSavedFlags : m_exportDescStripped->trackSavedFlags;

	m_song->BuildRelocatableAsm(
		s,
		m_sfxSupport ? m_exportDescWithSFX : m_exportDescStripped,
		"",
		m_wantRelocatableTracks ? m_strAsmTracksLabel : (CString)"",
		m_wantRelocatableSongLines ? m_strAsmSongLinesLabel : (CString)"",
		m_wantRelocatableInstruments ? m_strAsmInstrumentsLabel : (CString)"",
		m_assemblerFormat,
		m_sfxSupport,
		false,
		false,
		true		// Just give me the size info
	);
	m_c_rmtfeat.SetWindowText(s);
}

void CExportRelocatableAsmForRmtPlayer::OnChangeCheckbox()
{
	ChangeParams();
}

void CExportRelocatableAsmForRmtPlayer::OnChangeLabel()
{
	ChangeParams();
}

void CExportRelocatableAsmForRmtPlayer::OnCbnSelchange()
{
	ChangeParams();
}


