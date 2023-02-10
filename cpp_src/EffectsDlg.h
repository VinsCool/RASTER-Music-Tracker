#if !defined(AFX_EFFECTSDLG_H__9E855445_82D2_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_EFFECTSDLG_H__9E855445_82D2_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EffectsDlg.h : header file

/////////////////////////////////////////////////////////////////////////////
// CEffectsDlg dialog

class CEffectsDlg : public CDialog
{
// Construction
public:
	CEffectsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEffectsDlg)
	enum { IDD = IDD_EFFECTS };
	CStatic	m_p3;
	CStatic	m_p2;
	CStatic	m_p1;
	CEdit	m_edit3;
	CEdit	m_edit2;
	CEdit	m_edit1;
	CComboBox	m_eff_combo;
	CString	m_info;
	//}}AFX_DATA

	int m_effai;
	int m_bfro;
	int m_bto;
	int m_ainstr;
	int m_all;

	struct TTrack *m_trackptr;
	struct TTrack *m_trackorig;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEffectsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

private:

	void PerformEffect();

protected:

	// Generated message map functions
	//{{AFX_MSG(CEffectsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeEffCombo();
	virtual void OnOK();
	afx_msg void OnDefault();
	afx_msg void OnTry();
	afx_msg void OnRestore();
	virtual void OnCancel();
	afx_msg void OnPlaystop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSongTracksOrderDlg dialog

class CSongTracksOrderDlg : public CDialog
{
// Construction
public:
	CSongTracksOrderDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSongTracksOrderDlg)
	enum { IDD = IDD_SONGTRACKSORDER };
	CString	m_songlinefrom;
	CString	m_songlineto;
	//}}AFX_DATA

	int m_tracksorder[8];
	int m_fromtrack,m_totrack;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSongTracksOrderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSongTracksOrderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnDefault();
	afx_msg void OnMonostereo();
	afx_msg void OnL1R4();
	afx_msg void OnStereomono();
	afx_msg void OnNothing();
	afx_msg void OnCopyleftright();
	afx_msg void OnCopyrightleft();
	afx_msg void OnClearall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CInstrumentChangeDlg dialog

class CInstrumentChangeDlg : public CDialog
{
// Construction
public:
	CInstrumentChangeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInstrumentChangeDlg)
	enum { IDD = IDD_INSTRCHANGE };
	CButton	m_check6;
	CEdit	m_edit2;
	CEdit	m_edit1;
	CButton	m_check5;
	CButton	m_checkoneinstr;
	CStatic	m_ctitle2;
	CStatic	m_ctitle;
	CButton	m_check4;
	CButton	m_check3;
	CButton	m_check2;
	CButton	m_check1;
	int		m_combo1;
	int		m_combo2;
	int		m_combo3;
	int		m_combo4;
	int		m_combo5;
	int		m_combo6;
	int		m_combo7;
	int		m_combo8;
	int		m_combo9;
	int		m_combo10;
	int		m_combo11;
	int		m_combo12;
	//}}AFX_DATA

	int m_onlytrack;
	int m_onlychannels;
	int m_onlysonglinefrom,m_onlysonglineto;

	void SelChangeComboX();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInstrumentChangeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInstrumentChangeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDefault();
	afx_msg void OnSelchangeComboX();
	afx_msg void OnSelchangeComboInstrs();
	afx_msg void OnSameNoteRanges();
	afx_msg void OnSameVolumeRange();
	afx_msg void OnSameInstrRange();
	virtual void OnOK();
	afx_msg void OnFullRanges();
	afx_msg void OnCheckoneinstrument();
	afx_msg void OnCheckSomeChannelsOnly();
	afx_msg void OnCheckTrackOnly();
	afx_msg void OnCheckSomeSonglinesOnly();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CRenumberTracksDlg dialog

class CRenumberTracksDlg : public CDialog
{
// Construction
public:
	CRenumberTracksDlg(CWnd* pParent = NULL);   // standard constructor

	int m_radio;

// Dialog Data
	//{{AFX_DATA(CRenumberTracksDlg)
	enum { IDD = IDD_RENUMBERTRACKS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRenumberTracksDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRenumberTracksDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CRenumberInstrumentsDlg dialog

class CRenumberInstrumentsDlg : public CDialog
{
// Construction
public:
	CRenumberInstrumentsDlg(CWnd* pParent = NULL);   // standard constructor

	int m_radio;

// Dialog Data
	//{{AFX_DATA(CRenumberInstrumentsDlg)
	enum { IDD = IDD_RENUMBERINSTRUMENTS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRenumberInstrumentsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRenumberInstrumentsDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CInsertCopyOrCloneOfSongLinesDlg dialog

class CInsertCopyOrCloneOfSongLinesDlg : public CDialog
{
// Construction
public:
	CInsertCopyOrCloneOfSongLinesDlg(CWnd* pParent = NULL);   // standard constructor
	BOOL ValuesTest();

// Dialog Data
	//{{AFX_DATA(CInsertCopyOrCloneOfSongLinesDlg)
	enum { IDD = IDD_SONGINSERTCOPYORCLONEOFSONGLINES };
	CStatic	m_c_info;
	CStatic	m_c_text2;
	CStatic	m_c_text1;
	CEdit	m_c_volumep;
	CEdit	m_c_tuning;
	CEdit	m_c_lineto;
	CEdit	m_c_linefrom;
	CButton	m_c_clone;
	//}}AFX_DATA

	int m_linefrom,m_lineto,m_lineinto;
	BOOL m_clone;
	int m_tuning,m_volumep;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertCopyOrCloneOfSongLinesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInsertCopyOrCloneOfSongLinesDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClonetracks();
	afx_msg void OnChangeSonglinerange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// COctaveSelectDlg dialog

class COctaveSelectDlg : public CDialog
{
// Construction
public:
	COctaveSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COctaveSelectDlg)
	enum { IDD = IDD_OCTAVESELECT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CPoint m_pos;
	int m_octave;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COctaveSelectDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
	
	// Generated message map functions
	//{{AFX_MSG(COctaveSelectDlg)
	virtual void OnOK();
	afx_msg void OnOctave();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CInstrumentSelectDlg dialog

class CInstrumentSelectDlg : public CDialog
{
// Construction
public:
	CInstrumentSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInstrumentSelectDlg)
	enum { IDD = IDD_INSTRUMENTSELECT };
	CListBox	m_list1;
	//}}AFX_DATA

	CPoint m_pos;
	int m_selected;
	//class CInstruments* m_instrs;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInstrumentSelectDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInstrumentSelectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CVolumeSelectDlg dialog

class CVolumeSelectDlg : public CDialog
{
// Construction
public:
	CVolumeSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVolumeSelectDlg)
	enum { IDD = IDD_VOLUMESELECT };
	CListBox	m_list1;
	BOOL	m_respectvolume;
	//}}AFX_DATA

	CPoint m_pos;
	int m_volume;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVolumeSelectDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVolumeSelectDlg)
	afx_msg void OnSelchangeList1();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CChannelsSelectionDlg dialog

class CChannelsSelectionDlg : public CDialog
{
// Construction
public:
	CChannelsSelectionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChannelsSelectionDlg)
	enum { IDD = IDD_CHANNELSSELECT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	int m_channelyes;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChannelsSelectionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChannelsSelectionDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EFFECTSDLG_H__9E855445_82D2_11D7_BEB0_00600854AFCA__INCLUDED_)
