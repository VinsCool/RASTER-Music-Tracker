#if !defined(AFX_FILEPATHDLG_H__37FA894A_70D3_4F71_9A46_E764DA2788CE__INCLUDED_)
#define AFX_FILEPATHDLG_H__37FA894A_70D3_4F71_9A46_E764DA2788CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilePathDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilePathDlg dialog

class CFilePathDlg : public CDialog
{
// Construction
public:
	CFilePathDlg(CWnd* pParent = NULL);   // standard constructor

	void ActiveIndex(int idx);

// Dialog Data
	//{{AFX_DATA(CFilePathDlg)
	enum { IDD = IDD_FILEPATHDLG };
	CListBox	m_drivelist;
	CListBox	m_dirlist;
	CString	m_path;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilePathDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilePathDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkDirlist();
	afx_msg void OnDblclkDrivelist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEPATHDLG_H__37FA894A_70D3_4F71_9A46_E764DA2788CE__INCLUDED_)
