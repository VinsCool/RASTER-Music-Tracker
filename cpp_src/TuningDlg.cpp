// TuningDlg.cpp : implementation file
//

#include <iostream>
#include <fstream>
#include <iomanip>

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
	m_baseTuning = 0;
	m_baseNote = 0;
	m_baseOctave = 0;
}

void TuningDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BASETUNING, m_baseTuning);
	DDV_MinMaxDouble(pDX, m_baseTuning, 6.875, 7040);	//should be more than enough...
	DDX_Control(pDX, IDC_BASENOTE, m_baseNoteSelection);
	DDX_Control(pDX, IDC_BASEOCTAVE, m_baseOctaveSelection);
}

BEGIN_MESSAGE_MAP(TuningDlg, CDialog)
	ON_BN_CLICKED(IDTESTNOW, OnClickedIdTestNow)
	ON_BN_CLICKED(IDRESET, OnClickedIdReset)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()

// TuningDlg message handlers
BOOL TuningDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Backup all values currently used
	m_backupTuning = g_baseTuning;
	m_backupNote = g_baseNote;
	m_backupOctave = g_baseOctave;

	m_baseNoteSelection.AddString("C-");
	m_baseNoteSelection.AddString("B-");
	m_baseNoteSelection.AddString("A#");
	m_baseNoteSelection.AddString("A-");
	m_baseNoteSelection.AddString("G#");
	m_baseNoteSelection.AddString("G-");
	m_baseNoteSelection.AddString("F#");
	m_baseNoteSelection.AddString("F-");
	m_baseNoteSelection.AddString("E-");
	m_baseNoteSelection.AddString("D#");
	m_baseNoteSelection.AddString("D-");
	m_baseNoteSelection.AddString("C#");
	m_baseNoteSelection.SetCurSel(m_baseNote);

	m_baseOctaveSelection.AddString("-4");
	m_baseOctaveSelection.AddString("-3");
	m_baseOctaveSelection.AddString("-2");
	m_baseOctaveSelection.AddString("-1");
	m_baseOctaveSelection.AddString("+0");
	m_baseOctaveSelection.AddString("+1");
	m_baseOctaveSelection.AddString("+2");
	m_baseOctaveSelection.AddString("+3");
	m_baseOctaveSelection.AddString("+4");
	m_baseOctaveSelection.SetCurSel(m_baseOctave);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void TuningDlg::OnOK()
{
	m_baseNote = m_baseNoteSelection.GetCurSel();
	m_baseOctave = m_baseOctaveSelection.GetCurSel();

	CDialog::OnOK();
}

void TuningDlg::OnClickedIdTestNow()
{
	TuningDlg::UpdateData();

	// Retrieve the values to be tested
	g_baseTuning = m_baseTuning;
	g_baseNote = m_baseNote = m_baseNoteSelection.GetCurSel();
	g_baseOctave = m_baseOctave = m_baseOctaveSelection.GetCurSel();
}

void TuningDlg::OnClickedIdReset()
{
	TuningDlg::UpdateData();

	// Retrieve the backed up values
	g_baseTuning = m_baseTuning = m_backupTuning;
	g_baseNote = m_baseNote = m_backupNote;
	g_baseOctave = m_baseOctave = m_backupOctave;
	m_baseNoteSelection.SetCurSel(m_baseNote);
	m_baseOctaveSelection.SetCurSel(m_baseOctave);

	// Force the Dialog Box to refresh the Base Tuning field
	OnInitDialog();
}

void TuningDlg::OnBnClickedCancel()
{
	// Reset the values from the last backup
	OnClickedIdReset();

	// And done, nothing else to be done here 
	CDialog::OnCancel();
}
