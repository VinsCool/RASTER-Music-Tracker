#pragma once

// TuningDlg dialog
class TuningDlg : public CDialog
{
// Construction
public:
	TuningDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_TUNING };
	CComboBox m_baseNoteSelection;
	CComboBox m_baseOctaveSelection;
	double m_baseTuning;
	int m_baseNote;
	int m_baseOctave;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Backup values loaded during initialisation, which can then be retrieved in case the dialog was canceled
	double m_backupTuning;
	int m_backupNote;
	int m_backupOctave;

	// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnClickedIdTestNow();
	afx_msg void OnClickedIdReset();
	afx_msg void OnBnClickedCancel();
};
