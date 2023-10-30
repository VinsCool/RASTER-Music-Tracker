#if !defined(AFX_FILENEWDLG_H__BAD69D6D_100F_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_FILENEWDLG_H__BAD69D6D_100F_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FileNewDlg.h : header file
//
#include "resource.h"
#include "General.h"

/////////////////////////////////////////////////////////////////////////////
// CFileNewDlg dialog

class CFileNewDlg : public CDialog
{
// Construction
public:
	CFileNewDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_FILENEW };
	CString m_moduleName;
	CString m_moduleAuthor;
	CString m_moduleCopyright;
	CString m_subtuneName;
	UINT m_songLength;
	UINT m_patternLength;
	UINT m_songSpeed;
	UINT m_instrumentSpeed;
	UINT m_channelCount;
	CComboBox m_soundchipCount;

// Overrides
	// ClassWizard generated virtual function overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CChangeMaxtracklenDlg dialog
/*
class CChangeMaxtracklenDlg : public CDialog
{
// Construction
public:
	CChangeMaxtracklenDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChangeMaxtracklenDlg)
	enum { IDD = IDD_CHANGEMAXTRACKLEN };
	CString	m_info;
	int		m_maxtracklen;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChangeMaxtracklenDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChangeMaxtracklenDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
*/
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILENEWDLG_H__BAD69D6D_100F_11D7_BEB0_00600854AFCA__INCLUDED_)
