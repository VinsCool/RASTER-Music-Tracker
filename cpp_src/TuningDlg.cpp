// TuningDlg.cpp : implementation file
//

#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;

#include "stdafx.h"
#include "Rmt.h"
#include "TuningDlg.h"
#include "Tuning.h"
#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// TuningDlg dialog
TuningDlg::TuningDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TUNING, pParent)
{
	m_basetuning = 0;
	m_basenote = 0;
	m_temperament = 0;

	//ratio L
	UNISON_L = 1;
	MIN_2ND_L = 1;
	MAJ_2ND_L = 1;
	MIN_3RD_L = 1;
	MAJ_3RD_L = 1;
	PERF_4TH_L = 1;
	TRITONE_L = 1;
	PERF_5TH_L = 1;
	MIN_6TH_L = 1;
	MAJ_6TH_L = 1;
	MIN_7TH_L = 1;
	MAJ_7TH_L = 1;
	OCTAVE_L = 1;

	//ratio R
	UNISON_R = 1;
	MIN_2ND_R = 1;
	MAJ_2ND_R = 1;
	MIN_3RD_R = 1;
	MAJ_3RD_R = 1;
	PERF_4TH_R = 1;
	TRITONE_R = 1;
	PERF_5TH_R = 1;
	MIN_6TH_R = 1;
	MAJ_6TH_R = 1;
	MIN_7TH_R = 1;
	MAJ_7TH_R = 1;
	OCTAVE_R = 1;
}

void TuningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASETUNING, m_basetuning);
	DDV_MinMaxDouble(pDX, m_basetuning, 6.875, 7040);	//should be more than enough...
	DDX_CBIndex(pDX, IDC_BASENOTE, m_basenote);
	DDX_CBIndex(pDX, IDC_TEMPERAMENT, m_temperament);
	//ratio left values
	DDX_Text(pDX, IDC_UNISON_L, UNISON_L);
	DDX_Text(pDX, IDC_MINOR_2ND_L, MIN_2ND_L);
	DDX_Text(pDX, IDC_MAJOR_2ND_L, MAJ_2ND_L);
	DDX_Text(pDX, IDC_MINOR_3RD_L, MIN_3RD_L);
	DDX_Text(pDX, IDC_MAJOR_3RD_L, MAJ_3RD_L);
	DDX_Text(pDX, IDC_PERFECT_4TH_L, PERF_4TH_L);
	DDX_Text(pDX, IDC_TRITONE_L, TRITONE_L);
	DDX_Text(pDX, IDC_PERFECT_5TH_L, PERF_5TH_L);
	DDX_Text(pDX, IDC_MINOR_6TH_L, MIN_6TH_L);
	DDX_Text(pDX, IDC_MAJOR_6TH_L, MAJ_6TH_L);
	DDX_Text(pDX, IDC_MINOR_7TH_L, MIN_7TH_L);
	DDX_Text(pDX, IDC_MAJOR_7TH_L, MAJ_7TH_L);
	DDX_Text(pDX, IDC_OCTAVE_L, OCTAVE_L);
	//ratio right values
	DDX_Text(pDX, IDC_UNISON_R, UNISON_R);
	DDX_Text(pDX, IDC_MINOR_2ND_R, MIN_2ND_R);
	DDX_Text(pDX, IDC_MAJOR_2ND_R, MAJ_2ND_R);
	DDX_Text(pDX, IDC_MINOR_3RD_R, MIN_3RD_R);
	DDX_Text(pDX, IDC_MAJOR_3RD_R, MAJ_3RD_R);
	DDX_Text(pDX, IDC_PERFECT_4TH_R, PERF_4TH_R);
	DDX_Text(pDX, IDC_TRITONE_R, TRITONE_R);
	DDX_Text(pDX, IDC_PERFECT_5TH_R, PERF_5TH_R);
	DDX_Text(pDX, IDC_MINOR_6TH_R, MIN_6TH_R);
	DDX_Text(pDX, IDC_MAJOR_6TH_R, MAJ_6TH_R);
	DDX_Text(pDX, IDC_MINOR_7TH_R, MIN_7TH_R);
	DDX_Text(pDX, IDC_MAJOR_7TH_R, MAJ_7TH_R);
	DDX_Text(pDX, IDC_OCTAVE_R, OCTAVE_R);
}

BEGIN_MESSAGE_MAP(TuningDlg, CDialog)
	ON_BN_CLICKED(IDTESTNOW, OnClickedIdtestnow)
	ON_BN_CLICKED(IDRESET, OnClickedIdreset)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()

// TuningDlg message handlers
BOOL TuningDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//backup all current values first
	b_basetuning = g_basetuning;
	b_basenote = g_basenote;
	b_temperament = g_temperament;

	//ratio L
	b_UNISON_L = g_UNISON_L;
	b_MIN_2ND_L = g_MIN_2ND_L;
	b_MAJ_2ND_L = g_MAJ_2ND_L;
	b_MIN_3RD_L = g_MIN_3RD_L;
	b_MAJ_3RD_L = g_MAJ_3RD_L;
	b_PERF_4TH_L = g_PERF_4TH_L;
	b_TRITONE_L = g_TRITONE_L;
	b_PERF_5TH_L = g_PERF_5TH_L;
	b_MIN_6TH_L = g_MIN_6TH_L;
	b_MAJ_6TH_L = g_MAJ_6TH_L;
	b_MIN_7TH_L = g_MIN_7TH_L;
	b_MAJ_7TH_L = g_MAJ_7TH_L;
	b_OCTAVE_L = g_OCTAVE_L;

	//ratio R
	b_UNISON_R = g_UNISON_R;
	b_MIN_2ND_R = g_MIN_2ND_R;
	b_MAJ_2ND_R = g_MAJ_2ND_R;
	b_MIN_3RD_R = g_MIN_3RD_R;
	b_MAJ_3RD_R = g_MAJ_3RD_R;
	b_PERF_4TH_R = g_PERF_4TH_R;
	b_TRITONE_R = g_TRITONE_R;
	b_PERF_5TH_R = g_PERF_5TH_R;
	b_MIN_6TH_R = g_MIN_6TH_R;
	b_MAJ_6TH_R = g_MAJ_6TH_R;
	b_MIN_7TH_R = g_MIN_7TH_R;
	b_MAJ_7TH_R = g_MAJ_7TH_R;
	b_OCTAVE_R = g_OCTAVE_R;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void TuningDlg::OnOK()
{
	CDialog::OnOK();
}

void TuningDlg::OnClickedIdtestnow()
{
	TuningDlg::UpdateData();

	//backup all current values first
	g_basetuning = m_basetuning;
	g_basenote = m_basenote;
	g_temperament = m_temperament;

	//ratio L
	g_UNISON_L = UNISON_L;
	g_MIN_2ND_L = MIN_2ND_L;
	g_MAJ_2ND_L = MAJ_2ND_L;
	g_MIN_3RD_L = MIN_3RD_L;
	g_MAJ_3RD_L = MAJ_3RD_L;
	g_PERF_4TH_L = PERF_4TH_L;
	g_TRITONE_L = TRITONE_L;
	g_PERF_5TH_L = PERF_5TH_L;
	g_MIN_6TH_L = MIN_6TH_L;
	g_MAJ_6TH_L = MAJ_6TH_L;
	g_MIN_7TH_L = MIN_7TH_L;
	g_MAJ_7TH_L = MAJ_7TH_L;
	g_OCTAVE_L = OCTAVE_L;

	//ratio R
	g_UNISON_R = UNISON_R;
	g_MIN_2ND_R = MIN_2ND_R;
	g_MAJ_2ND_R = MAJ_2ND_R;
	g_MIN_3RD_R = MIN_3RD_R;
	g_MAJ_3RD_R = MAJ_3RD_R;
	g_PERF_4TH_R = PERF_4TH_R;
	g_TRITONE_R = TRITONE_R;
	g_PERF_5TH_R = PERF_5TH_R;
	g_MIN_6TH_R = MIN_6TH_R;
	g_MAJ_6TH_R = MAJ_6TH_R;
	g_MIN_7TH_R = MIN_7TH_R;
	g_MAJ_7TH_R = MAJ_7TH_R;
	g_OCTAVE_R = OCTAVE_R;

	g_Tuning.init_tuning();
}

void TuningDlg::OnClickedIdreset()
{
	//retrieve the last backed up values first
	g_basetuning = b_basetuning;
	g_basenote = b_basenote;
	g_temperament = b_temperament;

	//ratio L
	g_UNISON_L = b_UNISON_L;
	g_MIN_2ND_L = b_MIN_2ND_L;
	g_MAJ_2ND_L = b_MAJ_2ND_L;
	g_MIN_3RD_L = b_MIN_3RD_L;
	g_MAJ_3RD_L = b_MAJ_3RD_L;
	g_PERF_4TH_L = b_PERF_4TH_L;
	g_TRITONE_L = b_TRITONE_L;
	g_PERF_5TH_L = b_PERF_5TH_L;
	g_MIN_6TH_L = b_MIN_6TH_L;
	g_MAJ_6TH_L = b_MAJ_6TH_L;
	g_MIN_7TH_L = b_MIN_7TH_L;
	g_MAJ_7TH_L = b_MAJ_7TH_L;
	g_OCTAVE_L = b_OCTAVE_L;

	//ratio R
	g_UNISON_R = b_UNISON_R;
	g_MIN_2ND_R = b_MIN_2ND_R;
	g_MAJ_2ND_R = b_MAJ_2ND_R;
	g_MIN_3RD_R = b_MIN_3RD_R;
	g_MAJ_3RD_R = b_MAJ_3RD_R;
	g_PERF_4TH_R = b_PERF_4TH_R;
	g_TRITONE_R = b_TRITONE_R;
	g_PERF_5TH_R = b_PERF_5TH_R;
	g_MIN_6TH_R = b_MIN_6TH_R;
	g_MAJ_6TH_R = b_MAJ_6TH_R;
	g_MIN_7TH_R = b_MIN_7TH_R;
	g_MAJ_7TH_R = b_MAJ_7TH_R;
	g_OCTAVE_R = b_OCTAVE_R;

	g_Tuning.init_tuning();
}

void TuningDlg::OnBnClickedCancel()
{
	//reset the values from the last backup
	OnClickedIdreset();
	//and done, nothing else to be done here 
	CDialog::OnCancel();
}
