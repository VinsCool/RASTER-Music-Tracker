// RmtDoc.h : interface of the CRmtDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RMTDOC_H__1709C74B_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_RMTDOC_H__1709C74B_06D0_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRmtDoc : public CDocument
{
protected: // create from serialization only
	CRmtDoc() {}
	DECLARE_DYNCREATE(CRmtDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRmtDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar) {};
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRmtDoc() {}

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRmtDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RMTDOC_H__1709C74B_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
