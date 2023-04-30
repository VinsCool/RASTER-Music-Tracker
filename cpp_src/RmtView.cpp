//
// RmtView.cpp : implementation of the CRmtView class
// originally made by Raster, 2002-2009
// reworked by VinsCool, 2021-2022
//

#include "stdafx.h"
#include "Rmt.h"
#include "RmtDoc.h"

#include "RmtView.h"
#include "MainFrm.h"
#include "ConfigDlg.h"
#include "FileNewDlg.h"
#include "TuningDlg.h"
#include "Atari6502.h"
#include "XPokey.h"

#include "EffectsDlg.h"

#include "global.h"

#include "GuiHelpers.h"
#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CSong	g_Song;
extern CRmtMidi	g_Midi;
extern CUndo	g_Undo;
extern CXPokey	g_Pokey;
extern CInstruments	g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CModule g_Module;

/////////////////////////////////////////////////////////////////////////////
// CRmtView

IMPLEMENT_DYNCREATE(CRmtView, CView)

BEGIN_MESSAGE_MAP(CRmtView, CView)
	//{{AFX_MSG_MAP(CRmtView)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYDOWN()
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_EXPORT_AS, OnFileExportAs)
	ON_COMMAND(ID_INSTR_LOAD, OnInstrLoad)
	ON_COMMAND(ID_INSTR_SAVE, OnInstrSave)
	ON_COMMAND(ID_INSTR_COPY, OnInstrCopy)
	ON_COMMAND(ID_INSTR_PASTE, OnInstrPaste)
	ON_COMMAND(ID_INSTR_CUT, OnInstrCut)
	ON_COMMAND(ID_INSTR_DELETE, OnInstrDelete)
	ON_COMMAND(ID_TRACK_DELETE, OnTrackDelete)
	ON_COMMAND(ID_TRACK_COPY, OnTrackCopy)
	ON_COMMAND(ID_TRACK_PASTE, OnTrackPaste)
	ON_COMMAND(ID_TRACK_CUT, OnTrackCut)
	ON_COMMAND(ID_SONG_COPYLINE, OnSongCopyline)
	ON_COMMAND(ID_SONG_PASTELINE, OnSongPasteline)
	ON_COMMAND(ID_SONG_CLEARLINE, OnSongClearline)
	ON_COMMAND(ID_PLAY1, OnPlay1)
	ON_COMMAND(ID_PLAY2, OnPlay2)
	ON_COMMAND(ID_PLAY3, OnPlay3)
	ON_COMMAND(ID_PLAYSTOP, OnPlaystop)
	ON_COMMAND(ID_PLAYFOLLOW, OnPlayfollow)
	ON_COMMAND(ID_EM_INFO, OnEmInfo)
	ON_COMMAND(ID_EM_INSTRUMENTS, OnEmInstruments)
	ON_COMMAND(ID_EM_SONG, OnEmSong)
	ON_COMMAND(ID_EM_TRACKS, OnEmTracks)
	ON_UPDATE_COMMAND_UI(ID_EM_TRACKS, OnUpdateEmTracks)
	ON_UPDATE_COMMAND_UI(ID_EM_INSTRUMENTS, OnUpdateEmInstruments)
	ON_UPDATE_COMMAND_UI(ID_EM_INFO, OnUpdateEmInfo)
	ON_UPDATE_COMMAND_UI(ID_EM_SONG, OnUpdateEmSong)
	ON_UPDATE_COMMAND_UI(ID_PLAYFOLLOW, OnUpdatePlayfollow)
	ON_UPDATE_COMMAND_UI(ID_PLAY1, OnUpdatePlay1)
	ON_UPDATE_COMMAND_UI(ID_PLAY2, OnUpdatePlay2)
	ON_UPDATE_COMMAND_UI(ID_PLAY3, OnUpdatePlay3)
	ON_COMMAND(ID_PROVEMODE, OnProvemode)
	ON_UPDATE_COMMAND_UI(ID_PROVEMODE, OnUpdateProvemode)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_VOLUMEANALYZER, OnViewVolumeanalyzer)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VOLUMEANALYZER, OnUpdateViewVolumeanalyzer)
	ON_COMMAND(ID_VIEW_PLAYTIMECOUNTER, OnViewPlaytimecounter)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAYTIMECOUNTER, OnUpdateViewPlaytimecounter)
	ON_COMMAND(ID_VIEW_INSTRUMENTACTIVEHELP, OnViewInstrumentactivehelp)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INSTRUMENTACTIVEHELP, OnUpdateViewInstrumentactivehelp)
	ON_COMMAND(ID_VIEW_BLOCKTOOLBAR, OnViewBlocktoolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BLOCKTOOLBAR, OnUpdateViewBlocktoolbar)
	ON_COMMAND(ID_BLOCK_NOTEUP, OnBlockNoteup)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_NOTEUP, OnUpdateBlockNoteup)
	ON_COMMAND(ID_BLOCK_VOLUMEDOWN, OnBlockVolumedown)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_VOLUMEDOWN, OnUpdateBlockVolumedown)
	ON_COMMAND(ID_BLOCK_VOLUMEUP, OnBlockVolumeup)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_VOLUMEUP, OnUpdateBlockVolumeup)
	ON_COMMAND(ID_BLOCK_NOTEDOWN, OnBlockNotedown)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_NOTEDOWN, OnUpdateBlockNotedown)
	ON_COMMAND(ID_BLOCK_INSTRLEFT, OnBlockInstrleft)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_INSTRLEFT, OnUpdateBlockInstrleft)
	ON_COMMAND(ID_BLOCK_INSTRRIGHT, OnBlockInstrright)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_INSTRRIGHT, OnUpdateBlockInstrright)
	ON_COMMAND(ID_BLOCK_INSTRALL, OnBlockInstrall)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_INSTRALL, OnUpdateBlockInstrall)
	ON_COMMAND(ID_BLOCK_BACKUP, OnBlockBackup)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_BACKUP, OnUpdateBlockBackup)
	ON_COMMAND(ID_BLOCK_PLAY, OnBlockPlay)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_PLAY, OnUpdateBlockPlay)
	ON_COMMAND(ID_CHAN1, OnChan1)
	ON_COMMAND(ID_CHAN2, OnChan2)
	ON_COMMAND(ID_CHAN3, OnChan3)
	ON_COMMAND(ID_CHAN4, OnChan4)
	ON_COMMAND(ID_CHAN5, OnChan5)
	ON_COMMAND(ID_CHAN6, OnChan6)
	ON_COMMAND(ID_CHAN7, OnChan7)
	ON_COMMAND(ID_CHAN8, OnChan8)
	ON_UPDATE_COMMAND_UI(ID_CHAN1, OnUpdateChan1)
	ON_UPDATE_COMMAND_UI(ID_CHAN2, OnUpdateChan2)
	ON_UPDATE_COMMAND_UI(ID_CHAN3, OnUpdateChan3)
	ON_UPDATE_COMMAND_UI(ID_CHAN4, OnUpdateChan4)
	ON_UPDATE_COMMAND_UI(ID_CHAN5, OnUpdateChan5)
	ON_UPDATE_COMMAND_UI(ID_CHAN6, OnUpdateChan6)
	ON_UPDATE_COMMAND_UI(ID_CHAN7, OnUpdateChan7)
	ON_UPDATE_COMMAND_UI(ID_CHAN8, OnUpdateChan8)
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_COMMAND(ID_VIEW_POKEYREGS, OnViewPokeyregs)
	ON_UPDATE_COMMAND_UI(ID_VIEW_POKEYREGS, OnUpdateViewPokeyregs)
	ON_COMMAND(ID_MIDIONOFF, OnMidionoff)
	ON_UPDATE_COMMAND_UI(ID_MIDIONOFF, OnUpdateMidionoff)
	ON_COMMAND(ID_VIEW_CONFIGURATION, OnViewConfiguration)
	ON_COMMAND(ID_VIEW_TUNING, OnViewTuning)
	ON_COMMAND(ID_BLOCK_COPY, OnBlockCopy)
	ON_COMMAND(ID_BLOCK_CUT, OnBlockCut)
	ON_COMMAND(ID_BLOCK_DELETE, OnBlockDelete)
	ON_COMMAND(ID_BLOCK_PASTE, OnBlockPaste)
	ON_COMMAND(ID_BLOCK_EXCHANGE, OnBlockExchange)
	ON_COMMAND(ID_BLOCK_EFFECT, OnBlockEffect)
	ON_COMMAND(ID_BLOCK_SELECTALL, OnBlockSelectall)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_CUT, OnUpdateBlockCut)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_DELETE, OnUpdateBlockDelete)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_EFFECT, OnUpdateBlockEffect)
	ON_UPDATE_COMMAND_UI(ID_BLOCK_EXCHANGE, OnUpdateBlockExchange)
	ON_COMMAND(ID_TRACK_ALLTRACKSCLEANUP, OnTrackAlltrackscleanup)
	ON_COMMAND(ID_INSTR_ALLINSTRUMENTSCLEANUP, OnInstrAllinstrumentscleanup)
	ON_COMMAND(ID_SONG_DELETEACTUALLINE, OnSongDeleteactualline)
	ON_COMMAND(ID_SONG_INSERTNEWEMPTYLINE, OnSongInsertnewemptyline)
	ON_COMMAND(ID_SONG_INSERTNEWLINEWITHUNUSEDTRACKS, OnSongInsertnewlinewithunusedtracks)
	ON_COMMAND(ID_SONG_INSERTCOPYORCLONEOFSONGLINES, OnSongInsertcopyorcloneofsonglines)
	ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
	ON_UPDATE_COMMAND_UI(ID_SONG_SONGSWITCH4_8, OnUpdateSongSongswitch4_8)
	ON_COMMAND(ID_SONG_SONGSWITCH4_8, OnSongSongswitch4_8)
	ON_COMMAND(ID_SONG_TRACKSORDERCHANGE, OnSongTracksorderchange)
	ON_COMMAND(ID_INSTRUMENT_INFO, OnInstrumentInfo)
	ON_COMMAND(ID_INSTRUMENT_CHANGE, OnInstrumentChange)
	ON_COMMAND(ID_TRACK_SEARCHANDBUILDLOOP, OnTrackSearchandbuildloop)
	ON_UPDATE_COMMAND_UI(ID_TRACK_SEARCHANDBUILDLOOP, OnUpdateTrackSearchandbuildloop)
	ON_COMMAND(ID_SONG_SEARCHANDBUILDLOOPSINALLTRACKS, OnSongSearchandrebuildloopsinalltracks)
	ON_COMMAND(ID_TRACK_EXPANDLOOP, OnTrackExpandloop)
	ON_UPDATE_COMMAND_UI(ID_TRACK_EXPANDLOOP, OnUpdateTrackExpandloop)
	ON_COMMAND(ID_SONG_EXPANDLOOPSINALLTRACKS, OnSongExpandloopsinalltracks)
	ON_COMMAND(ID_SONG_SIZEOPTIMIZATION, OnSongSizeoptimization)
	ON_COMMAND(ID_INSTRUMENT_CLEARALLUNUSEDINSTRUMENTS, OnInstrumentClearallunusedinstruments)
	ON_COMMAND(ID_TRACK_CLEARALLTRACKSUNUSEDINSONG, OnTrackClearalltracksunusedinsong)
	ON_UPDATE_COMMAND_UI(ID_TRACK_INFOABOUTUSINGOFACTUALTRACK, OnUpdateTrackInfoaboutusingofactualtrack)
	ON_COMMAND(ID_TRACK_INFOABOUTUSINGOFACTUALTRACK, OnTrackInfoaboutusingofactualtrack)
	ON_COMMAND(ID_TRACK_RENUMBERALLTRACKS, OnTrackRenumberalltracks)
	ON_COMMAND(ID_INSTRUMENT_RENUMBERALLINSTRUMENTS, OnInstrumentRenumberallinstruments)
	ON_UPDATE_COMMAND_UI(ID_TRACK_COPY, OnUpdateTrackCopy)
	ON_UPDATE_COMMAND_UI(ID_TRACK_CUT, OnUpdateTrackCut)
	ON_UPDATE_COMMAND_UI(ID_TRACK_DELETE, OnUpdateTrackDelete)
	ON_UPDATE_COMMAND_UI(ID_TRACK_PASTE, OnUpdateTrackPaste)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMELRENVELOPESONLY, OnInstrumentPastespecialVolumeLRenvelopesonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMELENVELOPEONLY, OnInstrumentPastespecialVolumeLenvelopeonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_TABLEONLY, OnInstrumentPastespecialTableonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMERENVELOPEONLY, OnInstrumentPastespecialVolumeRenvelopeonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_ENVELOPEPARAMETERSONLY, OnInstrumentPastespecialEnvelopeparametersonly)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_PASTESPECIAL_VOLUMERENVELOPEONLY, OnUpdateInstrumentPastespecialVolumerenvelopeonly)
	ON_COMMAND(ID_BLOCK_PASTESPECIAL_VOLUMEVALUESONLY, OnBlockPastespecialVolumevaluesonly)
	ON_COMMAND(ID_BLOCK_PASTESPECIAL_SPEEDVALUESONLY, OnBlockPastespecialSpeedvaluesonly)
	ON_COMMAND(ID_BLOCK_PASTESPECIAL_MERGEWITHCURRENTCONTENT, OnBlockPastespecialMergewithcurrentcontent)
	ON_COMMAND(ID_SONG_PUTNEWEMPTYUNUSEDTRACK, OnSongPutnewemptyunusedtrack)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_TRACK_LOAD, OnTrackLoad)
	ON_COMMAND(ID_TRACK_SAVE, OnTrackSave)
	ON_UPDATE_COMMAND_UI(ID_TRACK_LOAD, OnUpdateTrackLoad)
	ON_UPDATE_COMMAND_UI(ID_TRACK_SAVE, OnUpdateTrackSave)
	ON_COMMAND(ID_TRACK_CLEARALLDUPLICATEDTRACKS, OnTrackClearallduplicatedtracks)
	ON_COMMAND(ID_SONG_MAKETRACKSDUPLICATE, OnSongMaketracksduplicate)
	ON_UPDATE_COMMAND_UI(ID_SONG_MAKETRACKSDUPLICATE, OnUpdateSongMaketracksduplicate)
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_PLAY0, OnPlay0)
	ON_UPDATE_COMMAND_UI(ID_PLAY0, OnUpdatePlay0)
	ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
	ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD, OnUpdateFileReload)
	ON_COMMAND(ID_UNDO_UNDO, OnUndoUndo)
	ON_UPDATE_COMMAND_UI(ID_UNDO_UNDO, OnUpdateUndoUndo)
	ON_COMMAND(ID_UNDO_REDO, OnUndoRedo)
	ON_UPDATE_COMMAND_UI(ID_UNDO_REDO, OnUpdateUndoRedo)
	ON_COMMAND(ID_UNDO_CLEARUNDOREDO, OnUndoClearundoredo)
	ON_UPDATE_COMMAND_UI(ID_UNDO_CLEARUNDOREDO, OnUpdateUndoClearundoredo)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_INSERTVOLUMEENVSANDENVELOPEPARSTOCURSORPOSITION, OnInstrumentPastespecialInsertvolenvsandenvparstocurpos)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_PASTESPECIAL_INSERTVOLUMEENVSANDENVELOPEPARSTOCURSORPOSITION, OnUpdateInstrumentPastespecialInsertvolenvsandenvparstocurpos)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMEENVANDENVELOPEPARSONLY, OnInstrumentPastespecialVolumeenvandenvelopeparsonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMELTORENVELOPEONLY, OnInstrumentPastespecialVolumeltorenvelopeonly)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_PASTESPECIAL_VOLUMELTORENVELOPEONLY, OnUpdateInstrumentPastespecialVolumeltorenvelopeonly)
	ON_COMMAND(ID_INSTRUMENT_PASTESPECIAL_VOLUMERTOLENVELOPEONLY, OnInstrumentPastespecialVolumertolenvelopeonly)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_PASTESPECIAL_VOLUMERTOLENVELOPEONLY, OnUpdateInstrumentPastespecialVolumertolenvelopeonly)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_PASTESPECIAL_VOLUMELENVELOPEONLY, OnUpdateInstrumentPastespecialVolumelenvelopeonly)
	ON_COMMAND(ID_TRACK_CURSORGOTOTHESPEEDCOLUMN, OnTrackCursorgotothespeedcolumn)
	ON_UPDATE_COMMAND_UI(ID_TRACK_CURSORGOTOTHESPEEDCOLUMN, OnUpdateTrackCursorgotothespeedcolumn)
	ON_COMMAND(ID_VIEW_TOOLBAR, OnViewToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateViewToolbar)
	ON_COMMAND(ID_VIEW_STATUS_BAR, OnViewStatusBar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateViewStatusBar)
	ON_COMMAND(ID_SONG_SONGCHANGEMAXIMALLENGTHOFTRACKS, OnSongSongchangemaximallengthoftracks)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_WANTEXIT, OnWantExit)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRmtView construction/destruction

CRmtView::CRmtView()
{
	m_width = 0;
	m_height = 0;
	m_pen1 = NULL;
}

CRmtView::~CRmtView()
{
	if (g_mem_dc) g_mem_dc->SelectObject(m_penorig);		//The return of the original Pen
	if (m_pen1) delete m_pen1;							//unloaded m_pen1
}

void CRmtView::OnDestroy()
{
	// Unload Pokey DLL
	g_Pokey.DeInitSound();

	// Unload 6502 DLL
	Atari6502_DeInit();

	StopTimer(m_timerDisplay);

	CView::OnDestroy();
}

void CRmtView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_timerDisplay)
	{
		//ChangeTimer(m_timerDisplay, g_timerTick[++g_timerDisplayCount % 3]);
		ChangeTimer(m_timerDisplay, 16);
		g_Song.CalculateDisplayFPS();
		AfxGetApp()->GetMainWnd()->Invalidate();
		g_timerGlobalCount++;
		SCREENUPDATE;
	}

	CView::OnTimer(nIDEvent);
}

void CRmtView::ChangeTimer(UINT_PTR nIDEvent, int ms)
{
	if (nIDEvent == m_timerDisplay)
	{
		KillTimer(m_timerDisplay);
		m_timerDisplay = SetTimer(1, ms, NULL);
	}
}

void CRmtView::StopTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_timerDisplay)
	{
		KillTimer(m_timerDisplay);
		m_timerDisplay = NULL;
	}
}

BOOL CRmtView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

BOOL CRmtView::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
	{
		// Workaround in order to differentiate the Left and Right SHIFT, CTRL and ALT keys between each others
		switch (pMsg->wParam)
		{
		case VK_SHIFT:	// SHIFT keys
			pMsg->wParam = MapVirtualKey((pMsg->lParam & 0x00FF0000) >> 16, MAPVK_VSC_TO_VK_EX);
			break;

		case VK_CONTROL:// CTRL keys
			pMsg->wParam = pMsg->lParam & 0x01000000 ? VK_RCONTROL : VK_LCONTROL;
			break;

		case VK_MENU:	// ALT keys
			pMsg->wParam = pMsg->lParam & 0x01000000 ? VK_RMENU : VK_LMENU;
			break;

		case VK_F10:	// Workaround in order to use the F10 key, otherwise it would be ignored
			OnSysKeyDown((UINT)pMsg->wParam, 0, 0);
			return TRUE;
		}
	}

	return CView::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CRmtView drawing

//screen drawing changes involved in order to make use of the window dimensions
void CRmtView::OnSize(UINT nType, int cx, int cy)
{
	TRACE("Rect: %d %d\n", cx, cy);
}

void CRmtView::OnDraw(CDC* pDC)
{
	if (!pDC)
		return;

	// Redraw the screen if needed
	// TODO: Improve this procedure to avoid redrawing everything at once every time
	if (g_screenupdate)
	{
		Resize();
		DrawAll();
		pDC->StretchBlt(0, 0, m_width, m_height, &m_mem_dc, 0, 0, g_width, g_height, SRCCOPY);
		NO_SCREENUPDATE;
	}
}

void CRmtView::DrawAll()
{
	// Clear the screen with the background colour
	m_mem_dc.FillSolidRect(0, 0, m_width, m_height, RGB_BACKGROUND);

	// Get the current Module Subtune pointer
	TSubtune* p = g_Module.GetSubtuneIndex();

	// Draw the primary screen block first
	if (g_active_ti == PART_TRACKS)
		g_Song.DrawPatternEditor(p);
	else
		g_Song.DrawInstrumentEditor(p);

	// Draw the secondary screen block afterwards
	g_Song.DrawSonglines(p);
	g_Song.DrawSubtuneInfos(p);
	g_Song.DrawRegistersState(p);

	// Draw the debug stuff if needed
	g_Song.DrawDebugInfos(p);
}

BOOL CRmtView::OnEraseBkgnd(CDC* pDC) 
{	
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CRmtView printing

BOOL CRmtView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CRmtView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CRmtView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CRmtView diagnostics

#ifdef _DEBUG
void CRmtView::AssertValid() const
{
	CView::AssertValid();
}

void CRmtView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CRmtDoc* CRmtView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRmtDoc)));
	return (CRmtDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRmtView message handlers

void CRmtView::ReadRMTConfig()
{
#define NAME(a)	(strcmp(a,name)==0)

	CString s;
	char line[1024];
	char *tmp, *name, *value;
	s.Format("%s%s", g_prgpath, CONFIG_FILENAME);
	std::ifstream in(s);
	if (!in)
	{
		MessageBox("Could not find: '" + s + "'\n\nRMT will use the default configuration.\n", "RMT", MB_ICONEXCLAMATION);
		ResetRMTConfig();	// In order to save the default configuration file 
		return;
	}

	// Parse individual lines until the end of the file is reached 
	while (!in.eof())
	{
		in.getline(line, 1023);		// Seek for the next character in memory 
		tmp = strchr(line, '=');	// The tmp pointer will be set at the position of '=' 
		if (!tmp) continue;			// Seek for the character until a match is found 
		tmp[-1] = 0;				// Offset by 1 to compensate the Space   
		name = line;				// Name set to the current line, terminated by tmp 
		value = tmp + 2;			// Offset by 1 to compensate the Space 

		// GENERAL
		if (NAME("SCALEPERCENTAGE")) { g_scaling_percentage = atoi(value); continue; }
		if (NAME("TRACKLINEPRIMARYHIGHLIGHT")) { g_trackLinePrimaryHighlight = atoi(value); continue; }
		if (NAME("TRACKLINESECONDARYHIGHLIGHT")) { g_trackLineSecondaryHighlight = atoi(value); continue; }
		if (NAME("TRACKLINEALTNUMBERING")) { g_tracklinealtnumbering = atoi(value); continue; }
		if (NAME("DISPLAYFLATNOTES")) { g_displayflatnotes = atoi(value); continue; }
		if (NAME("USEGERMANNOTATION")) { g_usegermannotation = atoi(value); continue; }
		if (NAME("NTSC_SYSTEM")) { g_ntsc = atoi(value); continue; }
		if (NAME("SMOOTH_SCROLL")) { g_viewDoSmoothScrolling = atoi(value); continue; }
		if (NAME("NOHWSOUNDBUFFER")) { g_nohwsoundbuffer = atoi(value); continue; }
		if (NAME("TRACKERDRIVERVERSION")) { g_trackerDriverVersion = atoi(value); continue; }

		// KEYBOARD
		if (NAME("KEYBOARD_LAYOUT")) { g_keyboard_layout = atoi(value); continue; }
		if (NAME("KEYBOARD_UPDOWNCONTINUE")) { g_keyboard_updowncontinue = atoi(value); continue; }
		if (NAME("KEYBOARD_REMEMBEROCTAVESANDVOLUMES")) { g_keyboard_RememberOctavesAndVolumes = atoi(value); continue; }
		if (NAME("KEYBOARD_ESCRESETATARISOUND")) { g_keyboard_escresetatarisound = atoi(value); continue; }
		if (NAME("KEYBOARD_ASKWHENCONTROL_S")) { g_keyboard_askwhencontrol_s = atoi(value); continue; }

		// MIDI
		if (NAME("MIDI_IN")) { g_Midi.SetDevice(value); continue; }
		if (NAME("MIDI_TR")) { g_Midi.m_TouchResponse = atoi(value); continue; }
		if (NAME("MIDI_VOLUMEOFFSET")) { g_Midi.m_VolumeOffset = atoi(value); continue; }
		if (NAME("MIDI_NOTEOFF")) { g_Midi.m_NoteOff = atoi(value); continue; }
	
		// PATHS
		if (NAME("PATH_DEFAULTSONGS")) { g_defaultSongsPath = value; continue; }
		if (NAME("PATH_DEFAULTINSTRUMENTS")) { g_defaultInstrumentsPath = value; continue; }
		if (NAME("PATH_DEFAULTTRACKS")) { g_defaultTracksPath = value; continue; }
		if (NAME("PATH_LASTSONGS")) { g_lastLoadPath_Songs = value; continue; }
		if (NAME("PATH_LASTINSTRUMENTS")) { g_lastLoadPath_Instruments = value; continue; }
		if (NAME("PATH_LASTTRACKS")) { g_lastLoadPath_Tracks = value; continue; }

		// VIEW 
		if (NAME("VIEW_MAINTOOLBAR")) { g_viewMainToolbar = atoi(value); continue; }
		if (NAME("VIEW_BLOCKTOOLBAR")) { g_viewBlockToolbar = atoi(value); continue; }
		if (NAME("VIEW_STATUSBAR")) { g_viewStatusBar = atoi(value); continue; }
		if (NAME("VIEW_PLAYTIMECOUNTER")) { g_viewPlayTimeCounter = atoi(value); continue; }
		if (NAME("VIEW_VOLUMEANALYZER")) { g_viewVolumeAnalyzer = atoi(value); continue; }
		if (NAME("VIEW_POKEYCHIPREGISTERS")) { g_viewPokeyRegisters = atoi(value); continue; }
		if (NAME("VIEW_INSTRUMENTACTIVEHELP")) { g_viewInstrumentEditHelp = atoi(value); continue; }
		if (NAME("VIEW_DEBUGDISPLAY")) { g_viewDebugDisplay = atoi(value); continue; }
	}
	in.close();
}

void CRmtView::WriteRMTConfig()
{
	CString s;
	s.Format("%s%s", g_prgpath, CONFIG_FILENAME);
	std::ofstream ou(s);
	if (!ou)
	{
		MessageBox("Could not create: '" + s + "'\n\nThe RMT configuration won't be saved.\n", "RMT", MB_ICONEXCLAMATION);
		return;
	}

	ou << "# RMT CONFIGURATION FILE" << std::endl;
	CString version;
	version.LoadString(IDS_RMTVERSION);
	ou << "# " << version << std::endl;
	ou << std::setprecision(16);

	ou << "\n# GENERAL\n" << std::endl;
	ou << "SCALEPERCENTAGE = " << g_scaling_percentage << std::endl;
	ou << "TRACKLINEPRIMARYHIGHLIGHT = " << g_trackLinePrimaryHighlight << std::endl;
	ou << "TRACKLINESECONDARYHIGHLIGHT = " << g_trackLineSecondaryHighlight << std::endl;
	ou << "TRACKLINEALTNUMBERING = " << g_tracklinealtnumbering << std::endl;
	ou << "DISPLAYFLATNOTES = " << g_displayflatnotes << std::endl;
	ou << "USEGERMANNOTATION = " << g_usegermannotation << std::endl;
	ou << "NTSC_SYSTEM = " << g_ntsc << std::endl;
	ou << "SMOOTH_SCROLL = " << g_viewDoSmoothScrolling << std::endl;
	ou << "NOHWSOUNDBUFFER = " << g_nohwsoundbuffer << std::endl;
	ou << "TRACKERDRIVERVERSION = " << g_trackerDriverVersion << std::endl;

	ou << "\n# KEYBOARD\n" << std::endl;
	ou << "KEYBOARD_LAYOUT = " << g_keyboard_layout << std::endl;
	ou << "KEYBOARD_UPDOWNCONTINUE = " << g_keyboard_updowncontinue << std::endl;
	ou << "KEYBOARD_REMEMBEROCTAVESANDVOLUMES = " << g_keyboard_RememberOctavesAndVolumes << std::endl;
	ou << "KEYBOARD_ESCRESETATARISOUND = " << g_keyboard_escresetatarisound << std::endl;
	ou << "KEYBOARD_ASKWHENCONTROL_S = " << g_keyboard_askwhencontrol_s << std::endl;

	ou << "\n# MIDI\n" << std::endl;
	ou << "MIDI_IN = " << g_Midi.GetMidiDevName() << std::endl;
	ou << "MIDI_TR = " << g_Midi.m_TouchResponse << std::endl;
	ou << "MIDI_VOLUMEOFFSET = " << g_Midi.m_VolumeOffset << std::endl;
	ou << "MIDI_NOTEOFF = " << g_Midi.m_NoteOff << std::endl;

	ou << "\n# PATHS\n" << std::endl;
	ou << "PATH_DEFAULTSONGS = " << g_defaultSongsPath << std::endl;
	ou << "PATH_DEFAULTINSTRUMENTS = " << g_defaultInstrumentsPath << std::endl;
	ou << "PATH_DEFAULTTRACKS = " << g_defaultTracksPath << std::endl;
	ou << "PATH_LASTSONGS = " << g_lastLoadPath_Songs << std::endl;
	ou << "PATH_LASTINSTRUMENTS = " << g_lastLoadPath_Instruments << std::endl;
	ou << "PATH_LASTTRACKS = " << g_lastLoadPath_Tracks << std::endl;

	ou << "\n# VIEW\n" << std::endl;
	ou << "VIEW_MAINTOOLBAR = " << g_viewMainToolbar << std::endl;
	ou << "VIEW_BLOCKTOOLBAR = " << g_viewBlockToolbar << std::endl;
	ou << "VIEW_STATUSBAR = " << g_viewStatusBar << std::endl;
	ou << "VIEW_PLAYTIMECOUNTER = " << g_viewPlayTimeCounter << std::endl;
	ou << "VIEW_VOLUMEANALYZER = " << g_viewVolumeAnalyzer << std::endl;
	ou << "VIEW_POKEYCHIPREGISTERS = " << g_viewPokeyRegisters << std::endl;
	ou << "VIEW_INSTRUMENTACTIVEHELP = " << g_viewInstrumentEditHelp << std::endl;
	ou << "VIEW_DEBUGDISPLAY = " << g_viewDebugDisplay << std::endl;

	ou.close();
}

void CRmtView::ResetRMTConfig()
{
	g_scaling_percentage = 100;					// RMT interface scaling (in percentage) 
	g_trackLinePrimaryHighlight = 8;			// Primary line highlighted every x lines
	g_trackLineSecondaryHighlight = 4;			// Secondary line highlighted every x lines
	g_tracklinealtnumbering = 0;				// Alternative way of line numbering in tracks 
	g_linesafter = 1;							// Number of lines to scroll after inserting a note 
	g_ntsc = 0;									// NTSC (60Hz)
	g_nohwsoundbuffer = 0;						// Don't use hardware soundbuffer
	g_trackerDriverVersion = TRACKER_DRIVER_PATCH16;
	g_displayflatnotes = 0;						// Display accidentals as Flats instead of Sharps
	g_usegermannotation = 0;					// Display H notes instead of B
	g_viewMainToolbar = 1;						// Display the Main Toolbar
	g_viewBlockToolbar = 1;						// Display the Block Toolbar 
	g_viewStatusBar = 1;						// Display the Status Bar
	g_viewPlayTimeCounter = 1;					// Display the Play Time and BPM Counter
	g_viewVolumeAnalyzer = 1;					// Display the Volume Analyser Bars
	g_viewPokeyRegisters = 1;					// Display the POKEY Registers (TODO: Move the Detailed Registers to its own entry) 
	g_viewInstrumentEditHelp = 1;				// Display useful info when editing various parts of an instrument
	g_viewDoSmoothScrolling = 1;				// Smoothly scroll the track and song line data is smooth during playback 
	g_viewDebugDisplay = 1;						// Debug display for a bunch of variables used for various tasks 
	g_lastLoadPath_Songs = "";					// Path of the last song loaded
	g_lastLoadPath_Instruments = "";			// Path of the last instrument loaded
	g_lastLoadPath_Tracks = "";					// Path of the last track loaded
	g_defaultSongsPath = "";					// Default path for songs
	g_defaultInstrumentsPath = "";				// Default path for instruments
	g_defaultTracksPath = "";					// Default path for tracks
	g_keyboard_layout = KEYBOARD_QWERTY;		// Keyboard layout used by RMT. eg: QWERTY, AZERTY, etc 
	g_keyboard_updowncontinue = 1;				// Scroll to the next/previous Songline when the Pattern limits are crossed 
	g_keyboard_RememberOctavesAndVolumes = 1;	// Remember the last octave and volume values used with an Instrument 
	g_keyboard_escresetatarisound = 1;			// Reset the RMT Atari routines if the ESC key is pressed 
	g_keyboard_askwhencontrol_s = 1;			// Prompt a dialog box upon hitting CTRL+S to ask if it is OK to overwrite the file 
	g_Midi.SetDevice("");						// MIDI Device
	g_Midi.m_TouchResponse = 0;					// MIDI Touch response
	g_Midi.m_VolumeOffset = 0;					// MIDI Volume offset
	g_Midi.m_NoteOff = 0;						// MIDI Note Off 
	g_Midi.MidiInit();							// MIDI must be initialised just in case 
	WriteRMTConfig();							// Write the default configuration file 
}

void CRmtView::ReadTuningConfig()
{
#define NAME(a)	(strcmp(a,name)==0)

	CString s;
	char line[1024];
	char* tmp, * div, * name, * value, * value2;
	s.Format("%s%s", g_prgpath, TUNING_FILENAME);
	std::ifstream in(s);
	if (!in)
	{
		MessageBox("Could not find: '" + s + "'\n\nRMT will use the default Tuning parameters.\n", "RMT", MB_ICONEXCLAMATION);
		g_Song.ResetTuningVariables();
		WriteTuningConfig();	// In order to save the default Tuning configuration file 
		return;
	}

	// Parse individual lines until the end of the file is reached 
	while (!in.eof())
	{
		in.getline(line, 1023);		// Seek for the next character in memory 
		tmp = strchr(line, '=');	// The tmp pointer will be set at the position of '=' 
		div = strchr(line, '/');	// The div pointer will be set at the position of '/'
		if (!tmp) continue;			// Seek for the character until a match is found 
		tmp[-1] = 0;				// Offset by 1 to compensate the Space   
		name = line;				// Name set to the current line, terminated by tmp 
		value = tmp + 2;			// Offset by 1 to compensate the Space 
		if (div)					// The div pointer is used to get the 2nd Ratio value 
		{
			div[-1] = 0;			// Same as above, offset by 1 to compensate the Space(s) 
			value2 = div + 2;
		}

		// TUNING 
		if (NAME("TUNING")) { g_basetuning = atof(value); continue; }
		if (NAME("BASENOTE")) { g_basenote = atoi(value); continue; }
		if (NAME("TEMPERAMENT")) { g_temperament = atoi(value); continue; }

		// RATIO
		if (NAME("UNISON")) { g_UNISON_L = atoi(value); if (div) g_UNISON_R = atoi(value2); continue; }
		if (NAME("MIN_2ND")) { g_MIN_2ND_L = atoi(value); if (div) g_MIN_2ND_R = atoi(value2); continue; }
		if (NAME("MAJ_2ND")) { g_MAJ_2ND_L = atoi(value); if (div) g_MAJ_2ND_R = atoi(value2); continue; }
		if (NAME("MIN_3RD")) { g_MIN_3RD_L = atoi(value); if (div) g_MIN_3RD_R = atoi(value2); continue; }
		if (NAME("MAJ_3RD")) { g_MAJ_3RD_L = atoi(value); if (div) g_MAJ_3RD_R = atoi(value2); continue; }
		if (NAME("PERF_4TH")) { g_PERF_4TH_L = atoi(value); if (div) g_PERF_4TH_R = atoi(value2); continue; }
		if (NAME("TRITONE")) { g_TRITONE_L = atoi(value); if (div) g_TRITONE_R = atoi(value2); continue; }
		if (NAME("PERF_5TH")) { g_PERF_5TH_L = atoi(value); if (div) g_PERF_5TH_R = atoi(value2); continue; }
		if (NAME("MIN_6TH")) { g_MIN_6TH_L = atoi(value); if (div) g_MIN_6TH_R = atoi(value2); continue; }
		if (NAME("MAJ_6TH")) { g_MAJ_6TH_L = atoi(value); if (div) g_MAJ_6TH_R = atoi(value2); continue; }
		if (NAME("MIN_7TH")) { g_MIN_7TH_L = atoi(value); if (div) g_MIN_7TH_R = atoi(value2); continue; }
		if (NAME("MAJ_7TH")) { g_MAJ_7TH_L = atoi(value); if (div) g_MAJ_7TH_R = atoi(value2); continue; }
		if (NAME("OCTAVE")) { g_OCTAVE_L = atoi(value); if (div) g_OCTAVE_R = atoi(value2); continue; }
	}
	in.close();
}

void CRmtView::WriteTuningConfig()
{
	CString s;
	s.Format("%s%s", g_prgpath, TUNING_FILENAME);
	std::ofstream ou(s);
	if (!ou)
	{
		MessageBox("Could not create: '" + s + "'\n\nThe Tuning parameters won't be saved.\n", "RMT", MB_ICONEXCLAMATION);
		return;
	}

	ou << "# RMT CONFIGURATION FILE" << std::endl;
	CString version;
	version.LoadString(IDS_RMTVERSION);
	ou << "# " << version << std::endl;
	ou << std::setprecision(16);

	ou << "\n# TUNING\n" << std::endl;
	ou << "TUNING = " << g_basetuning << std::endl;
	ou << "BASENOTE = " << g_basenote << std::endl;
	ou << "TEMPERAMENT = " << g_temperament << std::endl;

	ou << "\n# RATIO\n" << std::endl;
	ou << "UNISON = " << g_UNISON_L << " / " << g_UNISON_R << std::endl;
	ou << "MIN_2ND = " << g_MIN_2ND_L << " / " << g_MIN_2ND_R << std::endl;
	ou << "MAJ_2ND = " << g_MAJ_2ND_L << " / " << g_MAJ_2ND_R << std::endl;
	ou << "MIN_3RD = " << g_MIN_3RD_L << " / " << g_MIN_3RD_R << std::endl;
	ou << "MAJ_3RD = " << g_MAJ_3RD_L << " / " << g_MAJ_3RD_R << std::endl;
	ou << "PERF_4TH = " << g_PERF_4TH_L << " / " << g_PERF_4TH_R << std::endl;
	ou << "TRITONE = " << g_TRITONE_L << " / " << g_TRITONE_R << std::endl;
	ou << "PERF_5TH = " << g_PERF_5TH_L << " / " << g_PERF_5TH_R << std::endl;
	ou << "MIN_6TH = " << g_MIN_6TH_L << " / " << g_MIN_6TH_R << std::endl;
	ou << "MAJ_6TH = " << g_MAJ_6TH_L << " / " << g_MAJ_6TH_R << std::endl;
	ou << "MIN_7TH = " << g_MIN_7TH_L << " / " << g_MIN_7TH_R << std::endl;
	ou << "MAJ_7TH = " << g_MAJ_7TH_L << " / " << g_MAJ_7TH_R << std::endl;
	ou << "OCTAVE = " << g_OCTAVE_L << " / " << g_OCTAVE_R << std::endl;

	ou.close();
}

void CRmtView::OnViewConfiguration() 
{
	CConfigDlg dlg;

	// GENERAL
	dlg.m_scaling_percentage = g_scaling_percentage;
	dlg.m_trackLinePrimaryHighlight = g_trackLinePrimaryHighlight;
	dlg.m_trackLineSecondaryHighlight = g_trackLineSecondaryHighlight;
	dlg.m_tracklinealtnumbering = g_tracklinealtnumbering;
	dlg.m_displayflatnotes = g_displayflatnotes;
	dlg.m_usegermannotation = g_usegermannotation;
	dlg.m_ntsc = g_ntsc;
	dlg.m_doSmoothScrolling = g_viewDoSmoothScrolling;
	dlg.m_nohwsoundbuffer = g_nohwsoundbuffer;
	dlg.m_viewDebugDisplay = g_viewDebugDisplay;
	dlg.m_trackerDriverVersion = g_trackerDriverVersion;
	
	// KEYBOARD
	dlg.m_keyboard_layout = g_keyboard_layout;
	dlg.m_keyboard_escresetatarisound = g_keyboard_escresetatarisound;
	dlg.m_keyboard_updowncontinue = g_keyboard_updowncontinue;
	dlg.m_keyboard_rememberoctavesandvolumes = g_keyboard_RememberOctavesAndVolumes;
	dlg.m_keyboard_askwhencontrol_s = g_keyboard_askwhencontrol_s;

	// MIDI 
	dlg.m_midi_device = g_Midi.GetMidiDevId();
	dlg.m_midi_TouchResponse = g_Midi.m_TouchResponse;
	dlg.m_midi_VolumeOffset = g_Midi.m_VolumeOffset;
	dlg.m_midi_NoteOff = g_Midi.m_NoteOff;
	
	if (dlg.DoModal()==IDOK)
	{
		// GENERAL
		if (g_scaling_percentage != dlg.m_scaling_percentage)
		{
			g_scaling_percentage = dlg.m_scaling_percentage;
			m_width = m_height = 0;
			Resize();	// Necessary to scale everything without manually resizing the window first
		}

		if (g_nohwsoundbuffer != dlg.m_nohwsoundbuffer)
		{
			g_Pokey.ReInitSound();	//the sound needs to be reinitialized
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		g_nohwsoundbuffer = dlg.m_nohwsoundbuffer;

		if (g_ntsc != dlg.m_ntsc)
		{
			// PAL or NTSC
			g_ntsc = dlg.m_ntsc;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		g_ntsc = dlg.m_ntsc;

		if (g_trackerDriverVersion != dlg.m_trackerDriverVersion)
		{
			// Something here to reset the thing
			g_trackerDriverVersion = dlg.m_trackerDriverVersion;
			Atari_LoadRMTRoutines();
			Atari_InitRMTRoutine();
		}
		g_trackerDriverVersion = dlg.m_trackerDriverVersion;

		g_viewDoSmoothScrolling = dlg.m_doSmoothScrolling;
		g_viewDebugDisplay = dlg.m_viewDebugDisplay;

		g_trackLinePrimaryHighlight = dlg.m_trackLinePrimaryHighlight;
		g_trackLineSecondaryHighlight = dlg.m_trackLineSecondaryHighlight;
		g_tracklinealtnumbering = dlg.m_tracklinealtnumbering;
		g_displayflatnotes = dlg.m_displayflatnotes;
		g_usegermannotation = dlg.m_usegermannotation;

		// KEYBOARD
		g_keyboard_layout = dlg.m_keyboard_layout;
		g_keyboard_escresetatarisound = dlg.m_keyboard_escresetatarisound;
		g_keyboard_updowncontinue=dlg.m_keyboard_updowncontinue;
		g_keyboard_RememberOctavesAndVolumes = dlg.m_keyboard_rememberoctavesandvolumes;
		g_keyboard_askwhencontrol_s = dlg.m_keyboard_askwhencontrol_s;

		// MIDI
		if (dlg.m_midi_device>=0)
		{
			MIDIINCAPS micaps;
			midiInGetDevCaps(dlg.m_midi_device,&micaps, sizeof(MIDIINCAPS));
			g_Midi.SetDevice(micaps.szPname);
		}
		else
			g_Midi.SetDevice("");
		g_Midi.m_TouchResponse = dlg.m_midi_TouchResponse;
		g_Midi.m_VolumeOffset = dlg.m_midi_VolumeOffset;
		g_Midi.m_NoteOff = dlg.m_midi_NoteOff;
		g_Midi.MidiInit();
	}
}

void CRmtView::OnViewTuning()
{
	TuningDlg dlg;
	dlg.m_basetuning = g_basetuning;
	dlg.m_basenote = g_basenote; 
	dlg.m_temperament = g_temperament;

	// Ratio left
	dlg.UNISON_L = g_UNISON_L;
	dlg.MIN_2ND_L = g_MIN_2ND_L;
	dlg.MAJ_2ND_L = g_MAJ_2ND_L;
	dlg.MIN_3RD_L = g_MIN_3RD_L;
	dlg.MAJ_3RD_L = g_MAJ_3RD_L;
	dlg.PERF_4TH_L = g_PERF_4TH_L;
	dlg.TRITONE_L = g_TRITONE_L;
	dlg.PERF_5TH_L = g_PERF_5TH_L;
	dlg.MIN_6TH_L = g_MIN_6TH_L;
	dlg.MAJ_6TH_L = g_MAJ_6TH_L;
	dlg.MIN_7TH_L = g_MIN_7TH_L;
	dlg.MAJ_7TH_L = g_MAJ_7TH_L;
	dlg.OCTAVE_L = g_OCTAVE_L;

	// Ratio right
	dlg.UNISON_R = g_UNISON_R;
	dlg.MIN_2ND_R = g_MIN_2ND_R;
	dlg.MAJ_2ND_R = g_MAJ_2ND_R;
	dlg.MIN_3RD_R = g_MIN_3RD_R;
	dlg.MAJ_3RD_R = g_MAJ_3RD_R;
	dlg.PERF_4TH_R = g_PERF_4TH_R;
	dlg.TRITONE_R = g_TRITONE_R;
	dlg.PERF_5TH_R = g_PERF_5TH_R;
	dlg.MIN_6TH_R = g_MIN_6TH_R;
	dlg.MAJ_6TH_R = g_MAJ_6TH_R;
	dlg.MIN_7TH_R = g_MIN_7TH_R;
	dlg.MAJ_7TH_R = g_MAJ_7TH_R;
	dlg.OCTAVE_R = g_OCTAVE_R;

	if (dlg.DoModal() == IDOK)
	{
		// Ratio left
		g_UNISON_L = dlg.UNISON_L;
		g_MIN_2ND_L = dlg.MIN_2ND_L;
		g_MAJ_2ND_L = dlg.MAJ_2ND_L;
		g_MIN_3RD_L = dlg.MIN_3RD_L;
		g_MAJ_3RD_L = dlg.MAJ_3RD_L;
		g_PERF_4TH_L = dlg.PERF_4TH_L;
		g_TRITONE_L = dlg.TRITONE_L;
		g_PERF_5TH_L = dlg.PERF_5TH_L;
		g_MIN_6TH_L = dlg.MIN_6TH_L;
		g_MAJ_6TH_L = dlg.MAJ_6TH_L;
		g_MIN_7TH_L = dlg.MIN_7TH_L;
		g_MAJ_7TH_L = dlg.MAJ_7TH_L;
		g_OCTAVE_L = dlg.OCTAVE_L;

		// Ratio right
		g_UNISON_R = dlg.UNISON_R;
		g_MIN_2ND_R = dlg.MIN_2ND_R;
		g_MAJ_2ND_R = dlg.MAJ_2ND_R;
		g_MIN_3RD_R = dlg.MIN_3RD_R;
		g_MAJ_3RD_R = dlg.MAJ_3RD_R;
		g_PERF_4TH_R = dlg.PERF_4TH_R;
		g_TRITONE_R = dlg.TRITONE_R;
		g_PERF_5TH_R = dlg.PERF_5TH_R;
		g_MIN_6TH_R = dlg.MIN_6TH_R;
		g_MAJ_6TH_R = dlg.MAJ_6TH_R;
		g_MIN_7TH_R = dlg.MIN_7TH_R;
		g_MAJ_7TH_R = dlg.MAJ_7TH_R;
		g_OCTAVE_R = dlg.OCTAVE_R;
		
		// Update tuning
		g_basetuning = dlg.m_basetuning;
		g_basenote = dlg.m_basenote;
		g_temperament = dlg.m_temperament;
		g_Tuning.init_tuning();
	}
}

void GetCommandLineItem(CString& commandline,int& fromidx,int& toidx)
{
	BOOL uvo=0;
	while(fromidx<commandline.GetLength() && commandline.GetAt(fromidx)==' ') fromidx++;
	if (fromidx>=commandline.GetLength()) { fromidx=toidx=0; return; }
	for(toidx=fromidx; toidx<commandline.GetLength(); toidx++)
	{
		char a=commandline.GetAt(toidx);
		if (a=='"')
		{
			uvo^=1;
			if (uvo==1) fromidx=toidx+1; else return;
		}
		else
		if (a==' ' && uvo==0) return;
	}
}

// Check for resized window, return without changing anything if the window was not resized 
void CRmtView::Resize()
{
	RECT r;
	GetClientRect(&r);

	int width = r.right - r.left;
	int height = r.bottom - r.top;

	// If the current dimensions are the same, there is nothing to be done here
	if (width == m_width && height == m_height) return;

	// If the values are beyond those limits, reset the default scaling as a failsafe
	if (g_scaling_percentage > 300 || g_scaling_percentage < 100) g_scaling_percentage = 100;

	// Set the screen dimensions as well as the scaled screen dimensions
	m_width = width;
	m_height = height;
	g_width = INVERSE_SCALE(m_width);
	g_height = INVERSE_SCALE(m_height);

	// The number of track lines that can be displayed is based on the scaled window height
	g_tracklines = (g_height - (TRACKS_Y + 3 * 16) - 40) / 16;
	g_line_y = g_tracklines / 2;

	// Clear the current Bitmap object
	m_mem_dc.SelectObject((CBitmap*)0);
	m_mem_bitmap.DeleteObject();
	m_mem_dc.DeleteDC();

	// Initialise the parameters for the resized screen
	CDC* dc = GetDC();
	m_mem_bitmap.CreateCompatibleBitmap(dc, m_width, m_height);
	m_mem_dc.CreateCompatibleDC(dc);
	m_mem_dc.SelectObject(&m_mem_bitmap);
	g_mem_dc = &m_mem_dc;
	if (m_pen1) delete m_pen1;
	m_pen1 = new CPen(PS_SOLID, 1, RGB_LINES);
	m_penorig = g_mem_dc->SelectObject(m_pen1);
	ReleaseDC(dc);
}

void CRmtView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	//command line
	CString cmdl = GetCommandLine();
	CString commandLineFilename = "";
	g_prgpath = "";
	g_lastLoadPath_Songs = g_lastLoadPath_Instruments = g_lastLoadPath_Tracks = "";
	if (cmdl!="")
	{
		int i1=0,i2=0;
		GetCommandLineItem(cmdl,i2,i1);	//path/name.exe
		if (i1!=i2)
		{
			CString exefilename=cmdl.Mid(i2,i1-i2);
			int l=exefilename.ReverseFind('/');
			if (l<0) l=exefilename.ReverseFind('\\');
			if (l>=0)
			{
				g_prgpath = exefilename.Left(l+1);	//including slash
			}
		}
		i1++;
		GetCommandLineItem(cmdl,i1,i2);	//parameter
		if (i1!=i2)
		{
			commandLineFilename = cmdl.Mid(i1, i2 - i1);
		}
	}

	CDC* dc = GetDC();
	m_gfx_bitmap.LoadBitmap(MAKEINTRESOURCE(IDB_GFX));
	m_gfx_dc.CreateCompatibleDC(dc);
	m_gfx_dc.SelectObject(&m_gfx_bitmap);
	g_gfx_dc = &m_gfx_dc;
	g_hwnd = AfxGetApp()->GetMainWnd()->m_hWnd;
	g_viewhwnd = this->m_hWnd;
	ReleaseDC(dc);

	//cursor
	m_cursororig = LoadCursor(NULL,IDC_ARROW);
	m_cursorChanbelOnOff = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORCHANNELONOFF));
	m_cursorEnvelopVolume = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORENVVOLUME));
	m_cursorGoto = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORGOTO));
	m_cursorDialog = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORDLG));
	m_cursorSetPosition = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORSETPOS));

	//current parts
	g_activepart = PART_TRACKS;	//tracks
	g_active_ti = PART_TRACKS;	//below the active tracks

	//turn on all channels
	SetChannelOnOff(-1,1);

	//CONFIGURATION
	ReadRMTConfig();

	//tuning
	ReadTuningConfig(); 

	//view elements
	ChangeViewElements(0); //without write!

	//INITIAL POKEY INITIALISATION (DLL)
	if (!g_Pokey.InitSound())
	{
		g_Pokey.DeInitSound();
		exit(1);
	}

	//INITIAL 6502 INITIALIZATION (DLL)
	if (!Atari6502_Init())
	{
		Atari6502_DeInit();
		exit(1);
	}

	//INITIALISATION OF ATARI RMT ROUTINES
	Memory_Clear();
	Atari_LoadRMTRoutines();
	Atari_InitRMTRoutine();
	g_Song.SetRMTTitle();

	// RMTView Timer Initialisation
	ChangeTimer(m_timerDisplay, 16);
	
	//Displays the ABOUT dialog if there is no Pokey or 6502 initialized...
	if (!g_Pokey.IsSoundDriverLoaded() || !g_is6502)
	{
		AfxGetApp()->GetMainWnd()->PostMessage(WM_COMMAND,ID_APP_ABOUT,0);
	}

	//Initialise MIDI
	g_Midi.MidiInit();
	g_Midi.MidiOn();

	//Pal or NTSC
	g_Song.ChangeTimer((g_ntsc)? 17 : 20);

	//If the tracker was started with an argument, it attempts to load the file, and will return an error if the extention isn't .rmt. 
	//When no argument is passed, the initialisation continues like normal.
	if (commandLineFilename != "")
	{
		if (commandLineFilename.Right(4) == ".rmt")
		{
			g_Song.FileOpen((LPCTSTR)commandLineFilename, FALSE);
		}
		else
		{
			MessageBox("Invalid .rmt file!", "Error", MB_ICONERROR);
			exit(1);
		}
	}
}

/*
int CRmtView::MouseAction(CPoint point,UINT mousebutt,short wheelzDelta=0)
{
	int i;
	int px,py;

	// Scale the mouse XY coordinates to the actual display scaling, so the hitboxes will match everything visually rendered
	point.x = INVERSE_SCALE(point.x);
	point.y = INVERSE_SCALE(point.y);

	// Store the last known mouse XY coordinates and buttons used
	GetMouseXY(point.x, point.y, mousebutt, wheelzDelta);

	//TODO: make those parameters global so they won't have to be re-initialised in multiple functions separately
	int MINIMAL_WIDTH_TRACKS = (g_tracks4_8 > 4 && g_active_ti == PART_TRACKS) ? 1420 : 960;
	int MINIMAL_WIDTH_INSTRUMENTS = (g_tracks4_8 > 4 && g_active_ti == PART_INSTRUMENTS) ? 1220 : 1220;
	int WINDOW_OFFSET = (g_width < 1320 && g_tracks4_8 > 4 && g_active_ti == PART_TRACKS) ? -250 : 0;	//test displacement with the window size
	int INSTRUMENT_OFFSET = (g_active_ti == PART_INSTRUMENTS && g_tracks4_8 > 4) ? -250 : 0;
	if (g_tracks4_8 == 4 && g_active_ti == PART_INSTRUMENTS && g_width > MINIMAL_WIDTH_INSTRUMENTS - 220) INSTRUMENT_OFFSET = 260;
	int SONG_OFFSET = SONG_X + WINDOW_OFFSET + INSTRUMENT_OFFSET + ((g_tracks4_8 == 4) ? -200 : 310);	//displace the SONG block depending on certain parameters

	int linescount = (WINDOW_OFFSET) ? 5 : 9;	//songlines displayed depend on the window offset, if it's displaced to the left side, only 5 lines will be visible, else, 9 will be displayed

	//SONG PARTS
	CRect rec(SONG_OFFSET + 6 * 8, SONG_Y + 16, SONG_OFFSET + 6 * 8 + g_tracks4_8 * 3 * 8 - 8, SONG_Y + 16 + linescount * 16); 
	if (rec.PtInRect(point))
	{
		//Song
		SetCursor(m_cursorGoto);
		
		if (mousebutt & MK_LBUTTON)
		{
			int lineoffset = (WINDOW_OFFSET) ? SONG_Y + 16 : SONG_Y + 48;
			g_Song.SongCursorGoto(CPoint(point.x - (SONG_OFFSET + 6 * 8), point.y - lineoffset));
		}
		if (wheelzDelta != 0)
		{
			//g_Song.SongJump((wheelzDelta / 256) * -1);
			if (wheelzDelta > 0) g_Song.SongKey(VK_UP, 0, 0);
			if (wheelzDelta < 0) g_Song.SongKey(VK_DOWN, 0, 0);
		}
		return 5;
	}

	rec.SetRect(SONG_OFFSET+6*8,SONG_Y,SONG_OFFSET+6*8+g_tracks4_8*3*8-8,SONG_Y+16);
	if (rec.PtInRect(point))
	{
		//over Song L1-R4 for channel on/off/solo/inversion
		i = (point.x + 4 - (SONG_OFFSET+6*8)) / (8*3);
		if (i<0) i=0;
		else
		{	if (i>=g_tracks4_8) i=g_tracks4_8-1; }
		px = i;
		SetCursor(m_cursorChanbelOnOff);
		if (mousebutt & MK_LBUTTON)
		{
			SetChannelOnOff(px,-1);	//inversion
		}
		if (mousebutt & MK_RBUTTON)
		{
			SetChannelSolo(px);		//solo/mute on off
		}
		return 1;
	}

	//INFO PARTS
	rec.SetRect(64, 32, 64 + SONG_NAME_MAX_LEN * 8, 32 + 16);
	if (rec.PtInRect(point))
	{
		//Song name
		SetCursor(m_cursorGoto);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoSongname(point.x - 64);
		}
		return 6;
	}

	rec.SetRect(120, 48, 120 + 7 * 8, 48 + 16);
	if (rec.PtInRect(point))
	{
		//Song speed
		SetCursor(m_cursorGoto);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoSpeed(point.x - 120);
		}
		return 6;
	}

	rec.SetRect(336, 48, 336 + 2 * 8, 48 + 16);
	if (rec.PtInRect(point))
	{
		//MAXTRACKLENGTH
		SetCursor(m_cursorDialog);
		if (mousebutt & MK_LBUTTON)
		{
			OnSongSongchangemaximallengthoftracks();
		}
		return 6;
	}

	rec.SetRect(384, 48, 384 + 8 * ((g_tracks4_8 == 8) ? 15 : 13), 48 + 16);
	if (rec.PtInRect(point))
	{
		//MONO-4-TRACKS or STEREO-8-TRACKS
		SetCursor(m_cursorDialog);
		if (mousebutt & MK_LBUTTON)
		{
			OnSongSongswitch4_8();
		}
		return 6;
	}

	rec.SetRect(280, 16, 280 + 8 * ((g_ntsc) ? 4 : 3), 16 + 16);
	if (rec.PtInRect(point))
	{
		//PAL or NTSC
		SetCursor(m_cursorGoto);
		if (mousebutt & MK_LBUTTON)
		{
			g_ntsc ^=1;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		return 6;
	}

	rec.SetRect(432, 16, 432 + 8 * 5, 16 + 16);
	if (rec.PtInRect(point))
	{
		//track line highlights
		int ma = g_Tracks.GetMaxTrackLength() / 2;
		int px = (point.x - 432 - 4) / 8;
		SetCursor(m_cursorGoto);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoHighlight(point.x - 432);
		}
		if (wheelzDelta != 0)
		{
			if (px < 2)	//primary line highlight
			{
				if (wheelzDelta < 0)
				{
					g_trackLinePrimaryHighlight++;
					if (g_trackLinePrimaryHighlight > ma) g_trackLinePrimaryHighlight = ma;
				}
				else if (wheelzDelta > 0)
				{
					g_trackLinePrimaryHighlight--;
					if (g_trackLinePrimaryHighlight < 1) g_trackLinePrimaryHighlight = 1;
				}
			}
			else //secondary line highlight 
			{
				if (wheelzDelta < 0)
				{
					g_trackLineSecondaryHighlight++;
					if (g_trackLineSecondaryHighlight > ma) g_trackLineSecondaryHighlight = ma;
				}
				else if (wheelzDelta > 0)
				{
					g_trackLineSecondaryHighlight--;
					if (g_trackLineSecondaryHighlight < 1) g_trackLineSecondaryHighlight = 1;
				}
			}
		}
		return 6;
	}

	rec.SetRect(456, 64, 456 + 8 * 10, 64 + 16);
	if (rec.PtInRect(point))
	{
		//Octave Select Dialog
		SetCursor(m_cursorDialog);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoOctaveSelect(point.x, point.y);
		}
		if (wheelzDelta != 0)
		{
			if (wheelzDelta < 0) g_Song.OctaveUp();
			else if (wheelzDelta > 0) g_Song.OctaveDown();
		}
		return 6;
	}

	rec.SetRect(472, 80, 472 + 8 * 8, 80 + 16);
	if (rec.PtInRect(point))
	{
		//Volume Select Dialog
		SetCursor(m_cursorDialog);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoVolumeSelect(point.x, point.y);
		}
		if (wheelzDelta != 0)
		{
			if (wheelzDelta < 0) g_Song.VolumeUp();
			else if (wheelzDelta > 0) g_Song.VolumeDown();
		}
		return 6;
	}

	rec.SetRect(136, 80, 136 + INSTRUMENT_NAME_MAX_LEN * 8, 80 + 16);
	if (rec.PtInRect(point))
	{
		//Instrument Select Dialog
	Instrument_Select_Dialog:

		SetCursor(m_cursorDialog);
		if (mousebutt & MK_LBUTTON)
		{
			g_Song.InfoCursorGotoInstrumentSelect(point.x, point.y);
		}
		if (wheelzDelta != 0)
		{
			if (wheelzDelta > 0) g_Song.ActiveInstrPrev();
			else if (wheelzDelta < 0) g_Song.ActiveInstrNext();
		}
		return 6;
	}

	//LOWER PARTS
	if (g_active_ti==PART_TRACKS)
	{
		rec.SetRect(TRACKS_X + 3 * 16, TRACKS_Y - 12, TRACKS_X + 3 * 8 + g_tracks4_8 * 8 * 16, TRACKS_Y + 32);

		if (rec.PtInRect(point))
		{
			i = (point.x - (TRACKS_X + 5 * 8)) / (8 * 16);
			if (i<0) i=0;
			else
			if (i>=g_tracks4_8) i=g_tracks4_8-1;
			px = i;

			SetCursor(m_cursorChanbelOnOff);

			if (mousebutt & MK_LBUTTON)
			{
				SetChannelOnOff(px,-1);	//inversion
			}
			if (mousebutt & MK_RBUTTON)
			{
				SetChannelSolo(px);		//solo/mute/on/off
			}
			return 1;
		}
		//the number of tracklines is adjusted based on the window height
		rec.SetRect(TRACKS_X + 6 * 8, TRACKS_Y + 48, TRACKS_X + 3 * 8 + g_tracks4_8 * 8 * 16, TRACKS_Y + 48 + g_tracklines * 16);
		if (rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Song.TrackCursorGoto(CPoint(point.x - (TRACKS_X + 6 * 8), point.y - (TRACKS_Y + 48)));
			}
			if (wheelzDelta!=0)
			{
				if (wheelzDelta>0) g_Song.TrackKey(VK_UP,0,0);
				else
				if (wheelzDelta<0) g_Song.TrackKey(VK_DOWN,0,0);
			}
			return 4;
		}
	}
	else
	if (g_active_ti==PART_INSTRUMENTS)
	{
		// Detect bounding box hits on various parts of the instrument display
		// Algo:
		// 1. Find the area that a specific GUI part (zone) will cover.
		//		Size depends on instrument data
		// 2. Check if the action point is within the zone
		// 3. Do zone specific action

		int activeInstrNum = g_Song.GetActiveInstr();
		//VOLUME LEFT (bottom)
		int r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_ENVELOPE_LEFT_ENVELOPE, rec);
		if (r && rec.PtInRect(point))
		{
			// In the left volume envelope area
			px = (point.x - rec.left) /8;			// px is how far into the envelop the cursor is
			py = 15- ((point.y - rec.top) /4);		// py is the volume at the cursor position
			SetCursor(m_cursorEnvelopVolume);
			if (g_mousebutt & MK_LBUTTON) //compares g_mousebutt to make it work while moving
			{
				// Set the volume
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvelopeVolume(activeInstrNum,0,px,py);
			}
			if (g_mousebutt & MK_RBUTTON) //compares g_mousebutt to make it work while moving
			{
				// Clear the volume
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvelopeVolume(activeInstrNum,0,px,0);
			}
			return 2;
		}

		//VOLUME RIGHT (upper)
		r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_ENVELOPE_RIGHT_ENVELOPE, rec);
		if (r && rec.PtInRect(point))
		{
			// In the right volume envelope area
			px = (point.x - rec.left) /8;
			py = 15- ((point.y - rec.top) /4);
			SetCursor(m_cursorEnvelopVolume);
			if (g_mousebutt & MK_LBUTTON) //compares g_mousebutt to make it work while moving
			{
				// Set the volume
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvelopeVolume(activeInstrNum,1,px,py);
			}
			if (g_mousebutt & MK_RBUTTON) //compares g_mousebutt to make it work while moving
			{
				// Clear the volume
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvelopeVolume(activeInstrNum,1,px,0);
			}
			return 3;
		}

		//ENVELOPE PARAMETERS large table
		r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_ENVELOPE_PARAM_TABLE, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),0);
			}
			return 3;
		}

		//ENVELOPE PARAMETERS series of numbers for the right channel volume
		r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_ENVELOPE_RIGHT_VOL_NUMS, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),1);
			}
			return 3;
		}

		//INSTRUMENT NOTE TABLE
		r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_NOTE_TABLE, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),2);
			}
			return 3;
		}

		//INSTRUMENT NAME
		r = g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_INSTRUMENT_NAME, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),3);
			}
			return 3;
		}

		//INSTRUMENT PARAMETERS
		r= g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_PARAMETERS, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorGoto);
			if (mousebutt & MK_LBUTTON)
			{
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),4);
			}
			return 3;
		}

		//INSTRUMENT SELECT DIALOG
		r= g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_INSTRUMENT_NUMBER_DLG, rec);
		if (r && rec.PtInRect(point))
		{
			point.y-=(4*16+8);
			point.x+=64;
			goto Instrument_Select_Dialog;
		}

		//ENVELOPE LEN a GO PARAMETER - length and loop to help the mouse
		r= g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_LEN_AND_GOTO_ARROWS, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorSetPosition);
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				// Set the
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),5);
			}
			if (mousebutt & MK_RBUTTON)
			{
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),6);
			}
			return 7;
		}

		//TABLE LEN a GO PARAMETER - length and loop to help the mouse
		r= g_Instruments.GetGUIArea(activeInstrNum, INSTR_GUI_ZONE_NOTE_TBL_LEN_AND_GOTO, rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorSetPosition);
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),7);
			}
			if (mousebutt & MK_RBUTTON)
			{
				g_Undo.ChangeInstrument(activeInstrNum,0,UETYPE_INSTRDATA);
				g_Instruments.CursorGoto(activeInstrNum,CPoint(point.x-rec.left,point.y-rec.top),8);
			}
			return 7;
		}

	}
	SetCursor(m_cursororig);
	return 0;
}
*/

void CRmtView::MouseAction(CPoint point, UINT mousebutt, short wheelzDelta)
{
	// Scale the mouse XY coordinates to the actual display scaling, so the hitboxes will match everything visually rendered
	point.x = INVERSE_SCALE(point.x);
	point.y = INVERSE_SCALE(point.y);

	// Store the last known mouse XY coordinates and buttons used
	g_mouseLastPointX = point.x;
	g_mouseLastPointY = point.y;
	g_mouseLastButton = mousebutt;
	g_mouseLastWheelDelta = wheelzDelta;

	// Set the Cursor back to the original once everything was processed
	SetCursor(m_cursororig);
}

void CRmtView::OnLButtonDown(UINT nFlags, CPoint point)
{
	g_Undo.Separator();
	g_mousebutt |= MK_LBUTTON;
	MouseAction(point, MK_LBUTTON);
	CView::OnLButtonDown(nFlags, point);
}

void CRmtView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	g_Undo.Separator();
	g_mousebutt&=~MK_LBUTTON;
	CView::OnLButtonUp(nFlags, point);
}

void CRmtView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	g_Undo.Separator();
	OnLButtonDown(nFlags, point);
	OnLButtonUp(nFlags, point);
}

void CRmtView::OnRButtonDown(UINT nFlags, CPoint point)
{
	g_Undo.Separator();
	g_mousebutt |= MK_RBUTTON;
	MouseAction(point, MK_RBUTTON);
	CView::OnRButtonDown(nFlags, point);
}

void CRmtView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	g_Undo.Separator();
	g_mousebutt&=~MK_RBUTTON;
	CView::OnRButtonUp(nFlags, point);
}

void CRmtView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	g_Undo.Separator();
	OnRButtonDown(nFlags, point);
	OnRButtonUp(nFlags, point);
}

void CRmtView::OnMouseMove(UINT nFlags, CPoint point)
{
	MouseAction(point, 0);
	CView::OnMouseMove(nFlags, point);
}

BOOL CRmtView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CRect rec;
	::GetWindowRect(g_viewhwnd, &rec);
	CPoint np(pt - rec.TopLeft());
	MouseAction(np, 0, zDelta);
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

BOOL CRmtView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	return 1;
}

/*
const int  NChaCode[]={  36,  38,  33, VK_SUBTRACT,  37,  12,  39, VK_ADD,  35,  40,  34,  45};
const char FlaToCha[]={0x67,0x68,0x69,109,0x64,0x65,0x66,107,0x61,0x62,0x63,0x60};
//const char layout2[]={VK_F5,VK_F6,VK_F7,VK_F8, VK_F3,VK_F2,VK_F4,VK_ESCAPE};

//TODO: cleanup and reconfigure, since testing keys in Stereo is not working correctly due to all the shortcuts being intermixed into the inputs
void CRmtView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UINT vk = nChar;
	UINT nfb = nFlags & 0x1ff;	//when autorepeat is set in nFlags bit 16384
	//this seems to work around possible problems and causes no harm... so let's leave this untouched
	if (nfb >= 71 && nfb <= 82) //shift + numblock 0-9 + -
	{
		if ((int)nChar == NChaCode[nfb - 71])
		{
			vk = FlaToCha[nfb - 71];
		}
	}

	if (g_viewDebugDisplay) g_lastKeyPressed = vk;	//debug key reading for setting up keyboard layouts withought having to guess which key is where

	switch (vk)
	{
	case 0x5A: //Z
		if (g_controlkey && !g_shiftkey) //or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			if (g_Song.Undo()) //CTRL+Z
			{
				return;
			}
		}
		goto AllModesDefaultKey;
		break;

	case 0x59: //Y
		if (g_controlkey && !g_shiftkey) //or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			if (g_Song.Redo()) //CTRL+Y
			{
				return;
			}
		}
		goto AllModesDefaultKey;
		break;

	case VK_SPACE: //SPACEBAR
		if (g_controlkey)
		{
			g_Undo.Separator(); //CTRL+SPACEBAR
			OnProvemode();
		}
		goto AllModesDefaultKey;
		break;

	case VK_ESCAPE:
		g_Song.Stop();	//stops everything
		if (g_keyboard_escresetatarisound)
			Atari_InitRMTRoutine(); //reset RMT routines automatically
		goto AllModesDefaultKey;
		break;

	case VK_SUBTRACT:
		if (g_controlkey && !g_shiftkey)
		{
			g_linesafter--;
			if (g_linesafter < 0) g_linesafter = 8;
			CMainFrame* mf = ((CMainFrame*)AfxGetMainWnd());
			if (mf) mf->m_comboSkipLinesAfterNoteInsert.SetCurSel(g_linesafter);
		}
		else
			goto AllModesDefaultKey;
		break;

	case VK_ADD:
		if (g_controlkey && !g_shiftkey)
		{
			g_linesafter++;
			if (g_linesafter > 8) g_linesafter = 0;
			CMainFrame* mf = ((CMainFrame*)AfxGetMainWnd());
			if (mf) mf->m_comboSkipLinesAfterNoteInsert.SetCurSel(g_linesafter);
		}
		else
			goto AllModesDefaultKey;
		break;

	case VK_MULTIPLY:
		goto AllModesDefaultKey;
		break;

	case VK_DIVIDE:
		goto AllModesDefaultKey;
		break;

	case VK_F1:
		if (g_controlkey) goto AllModesDefaultKey;	//would conflict with transposition hotkeys otherwise
		g_Undo.Separator();
		OnEmTracks();
		break;

	case VK_F2:
		if (g_controlkey) goto AllModesDefaultKey;	//would conflict with transposition hotkeys otherwise
		g_Undo.Separator();
		OnEmInstruments();
		break;

	case VK_F3:
		if (g_controlkey) goto AllModesDefaultKey;	//would conflict with transposition hotkeys otherwise
		g_Undo.Separator();
		OnEmInfo();
		break;

	case VK_F4:
		if (g_controlkey) goto AllModesDefaultKey;	//would conflict with transposition hotkeys otherwise
		g_Undo.Separator();
		OnEmSong();
		break;

	case VK_F5:
		if (g_controlkey && g_shiftkey)
		{
			g_prove = PROVE_POKEY_EXPLORER_MODE;	//POKEY EXPLORER MODE -- KEYBOARD INPUT AND FORMULAE DISPLAY
			break;
		}
		g_Song.Play(MPLAY_SONG, g_Song.GetFollowPlayMode());	//play song from start
		break;

	case VK_F6:
		if (g_shiftkey) g_Song.Play(MPLAY_BLOCK, g_Song.GetFollowPlayMode());	//play block and follow
		else g_Song.Play(MPLAY_TRACK, g_Song.GetFollowPlayMode());				//play pattern and follow	
		break;

	case VK_F7:
		if (g_Song.IsBookmark() && g_shiftkey) g_Song.Play(MPLAY_BOOKMARK, g_Song.GetFollowPlayMode());	//play song from bookmark
		else g_Song.Play(MPLAY_FROM, g_Song.GetFollowPlayMode());							//play song from current position
		break;

	case VK_F8:
		if (g_controlkey) g_Song.ClearBookmark();	//clear bookmark
		else g_Song.SetBookmark();					//set song bookmark
		break;

	case VK_F9:
		if (!g_controlkey && g_shiftkey)
		{
			SetChannelOnOff(-1, -1);		//switch all channels on or off
		}
		else if (g_controlkey)
		{
			int ch = g_Song.GetActiveColumn(); //solo current channel
			SetChannelSolo(ch);
		}
		else
		{
			int ch = g_Song.GetActiveColumn(); //mute current channel
			SetChannelOnOff(ch, -1);
		}
		break;

		//F10 can't be used for some reason... it seems to be binded to native Windows functions and so it would take priority instead of any shortcut I would like to use for it.

	case VK_F11:
		g_Undo.Separator();	//respect volume
		g_respectvolume ^= 1;
		break;

	case VK_F12:
		if (g_controlkey)
		{
			g_ntsc ^= 1;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		else OnPlayfollow(); //toggle follow position
		break;

	case VK_MEDIA_PLAY_PAUSE:
		if (g_Song.GetPlayMode() == 0)
			g_Song.Play(MPLAY_SONG, g_Song.GetFollowPlayMode());	//play song from start
		else g_Song.Stop();								//if playing, stop
		break;

	case VK_MEDIA_NEXT_TRACK:
		g_Song.Play(MPLAY_SEEK_NEXT, g_Song.GetFollowPlayMode()); //seek next and play from track
		break;

	case VK_MEDIA_PREV_TRACK:
		g_Song.Play(MPLAY_SEEK_PREV, g_Song.GetFollowPlayMode()); //seek prev and play from track
		break;

	case VK_SHIFT:
		g_shiftkey = 1;
		goto KeyDownNoUndoCheckPoint;
		break;

	case VK_CONTROL:
		g_controlkey = 1;
		goto KeyDownNoUndoCheckPoint;
		break;

		//TODO: Add ALT key support for the "is held" flag, for some reason I am unable to make it work, it seems to behave like the F10 key and take priority over everything else.
	case VK_LMENU:
		g_altkey = 1;
		goto KeyDownNoUndoCheckPoint;
		break;

	case 49:	//VK_1
	case 50:	//VK_2
	case 51:	//VK_3
	case 52:	//VK_4
	case 53:	//VK_5
	case 54:	//VK_6
	case 55:	//VK_7
	case 56:	//VK_8
		if (g_controlkey && !g_shiftkey)	//CONTROL + 1-8
		{
			SetChannelOnOff(vk - 49, -1);		//inverts channel status 1-8 (=> on / off)
		}
		else
			goto AllModesDefaultKey;
		break;

	case 76:	//VK_L
		if (g_controlkey && !g_shiftkey) //CTRL+L, or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			SetStatusBarText("Load...");
			OnFileOpen();
			SetStatusBarText("");
		}
		else
			goto AllModesDefaultKey;
		break;

	case 87: //VK_W
		if (g_controlkey && !g_shiftkey) //CTRL+W, or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			//g_Song.Stop();
			int r = MessageBox("Would you like to create a new song?", "Create new song", MB_YESNOCANCEL | MB_ICONQUESTION);
			if (r == IDYES) g_Song.FileNew();
		}
		else
			goto AllModesDefaultKey;
		break;

	case 83:	//VK_S
		if (g_controlkey && !g_shiftkey) //CTRL+S, or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			//g_Song.Stop();
			SetStatusBarText("Save...");
			CString filename = g_Song.GetFilename();
			if (g_keyboard_askwhencontrol_s
				&& (filename != "" || g_Song.GetFiletype() != 0))
			{
				//if a question is asked and if a file already exists
				//(=> there will be a "Save as ..." dialog)
				CString s;
				s.Format("Do you want to save song file '%s'?\nIs it okay to overwrite?", filename);
				int r = MessageBox(s, "Save song", MB_YESNOCANCEL | MB_ICONQUESTION);
				if (r == IDNO) { OnFileSaveAs(); goto end_save_control_s; }
				if (r != IDYES) goto end_save_control_s;
			}
			Sleep(128);
			OnFileSave();
			Sleep(128);

		end_save_control_s:
			SetStatusBarText("");
		}
		else
			goto AllModesDefaultKey;
		break;

	case 82:	//VK_R
		if (g_controlkey && !g_shiftkey) //CTRL+R, or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			//g_Song.Stop();
			g_Song.FileReload();	//turns out this function handles the rest already, so jump right to it instead
		}
		else
			goto AllModesDefaultKey;
		break;

	default:
	AllModesDefaultKey:
		BOOL CAPSLOCK = GetKeyState(20);	//VK_CAPS_LOCK
		switch (g_activepart)
		{
		case PART_INFO:
			if (g_shiftkey && !is_editing_infos && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);	//plays a note while the SHIFT key is held, except on the Song Name field, it will be ignored
			else if (g_shiftkey && !is_editing_infos && (NoteKey(vk) < 0))
				if (vk == VK_TAB || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_PAGE_UP || vk == VK_PAGE_DOWN) goto do_infokey_anyway;
				else break;	//prevents inputing incorrect infos by accident while testing notes holding SHIFT
			else if (is_editing_infos && CAPSLOCK && !g_shiftkey)
			{
				g_shiftkey = 1;
				g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
				g_shiftkey = 0;	//workaround: so it won't *stay* locked when CAPSLOCK isn't active
				break;
			}
			else if (is_editing_infos && CAPSLOCK && g_shiftkey)
			{
				g_shiftkey = 0;
				g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
				g_shiftkey = 1;	//workaround: so it will *stay* locked when CAPSLOCK isn't active
				break;
			}
			else
			{
			do_infokey_anyway:
				if (vk == VK_PAGE_UP || vk == VK_PAGE_DOWN)
					g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
				else
					g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
			}
			break;

		case PART_TRACKS:
			if (g_prove)
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
			else if (g_shiftkey && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
			else
				g_Song.TrackKey(vk, g_shiftkey, g_controlkey);
			break;

		case PART_INSTRUMENTS:
			if (g_shiftkey && !g_isEditingInstrumentName && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);	//plays a note while the SHIFT key is held, except on the Instrument Name field, it will be ignored
			else if (g_shiftkey && !g_isEditingInstrumentName && (NoteKey(vk) < 0))
				if (vk == VK_TAB || vk == VK_INSERT || vk == VK_DELETE || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_DIVIDE || vk == VK_MULTIPLY || vk == VK_SUBTRACT || vk == VK_ADD || vk == VK_PAGE_UP || vk == VK_PAGE_DOWN) goto do_instrkey_anyway;
				else break;	//prevents inputing incorrect infos by accident while testing notes holding SHIFT
			else if (g_isEditingInstrumentName && CAPSLOCK && !g_shiftkey)
			{
				g_shiftkey = 1;
				g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
				g_shiftkey = 0;	//workaround: so it won't *stay* locked when CAPSLOCK isn't active
				break;
			}
			else if (g_isEditingInstrumentName && CAPSLOCK && g_shiftkey)
			{
				g_shiftkey = 0;
				g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
				g_shiftkey = 1;	//workaround: so it will *stay* locked when CAPSLOCK isn't active
				break;
			}
			else
			{
			do_instrkey_anyway:
				if (vk == VK_PAGE_UP || vk == VK_PAGE_DOWN)
					g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
				else
					g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
			}
			break;

		case PART_SONG:
			if (g_prove)
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
			else if (g_shiftkey && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
				g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
			else
				g_Song.SongKey(vk, g_shiftkey, g_controlkey);
			break;
		}
	}

	// UndoCheckPoint does not work if it is only a shift or just a control (to which the next key will come)
KeyDownNoUndoCheckPoint:
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
*/

void CRmtView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Store the last pressed key in memory for debugging purposes
	g_lastKeyPressed = nChar;

	// If nothing was pressed, or a key without a function was pressed, nothing will happen
	switch (nChar)
	{
	case VK_ESCAPE:
		g_Song.Stop();
		break;

	case VK_F5:
		g_Song.Play(MPLAY_SONG, g_Song.GetFollowPlayMode());
		break;
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRmtView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CRmtView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// All keys will be processed at the same place for convenience
	OnKeyDown(nChar, nRepCnt, nFlags);
	//CView::OnSysKeyDown(nChar, nRepCnt, nFlags);	// Not actually required?
}

void CRmtView::OnFileOpen() 
{
	g_Song.FileOpen();
}

void CRmtView::OnFileReload() 
{
	g_Song.FileReload();	
}

void CRmtView::OnUpdateFileReload(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.FileCanBeReloaded());
}

void CRmtView::OnFileSave()
{
	g_Song.FileSave();
}

void CRmtView::OnFileSaveAs() 
{
	g_Song.FileSaveAs();
}

void CRmtView::OnFileNew() 
{
	g_Song.FileNew();
}

void CRmtView::OnFileImport() 
{
	g_Song.FileImport();
}

void CRmtView::OnFileExportAs() 
{
	g_Song.FileExportAs();
}

void CRmtView::OnInstrLoad() 
{
	g_Song.FileInstrumentLoad();
}

void CRmtView::OnInstrSave() 
{
	g_Song.FileInstrumentSave();	
}

void CRmtView::OnInstrCopy() 
{
	g_Song.InstrCopy();	
}

void CRmtView::OnInstrPaste() 
{
	g_Song.InstrPaste();
}

void CRmtView::OnInstrCut() 
{
	g_Undo.ChangeInstrument(g_Song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	g_Song.InstrCut();
}

void CRmtView::OnInstrDelete() 
{
	g_Undo.ChangeInstrument(g_Song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	g_Song.InstrDelete();
}

void CRmtView::OnInstrumentPastespecialVolumeLRenvelopesonly() 
{
	g_Song.InstrPaste(1); //L/R
}

void CRmtView::OnInstrumentPastespecialVolumeRenvelopeonly() 
{
	g_Song.InstrPaste(2); //R
}

void CRmtView::OnInstrumentPastespecialVolumeLenvelopeonly() 
{
	g_Song.InstrPaste(3); //L
}

void CRmtView::OnInstrumentPastespecialEnvelopeparametersonly() 
{
	g_Song.InstrPaste(4); //ENVELOPE PARS
}

void CRmtView::OnInstrumentPastespecialTableonly() 
{
	g_Song.InstrPaste(5); //TABLE
}

void CRmtView::OnInstrumentPastespecialVolumeenvandenvelopeparsonly() 
{
	g_Song.InstrPaste(6); //VOL+ENV
}

void CRmtView::OnInstrumentPastespecialInsertvolenvsandenvparstocurpos() 
{
	g_Song.InstrPaste(7); //VOL+ENV TO CURPOS
}

void CRmtView::OnInstrumentPastespecialVolumeltorenvelopeonly() 
{
	g_Song.InstrPaste(8); //volume L to R
}

void CRmtView::OnInstrumentPastespecialVolumertolenvelopeonly() 
{
	g_Song.InstrPaste(9); //volume R to L
}

//update
void CRmtView::OnUpdateInstrumentPastespecialInsertvolenvsandenvparstocurpos(CCmdUI* pCmdUI) 
{
	//to cur pos
	int i = g_Song.GetActiveInstr();
	TInstrument* ai = g_Instruments.GetInstrument(i);
	pCmdUI->Enable(g_activepart==PART_INSTRUMENTS && (ai->activeEditSection==2)); //when the envelope is being edited
}

void CRmtView::OnUpdateInstrumentPastespecialVolumelenvelopeonly(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_tracks4_8>4);	//L only
}

void CRmtView::OnUpdateInstrumentPastespecialVolumerenvelopeonly(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_tracks4_8>4);	//R only
}

void CRmtView::OnUpdateInstrumentPastespecialVolumertolenvelopeonly(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_tracks4_8>4); //R to L
}

void CRmtView::OnUpdateInstrumentPastespecialVolumeltorenvelopeonly(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_tracks4_8>4); //L to R
}

void CRmtView::OnTrackCopy() 
{
	g_Song.TrackCopy();
}

void CRmtView::OnTrackPaste() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA);
	g_Song.TrackPaste();
}

void CRmtView::OnTrackCut() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA);
	g_Song.TrackCut();
}

void CRmtView::OnTrackDelete() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA);
	g_Song.TrackDelete();
}

void CRmtView::OnUpdateTrackCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PART_INSTRUMENTS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PART_INSTRUMENTS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PART_INSTRUMENTS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PART_INSTRUMENTS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnTrackLoad() 
{
	g_Song.FileTrackLoad();
}

void CRmtView::OnTrackSave() 
{
	g_Song.FileTrackSave();
}

void CRmtView::OnUpdateTrackLoad(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.SongGetActiveTrack()>=0);
}

void CRmtView::OnUpdateTrackSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.SongGetActiveTrack()>=0);
}

void CRmtView::OnSongCopyline() 
{
	g_Song.SongCopyLine();	
}

void CRmtView::OnSongPasteline() 
{
	g_Song.SongPasteLine();
}

void CRmtView::OnSongClearline() 
{
	g_Song.SongClearLine();
}

void CRmtView::OnSongDeleteactualline() 
{
	g_Song.SongDeleteLine(g_Song.SongGetActiveLine());
}

void CRmtView::OnSongInsertnewemptyline() 
{
	g_Song.SongInsertLine(g_Song.SongGetActiveLine());
}

void CRmtView::OnSongInsertnewlinewithunusedtracks() 
{
	int line = g_Song.SongGetActiveLine();
	g_Song.SongPrepareNewLine(line);
	g_Song.SongSetActiveLine(line);
}

void CRmtView::OnSongInsertcopyorcloneofsonglines() 
{
	int line = g_Song.SongGetActiveLine();
	g_Song.SongInsertCopyOrCloneOfSongLines(line);
	g_Song.SongSetActiveLine(line);
}

void CRmtView::OnSongPutnewemptyunusedtrack() 
{
	g_Song.SongPutnewemptyunusedtrack();
}

void CRmtView::OnSongMaketracksduplicate() 
{
	g_Song.SongMaketracksduplicate();
}

void CRmtView::OnUpdateSongMaketracksduplicate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.SongGetActiveTrack()>=0);
}

void CRmtView::OnPlay0() 
{
	g_Song.Play(MPLAY_BOOKMARK,g_Song.GetFollowPlayMode());	//from the bookmark - with respect to followplay
}

void CRmtView::OnPlay1() 
{
	g_Song.Play(MPLAY_SONG,g_Song.GetFollowPlayMode());		//whole song from start - with respect to followplay
}

void CRmtView::OnPlay2() 
{
	g_Song.Play(MPLAY_FROM,g_Song.GetFollowPlayMode());		//from the current position - with respect to followplay
}

void CRmtView::OnPlay3() 
{
	g_Song.Play(MPLAY_TRACK,g_Song.GetFollowPlayMode());		//current pattern and loop - with respect to followplay
}

void CRmtView::OnPlaystop() 
{
	g_Song.Stop();
}

void CRmtView::OnPlayfollow() 
{
	g_Song.SetFollowPlayMode(g_Song.GetFollowPlayMode() ^ 1);
}

void CRmtView::OnEmTracks() 
{
	g_activepart=g_active_ti=PART_TRACKS;	//tracks
}

void CRmtView::OnEmInstruments() 
{
	g_activepart=g_active_ti=PART_INSTRUMENTS;	//instrs
	g_TrackClipboard.BlockDeselect();
}

void CRmtView::OnEmInfo() 
{
	g_activepart=PART_INFO;		//info
	g_TrackClipboard.BlockDeselect();
}

void CRmtView::OnEmSong() 
{
	g_activepart=PART_SONG;		//song
	g_TrackClipboard.BlockDeselect();
}

void CRmtView::OnUpdatePlay0(CCmdUI* pCmdUI) 
{
	//int ch= g_Song.IsBookmark();
	//pCmdUI->Enable(ch);
	//ch= (g_Song.GetPlayMode()==MPLAY_BOOKMARK);
	//pCmdUI->SetCheck(ch);
	pCmdUI->Enable(0);
	pCmdUI->SetCheck(0);
}

void CRmtView::OnUpdatePlay1(CCmdUI* pCmdUI) 
{
	int ch= (g_Song.GetPlayMode()==1)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlay2(CCmdUI* pCmdUI) 
{
	int ch= (g_Song.GetPlayMode()==2)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlay3(CCmdUI* pCmdUI) 
{
	int ch= (g_Song.GetPlayMode()==3)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlayfollow(CCmdUI* pCmdUI) 
{
	int ch = g_Song.GetFollowPlayMode();
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmTracks(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PART_TRACKS && g_active_ti==PART_TRACKS)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmInstruments(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PART_INSTRUMENTS && g_active_ti==PART_INSTRUMENTS)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmInfo(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PART_INFO)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmSong(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PART_SONG)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

/// <summary>
/// Switch the edit mode
/// 0 = edit
/// 1 = Mono jam
/// 2 = Stereo jam
/// </summary>
void CRmtView::OnProvemode() 
{
	if (g_prove == PROVE_EDIT_MODE) g_prove = PROVE_JAM_MONO_MODE;
	else if (g_prove >= PROVE_EDIT_AND_JAM_MODES) g_prove = PROVE_EDIT_MODE;		//disable the special test modes immediately
	else
	{
		if (g_prove == PROVE_JAM_MONO_MODE && g_tracks4_8 > 4)	//PROVE 2 only works for 8 tracks
			g_prove = PROVE_JAM_STEREO_MODE;
		else
			g_prove = PROVE_EDIT_MODE;
	}
}

void CRmtView::OnUpdateProvemode(CCmdUI* pCmdUI) 
{
	int ch = (g_prove > PROVE_EDIT_MODE) ? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::ChangeViewElements(BOOL writeconfig)
{
	CMainFrame* mf = (CMainFrame*)AfxGetApp()->GetMainWnd();
	mf->ShowControlBar((CControlBar*)(&mf->m_wndToolBar),g_viewMainToolbar,0);
	mf->ShowControlBar((CControlBar*)(&mf->m_ToolBarBlock),g_viewBlockToolbar,0);
	mf->ShowControlBar((CControlBar*)(&mf->m_wndStatusBar),g_viewStatusBar,0);
	if (writeconfig) WriteRMTConfig();
}

void CRmtView::OnViewToolbar() 
{
	g_viewMainToolbar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewToolbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewMainToolbar);
}

void CRmtView::OnViewBlocktoolbar() 
{
	g_viewBlockToolbar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewBlocktoolbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewBlockToolbar);
}

void CRmtView::OnViewStatusBar() 
{
	g_viewStatusBar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewStatusBar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewStatusBar);
}

void CRmtView::OnViewPlaytimecounter() 
{
	g_viewPlayTimeCounter ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewPlaytimecounter(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewPlayTimeCounter);
}

void CRmtView::OnViewVolumeanalyzer() 
{
	g_viewVolumeAnalyzer ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewVolumeanalyzer(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewVolumeAnalyzer);	
}

void CRmtView::OnViewPokeyregs() 
{
	g_viewPokeyRegisters ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewPokeyregs(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewPokeyRegisters);
	pCmdUI->Enable(g_viewVolumeAnalyzer);
}

void CRmtView::OnViewInstrumentactivehelp() 
{
	g_viewInstrumentEditHelp ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewInstrumentactivehelp(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewInstrumentEditHelp);	
}

void CRmtView::OnBlockNoteup() 
{
	g_TrackClipboard.BlockNoteTransposition(g_Song.GetActiveInstr(),1);
}

void CRmtView::OnUpdateBlockNoteup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockNotedown() 
{
	g_TrackClipboard.BlockNoteTransposition(g_Song.GetActiveInstr(),-1);
}

void CRmtView::OnUpdateBlockNotedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockVolumeup() 
{
	g_TrackClipboard.BlockVolumeChange(g_Song.GetActiveInstr(),1);	
}

void CRmtView::OnUpdateBlockVolumeup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockVolumedown() 
{
	g_TrackClipboard.BlockVolumeChange(g_Song.GetActiveInstr(),-1);	
}

void CRmtView::OnUpdateBlockVolumedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockInstrleft() 
{
	g_TrackClipboard.BlockInstrumentChange(g_Song.GetActiveInstr(),-1);	
}

void CRmtView::OnUpdateBlockInstrleft(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockInstrright() 
{
	g_TrackClipboard.BlockInstrumentChange(g_Song.GetActiveInstr(),1);	
}

void CRmtView::OnUpdateBlockInstrright(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockInstrall() 
{
	g_TrackClipboard.BlockAllOnOff();	
}

void CRmtView::OnUpdateBlockInstrall(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_TrackClipboard.m_all);
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockBackup() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA,1);
	g_TrackClipboard.BlockRestoreFromBackup();	
}

void CRmtView::OnUpdateBlockBackup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockPlay() 
{
	g_Song.Play(MPLAY_BLOCK,g_Song.GetFollowPlayMode());	//selected block and loop - with respect to followplay
}

void CRmtView::OnUpdateBlockPlay(CCmdUI* pCmdUI) 
{
	int ch= (g_Song.GetPlayMode()==4)? 1 : 0;
	pCmdUI->SetCheck(ch);
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

//----------------------------------------------------------------------------
//CHANNELS

void CRmtView::OnChan1() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan2() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan3() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan4() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan5() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan6() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan7() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnChan8() 
{
	// TODO: Add your command handler code here
}

void CRmtView::OnUpdateChan1(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
}

void CRmtView::OnUpdateChan2(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
}

void CRmtView::OnUpdateChan3(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
}

void CRmtView::OnUpdateChan4(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
}

void CRmtView::OnUpdateChan5(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_tracks4_8>4));
}

void CRmtView::OnUpdateChan6(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_tracks4_8>4));
}

void CRmtView::OnUpdateChan7(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_tracks4_8>4));
}

void CRmtView::OnUpdateChan8(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_tracks4_8>4));
}

void CRmtView::OnMidionoff() 
{
	if (g_Midi.IsOn()) 
		g_Midi.MidiOff();
	else
		g_Midi.MidiOn();
}

void CRmtView::OnUpdateMidionoff(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Midi.IsOn());	
}

void CRmtView::OnBlockCopy() 
{
	//g_Song.TrackKey(67,0,1);	//Ctrl+C
}

void CRmtView::OnBlockCut() 
{
	//g_Song.TrackKey(88,0,1);	//Ctrl+X
}

void CRmtView::OnBlockDelete() 
{
	//g_Song.TrackKey(VK_DELETE,0,1);	//Del
}

void CRmtView::OnBlockPaste() 
{
	g_Song.BlockPaste();	//paste normal
}

void CRmtView::OnBlockPastespecialMergewithcurrentcontent() 
{
	g_Song.BlockPaste(1);	//paste special - merge
}

void CRmtView::OnBlockPastespecialVolumevaluesonly() 
{
	g_Song.BlockPaste(2);	//paste special - volumes only
}

void CRmtView::OnBlockPastespecialSpeedvaluesonly() 
{
	g_Song.BlockPaste(3);	//paste special - speeds only
}

void CRmtView::OnBlockExchange() 
{
	//g_Song.TrackKey(69,0,1);	//Ctrl+E
}

void CRmtView::OnBlockEffect() 
{
	//g_Song.TrackKey(70,0,1);	//Ctrl+F
}

void CRmtView::OnBlockSelectall() 
{
	//g_Song.TrackKey(65,0,1);	//Ctrl+A
}

void CRmtView::OnUpdateBlockCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnUpdateBlockDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());	
}

void CRmtView::OnUpdateBlockEffect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());	
}

void CRmtView::OnUpdateBlockExchange(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());	
}

void CRmtView::OnTrackClearallduplicatedtracks() 
{
	g_Song.Stop();
	int r=MessageBox("Are you sure you want to clear all duplicated tracks and adjust song?","Clear all duplicated tracks",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
	g_Undo.ChangeSong(0,0,UETYPE_SONGDATA,1);

	int clearedtracks;
	clearedtracks = g_Song.SongClearDuplicatedTracks();

	CString s;
	s.Format("Deleted %i duplicated tracks.",clearedtracks);
	MessageBox((LPCTSTR)s,"Clear all duplicated tracks",MB_OK);
}

void CRmtView::OnTrackClearalltracksunusedinsong() 
{
	g_Song.Stop();
	int r=MessageBox("Are you sure you want to delete all tracks unused in song?","Clear all unused tracks",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL);

	int clearedtracks;
	clearedtracks = g_Song.SongClearUnusedTracks();

	CString s;
	s.Format("Deleted %i tracks unused in song.",clearedtracks);
	MessageBox((LPCTSTR)s,"Clear all unused tracks",MB_OK);
}

void CRmtView::OnTrackAlltrackscleanup() 
{
	//Delete all tracks
	g_Song.Stop();

	int r=MessageBox("WARNING:\nReally cleanup all tracks?","All tracks cleanup",MB_YESNO | MB_ICONWARNING);
	if (r==IDYES)
	{
		g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL);
		g_Tracks.InitTracks();
	}
}

void CRmtView::OnUpdateTrackSearchandbuildloop(CCmdUI* pCmdUI) 
{
	int track=g_Song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) && (g_Tracks.GetGoLine(track)<0) );
}

void CRmtView::OnTrackSearchandbuildloop() 
{
	g_Song.Stop();

	int track=g_Song.SongGetActiveTrack();
	if (track>=0)
	{
		g_Undo.ChangeTrack(track,g_Song.GetActiveLine(),UETYPE_TRACKDATA);
		g_Tracks.TrackBuildLoop(track);
	}
}

void CRmtView::OnUpdateTrackExpandloop(CCmdUI* pCmdUI) 
{
	int track=g_Song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) && (g_Tracks.GetGoLine(track)>=0) );
}

void CRmtView::OnTrackExpandloop() 
{
	g_Song.Stop();

	int track=g_Song.SongGetActiveTrack();
	if (track>=0)
	{
		g_Undo.ChangeTrack(track,g_Song.GetActiveLine(),UETYPE_TRACKDATA);
		g_Tracks.TrackExpandLoop(track);
	}
}

void CRmtView::OnUpdateTrackInfoaboutusingofactualtrack(CCmdUI* pCmdUI) 
{
	int track=g_Song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) );
}

void CRmtView::OnTrackInfoaboutusingofactualtrack() 
{
	g_Song.TrackInfo(g_Song.SongGetActiveTrack());
}

void CRmtView::OnInstrumentInfo() 
{
	g_Song.InstrInfo(g_Song.GetActiveInstr());
}

void CRmtView::OnInstrumentChange() 
{
	g_Song.InstrChange(g_Song.GetActiveInstr());
}

void CRmtView::OnInstrumentClearallunusedinstruments() 
{
	g_Song.Stop();
	int r=MessageBox("Are you sure you want to delete all unused instruments in any tracks?","Clear unused instruments",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeInstrument(0,0,UETYPE_INSTRSALL);

	int clearedinstrs = g_Song.ClearAllInstrumentsUnusedInAnyTrack();
	CString s;
	s.Format("Deleted %i unused instruments.",clearedinstrs);
	MessageBox((LPCTSTR)s,"Clear unused instruments",MB_OK);
}

void CRmtView::OnInstrAllinstrumentscleanup() 
{
	//Delete all instruments
	g_Song.Stop();

	int r=MessageBox("WARNING:\nAre you sure you want to cleanup all the instruments?","All instruments cleanup",MB_YESNO | MB_ICONWARNING);
	if (r==IDYES)
	{
		g_Undo.ChangeInstrument(0,0,UETYPE_INSTRSALL);
		g_Instruments.InitInstruments();
	}
}

void CRmtView::OnSongTracksorderchange() 
{
	g_Song.Stop();
	g_Song.TracksOrderChange();
}

void CRmtView::OnUpdateSongSongswitch4_8(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText((g_tracks4_8<=4)? "Switch song to Stereo 8 tracks..." : "Switch song to Mono 4 tracks...");
}

void CRmtView::OnSongSongswitch4_8() 
{
	g_Song.Stop();
	g_Song.Songswitch4_8((g_tracks4_8<=4)? 8 : 4);
}

void CRmtView::OnSongSongchangemaximallengthoftracks()
{
	g_Song.Stop();

	int ma = g_Song.GetEffectiveMaxtracklen();

	CChangeMaxtracklenDlg dlg;
	dlg.m_info.Format("Current value: %i\nComputed effective value for current song: %i", g_Tracks.GetMaxTrackLength(), ma);
	dlg.m_maxtracklen = ma;
	if (dlg.DoModal() == IDOK)
	{
		//Undo
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL);
		ma = dlg.m_maxtracklen;
		g_Song.ChangeMaxtracklen(ma);
	}
}

void CRmtView::OnSongSearchandrebuildloopsinalltracks() 
{
	g_Song.Stop();	//stop music

	int r=MessageBox("Are you sure you want to search and rebuild wise loops in all tracks?","Search and rebuild loops",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL);

	//first unpack all existing loops
	int tracksmodified=0,loopsexpanded=0;
	g_Song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	//and now search all and create loops again
	int optitracks=0,optibeats=0;
	g_Song.TracksAllBuildLoops(optitracks,optibeats);
	CString s;
	s.Format("Found and rebuilt loops in %i tracks (%i beats/lines).",optitracks,optibeats);
	MessageBox((LPCTSTR)s,"Search and rebuild loops",MB_OK);
}

void CRmtView::OnSongExpandloopsinalltracks() 
{
	g_Song.Stop();	//stop music

	int r=MessageBox("Are you sure you want to expand loops in all tracks?","Expand loops",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL);

	int tracksmodified=0,loopsexpanded=0;
	g_Song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	CString s;
	s.Format("Found and expanded loops in %i tracks (%i beats/lines).",tracksmodified,loopsexpanded);
	MessageBox((LPCTSTR)s,"Expand loops",MB_OK);
}

void CRmtView::OnSongSizeoptimization()
{
	// ALL size optimalizations
	int r = MessageBox("Are you sure you want to delete all tracks and instruments unused in song,\ntruncate unused tracks and rebuild wise tracks loops,\ndelete all duplicated tracks, renumber all tracks and instruments\nand change maximal tracks length to effective computed value?", "All size optimizations", MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r != IDYES) return;

	g_Song.Stop();	// Stop music

	g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, -1);
	g_Undo.ChangeInstrument(0, 0, UETYPE_INSTRSALL, -1);
	g_Undo.ChangeSong(0, 0, UETYPE_SONGDATA, 1);

	int chmaxtl = 0;

	// First unpack all existing loops
	int tracksmodified = 0, loopsexpanded = 0;
	g_Song.TracksAllExpandLoops(tracksmodified, loopsexpanded);

	// Find the effective length of maxtracklen and shorten it if necessary
	int maxtracklen = g_Tracks.GetMaxTrackLength();
	int effemaxtracklen = g_Song.GetEffectiveMaxtracklen();
	if (effemaxtracklen < maxtracklen)
	{
		g_Song.ChangeMaxtracklen(effemaxtracklen);
		chmaxtl = 1;
	}

	// Now it will back up
	int optitracks = 0, optibeats = 0;
	g_Song.TracksAllBuildLoops(optitracks, optibeats);

	// And until the end, don't use the tracks and their parts
	int clearedtracks = 0, truncatedtracks = 0, truncatedbeats = 0;
	g_Song.SongClearUnusedTracksAndParts(clearedtracks, truncatedtracks, truncatedbeats);

	// And only now (after clearing unused tracks) it will remove unused instruments
	int clearedinstruments;
	clearedinstruments = g_Song.ClearAllInstrumentsUnusedInAnyTrack();

	// And now it eliminates double tracks and corrects their occurrences in the song
	// (may have been created by previous edits)
	int duplicatedtracks;
	duplicatedtracks = g_Song.SongClearDuplicatedTracks();

	// Now refines the tracks (to remove any gaps)
	g_Song.RenumberAllTracks(1);

	// And now refines the instruments (to remove any gaps)
	g_Song.RenumberAllInstruments(1);

	CString s;
	s.Format("Deleted %i unused tracks, %i unused instruments,\ntruncated %i tracks (%i beats/lines),\nfound and rebuilt loops in %i tracks (%i beats/lines),\ndeleted %i duplicated tracks.", clearedtracks, clearedinstruments, truncatedtracks, truncatedbeats, optitracks, optibeats, duplicatedtracks);
	if (chmaxtl)
	{
		CString s2;
		s2.Format("\nMaximal length of tracks changed to %u.", effemaxtracklen);
		s += s2;
	}
	MessageBox((LPCTSTR)s, "All size optimizations", MB_OK);
}

void CRmtView::OnTrackRenumberalltracks()
{
	CRenumberTracksDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
		g_Song.Stop();

		// Hide the tracks and song
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, -1);
		g_Undo.ChangeSong(0, 0, UETYPE_SONGDATA, 1);
		
		// Type = 1 -> Order by songcolumns, Type = 2 -> Order by songlines
		g_Song.RenumberAllTracks(dlg.m_radio);
	}
}

void CRmtView::OnInstrumentRenumberallinstruments() 
{
	g_Song.Stop();

	CRenumberInstrumentsDlg dlg;
	if (dlg.DoModal()==IDOK)
	{
		//hide instruments and tracks
		g_Undo.ChangeInstrument(0,0,UETYPE_INSTRSALL,-1);
		g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL,1);
		g_Song.RenumberAllInstruments( dlg.m_radio );	//type=1...remove gaps, 2=order by using in tracks, type=3...order by instrument names
	}
}

void CRmtView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	g_RmtHasFocus = 1;	// RMT main window has focus
}

void CRmtView::OnKillFocus(CWnd* pNewWnd) 
{
	CView::OnKillFocus(pNewWnd);
	g_RmtHasFocus = 0;	// RMT main window does not have focus
}

void CRmtView::OnUndoUndo() 
{
	g_Song.Undo();
}

void CRmtView::OnUpdateUndoUndo(CCmdUI* pCmdUI) 
{
	int u = g_Song.UndoGetUndoSteps();
	if (u>0)
	{
		pCmdUI->Enable(1);
		CString s;
		s.Format("&Undo (%u)\tCtrl+Z",u);
		pCmdUI->SetText(s);
	}
	else
	{
		pCmdUI->Enable(0);
		pCmdUI->SetText("&Undo\tCtrl+Z");
	}
}

void CRmtView::OnUndoRedo() 
{
	g_Song.Redo();
}

void CRmtView::OnUpdateUndoRedo(CCmdUI* pCmdUI) 
{
	int u = g_Song.UndoGetRedoSteps();
	if (u>0)
	{
		pCmdUI->Enable(1);
		CString s;
		s.Format("&Redo (%u)\tCtrl+Y",u);
		pCmdUI->SetText(s);
	}
	else
	{
		pCmdUI->Enable(0);
		pCmdUI->SetText("&Redo\tCtrl+Y");
	}
}

void CRmtView::OnUndoClearundoredo() 
{
	g_Undo.Clear();
}

void CRmtView::OnUpdateUndoClearundoredo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.UndoGetUndoSteps() || g_Song.UndoGetRedoSteps());
}

void CRmtView::OnWantExit() // Called from the menu File/Exit ID_WANTEXIT instead of the original ID_APP_EXIT
{
	if (g_Song.WarnUnsavedChanges())
		return; // There is no exit

	g_Song.Stop();
	g_closeApplication = 1;
	g_Song.KillTimer();
	WriteRMTConfig();		// Save the current configuration 
	WriteTuningConfig();	// Save the current Tuning parameters 
	AfxGetApp()->GetMainWnd()->PostMessage(WM_CLOSE, 0, 0);
}

void CRmtView::OnTrackCursorgotothespeedcolumn() 
{
	//g_Song.CursorToSpeedColumn();
}

void CRmtView::OnUpdateTrackCursorgotothespeedcolumn(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	g_activepart==PART_TRACKS && g_Song.SongGetActiveTrack()>=0 );
}
