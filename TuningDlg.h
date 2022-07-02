#pragma once

// TuningDlg dialog
class TuningDlg : public CDialog
{
// Construction
public:
	TuningDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	enum { IDD = IDD_TUNING };

	double	m_basetuning;
	int m_basenote;
	int m_temperament;

	//ratio L
	int UNISON_L;
	int MIN_2ND_L;
	int MAJ_2ND_L;
	int MIN_3RD_L;
	int MAJ_3RD_L;
	int PERF_4TH_L;
	int TRITONE_L;
	int PERF_5TH_L;
	int MIN_6TH_L;
	int MAJ_6TH_L;
	int MIN_7TH_L;
	int MAJ_7TH_L;
	int OCTAVE_L;

	//ratio R
	int UNISON_R;
	int MIN_2ND_R;
	int MAJ_2ND_R;
	int MIN_3RD_R;
	int MAJ_3RD_R;
	int PERF_4TH_R;
	int TRITONE_R;
	int PERF_5TH_R;
	int MIN_6TH_R;
	int MAJ_6TH_R;
	int MIN_7TH_R;
	int MAJ_7TH_R;
	int OCTAVE_R;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//backup values loaded during initialisation, which can then be retrieved in case the dialog was canceled
	double	b_basetuning = 0;
	int b_basenote = 0;
	int b_temperament = 0;

	//ratio L
	int b_UNISON_L = 0;
	int b_MIN_2ND_L = 0;
	int b_MAJ_2ND_L = 0;
	int b_MIN_3RD_L = 0;
	int b_MAJ_3RD_L = 0;
	int b_PERF_4TH_L = 0;
	int b_TRITONE_L = 0;
	int b_PERF_5TH_L = 0;
	int b_MIN_6TH_L = 0;
	int b_MAJ_6TH_L = 0;
	int b_MIN_7TH_L = 0;
	int b_MAJ_7TH_L = 0;
	int b_OCTAVE_L = 0;

	//ratio R
	int b_UNISON_R = 0;
	int b_MIN_2ND_R = 0;
	int b_MAJ_2ND_R = 0;
	int b_MIN_3RD_R = 0;
	int b_MAJ_3RD_R = 0;
	int b_PERF_4TH_R = 0;
	int b_TRITONE_R = 0;
	int b_PERF_5TH_R = 0;
	int b_MIN_6TH_R = 0;
	int b_MAJ_6TH_R = 0;
	int b_MIN_7TH_R = 0;
	int b_MAJ_7TH_R = 0;
	int b_OCTAVE_R = 0;

	// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickedIdtestnow();
	afx_msg void OnClickedIdreset();
	afx_msg void OnBnClickedCancel();
};
