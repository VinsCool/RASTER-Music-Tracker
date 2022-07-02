#if !defined(AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExportDlgs.h : header file
//

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

class CExpRMTDlg : public CDialog
{
// Construction
public:
	CExpRMTDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpRMTDlg)
	enum { IDD = IDD_EXPRMT };
	CButton	m_c_gvf;
	CButton	m_c_nos;
	CStatic	m_c_warning;
	CButton	m_c_sfx;
	CEdit	m_c_rmtfeat;
	CEdit	m_c_addr;
	CStatic	m_c_info;
	//}}AFX_DATA

	int m_adr;
	int m_len;
	int m_len2;

	BOOL m_sfx;
	BOOL m_gvf;
	BOOL m_nos;

	CSong* m_song;
	char* m_filename;
	BYTE* m_instrsaved;
	BYTE* m_instrsaved2;
	BYTE* m_tracksaved;
	BYTE* m_tracksaved2;

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
// CExpASMDlg dialog

class CExpASMDlg : public CDialog
{
// Construction
public:
	CExpASMDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpASMDlg)
	enum { IDD = IDD_EXPASM };
	CString	m_labelsprefix;
	//}}AFX_DATA

	int m_type;
	int m_notes;
	int m_durations;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpASMDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExpASMDlg)
	virtual void OnOK();
	afx_msg void OnRadio();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTDLGS_H__9D678324_1FB4_11D7_BEB0_00600854AFCA__INCLUDED_)
