#if !defined(AFX_IMPORTDLGS_H__75C2CF47_F291_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_IMPORTDLGS_H__75C2CF47_F291_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// importdlgs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportModDlg dialog

class CImportModDlg : public CDialog
{
// Construction
public:
	CImportModDlg(CWnd* pParent = NULL);   // standard constructor

	CString m_txtradio1,m_txtradio2;

// Dialog Data
	//{{AFX_DATA(CImportModDlg)
	enum { IDD = IDD_IMPORTMOD };
	CString	m_info;
	BOOL	m_check1;
	BOOL	m_check2;
	BOOL	m_check3;
	BOOL	m_check4;
	BOOL	m_check5;
	BOOL	m_check6;
	BOOL	m_check7;
	BOOL	m_check8;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportModDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportModDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCheck2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CImportModFinishedDlg dialog

class CImportModFinishedDlg : public CDialog
{
// Construction
public:
	CImportModFinishedDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportModFinishedDlg)
	enum { IDD = IDD_IMPORTMODFINISHED };
	CStatic	m_info2;
	CButton	m_okbutt;
	CButton	m_check1;
	CString	m_info;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportModFinishedDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportModFinishedDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheck1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CImportTmcDlg dialog

class CImportTmcDlg : public CDialog
{
// Construction
public:
	CImportTmcDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportTmcDlg)
	enum { IDD = IDD_IMPORTTMC };
	BOOL	m_check1;
	BOOL	m_check6;
	BOOL	m_check7;
	CString	m_info;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportTmcDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportTmcDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CImportTmcFinishedDlg dialog

class CImportTmcFinishedDlg : public CDialog
{
// Construction
public:
	CImportTmcFinishedDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportTmcFinishedDlg)
	enum { IDD = IDD_IMPORTTMCFINISHED };
	CStatic	m_info2;
	CButton	m_okbutt;
	CButton	m_check1;
	CString	m_info;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportTmcFinishedDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportTmcFinishedDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheck1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CTracksLoadDlg dialog

class CTracksLoadDlg : public CDialog
{
// Construction
public:
	CTracksLoadDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTracksLoadDlg)
	enum { IDD = IDD_TRACKSLOAD };
	CStatic	m_text1;
	//}}AFX_DATA

	int m_trackfrom;
	int m_tracknum;
	int m_radio;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTracksLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTracksLoadDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMPORTDLGS_H__75C2CF47_F291_11D7_BEB0_00600854AFCA__INCLUDED_)
