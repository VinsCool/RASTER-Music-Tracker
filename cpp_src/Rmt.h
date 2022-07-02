// Rmt.h : main header file for the RMT application
//

#if !defined(AFX_RMT_H__1709C745_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_RMT_H__1709C745_06D0_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

extern CString g_aboutpokey;
extern CString g_about6502;

extern CString g_driverversion;	//used to display the RMT Driver "tracker.obx" version number

/////////////////////////////////////////////////////////////////////////////
// CRmtApp:
// See Rmt.cpp for the implementation of this class
//

class CRmtApp : public CWinApp
{
public:
	CRmtApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRmtApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CRmtApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RMT_H__1709C745_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
