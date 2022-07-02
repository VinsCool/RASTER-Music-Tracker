// RmtDoc.cpp : implementation of the CRmtDoc class
//

#include "stdafx.h"
#include "Rmt.h"
#include "RmtDoc.h"
#include "RmtView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRmtDoc

IMPLEMENT_DYNCREATE(CRmtDoc, CDocument)

BEGIN_MESSAGE_MAP(CRmtDoc, CDocument)
	//{{AFX_MSG_MAP(CRmtDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRmtDoc construction/destruction

CRmtDoc::CRmtDoc()
{
	// TODO: add one-time construction code here
}

CRmtDoc::~CRmtDoc()
{
}

BOOL CRmtDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRmtDoc serialization

void CRmtDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRmtDoc diagnostics

#ifdef _DEBUG
void CRmtDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRmtDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRmtDoc commands

