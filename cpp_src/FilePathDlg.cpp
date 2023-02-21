// FilePathDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "FilePathDlg.h"
#include <io.h>		//due to findfirst, findnext

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilePathDlg dialog

CFilePathDlg::CFilePathDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilePathDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilePathDlg)
	m_path = _T("");
	//}}AFX_DATA_INIT
}

void CFilePathDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePathDlg)
	DDX_Control(pDX, IDC_DRIVELIST, m_drivelist);
	DDX_Control(pDX, IDC_DIRLIST, m_dirlist);
	DDX_Text(pDX, IDC_EDIT1, m_path);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFilePathDlg, CDialog)
	//{{AFX_MSG_MAP(CFilePathDlg)
	ON_LBN_DBLCLK(IDC_DIRLIST, OnDblclkDirlist)
	ON_LBN_DBLCLK(IDC_DRIVELIST, OnDblclkDrivelist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilePathDlg message handlers

BOOL CFilePathDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	char bf[1024];
	bf[0]=0;
	DlgDirList(bf, IDC_DRIVELIST, 0, DDL_DRIVES);
	if (m_path!="")
	{
		ActiveIndex(-1); //not to display content until it selects "drive:"
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFilePathDlg::ActiveIndex(int idx)
{
	CString s="";
	int i;
	char a;
	int len=m_path.GetLength();
	if (len>0 && (a=m_path[len-1])!='/' && a!='\\') m_path+="\\";
	if (idx>=0)
	{
		m_dirlist.GetText(idx,s);
	}
	if (s=="..")
	{
		for(i=len-2; i>=0; i--)
		{
			a=m_path[i];
			if (a=='/' || a=='\\') break;
		}
		m_path=m_path.Left(i+1);
	}
	else
	{
		m_path+=s;
		if (s!="") m_path+="\\";
	}
	char bf[1024];
	strcpy(bf,(LPCTSTR)m_path);
	m_dirlist.ResetContent();
	strcat(bf,"*.*");
    struct _finddata_t c_file;
    intptr_t hFile;
    if( (hFile = _findfirst( bf, &c_file )) != -1L )
	{
		do
		{
			if ( (c_file.attrib & _A_SUBDIR)
				&& !(c_file.name[0]=='.' && c_file.name[1]==0) ) //except "."
				m_dirlist.AddString(c_file.name);
		}
		while ( _findnext( hFile, &c_file )==0);
       _findclose( hFile );
	}
	((CWnd*)GetDlgItem(IDC_EDIT1))->SetWindowText(m_path);
}

void CFilePathDlg::OnDblclkDirlist() 
{
	int idx=m_dirlist.GetCurSel();
	ActiveIndex(idx);
}

void CFilePathDlg::OnDblclkDrivelist() 
{
	int idx=m_drivelist.GetCurSel();
	if (idx>=0)
	{
		CString s;
		m_drivelist.GetText(idx,s);
		//[-d-]
		m_path.Format("%c:\\",s[2]);
		ActiveIndex(-1);
	}
}
