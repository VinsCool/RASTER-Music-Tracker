#if !defined(AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExportDlgs.h : header file
//
#include "resource.h"
#include "General.h"

class CSong;


/////////////////////////////////////////////////////////////////////////////
// CExpSAPDlg dialog

class CExpSAPDlg : public CDialog
{
// Construction
public:
	CExpSAPDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpSAPDlg)
	enum { IDD = IDD_EXPSAP };
	CString	m_author;
	CString	m_date;
	CString	m_name;
	CString	m_subsongs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpSAPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExpSAPDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CExpRMTDlg dialog

class CExportStrippedRMTDialog : public CDialog
{
// Construction
public:
	CExportStrippedRMTDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpRMTDlg)
	enum { IDD = IDD_EXPORT_STRIPPED_RMT };
	CComboBox m_cmbAsmFormat;
	CButton	m_c_gvf;
	CButton	m_c_nos;
	CStatic	m_c_warning;
	CButton	m_ctrlWithSfx;
	CEdit	m_c_rmtfeat;
	CEdit	m_ctrlExportAddress;
	CStatic	m_c_info;
	//}}AFX_DATA

	int m_exportAddr;
	int m_moduleLengthForStrippedRMT;
	int m_moduleLengthForSFX;

	int m_assemblerFormat;						// 0 = Atasm, 1 = Xasm

	BOOL m_sfxSupport;
	BOOL m_globalVolumeFade;
	BOOL m_noStartingSongLine;

	CSong* m_song;
	char* m_filename;
	// Stripped RMT data
	BYTE* m_savedInstrFlagsForStrippedRMT;			// Array of flags indicating which instruments are being used
	BYTE* m_savedTracksFlagsForStrippedRMT;			// Array of flags to indicate which tracks are being used

	// Full RMT (with SFX) data
	BYTE* m_savedInstrFlagsForSFX;					// Array of flags indicating which instruments are not empty or used
	BYTE* m_savedTracksFlagsForSFX;					// Array of flags to indicate which tracks are not empty

	void ChangeParams();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpRMTDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExpRMTDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeAddr();
	afx_msg void OnCopytoclipboard();
	afx_msg void OnSfx();
	//afx_msg void OnGlobalvolumeslide();
	afx_msg void OnNostartingsongline();
	afx_msg void OnGlobalvolumefade();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeComboAsmFormat();
};
/////////////////////////////////////////////////////////////////////////////
// CExpMSXDlg dialog

class CExpMSXDlg : public CDialog
{
// Construction
public:
	CExpMSXDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpMSXDlg)
	enum { IDD = IDD_EXPMSX };
	CScrollBar	m_scroll1;
	CStatic	m_colorinfotext;
	CButton	m_checkkeytest;
	CEdit	m_prev;
	CEdit	m_edit;
	BOOL	m_meter;
	BOOL	m_msx_shuffle;
	BOOL	m_region_auto;

	CString	m_speedinfo;
	//}}AFX_DATA

	int m_metercolor;

	CString m_txt;
	CFont m_font;

	void ChangeParams();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpMSXDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExpMSXDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeTxtedit();
	afx_msg void OnCheck2();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual void OnOK();
	afx_msg void OnRasterline();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CExportAsmDlg dialog

class CExportAsmDlg : public CDialog
{
// Construction
public:
	CExportAsmDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExportAsmDlg)
	enum { IDD = IDD_EXPORT_ASM };
	CString	m_prefixForAllAsmLabels;
	//}}AFX_DATA

	int m_exportType;				// 1 = Tracks, 2 = Whole song
	int m_notesIndexOrFreq;
	int m_durationsType;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExportAsmDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExportAsmDlg)
	virtual void OnOK();
	afx_msg void OnRadio();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CExportRelocatableAsmForRmtPlayer dialog
class CExportRelocatableAsmForRmtPlayer : public CDialog
{
// Construction
public:
	CExportRelocatableAsmForRmtPlayer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExportRelocatableAsmForRmtPlayer)
	enum { IDD = IDD_EXPORT_RMTPLAYER_ASM };
	CEdit	m_editRmtModuleAsmLabel;
	CButton m_chkWantRelocatableTracks;
	CEdit	m_editTracksLabel;
	CButton m_chkWantRelocatableSongLines;
	CEdit	m_editSongLinesLabel;
	CButton m_chkWantRelocatableInstruments;
	CEdit	m_editInstrumentsLabel;

	CComboBox m_cmbAsmFormat;

	CButton	m_c_gvf;
	CButton	m_c_nos;
	CButton	m_ctrlWithSfx;
	CEdit	m_c_rmtfeat;
	CStatic	m_c_info;
	//}}AFX_DATA

	// Data transfer values
	CString m_strAsmLabelForStartOfSong;			// What label is the song data starting with?

	BOOL m_wantRelocatableInstruments;
	BOOL m_wantRelocatableTracks;
	BOOL m_wantRelocatableSongLines;
	CString m_strAsmInstrumentsLabel;
	CString m_strAsmTracksLabel;
	CString m_strAsmSongLinesLabel;

	int m_assemblerFormat;						// 0 = Atasm, 1 = Xasm

	BOOL m_sfxSupport;
	BOOL m_globalVolumeFade;
	BOOL m_noStartingSongLine;

	CSong* m_song;
	const char* m_filename;

	BOOL m_InitPhase;

	tExportDescription* m_exportDescStripped;
	tExportDescription* m_exportDescWithSFX;

	void ChangeParams();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExportRelocatableAsmForRmtPlayer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExportRelocatableAsmForRmtPlayer)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeCheckbox();
	afx_msg void OnChangeLabel();
	afx_msg void OnCbnSelchange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_)
