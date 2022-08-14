#if !defined(AFX_CONFIGDLG_H__71CF2041_5206_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_CONFIGDLG_H__71CF2041_5206_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
//

extern CString g_path_songs;		//default path for songs
extern CString g_path_instruments;	//default path for instruments
extern CString g_path_tracks;		//default path for tracks

extern CString g_lastloadpath_songs;
extern CString g_lastloadpath_instruments;
extern CString g_lastloadpath_tracks;

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIG };
	CComboBox	m_keyboard_c_layout;
	CComboBox	m_midi_c_device;
	BOOL	m_midi_TouchResponse;
	//BOOL	m_keyboard_playautofollow;
	int		m_midi_VolumeOffset;
	int		m_tracklinehighlight;
	int     m_scaling_percentage;
	//double	m_tuning;
	BOOL	m_ntsc;
	BOOL	m_doSmoothScrolling;
	BOOL	m_displayflatnotes;
	BOOL	m_usegermannotation;
	BOOL	m_midi_NoteOff;
	BOOL	m_keyboard_updowncontinue;
	BOOL	m_nohwsoundbuffer;
	BOOL	m_tracklinealtnumbering;
	//BOOL	m_keyboard_swapenter;
	BOOL	m_keyboard_rememberoctavesandvolumes;
	BOOL	m_keyboard_escresetatarisound;
	BOOL	m_keyboard_askwhencontrol_s;
	//}}AFX_DATA

	int		m_midi_device;
	int		m_keyboard_layout;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnMidiTouchResponseClicked();
	afx_msg void OnSelchangeKeyboardLayout();
	afx_msg void OnPaths();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnBnClickedDisplayflatnotes();
};

/////////////////////////////////////////////////////////////////////////////
// CConfigPathsDlg dialog

class CConfigPathsDlg : public CDialog
{
// Construction
public:
	CConfigPathsDlg(CWnd* pParent = NULL);   // standard constructor

	void BrowsePath(int itemID);

// Dialog Data
	//{{AFX_DATA(CConfigPathsDlg)
	enum { IDD = IDD_PATHS };
	CString	m_path_songs;
	CString	m_path_instruments;
	CString	m_path_tracks;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigPathsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigPathsDlg)
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__71CF2041_5206_11D7_BEB0_00600854AFCA__INCLUDED_)
