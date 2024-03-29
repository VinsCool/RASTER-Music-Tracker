// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__1709C747_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_STDAFX_H__1709C747_06D0_11D7_BEB0_00600854AFCA__INCLUDED_

#define _WIN32_WINNT _WIN32_WINNT_MAXVER

#define _CRT_SECURE_NO_WARNINGS 1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <stdint.h>
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define DIRECTSOUND_VERSION	0x0900	
#include <mmsystem.h>
#include <dsound.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1709C747_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
