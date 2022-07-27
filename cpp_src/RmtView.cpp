//
// RmtView.cpp : implementation of the CRmtView class
// originally made by Raster, 2002-2009
// reworked by VinsCool, 2021-2022
//

#include "stdafx.h"
#include "Rmt.h"
#include "RmtDoc.h"
#include <chrono>

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

extern BOOL g_closeapplication;
//#define SCREENUPDATE	g_screenupdate=1


extern CSong	g_Song;
extern CRmtMidi	g_Midi;
extern CUndo	g_Undo;
extern CXPokey	g_Pokey;
extern CInstruments	g_Instruments;
extern CTrackClipboard g_TrackClipboard;

/////////////////////////////////////////////////////////////////////////////
// CRmtView

IMPLEMENT_DYNCREATE(CRmtView, CView)

BEGIN_MESSAGE_MAP(CRmtView, CView)
	//{{AFX_MSG_MAP(CRmtView)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_SYSCHAR()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
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
	g_screena = 0;
	g_screenwait = 0;
	m_width = 0;
	m_height = 0;

	m_pen1 = NULL;
}

CRmtView::~CRmtView()
{
	g_mem_dc->SelectObject(m_penorig);		//The return of the original Pen
	delete m_pen1;							//unloaded m_pen1
}

void CRmtView::OnDestroy() 
{
	//cancels Pokey DLL
	g_Song.DeInitPokey();

	//cancels 6502 DLL
	Atari6502_DeInit();

	//turns off the timer
	if (m_timeranalyzer) 
	{
		KillTimer(m_timeranalyzer);
		m_timeranalyzer=0;
	}
	CView::OnDestroy();
}

void CRmtView::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == m_timeranalyzer)
	{
		DrawAnalyzer();
		DrawPlaytimecounter();
	}
	else
		CView::OnTimer(nIDEvent);
}

//debug function, poor attempt at a FPS counter
void CRmtView::GetFPS()
{
	using namespace std::chrono;
	uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	uint64_t sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

	real_fps++;
	int delta = (int)ms - (int)last_ms;
	avg_fps[real_fps % 120] = 1000.0 / delta;
	last_ms = ms;

	if (real_fps)
	{
		last_fps = 0;
		for (int i = 0; i < real_fps % 120; i++) { last_fps += avg_fps[i]; }
		last_fps /= real_fps % 120;
	}

	if (last_sec != sec)
	{
		real_fps = -1;
		last_sec = sec;
	}
}

//debug function, to get the pointer coordinates
void CRmtView::GetMouseXY(int px, int py, int mousebutt)
{
	g_mouse_px = px;
	g_mouse_py = py;
	g_mouselastbutt = mousebutt;
}

BOOL CRmtView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
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
	RECT r;
	GetClientRect(&r);
	if (g_scaling_percentage > 300 || g_scaling_percentage < 100) g_scaling_percentage = 100;	//set the default scaling as a failsafe if the values are beyond those limits
	bool resized = Resize(r.right - r.left + 1, r.bottom - r.top + 1);
	if (resized || (g_screenupdate && g_invalidatebytimer))
	{
		g_screenupdate=0;
		g_invalidatebytimer=0;
		g_screena = g_screenwait;
		DrawAll();
	}
	pDC->BitBlt(0, 0, m_width, m_height, &m_mem_dc, 0, 0, SRCCOPY);
	SCREENUPDATE;
}

BOOL CRmtView::DrawAll()
{
	m_mem_dc.FillSolidRect(0, 0, m_width, m_height, RGBBACKGROUND);		// Clear the screen

	GetFPS();	//it's crappy but it does an ok job

	g_Song.DrawInfo();
	g_Song.DrawSong();
	g_Song.DrawAnalyzer(NULL);
	g_Song.DrawPlaytimecounter(NULL);
	
	if (g_active_ti == PARTTRACKS)	//which one is the active screen?
	{
		g_Song.DrawTracks();
	}
	else
	{
		g_Song.DrawInstrument();
	}

	return 1;
}

void CRmtView::DrawAnalyzer()
{
	CDC* pDC = GetDC();
	if (pDC)
	{
		// g_Song.DrawAnalyzer(pDC);
		ReleaseDC(pDC);
	}
}

void CRmtView::DrawPlaytimecounter()
{
	CDC* pDC = GetDC();
	if (pDC)
	{
		// g_Song.DrawPlaytimecounter(pDC);
		ReleaseDC(pDC);
	}
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

void CRmtView::ReadConfig()
{
#define NAME(a)	(strcmp(a,name)==0)

	CString s;
	char line[1024];
	char *tmp,*div,*name,*value,*value2;
	s.Format("%s%s",g_prgpath,CONFIG_FILENAME);
	ifstream in(s);
	if (!in)
	{
		MessageBox("Can't read the config file\n"+s,"Read config",MB_ICONEXCLAMATION);
		WriteConfig(); //create config as soon as possible
		AfxGetApp()->GetMainWnd()->PostMessage(WM_COMMAND, ID_APP_ABOUT, 0);	//display the about dialog, assuming RMT was ran for the first time if no .ini file was found
		return;
	}
	while(!in.eof())
	{
		in.getline(line, 1023);
		tmp = strchr(line, '=');
		div = strchr(line, '/');
		if (div)
		{
			div[0] = 0;
			value2 = div + 1;
		}
		if (tmp)
		{
			tmp[0] = 0;
			name = line;
			value = tmp + 1;
		}
		else
			continue;
		//Individual lines config

		//general
		if (NAME("SCALEPERCENTAGE")) g_scaling_percentage = atoi(value);
		else
		if (NAME("TRACKLINEHIGHLIGHT")) g_tracklinehighlight = atoi(value);
		else
		if (NAME("TRACKLINEALTNUMBERING")) g_tracklinealtnumbering = atoi(value);
		else
		if (NAME("DISPLAYFLATNOTES")) g_displayflatnotes = atoi(value);
		else
		if (NAME("USEGERMANNOTATION")) g_usegermannotation = atoi(value);
		else
		if (NAME("NTSC_SYSTEM")) g_ntsc = atoi(value);
		else
		if (NAME("NOHWSOUNDBUFFER")) g_nohwsoundbuffer = atoi(value);
		else
		//keyboard
		if (NAME("KEYBOARD_LAYOUT")) g_keyboard_layout = atoi(value);
		else
		if (NAME("KEYBOARD_PLAYAUTOFOLLOW")) g_keyboard_playautofollow = atoi(value);
		else
		if (NAME("KEYBOARD_SWAPENTER")) g_keyboard_swapenter = atoi(value);
		else
		if (NAME("KEYBOARD_UPDOWNCONTINUE")) g_keyboard_updowncontinue = atoi(value);
		else
		if (NAME("KEYBOARD_REMEMBEROCTAVESANDVOLUMES")) g_keyboard_rememberoctavesandvolumes = atoi(value);
		else
		if (NAME("KEYBOARD_ESCRESETATARISOUND")) g_keyboard_escresetatarisound = atoi(value);
		else
		if (NAME("KEYBOARD_ASKWHENCONTROL_S")) g_keyboard_askwhencontrol_s = atoi(value);
		else
		//midi
		if (NAME("MIDI_IN")) g_Midi.SetDevice(value);
		else
		if (NAME("MIDI_TR")) g_midi_tr = atoi(value);
		else
		if (NAME("MIDI_VOLUMEOFFSET")) g_midi_volumeoffset = atoi(value);
		else
		if (NAME("MIDI_NOTEOFF")) g_midi_noteoff = atoi(value);
		else
		//paths
		if (NAME("PATH_SONGS")) g_path_songs = value;
		else
		if (NAME("PATH_INSTRUMENTS")) g_path_instruments = value;
		else
		if (NAME("PATH_TRACKS")) g_path_tracks = value;
		else
		//view
		if (NAME("VIEW_MAINTOOLBAR")) g_viewmaintoolbar = atoi(value);
		else
		if (NAME("VIEW_BLOCKTOOLBAR")) g_viewblocktoolbar = atoi(value);
		else
		if (NAME("VIEW_STATUSBAR")) g_viewstatusbar = atoi(value);
		else
		if (NAME("VIEW_PLAYTIMECOUNTER")) g_viewplaytimecounter = atoi(value);
		else
		if (NAME("VIEW_VOLUMEANALYZER")) g_viewanalyzer = atoi(value);
		else
		if (NAME("VIEW_POKEYCHIPREGISTERS")) g_viewpokeyregs = atoi(value);
		else
		if (NAME("VIEW_INSTRUMENTACTIVEHELP")) g_viewinstractivehelp = atoi(value);
		else
		//tuning
		if (NAME("TUNING")) g_basetuning = atof(value);
		else
		if (NAME("BASENOTE")) g_basenote = atoi(value);
		else
		if (NAME("TEMPERAMENT")) g_temperament = atoi(value);
		else
		//ratio
		if (NAME("UNISON")) { g_UNISON_L = atoi(value); if (div) g_UNISON_R = atoi(value2); }
		else
		if (NAME("MIN_2ND")) { g_MIN_2ND_L = atoi(value); if (div) g_MIN_2ND_R = atoi(value2); }
		else
		if (NAME("MAJ_2ND")) { g_MAJ_2ND_L = atoi(value); if (div) g_MAJ_2ND_R = atoi(value2); }
		else
		if (NAME("MIN_3RD")) { g_MIN_3RD_L = atoi(value); if (div) g_MIN_3RD_R = atoi(value2); }
		else
		if (NAME("MAJ_3RD")) { g_MAJ_3RD_L = atoi(value); if (div) g_MAJ_3RD_R = atoi(value2); }
		else
		if (NAME("PERF_4TH")) { g_PERF_4TH_L = atoi(value); if (div) g_PERF_4TH_R = atoi(value2); }
		else
		if (NAME("TRITONE")) { g_TRITONE_L = atoi(value); if (div) g_TRITONE_R = atoi(value2); }
		else
		if (NAME("PERF_5TH")) { g_PERF_5TH_L = atoi(value); if (div) g_PERF_5TH_R = atoi(value2); }
		else
		if (NAME("MIN_6TH")) { g_MIN_6TH_L = atoi(value); if (div) g_MIN_6TH_R = atoi(value2); }
		else
		if (NAME("MAJ_6TH")) { g_MAJ_6TH_L = atoi(value); if (div) g_MAJ_6TH_R = atoi(value2); }
		else
		if (NAME("MIN_7TH")) { g_MIN_7TH_L = atoi(value); if (div) g_MIN_7TH_R = atoi(value2); }
		else
		if (NAME("MAJ_7TH")) { g_MAJ_7TH_L = atoi(value); if (div) g_MAJ_7TH_R = atoi(value2); }
		else
		if (NAME("OCTAVE")) { g_OCTAVE_L = atoi(value); if (div) g_OCTAVE_R = atoi(value2); }
	}
	in.close();
}

void CRmtView::WriteConfig()
{
	CString s;
	s.Format("%s%s", g_prgpath, CONFIG_FILENAME);
	ofstream ou(s);
	if (!ou)
	{
		MessageBox("Can't create the config file\n"+s,"Write config",MB_ICONEXCLAMATION);
		return;
	}

	ou << "RMT CONFIGURATION FILE" << endl;
	CString version;
	version.LoadString(IDS_RMTVERSION);
	ou << version << endl;
	ou << setprecision(16);

	//general
	ou << "SCALEPERCENTAGE=" << g_scaling_percentage << endl;
	ou << "TRACKLINEHIGHLIGHT=" << g_tracklinehighlight << endl;
	ou << "TRACKLINEALTNUMBERING=" << g_tracklinealtnumbering << endl;
	ou << "DISPLAYFLATNOTES=" << g_displayflatnotes << endl;
	ou << "USEGERMANNOTATION=" << g_usegermannotation << endl;
	ou << "NTSC_SYSTEM=" << g_ntsc << endl;
	ou << "NOHWSOUNDBUFFER=" << g_nohwsoundbuffer << endl;
	//keyboard
	ou << "KEYBOARD_LAYOUT=" << g_keyboard_layout << endl;
	ou << "KEYBOARD_PLAYAUTOFOLLOW=" << g_keyboard_playautofollow << endl;
	ou << "KEYBOARD_SWAPENTER=" << g_keyboard_swapenter << endl;
	ou << "KEYBOARD_UPDOWNCONTINUE=" << g_keyboard_updowncontinue << endl;
	ou << "KEYBOARD_REMEMBEROCTAVESANDVOLUMES=" << g_keyboard_rememberoctavesandvolumes << endl;
	ou << "KEYBOARD_ESCRESETATARISOUND=" << g_keyboard_escresetatarisound << endl;
	ou << "KEYBOARD_ASKWHENCONTROL_S=" << g_keyboard_askwhencontrol_s << endl;
	//midi
	ou << "MIDI_IN=" << g_Midi.GetMidiDevName() << endl;
	ou << "MIDI_TR=" << g_midi_tr << endl;
	ou << "MIDI_VOLUMEOFFSET=" << g_midi_volumeoffset << endl;
	ou << "MIDI_NOTEOFF=" << g_midi_noteoff << endl;
	//paths
	ou << "PATH_SONGS=" << g_path_songs << endl;
	ou << "PATH_INSTRUMENTS=" << g_path_instruments << endl;
	ou << "PATH_TRACKS=" << g_path_tracks << endl;
	//view
	ou << "VIEW_MAINTOOLBAR=" << g_viewmaintoolbar << endl;
	ou << "VIEW_BLOCKTOOLBAR=" << g_viewblocktoolbar << endl;
	ou << "VIEW_STATUSBAR=" << g_viewstatusbar << endl;
	ou << "VIEW_PLAYTIMECOUNTER=" << g_viewplaytimecounter << endl;
	ou << "VIEW_VOLUMEANALYZER=" << g_viewanalyzer << endl;
	ou << "VIEW_POKEYCHIPREGISTERS=" << g_viewpokeyregs << endl;
	ou << "VIEW_INSTRUMENTACTIVEHELP=" << g_viewinstractivehelp << endl;
	//tuning
	ou << "TUNING=" << g_basetuning << endl;
	ou << "BASENOTE=" << g_basenote << endl;
	ou << "TEMPERAMENT=" << g_temperament << endl;
	//ratios
	ou << "UNISON=" << g_UNISON_L << "/" << g_UNISON_R << endl;
	ou << "MIN_2ND=" << g_MIN_2ND_L << "/" << g_MIN_2ND_R << endl;
	ou << "MAJ_2ND=" << g_MAJ_2ND_L << "/" << g_MAJ_2ND_R << endl;
	ou << "MIN_3RD=" << g_MIN_3RD_L << "/" << g_MIN_3RD_R << endl;
	ou << "MAJ_3RD=" << g_MAJ_3RD_L << "/" << g_MAJ_3RD_R << endl;
	ou << "PERF_4TH=" << g_PERF_4TH_L << "/" << g_PERF_4TH_R << endl;
	ou << "TRITONE=" << g_TRITONE_L << "/" << g_TRITONE_R << endl;
	ou << "PERF_5TH=" << g_PERF_5TH_L << "/" << g_PERF_5TH_R << endl;
	ou << "MIN_6TH=" << g_MIN_6TH_L << "/" << g_MIN_6TH_R << endl;
	ou << "MAJ_6TH=" << g_MAJ_6TH_L << "/" << g_MAJ_6TH_R << endl;
	ou << "MIN_7TH=" << g_MIN_7TH_L << "/" << g_MIN_7TH_R << endl;
	ou << "MAJ_7TH=" << g_MAJ_7TH_L << "/" << g_MAJ_7TH_R << endl;
	ou << "OCTAVE=" << g_OCTAVE_L << "/" << g_OCTAVE_R << endl;
	ou.close();
}

void CRmtView::OnViewConfiguration() 
{
	CConfigDlg dlg;
	//general
	dlg.m_scaling_percentage = g_scaling_percentage;
	dlg.m_tuning = g_basetuning;
	dlg.m_tracklinehighlight = g_tracklinehighlight;
	dlg.m_tracklinealtnumbering = g_tracklinealtnumbering;
	dlg.m_displayflatnotes = g_displayflatnotes;
	dlg.m_usegermannotation = g_usegermannotation;
	//
	dlg.m_ntsc = g_ntsc;
	dlg.m_nohwsoundbuffer = g_nohwsoundbuffer;
	//keyboard
	dlg.m_keyboard_layout = g_keyboard_layout;
	dlg.m_keyboard_escresetatarisound = g_keyboard_escresetatarisound;
	dlg.m_keyboard_playautofollow = g_keyboard_playautofollow;
	dlg.m_keyboard_swapenter = g_keyboard_swapenter;
	dlg.m_keyboard_updowncontinue = g_keyboard_updowncontinue;
	dlg.m_keyboard_rememberoctavesandvolumes = g_keyboard_rememberoctavesandvolumes;
	dlg.m_keyboard_askwhencontrol_s = g_keyboard_askwhencontrol_s;
	//midi
	dlg.m_midi_device = g_Midi.GetMidiDevId();
	dlg.m_midi_tr = g_midi_tr;
	dlg.m_midi_volumeoffset = g_midi_volumeoffset;
	dlg.m_midi_noteoff = g_midi_noteoff;
	//
	if (dlg.DoModal()==IDOK)
	{
		//general
		if (g_scaling_percentage != dlg.m_scaling_percentage)	//necessary to scale all the elements immetiately, without resizing the window first
		{
			g_width = (m_width * 100) / dlg.m_scaling_percentage;
			g_height = (m_height * 100) / dlg.m_scaling_percentage;
			g_tracklines = (g_height - (TRACKS_Y + 3 * 16) - 40) / 16; 
			SCREENUPDATE;
		}
		g_scaling_percentage = dlg.m_scaling_percentage;

		if (g_nohwsoundbuffer != dlg.m_nohwsoundbuffer)
		{
			g_Pokey.ReInitSound();	//the sound needs to be reinitialized
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		g_nohwsoundbuffer = dlg.m_nohwsoundbuffer;

		if (g_ntsc != dlg.m_ntsc)
		{
			//Pal or NTSC
			g_ntsc = dlg.m_ntsc;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			g_Song.ChangeTimer((g_ntsc) ? 17 : 20);
			g_Pokey.ReInitSound();	//the sound needs to be reinitialized
			Atari_InitRMTRoutine(); //reset RMT routines
			SCREENUPDATE;
		}
		g_ntsc = dlg.m_ntsc;

		g_tracklinehighlight = dlg.m_tracklinehighlight;
		g_tracklinealtnumbering = dlg.m_tracklinealtnumbering;
		g_displayflatnotes = dlg.m_displayflatnotes;
		g_usegermannotation = dlg.m_usegermannotation;

		//keyboard
		g_keyboard_layout = dlg.m_keyboard_layout;
		g_keyboard_escresetatarisound = dlg.m_keyboard_escresetatarisound;
		g_keyboard_playautofollow = dlg.m_keyboard_playautofollow;
		g_keyboard_swapenter = dlg.m_keyboard_swapenter;
		g_keyboard_updowncontinue=dlg.m_keyboard_updowncontinue;
		g_keyboard_rememberoctavesandvolumes = dlg.m_keyboard_rememberoctavesandvolumes;
		g_keyboard_askwhencontrol_s = dlg.m_keyboard_askwhencontrol_s;
		//midi
		if (dlg.m_midi_device>=0)
		{
			MIDIINCAPS micaps;
			midiInGetDevCaps(dlg.m_midi_device,&micaps, sizeof(MIDIINCAPS));
			g_Midi.SetDevice(micaps.szPname);
		}
		else
			g_Midi.SetDevice("");
		g_midi_tr = dlg.m_midi_tr;
		g_midi_volumeoffset = dlg.m_midi_volumeoffset;
		g_midi_noteoff = dlg.m_midi_noteoff;
		g_Midi.MidiInit();
		//
		WriteConfig();
		g_screenupdate=1;
	}
}

void CRmtView::OnViewTuning()
{
	TuningDlg dlg;
	dlg.m_basetuning = g_basetuning;
	dlg.m_basenote = g_basenote; 
	dlg.m_temperament = g_temperament;

	//ratio left
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

	//ratio right
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
		//ratio left
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

		//ratio right
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
		
		//update tuning
		g_basetuning = dlg.m_basetuning;
		g_basenote = dlg.m_basenote;
		g_temperament = dlg.m_temperament;
		WriteConfig();
		Atari_InitRMTRoutine(); //reset RMT routines
		g_screenupdate = 1;
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

//originally added in RMT 1.30, and reworked in 1.31+ 
bool CRmtView::Resize(int width, int height)
{
	int sp = g_scaling_percentage;
	if (width == m_width && height == m_height) return false;
	if (m_width != 0) {
		m_mem_dc.SelectObject((CBitmap*)0);
		m_mem_bitmap.DeleteObject();
		m_mem_dc.DeleteDC();		
	}
	m_width = width;
	m_height = height;
	if (m_width != 0) {
		g_width = (m_width * 100) / sp;
		g_height = (m_height * 100) / sp;
		CDC* dc = GetDC();
		m_mem_bitmap.CreateCompatibleBitmap(dc, m_width, m_height);
		m_mem_dc.CreateCompatibleDC(dc);
		m_mem_dc.SelectObject(&m_mem_bitmap);
		g_mem_dc = &m_mem_dc;
		g_tracklines = (g_height - (TRACKS_Y + 3 * 16) - 40) / 16;	//number of track lines that can be displayed based on the window height

		g_line_y = ( /*(m_trackactiveline + 8) -*/ (g_tracklines / 2));
		if (m_pen1) delete m_pen1;
		m_pen1 = new CPen(PS_SOLID, 1, RGBLINES);	
		m_penorig = g_mem_dc->SelectObject(m_pen1);
		m_mem_dc.FillSolidRect(0, 0, m_width, m_height, RGBBLACK); //initial black background
		ReleaseDC(dc);
		SCREENUPDATE;
	}
	return true;
}

void CRmtView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	//command line
	CString cmdl = GetCommandLine();
	CString commandlinefilename="";
	g_prgpath = "";
	g_lastloadpath_songs = g_lastloadpath_instruments = g_lastloadpath_tracks = "";
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
			commandlinefilename=cmdl.Mid(i1,i2-i1);
		}
	}
	CDC *dc=GetDC();
	g_Song.SetRMTTitle();
	Resize(800, 600); //default window dimensions if RMT was never used on a system, else, it will load the last known values
	m_gfx_bitmap.LoadBitmap(MAKEINTRESOURCE(IDB_GFX));
	m_gfx_dc.CreateCompatibleDC(dc);
	m_gfx_dc.SelectObject(&m_gfx_bitmap);
	g_mem_dc = &m_mem_dc;	//1.30 changes had this line commented out for some reason... however, I see no reason why this was necessary?
	g_gfx_dc = &m_gfx_dc;
	g_hwnd = AfxGetApp()->GetMainWnd()->m_hWnd;
	g_viewhwnd = this->m_hWnd;
	if (m_pen1) delete m_pen1;
	m_pen1 = new CPen(PS_SOLID, 1, RGBLINES);	
	m_penorig = g_mem_dc->SelectObject(m_pen1);
	m_mem_dc.FillSolidRect(0, 0, m_width, m_height, RGBBLACK); //initial black background
	ReleaseDC(dc);

	//cursor
	m_cursororig = LoadCursor(NULL,IDC_ARROW);
	m_cursorchanonoff = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORCHANNELONOFF));
	m_cursorenvvolume = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORENVVOLUME));
	m_cursorgoto = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORGOTO));
	m_cursordlg = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORDLG));
	m_cursorsetpos = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORSETPOS));

	//keyboard
	g_shiftkey=g_controlkey=0;	//TODO: add support for ALT key as well

	//current parts
	g_activepart = PARTTRACKS;	//tracks
	g_active_ti = PARTTRACKS;	//below the active tracks

	//turn on all channels
	SetChannelOnOff(-1,1);

	//CONFIGURATION
	ReadConfig();

	//view elements
	ChangeViewElements(0); //without write!

	//INITIAL POKEY INITIALISATION (DLL)
	if (!g_Song.InitPokey())
	{
		g_Song.DeInitPokey();
		exit(1);
	}

	//INITIAL 6502 INITIALIZATION (DLL)
	if (!Atari6502_Init())
	{
		Atari6502_DeInit();
		exit(1);
	}

	//INITIALISATION OF ATARI RMT ROUTINES
	Memory_Clear();	//clear the allocated memory space beforehand in order to avoid reading garbage
	if (!Atari_LoadRMTRoutines())
	{
		MessageBox("Fatal error with RMT ATARI system routines.\nCouldn't load 'RMT Binaries/tracker.obx'.","Error",MB_ICONERROR);
		exit(1);
	}
	Atari_InitRMTRoutine();

	Get_Driver_Version();	//get the driver version from the binary, nothing will appear if it does not exist

	g_screenupdate=1;	//first rendered
	m_timeranalyzer=0;
	m_timeranalyzer = SetTimer(1,20,NULL);

	Sleep(200);	//this will ensure there will be no false positive with the sound initialisation, else it would attempt to check too early and assume the plugins were not initialised

	//Displays the ABOUT dialog if there is no Pokey or 6502 initialized...
	if (!g_Pokey.GetRenderSound() || !g_is6502)
	{
		AfxGetApp()->GetMainWnd()->PostMessage(WM_COMMAND,ID_APP_ABOUT,0);
	}

	//Initialise MIDI
	g_Midi.MidiInit();
	g_Midi.MidiOn();

	//Pal or NTSC
	g_Song.ChangeTimer((g_ntsc)? 17 : 20);

	//If the tracker was started with an argument, it attempts to load the file, and will return an error if the extention isn't .rmt. 
	//When not argument is passed, the initialisation continues like normal.
	if (commandlinefilename != "")
	{
		if (commandlinefilename.Right(4) == ".rmt")
		{
			g_Song.FileOpen((LPCTSTR)commandlinefilename);
		}
		else
		{
			MessageBox("Invalid .rmt file!", "Error", MB_ICONERROR);
			exit(1);
		}
	}
}

int CRmtView::MouseAction(CPoint point,UINT mousebutt,short wheelzDelta=0)
{
	int i;
	int px,py;

	int sp = g_scaling_percentage;

	//scale the mouse XY coordinates to the actual display scaling, so the hitboxes will match everything visually rendered
	point.x = (point.x * 100) / sp;
	point.y = (point.y * 100) / sp;

	//debug
	GetMouseXY(point.x, point.y, mousebutt);

	//TODO: make those parameters global so they won't have to be re-initialised in multiple functions separately
	int MINIMAL_WIDTH_TRACKS = (g_tracks4_8 > 4 && g_active_ti == 1) ? 1420 : 960;
	int MINIMAL_WIDTH_INSTRUMENTS = (g_tracks4_8 > 4 && g_active_ti == 2) ? 1220 : 1220;
	int WINDOW_OFFSET = (g_width < 1320 && g_tracks4_8 > 4 && g_active_ti == 1) ? -250 : 0;	//test displacement with the window size
	int INSTRUMENT_OFFSET = (g_active_ti == 2 && g_tracks4_8 > 4) ? -250 : 0;
	if (g_tracks4_8 == 4 && g_active_ti == 2 && g_width > MINIMAL_WIDTH_INSTRUMENTS - 220) INSTRUMENT_OFFSET = 260;
	int SONG_OFFSET = SONG_X + WINDOW_OFFSET + INSTRUMENT_OFFSET + ((g_tracks4_8 == 4) ? -200 : 310);	//displace the SONG block depending on certain parameters

	int linescount = (WINDOW_OFFSET) ? 5 : 9;	//songlines displayed depend on the window offset, if it's displaced to the left side, only 5 lines will be visible, else, 9 will be displayed

	//SONG PARTS
	CRect rec(SONG_OFFSET + 6 * 8, SONG_Y + 16, SONG_OFFSET + 6 * 8 + g_tracks4_8 * 3 * 8 - 8, SONG_Y + 16 + linescount * 16); 
	if (rec.PtInRect(point))
	{
		//Song
		SetCursor(m_cursorgoto);
		
		if (mousebutt & MK_LBUTTON)
		{
			int lineoffset = (WINDOW_OFFSET) ? SONG_Y + 16 : SONG_Y + 48;
			BOOL r = g_Song.SongCursorGoto(CPoint(point.x - (SONG_OFFSET + 6 * 8), point.y - lineoffset));
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta!=0)
		{
			BOOL r=0;
			if (wheelzDelta > 0) r = g_Song.SongKey(VK_UP,0,0);
			else
			if (wheelzDelta < 0) r = g_Song.SongKey(VK_DOWN,0,0);
			if (r) SCREENUPDATE;
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
		SetCursor(m_cursorchanonoff);
		if (mousebutt & MK_LBUTTON)
		{
			SetChannelOnOff(px,-1);	//inversion
			SCREENUPDATE;
		}
		if (mousebutt & MK_RBUTTON)
		{
			SetChannelSolo(px);		//solo/mute on off
			SCREENUPDATE;
		}
		return 1;
	}

	//INFO PARTS
	rec.SetRect(64, 32, 64 + SONGNAMEMAXLEN * 8, 32 + 16);
	if (rec.PtInRect(point))
	{
		//Song name
		SetCursor(m_cursorgoto);
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r = g_Song.InfoCursorGotoSongname(point.x - 64);
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(120, 48, 120 + 7 * 8, 48 + 16);
	if (rec.PtInRect(point))
	{
		//Song speed
		SetCursor(m_cursorgoto);
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r = g_Song.InfoCursorGotoSpeed(point.x - 120);
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(336, 48, 336 + 2 * 8, 48 + 16);
	if (rec.PtInRect(point))
	{
		//MAXTRACKLENGTH
		SetCursor(m_cursordlg);
		if (mousebutt & MK_LBUTTON)
		{
			OnSongSongchangemaximallengthoftracks();
			SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(384, 48, 384 + 8 * ((g_tracks4_8 == 8) ? 15 : 13), 48 + 16);
	if (rec.PtInRect(point))
	{
		//MONO-4-TRACKS or STEREO-8-TRACKS
		SetCursor(m_cursordlg);
		if (mousebutt & MK_LBUTTON)
		{
			OnSongSongswitch4_8();
			SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(280, 16, 280 + 8 * ((g_ntsc) ? 4 : 3), 16 + 16);
	if (rec.PtInRect(point))
	{
		//PAL or NTSC
		SetCursor(m_cursorgoto);
		if (mousebutt & MK_LBUTTON)
		{
			g_ntsc ^=1;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			g_Song.ChangeTimer((g_ntsc)? 17 : 20);
			g_Pokey.ReInitSound();	//the sound needs to be reinitialized
			WriteConfig();
			Atari_InitRMTRoutine(); //reset RMT routines
			SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(440, 16, 440 + 8 * 2, 16 + 16);
	if (rec.PtInRect(point))
	{
		//Songline highlight
		SetCursor(m_cursorgoto);
		if (mousebutt & MK_LBUTTON)
		{
			//TODO
		}
		if (wheelzDelta != 0)
		{
			BOOL r = 0;
			int ma = (g_Tracks.m_maxtracklen) / 2;
			if (wheelzDelta < 0)
			{
				r = g_tracklinehighlight++;
				if (g_tracklinehighlight > ma) g_tracklinehighlight = ma;
			}
			else if (wheelzDelta > 0)
			{
				r = g_tracklinehighlight--;
				if (g_tracklinehighlight < 1) g_tracklinehighlight = 1;
			}
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(456, 64, 456 + 8 * 10, 64 + 16);
	if (rec.PtInRect(point))
	{
		//Octave Select Dialog
		SetCursor(m_cursordlg);
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r = g_Song.InfoCursorGotoOctaveSelect(point.x, point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta != 0)
		{
			BOOL r = 0;
			if (wheelzDelta < 0) r = g_Song.OctaveUp();
			else if (wheelzDelta > 0) r = g_Song.OctaveDown();
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(472, 80, 472 + 8 * 8, 80 + 16);
	if (rec.PtInRect(point))
	{
		//Volume Select Dialog
		SetCursor(m_cursordlg);
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r = g_Song.InfoCursorGotoVolumeSelect(point.x, point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta != 0)
		{
			BOOL r = 0;
			if (wheelzDelta < 0) r = g_Song.VolumeUp();
			else if (wheelzDelta > 0) r = g_Song.VolumeDown();
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(136, 80, 136 + INSTRNAMEMAXLEN * 8, 80 + 16);
	if (rec.PtInRect(point))
	{
		//Instrument Select Dialog
	Instrument_Select_Dialog:

		SetCursor(m_cursordlg);
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r = g_Song.InfoCursorGotoInstrumentSelect(point.x, point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta != 0)
		{
			if (wheelzDelta > 0) g_Song.ActiveInstrPrev();
			else if (wheelzDelta < 0) g_Song.ActiveInstrNext();
			SCREENUPDATE;
		}
		return 6;
	}

	//LOWER PARTS
	if (g_active_ti==PARTTRACKS)
	{
		rec.SetRect(TRACKS_X + 3 * 16, TRACKS_Y - 12, TRACKS_X + 3 * 8 + g_tracks4_8 * 8 * 16, TRACKS_Y + 32);

		if (rec.PtInRect(point))
		{
			i = (point.x - (TRACKS_X + 5 * 8)) / (8 * 16);
			if (i<0) i=0;
			else
			if (i>=g_tracks4_8) i=g_tracks4_8-1;
			px = i;

			SetCursor(m_cursorchanonoff);

			if (mousebutt & MK_LBUTTON)
			{
				SetChannelOnOff(px,-1);	//inversion
				SCREENUPDATE;
			}
			if (mousebutt & MK_RBUTTON)
			{
				SetChannelSolo(px);		//solo/mute/on/off
				SCREENUPDATE;
			}
			return 1;
		}
		//the number of tracklines is adjusted based on the window height
		rec.SetRect(TRACKS_X + 6 * 8, TRACKS_Y + 48, TRACKS_X + 3 * 8 + g_tracks4_8 * 8 * 16, TRACKS_Y + 48 + g_tracklines * 16);
		if (rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				BOOL r = g_Song.TrackCursorGoto(CPoint(point.x - (TRACKS_X + 6 * 8), point.y - (TRACKS_Y + 48)));
				if (r) SCREENUPDATE;
			}
			if (wheelzDelta!=0)
			{
				BOOL r=0;
				if (wheelzDelta>0) r=g_Song.TrackKey(VK_UP,0,0);
				else
				if (wheelzDelta<0) r=g_Song.TrackKey(VK_DOWN,0,0);
				if (r) SCREENUPDATE;
			}
			return 4;
		}
	}
	else
	if (g_active_ti==PARTINSTRS)
	{
		//InstrEdit
		int ainstr = g_Song.GetActiveInstr();
		//VOLUME LEFT (bottom)
		int r= g_Instruments.GetInstrArea(ainstr,0,rec);
		if (r && rec.PtInRect(point))
		{
			px = (point.x - rec.left) /8;
			py = 15- ((point.y - rec.top) /4);
			SetCursor(m_cursorenvvolume);
			if (g_mousebutt & MK_LBUTTON) //compares g_mousebutt to make it work while moving
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvVolume(ainstr,0,px,py);
				SCREENUPDATE;
			}
			if (g_mousebutt & MK_RBUTTON) //compares g_mousebutt to make it work while moving
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvVolume(ainstr,0,px,0);
				SCREENUPDATE;
			}
			return 2;
		}

		//VOLUME RIGHT (upper)
		r= g_Instruments.GetInstrArea(ainstr,1,rec);
		if (r && rec.PtInRect(point))
		{
			px = (point.x - rec.left) /8;
			py = 15- ((point.y - rec.top) /4);
			SetCursor(m_cursorenvvolume);
			if (g_mousebutt & MK_LBUTTON) //compares g_mousebutt to make it work while moving
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvVolume(ainstr,1,px,py);
				SCREENUPDATE;
			}
			if (g_mousebutt & MK_RBUTTON) //compares g_mousebutt to make it work while moving
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				g_Instruments.SetEnvVolume(ainstr,1,px,0);
				SCREENUPDATE;
			}
			return 3;
		}

		//ENVELOPE PARAMETERS large table
		r= g_Instruments.GetInstrArea(ainstr,2,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),0);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//ENVELOPE PARAMETERS series of numbers for the right channel volume
		r= g_Instruments.GetInstrArea(ainstr,3,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),1);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT TABLE
		r= g_Instruments.GetInstrArea(ainstr,4,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),2);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT NAME
		r= g_Instruments.GetInstrArea(ainstr,5,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),3);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT PARAMETERS
		r= g_Instruments.GetInstrArea(ainstr,6,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			if (mousebutt & MK_LBUTTON)
			{
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),4);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT SELECT DIALOG
		r= g_Instruments.GetInstrArea(ainstr,7,rec);
		if (r && rec.PtInRect(point))
		{
			point.y-=(4*16+8);
			point.x+=64;
			goto Instrument_Select_Dialog;
		}

		//ENVELOPE LEN a GO PARAMETER - length and loop to help the mouse
		r= g_Instruments.GetInstrArea(ainstr,8,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorsetpos);
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),5);
			}
			if (mousebutt & MK_RBUTTON)
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),6);
			}
			if (r) SCREENUPDATE;
			return 7;
		}

		//TABLE LEN a GO PARAMETER - length and loop to help the mouse
		r= g_Instruments.GetInstrArea(ainstr,9,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorsetpos);
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),7);
			}
			if (mousebutt & MK_RBUTTON)
			{
				g_Undo.ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=g_Instruments.CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),8);
			}
			if (r) SCREENUPDATE;
			return 7;
		}

	}
	SetCursor(m_cursororig);
	return 0;
}

void CRmtView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	g_Undo.Separator();
	g_mousebutt|=MK_LBUTTON;
	MouseAction(point,MK_LBUTTON);
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
	g_mousebutt|=MK_RBUTTON;
	MouseAction(point,MK_RBUTTON);
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
	MouseAction(point,0);
	CView::OnMouseMove(nFlags, point);
}

BOOL CRmtView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	return 1;
}

void CRmtView::OnSysChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	CView::OnSysChar( nChar, nRepCnt, nFlags );
}

const int  NChaCode[]={  36,  38,  33, VK_SUBTRACT,  37,  12,  39, VK_ADD,  35,  40,  34,  45};
const char FlaToCha[]={0x67,0x68,0x69,109,0x64,0x65,0x66,107,0x61,0x62,0x63,0x60};
const char layout2[]={VK_F5,VK_F6,VK_F7,VK_F8, VK_F3,VK_F2,VK_F4,VK_ESCAPE};

//TODO: cleanup and reconfigure, since testing keys in Stereo is not working correctly due to all the shortcuts being intermixed into the inputs
void CRmtView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	UINT vk = nChar;
	UINT nfb = nFlags & 0x1ff;	//when autorepeat is set in nFlags bit 16384
	//this seems to work around possible problems and causes no harm... so let's leave this untouched
	if (nfb>=71 && nfb<=82) //shift + numblock 0-9 + -
	{
		if ((int)nChar == NChaCode[nfb-71])
		{
			vk=FlaToCha[nfb-71];
		}
	}

	switch(vk)
	{
	case 0x5A: //Z
		if (g_controlkey && !g_shiftkey) //or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			if (g_Song.Undo()) //CTRL+Z
			{
				SCREENUPDATE;
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
				SCREENUPDATE;
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
		{
			Atari_InitRMTRoutine(); //reset RMT routines automatically
		}
		if (g_shiftkey) 
		{
			g_Pokey.ReInitSound(); 
			Atari_InitRMTRoutine(); //reset RMT routines
		}
		if (g_Song.GetPlayMode()==0) //only if the module is stopped
		{
			g_playtime=0;
			DrawPlaytimecounter();
		}
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
			g_prove = 4;	//POKEY EXPLORER MODE -- KEYBOARD INPUT AND FORMULAE DISPLAY
			break;
		}
		g_Song.ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
		g_Song.Play(MPLAY_SONG,g_Song.m_followplay);	//play song from start
		break;

	case VK_F6:
		g_Song.ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
		if (g_shiftkey) g_Song.Play(MPLAY_BLOCK,g_Song.m_followplay);	//play block and follow
		else g_Song.Play(MPLAY_TRACK,g_Song.m_followplay);				//play pattern and follow	
		break;

	case VK_F7:
		g_Song.ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
		if (g_Song.IsBookmark() && g_shiftkey) g_Song.Play(MPLAY_BOOKMARK,g_Song.m_followplay);	//play song from bookmark
		else g_Song.Play(MPLAY_FROM,g_Song.m_followplay);							//play song from current position
		break;

	case VK_F8:
		if (g_controlkey) g_Song.ClearBookmark();	//clear bookmark
		else g_Song.SetBookmark();					//set song bookmark
		break;

	case VK_F9:
		if (!g_controlkey && g_shiftkey)	
		{
			SetChannelOnOff(-1,-1);		//switch all channels on or off
		}		
		else if (g_controlkey)
		{
			int ch = g_Song.GetActiveColumn(); //solo current channel
			SetChannelSolo(ch);
		}
		else
		{
			int ch = g_Song.GetActiveColumn(); //mute current channel
			SetChannelOnOff(ch,-1);	
		}
		SCREENUPDATE;
		break;

	//F10 can't be used for some reason... it seems to be binded to native Windows functions and so it would take priority instead of any shortcut I would like to use for it.

	case VK_F11:						
		g_Undo.Separator();	//respect volume
		g_respectvolume ^=1;
		SCREENUPDATE;
		break;

	case VK_F12: 
		if (g_controlkey) 
		{
			g_ntsc ^= 1;
			g_basetuning = (g_ntsc) ? (g_basetuning * FREQ_17_NTSC) / FREQ_17_PAL : (g_basetuning * FREQ_17_PAL) / FREQ_17_NTSC;
			g_Song.ChangeTimer((g_ntsc) ? 17 : 20);
			g_Pokey.ReInitSound();	//the sound needs to be reinitialized
			WriteConfig();
			Atari_InitRMTRoutine(); //reset RMT routines
			SCREENUPDATE;
		}
		else OnPlayfollow(); //toggle follow position
		break;

	case VK_MEDIA_PLAY_PAUSE:
		if (g_Song.GetPlayMode()==0)
		g_Song.Play(MPLAY_SONG,g_Song.m_followplay);	//play song from start
		else g_Song.Stop();								//if playing, stop
		break;

	case VK_MEDIA_NEXT_TRACK:
		g_Song.Play(MPLAY_SEEK_NEXT,g_Song.m_followplay); //seek next and play from track
		break;

	case VK_MEDIA_PREV_TRACK:
		g_Song.Play(MPLAY_SEEK_PREV,g_Song.m_followplay); //seek prev and play from track
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
			SetChannelOnOff(vk-49,-1);		//inverts channel status 1-8 (=> on / off)
			SCREENUPDATE;
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
			g_Song.Stop();
			int r = MessageBox("Would you like to create a new song?", "Create new song", MB_YESNOCANCEL | MB_ICONQUESTION);
			if (r == IDYES) g_Song.FileNew();
		}
		else
			goto AllModesDefaultKey;
		break;

	case 83:	//VK_S
		if (g_controlkey && !g_shiftkey) //CTRL+S, or do nothing when SHIFT is also held, this deliberately makes it less likely to happen by accident and conflict with every other commands
		{
			g_Song.Stop();
			SetStatusBarText("Save...");
			CString filename=g_Song.GetFilename();
			if (g_keyboard_askwhencontrol_s
				&& (filename!="" || g_Song.GetFiletype()!=0))
			{
				//if a question is asked and if a file already exists
				//(=> there will be a "Save as ..." dialog)
				CString s;
				s.Format("Do you want to save song file '%s'?\nIs it okay to overwrite?",filename);
				int r=MessageBox(s,"Save song",MB_YESNOCANCEL | MB_ICONQUESTION);
				if (r==IDNO) { OnFileSaveAs(); goto end_save_control_s; }
				if (r!=IDYES) goto end_save_control_s;
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
			g_Song.Stop();
			g_Song.FileReload();	//turns out this function handles the rest already, so jump right to it instead
		}
		else
			goto AllModesDefaultKey;
		break;

	default:
	AllModesDefaultKey:
			int r=0;
			BOOL CAPSLOCK = GetKeyState(20);	//VK_CAPS_LOCK
			switch (g_activepart)
			{
			case PARTINFO:
				if (g_shiftkey && !is_editing_infos && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
					r = g_Song.ProveKey(vk, g_shiftkey, g_controlkey);	//plays a note while the SHIFT key is held, except on the Song Name field, it will be ignored
				else if (g_shiftkey && !is_editing_infos && (NoteKey(vk) < 0))
					if (vk == VK_TAB || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_PAGE_UP || vk == VK_PAGE_DOWN) goto do_infokey_anyway;
					else break;	//prevents inputing incorrect infos by accident while testing notes holding SHIFT
				else if (is_editing_infos && CAPSLOCK && !g_shiftkey)
				{
					g_shiftkey = 1;
					r = g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
					g_shiftkey = 0;	//workaround: so it won't *stay* locked when CAPSLOCK isn't active
					break;
				}
				else if (is_editing_infos && CAPSLOCK && g_shiftkey)
				{
					g_shiftkey = 0;
					r = g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
					g_shiftkey = 1;	//workaround: so it will *stay* locked when CAPSLOCK isn't active
					break;
				}
				else 
				{
do_infokey_anyway:
					if (vk == VK_PAGE_UP || vk == VK_PAGE_DOWN)
						r = g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
					else
					r = g_Song.InfoKey(vk, g_shiftkey, g_controlkey);
				}
				break;

			case PARTTRACKS:
				if (g_prove)
					r = g_Song.ProveKey(vk,g_shiftkey,g_controlkey);
				else if (g_shiftkey && (NoteKey(vk)>=0 || Numblock09Key(vk)>=0 || vk==VK_SPACE))
					r = g_Song.ProveKey(vk,g_shiftkey,g_controlkey);
				else 
					r = g_Song.TrackKey(vk,g_shiftkey,g_controlkey);
				break;

			case PARTINSTRS:
				if (g_shiftkey && !is_editing_instr && (NoteKey(vk) >= 0 || Numblock09Key(vk) >= 0 || vk == VK_SPACE))
					r = g_Song.ProveKey(vk, g_shiftkey, g_controlkey);	//plays a note while the SHIFT key is held, except on the Instrument Name field, it will be ignored
				else if (g_shiftkey && !is_editing_instr && (NoteKey(vk) < 0))
					if (vk == VK_TAB || vk == VK_INSERT || vk == VK_DELETE || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_DIVIDE || vk == VK_MULTIPLY || vk == VK_SUBTRACT || vk == VK_ADD || vk == VK_PAGE_UP || vk == VK_PAGE_DOWN) goto do_instrkey_anyway;
					else break;	//prevents inputing incorrect infos by accident while testing notes holding SHIFT
				else if (is_editing_instr && CAPSLOCK && !g_shiftkey)
				{
					g_shiftkey = 1;
					r = g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
					g_shiftkey = 0;	//workaround: so it won't *stay* locked when CAPSLOCK isn't active
					break;
				}
				else if (is_editing_instr && CAPSLOCK && g_shiftkey)
				{
					g_shiftkey = 0;
					r = g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
					g_shiftkey = 1;	//workaround: so it will *stay* locked when CAPSLOCK isn't active
					break;
				}
				else
				{
do_instrkey_anyway:
					if (vk == VK_PAGE_UP || vk == VK_PAGE_DOWN)
						r = g_Song.ProveKey(vk, g_shiftkey, g_controlkey);
					else
					r = g_Song.InstrKey(vk, g_shiftkey, g_controlkey);
				}
				break;

			case PARTSONG:
				if (g_prove)
					r = g_Song.ProveKey(vk,g_shiftkey,g_controlkey);
				else if (g_shiftkey && (NoteKey(vk)>=0 || Numblock09Key(vk)>=0 || vk==VK_SPACE))
					r = g_Song.ProveKey(vk,g_shiftkey,g_controlkey);
				else 
					r = g_Song.SongKey(vk,g_shiftkey,g_controlkey);
				break;
			}
		if (r) SCREENUPDATE;
	}
//UndoCheckPoint does not work if it is only a shift or just a control (to which the next key will come)
KeyDownNoUndoCheckPoint:
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRmtView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	//TODO: Add support for ALT key for the "is held" flag, currently it does not work for some reason
	if (nChar==VK_SHIFT)
	{
		g_shiftkey=0;
	}
	else
	if (nChar==VK_CONTROL) 
	{
		g_controlkey=0;
	}
	else
	if (nChar==VK_LMENU)
	{
		g_altkey=0;
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
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
	SCREENUPDATE;
}

void CRmtView::OnFileImport() 
{
	g_Song.FileImport();
	SCREENUPDATE;
}

void CRmtView::OnFileExportAs() 
{
	g_Song.FileExportAs();
}

void CRmtView::OnInstrLoad() 
{
	g_Song.FileInstrumentLoad();
	SCREENUPDATE;
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
	SCREENUPDATE;
}

void CRmtView::OnInstrCut() 
{
	g_Undo.ChangeInstrument(g_Song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	g_Song.InstrCut();
	SCREENUPDATE;
}

void CRmtView::OnInstrDelete() 
{
	g_Undo.ChangeInstrument(g_Song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	g_Song.InstrDelete();
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeLRenvelopesonly() 
{
	g_Song.InstrPaste(1); //L/R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeRenvelopeonly() 
{
	g_Song.InstrPaste(2); //R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeLenvelopeonly() 
{
	g_Song.InstrPaste(3); //L
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialEnvelopeparametersonly() 
{
	g_Song.InstrPaste(4); //ENVELOPE PARS
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialTableonly() 
{
	g_Song.InstrPaste(5); //TABLE
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeenvandenvelopeparsonly() 
{
	g_Song.InstrPaste(6); //VOL+ENV
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialInsertvolenvsandenvparstocurpos() 
{
	g_Song.InstrPaste(7); //VOL+ENV TO CURPOS
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeltorenvelopeonly() 
{
	g_Song.InstrPaste(8); //volume L to R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumertolenvelopeonly() 
{
	g_Song.InstrPaste(9); //volume R to L
	SCREENUPDATE;
}

//update
void CRmtView::OnUpdateInstrumentPastespecialInsertvolenvsandenvparstocurpos(CCmdUI* pCmdUI) 
{
	//to cur pos
	int i = g_Song.GetActiveInstr();
	TInstrument* ai = &g_Instruments.m_instr[i];
	pCmdUI->Enable(g_activepart==PARTINSTRS && (ai->act==2)); //when the envelope is being edited
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
	SCREENUPDATE;
}

void CRmtView::OnTrackCut() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA);
	g_Song.TrackCut();
	SCREENUPDATE;
}

void CRmtView::OnTrackDelete() 
{
	g_Undo.ChangeTrack(g_Song.SongGetActiveTrack(),g_Song.GetActiveLine(),UETYPE_TRACKDATA);
	g_Song.TrackDelete();
	SCREENUPDATE;
}

void CRmtView::OnUpdateTrackCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (g_Song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (g_Song.SongGetActiveTrack()>=0));
}

/*
void CRmtView::OnInstrumentRandominstrument() 
{
	g_Instruments.RandomInstrument(g_Song.GetActiveInstr()); //Random
	SCREENUPDATE;
}

void CRmtView::OnUpdateInstrumentRandominstrument(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_activepart==PARTINSTRS);	
}
*/

void CRmtView::OnTrackLoad() 
{
	g_Song.FileTrackLoad();
	SCREENUPDATE;
}

void CRmtView::OnTrackSave() 
{
	g_Song.FileTrackSave();
	SCREENUPDATE;
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
	SCREENUPDATE;
}

void CRmtView::OnSongClearline() 
{
	g_Song.SongClearLine();
	SCREENUPDATE;
}

void CRmtView::OnSongDeleteactualline() 
{
	g_Song.SongDeleteLine(g_Song.SongGetActiveLine());
	SCREENUPDATE;
}

void CRmtView::OnSongInsertnewemptyline() 
{
	g_Song.SongInsertLine(g_Song.SongGetActiveLine());
	SCREENUPDATE;
}

void CRmtView::OnSongInsertnewlinewithunusedtracks() 
{
	int line = g_Song.SongGetActiveLine();
	g_Song.SongPrepareNewLine(line);
	g_Song.SongSetActiveLine(line);
	SCREENUPDATE;
}

void CRmtView::OnSongInsertcopyorcloneofsonglines() 
{
	int line = g_Song.SongGetActiveLine();
	g_Song.SongInsertCopyOrCloneOfSongLines(line);
	g_Song.SongSetActiveLine(line);
	SCREENUPDATE;
}

void CRmtView::OnSongPutnewemptyunusedtrack() 
{
	g_Song.SongPutnewemptyunusedtrack();
	SCREENUPDATE;
}

void CRmtView::OnSongMaketracksduplicate() 
{
	g_Song.SongMaketracksduplicate();
	SCREENUPDATE;	
}

void CRmtView::OnUpdateSongMaketracksduplicate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_Song.SongGetActiveTrack()>=0);
}

void CRmtView::OnPlay0() 
{
	g_Song.Play(MPLAY_BOOKMARK,g_Song.m_followplay);	//from the bookmark - with respect to followplay
}

void CRmtView::OnPlay1() 
{
	g_Song.Play(MPLAY_SONG,g_Song.m_followplay);		//whole song from start - with respect to followplay
}

void CRmtView::OnPlay2() 
{
	g_Song.Play(MPLAY_FROM,g_Song.m_followplay);		//from the current position - with respect to followplay
}

void CRmtView::OnPlay3() 
{
	g_Song.Play(MPLAY_TRACK,g_Song.m_followplay);		//current pattern and loop - with respect to followplay
}

void CRmtView::OnPlaystop() 
{
	g_Song.Stop();	//stops everything
	if (g_Song.GetPlayMode()==0) //only if the module is stopped
	{
		g_playtime=0;
		DrawPlaytimecounter();
	}
}

void CRmtView::OnPlayfollow() 
{
	g_Song.m_followplay ^= 1;
}

void CRmtView::OnEmTracks() 
{
	g_activepart=g_active_ti=PARTTRACKS;	//tracks
	SCREENUPDATE;
}

void CRmtView::OnEmInstruments() 
{
	g_activepart=g_active_ti=PARTINSTRS;	//instrs
	g_TrackClipboard.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnEmInfo() 
{
	g_activepart=PARTINFO;		//info
	g_TrackClipboard.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnEmSong() 
{
	g_activepart=PARTSONG;		//song
	g_TrackClipboard.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnUpdatePlay0(CCmdUI* pCmdUI) 
{
	int ch= g_Song.IsBookmark();
	pCmdUI->Enable(ch);
	ch= (g_Song.GetPlayMode()==MPLAY_BOOKMARK);
	pCmdUI->SetCheck(ch);
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
	int ch= (g_Song.m_followplay)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmTracks(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PARTTRACKS && g_active_ti==PARTTRACKS)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmInstruments(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PARTINSTRS && g_active_ti==PARTINSTRS)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmInfo(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PARTINFO)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdateEmSong(CCmdUI* pCmdUI) 
{
	int ch= (g_activepart==PARTSONG)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnProvemode() 
{
	if (g_prove == 0) g_prove = 1;
	else if (g_prove >= 3) g_prove = 0;		//disable the special test modes immediately
	else
	{
		if (g_prove == 1 && g_tracks4_8 > 4)	//PROVE 2 only works for 8 tracks
			g_prove = 2;
		else
			g_prove = 0;
	}
	SCREENUPDATE;
}

void CRmtView::OnUpdateProvemode(CCmdUI* pCmdUI) 
{
	int ch= (g_prove>0)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::ChangeViewElements(BOOL writeconfig)
{
	CMainFrame* mf = (CMainFrame*)AfxGetApp()->GetMainWnd();
	mf->ShowControlBar((CControlBar*)(&mf->m_wndToolBar),g_viewmaintoolbar,0);
	mf->ShowControlBar((CControlBar*)(&mf->m_ToolBarBlock),g_viewblocktoolbar,0);
	mf->ShowControlBar((CControlBar*)(&mf->m_wndStatusBar),g_viewstatusbar,0);
	if (writeconfig) WriteConfig();
}

void CRmtView::OnViewToolbar() 
{
	g_viewmaintoolbar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewToolbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewmaintoolbar);
}

void CRmtView::OnViewBlocktoolbar() 
{
	g_viewblocktoolbar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewBlocktoolbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewblocktoolbar);
}

void CRmtView::OnViewStatusBar() 
{
	g_viewstatusbar ^= 1;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewStatusBar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewstatusbar);
}

void CRmtView::OnViewPlaytimecounter() 
{
	g_viewplaytimecounter ^= 1;
	SCREENUPDATE;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewPlaytimecounter(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewplaytimecounter);
}

void CRmtView::OnViewVolumeanalyzer() 
{
	g_viewanalyzer ^= 1;
	SCREENUPDATE;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewVolumeanalyzer(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewanalyzer);	
}

void CRmtView::OnViewPokeyregs() 
{
	g_viewpokeyregs ^= 1;
	SCREENUPDATE;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewPokeyregs(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewpokeyregs);
	pCmdUI->Enable(g_viewanalyzer);
}

void CRmtView::OnViewInstrumentactivehelp() 
{
	g_viewinstractivehelp ^= 1;
	SCREENUPDATE;
	ChangeViewElements();
}

void CRmtView::OnUpdateViewInstrumentactivehelp(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_viewinstractivehelp);	
}

void CRmtView::OnBlockNoteup() 
{
	g_TrackClipboard.BlockNoteTransposition(g_Song.GetActiveInstr(),1);
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockNoteup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockNotedown() 
{
	g_TrackClipboard.BlockNoteTransposition(g_Song.GetActiveInstr(),-1);
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockNotedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockVolumeup() 
{
	g_TrackClipboard.BlockVolumeChange(g_Song.GetActiveInstr(),1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockVolumeup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockVolumedown() 
{
	g_TrackClipboard.BlockVolumeChange(g_Song.GetActiveInstr(),-1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockVolumedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}


void CRmtView::OnBlockInstrleft() 
{
	g_TrackClipboard.BlockInstrumentChange(g_Song.GetActiveInstr(),-1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockInstrleft(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockInstrright() 
{
	g_TrackClipboard.BlockInstrumentChange(g_Song.GetActiveInstr(),1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockInstrright(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockInstrall() 
{
	g_TrackClipboard.BlockAllOnOff();	
	SCREENUPDATE;
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
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockBackup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_TrackClipboard.IsBlockSelected());
}

void CRmtView::OnBlockPlay() 
{
	g_Song.Play(MPLAY_BLOCK,g_Song.m_followplay);	//selected block and loop - with respect to followplay
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
	g_Song.TrackKey(67,0,1);	//Ctrl+C
	SCREENUPDATE;
}

void CRmtView::OnBlockCut() 
{
	g_Song.TrackKey(88,0,1);	//Ctrl+X
	SCREENUPDATE;
}

void CRmtView::OnBlockDelete() 
{
	g_Song.TrackKey(VK_DELETE,0,1);	//Del
	SCREENUPDATE;
}

void CRmtView::OnBlockPaste() 
{
	g_Song.BlockPaste();	//paste normal
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialMergewithcurrentcontent() 
{
	g_Song.BlockPaste(1);	//paste special - merge
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialVolumevaluesonly() 
{
	g_Song.BlockPaste(2);	//paste special - volumes only
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialSpeedvaluesonly() 
{
	g_Song.BlockPaste(3);	//paste special - speeds only
	SCREENUPDATE;
}

void CRmtView::OnBlockExchange() 
{
	g_Song.TrackKey(69,0,1);	//Ctrl+E
	SCREENUPDATE;
}

void CRmtView::OnBlockEffect() 
{
	g_Song.TrackKey(70,0,1);	//Ctrl+F
	SCREENUPDATE;
}

void CRmtView::OnBlockSelectall() 
{
	g_Song.TrackKey(65,0,1);	//Ctrl+A
	SCREENUPDATE;
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
	SCREENUPDATE;
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
	SCREENUPDATE;
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
		SCREENUPDATE;
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
		int res=g_Tracks.TrackBuildLoop(track);
		if (res) SCREENUPDATE;
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
		int res=g_Tracks.TrackExpandLoop(track);
		if (res) SCREENUPDATE;
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
	int r=g_Song.InstrChange(g_Song.GetActiveInstr());
	if (r) SCREENUPDATE;
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
	SCREENUPDATE;
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
		SCREENUPDATE;
	}
}

void CRmtView::OnSongTracksorderchange() 
{
	g_Song.Stop();

	g_Song.TracksOrderChange();
	SCREENUPDATE;
}

void CRmtView::OnUpdateSongSongswitch4_8(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText((g_tracks4_8<=4)? "Switch song to Stereo 8 tracks..." : "Switch song to Mono 4 tracks...");
}

void CRmtView::OnSongSongswitch4_8() 
{
	g_Song.Stop();
	g_Song.Songswitch4_8((g_tracks4_8<=4)? 8 : 4);
	SCREENUPDATE;
}

void CRmtView::OnSongSongchangemaximallengthoftracks() 
{
	g_Song.Stop();

	int ma=g_Song.GetEffectiveMaxtracklen();

	CChangeMaxtracklenDlg dlg;
	dlg.m_info.Format("Current value: %i\nComputed effective value for current song: %i",g_Tracks.m_maxtracklen,ma);
	dlg.m_maxtracklen=ma;
	if (dlg.DoModal()==IDOK)
	{
		//Undo
		g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL);
		ma=dlg.m_maxtracklen;
		g_Song.ChangeMaxtracklen(ma);
		SCREENUPDATE;
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
	SCREENUPDATE;
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
	SCREENUPDATE;
}

void CRmtView::OnSongSizeoptimization() 
{
	//ALL size optimalizations
	g_Song.Stop();	//stop music

	int r=MessageBox("Are you sure you want to delete all tracks and instruments unused in song,\ntruncate unused tracks and rebuild wise tracks loops,\ndelete all duplicated tracks, renumber all tracks and instruments\nand change maximal tracks length to effective computed value?","All size optimizations",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
	g_Undo.ChangeInstrument(0,0,UETYPE_INSTRSALL,-1);
	g_Undo.ChangeSong(0,0,UETYPE_SONGDATA,1);

	int chmaxtl=0;
	//first unpack all existing loops
	int tracksmodified=0,loopsexpanded=0;
	g_Song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	//find the effective length of maxtracklen and shorten it if necessary
	int maxtracklen = g_Tracks.m_maxtracklen;
	int effemaxtracklen = g_Song.GetEffectiveMaxtracklen();
	if (effemaxtracklen<maxtracklen)
	{
		g_Song.ChangeMaxtracklen(effemaxtracklen);
		chmaxtl=1;
	}
	//now it will back up
	int optitracks=0,optibeats=0;
	g_Song.TracksAllBuildLoops(optitracks,optibeats);
	//and until the end, don't use the tracks and their parts
	int clearedtracks=0,truncatedtracks=0,truncatedbeats=0;
	g_Song.SongClearUnusedTracksAndParts(clearedtracks,truncatedtracks,truncatedbeats);

	//and only now (after clearing unused tracks) it will remove unused instruments
	int clearedinstruments;
	clearedinstruments = g_Song.ClearAllInstrumentsUnusedInAnyTrack();

	//and now it eliminates double tracks and corrects their occurrences in the song
	//(may have been created by previous edits)
	int duplicatedtracks;
	duplicatedtracks = g_Song.SongClearDuplicatedTracks();

	//now refines the tracks (to remove any gaps)
	g_Song.RenumberAllTracks(1);

	//and now refines the instruments (to remove any gaps)
	g_Song.RenumberAllInstruments(1);

	CString s;
	s.Format("Deleted %i unused tracks, %i unused instruments,\ntruncated %i tracks (%i beats/lines),\nfound and rebuilt loops in %i tracks (%i beats/lines),\ndeleted %i duplicated tracks.",clearedtracks,clearedinstruments,truncatedtracks,truncatedbeats,optitracks,optibeats,duplicatedtracks);
	if (chmaxtl)
	{
		CString s2;
		s2.Format("\nMaximal length of tracks changed to %u.",effemaxtracklen);
		s+=s2;
	}
	MessageBox((LPCTSTR)s,"All size optimizations",MB_OK);
	SCREENUPDATE;
}

void CRmtView::OnTrackRenumberalltracks() 
{
	g_Song.Stop();

	CRenumberTracksDlg dlg;
	if (dlg.DoModal()==IDOK)
	{
		//hide the tracks and song
		g_Undo.ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
		g_Undo.ChangeSong(0,0,UETYPE_SONGDATA,1);
		g_Song.RenumberAllTracks( dlg.m_radio );	//type=1...order by songcolumns, type=2...order by songlines
		SCREENUPDATE;
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
		SCREENUPDATE;
	}
}

void CRmtView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	g_shiftkey = g_controlkey = 0;
	g_focus=1;	//main window focus
}

void CRmtView::OnKillFocus(CWnd* pNewWnd) 
{

	CView::OnKillFocus(pNewWnd);
	g_focus=0;	//the main window has no focus
}

BOOL CRmtView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	CRect rec;
	::GetWindowRect(g_viewhwnd,&rec);
	CPoint np(pt-rec.TopLeft());
	MouseAction(np,0,zDelta);
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CRmtView::OnUndoUndo() 
{
	if (g_Song.Undo()) SCREENUPDATE;
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
	if (g_Song.Redo()) SCREENUPDATE;
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

void CRmtView::OnWantExit() //called from the menu File/Exit ID_WANTEXIT instead of the original ID_APP_EXIT
{
	if ( g_Song.WarnUnsavedChanges() )
	{
		return; //there is no exit
	}
	g_Song.Stop();
	g_closeapplication=1;
	AfxGetApp()->GetMainWnd()->PostMessage(WM_CLOSE,0,0);
}

void CRmtView::OnTrackCursorgotothespeedcolumn() 
{
	if (g_Song.CursorToSpeedColumn()) SCREENUPDATE;
}

void CRmtView::OnUpdateTrackCursorgotothespeedcolumn(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	g_activepart==PARTTRACKS && g_Song.SongGetActiveTrack()>=0 );
}
