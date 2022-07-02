// RmtView.cpp : implementation of the CRmtView class
//

#include "stdafx.h"
#include "Rmt.h"

#include "RmtDoc.h"
#include "RmtView.h"

#include "MainFrm.h"			//!

#include "ConfigDlg.h"
#include "FileNewDlg.h"

#include "r_music.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL g_closeapplication;

#define SCREENUPDATE	g_screenupdate=1
void UpdateShiftControlKeys();
void SetLockKeys(int vkey, BOOL onoff);
BOOL GetCapsLock();
void SetCapsLock(BOOL onoff);
BOOL GetScrollLock();
void SetScrollLock(BOOL onoff);

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
	ON_COMMAND_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE5, OnFileOpenRecent)
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
	// TODO: add construction code here

	//
	//
	g_screena=0;
	g_screenwait=2;		//2 padesatiny
	//
	m_setnumlockfunction=1; //funguje prepinani numlocku

	m_width = 0;
	m_height = 0;
}

CRmtView::~CRmtView()
{
	g_mem_dc->SelectObject(m_penorig);		//vraceni puvodniho Penu
	delete m_pen1;							//uvolneni newnuteho m_pen1
}

void CRmtView::OnDestroy() 
{
	//zrusi Pokey DLL
	m_song.DeInitPokey();

	//zrusi 6502 DLL
	Atari6502_DeInit();

	//vypne timer
	if (m_timeranalyzer) 
	{
		KillTimer(m_timeranalyzer);
		m_timeranalyzer=0;
	}

	//vypne capslock
	//SetCapsLock(0);

	CView::OnDestroy();
}

void CRmtView::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent == m_timeranalyzer)
	{
		DrawAnalyzer();
		DrawPlaytimecounter();
	}
	else
		CView::OnTimer(nIDEvent);
}

BOOL CRmtView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRmtView drawing

void CRmtView::OnSize(UINT nType, int cx, int cy)
{
	TRACE("Rect: %d %d\n", cx, cy);
}

void CRmtView::OnDraw(CDC* pDC)
{

	RECT r;
	GetClientRect(&r);
//	TRACE("Rect2: %d..%d %d..%d\n", r.left, r.right, r.top, r.bottom);

	bool resized = Resize(r.right-r.left+1, r.bottom-r.top+1);
	UpdateModule();

	//CRmtDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
/*
	CRect rc;
	GetUpdateRect(&rc);	//, BOOL bErase = FALSE );
	
	if (rc!=r)
	{
		MessageBox("asas");
	}

	r=rc;
*/
	//pDC->SetBoundsRect( NULL, DCB_ENABLE );

	/*
	CString s;
	s.Format("NumLock=%u",g_numlock);
	SetStatusBarText(s);
	*/

	if (resized || (g_screenupdate && g_invalidatebytimer) )
	{
		g_screenupdate=0;
		g_invalidatebytimer=0;
		g_screena = g_screenwait;
//		MessageBeep(-1);
		DrawAll();
	}

	//MessageBeep(-1);
	pDC->BitBlt(0,0,m_width,m_height,&m_mem_dc,0,0,SRCCOPY);

}

BOOL CRmtView::DrawAll()
{
	m_mem_dc.FillSolidRect(0,0,m_width,m_height,RGB(72,72,72));

	m_song.DrawInfo();
	m_song.DrawSong();
	if (g_active_ti==PARTTRACKS)	//v dolni casti je aktivni?
	{
		m_song.DrawTracks();		
	}
	else
		m_song.DrawInstrument();

	m_song.DrawAnalyzer(NULL);
	m_song.DrawPlaytimecounter(NULL);

	return 1;
}

extern int g_rmtinstr[SONGTRACKS];

void CRmtView::DrawAnalyzer()
{
	CDC* pDC = GetDC();
	if (pDC)
	{
		/*
		for(int i=0; i<SONGTRACKS; i++)
		{
			char s[10];
			sprintf(s,"%02X",(BYTE)g_rmtinstr[i]);
			TextXY(s,i*8*8,0);
		}
		pDC->BitBlt(0,0,800,32,g_mem_dc,0,0,SRCCOPY);
		*/

		m_song.DrawAnalyzer(pDC);
		ReleaseDC(pDC);
	}
}

void CRmtView::DrawPlaytimecounter()
{
	CDC* pDC = GetDC();
	if (pDC)
	{
		m_song.DrawPlaytimecounter(pDC);
		ReleaseDC(pDC);
	}
}

BOOL CRmtView::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	//Invalidate(0);	
	return 1;	//CView::OnEraseBkgnd(pDC);
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
	char line[1025];
	char *tmp,*name,*value;
	s.Format("%s%s",g_prgpath,CONFIG_FILENAME);
	ifstream in(s);
	if (!in)
	{
		MessageBox("Can't read the config file\n"+s,"Read config",MB_ICONEXCLAMATION);
		return;
	}
	while(!in.eof())
	{
		in.getline(line,1024);
		tmp=strchr(line,'=');
		if (tmp)
		{
			tmp[0]=0;
			name=line;
			value=tmp+1;
		}
		else
			continue;
		//Jednotlive radky configu

		//genereral
		if (NAME("TRACKLINEHIGHLIGHT")) g_tracklinehighlight = atoi(value);
		else
		if (NAME("TRACKLINEALTNUMBERING")) g_tracklinealtnumbering = atoi(value);
		else
		if (NAME("TRACKCURSORVERTICALRANGE")) g_trackcursorverticalrange = atoi(value);
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
		if (NAME("KEYBOARD_USENUMLOCK")) g_keyboard_usenumlock = atoi(value);
		else
		//midi
		if (NAME("MIDI_IN")) m_midi.SetDevice(value);
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
		//	
	}
	in.close();
}

void CRmtView::WriteConfig()
{
	CString s;
	//char line[1025];
	s.Format("%s%s",g_prgpath,CONFIG_FILENAME);
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

	//general
	ou << "TRACKLINEHIGHLIGHT=" << g_tracklinehighlight << endl;
	ou << "TRACKLINEALTNUMBERING=" << g_tracklinealtnumbering << endl;
	ou << "TRACKCURSORVERTICALRANGE=" << g_trackcursorverticalrange << endl;
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
	ou << "KEYBOARD_USENUMLOCK=" << g_keyboard_usenumlock << endl;
	//midi
	ou << "MIDI_IN=" << m_midi.GetMidiDevName() << endl;
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
	//
	ou.close();
}

void CRmtView::OnViewConfiguration() 
{
	// TODO: Add your command handler code here
	CConfigDlg dlg;
	//general
	dlg.m_tracklinehighlight = g_tracklinehighlight;
	dlg.m_tracklinealtnumbering = g_tracklinealtnumbering;
	dlg.m_trackcursorverticalrange = g_trackcursorverticalrange;
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
	dlg.m_keyboard_usenumlock = g_keyboard_usenumlock;
	//midi
	dlg.m_midi_device = m_midi.GetMidiDevId();
	dlg.m_midi_tr = g_midi_tr;
	dlg.m_midi_volumeoffset = g_midi_volumeoffset;
	dlg.m_midi_noteoff = g_midi_noteoff;
	//
	if (dlg.DoModal()==IDOK)
	{
		//general
		g_tracklinehighlight = dlg.m_tracklinehighlight;
		g_tracklinealtnumbering = dlg.m_tracklinealtnumbering;
		g_trackcursorverticalrange = dlg.m_trackcursorverticalrange;
		g_ntsc = dlg.m_ntsc;
		if (g_nohwsoundbuffer != dlg.m_nohwsoundbuffer)
		{
			//zmenilo se HW/SW pouziti soundubfferu
			g_nohwsoundbuffer = dlg.m_nohwsoundbuffer;
			m_song.GetPokey()->ReInitSound();	//nutno reinicializovat zvuk
		}
		//keyboard
		g_keyboard_layout = dlg.m_keyboard_layout;
		g_keyboard_escresetatarisound = dlg.m_keyboard_escresetatarisound;
		g_keyboard_swapenter = dlg.m_keyboard_swapenter;
		g_keyboard_updowncontinue=dlg.m_keyboard_updowncontinue;
		g_keyboard_rememberoctavesandvolumes = dlg.m_keyboard_rememberoctavesandvolumes;
		g_keyboard_askwhencontrol_s = dlg.m_keyboard_askwhencontrol_s;
		g_keyboard_playautofollow = dlg.m_keyboard_playautofollow;
		g_keyboard_usenumlock = dlg.m_keyboard_usenumlock;
		//midi
		if (dlg.m_midi_device>=0)
		{
			MIDIINCAPS micaps;
			midiInGetDevCaps(dlg.m_midi_device,&micaps, sizeof(MIDIINCAPS));
			m_midi.SetDevice(micaps.szPname);
		}
		else
			m_midi.SetDevice("");
		g_midi_tr = dlg.m_midi_tr;
		g_midi_volumeoffset = dlg.m_midi_volumeoffset;
		g_midi_noteoff = dlg.m_midi_noteoff;

		m_midi.MidiInit();

		//Pal nebo NTSC
		m_song.ChangeTimer((g_ntsc)? 17 : 20);

		//
		WriteConfig();
		//
		g_screenupdate=1;
	}
}

//---

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

void CRmtView::UpdateModule()
{
	g_tracklines = (g_height - (TRACKS_Y+TRACKS_HEADER_H) - STATUS_H) / TRACK_LINE_H;
	if (g_tracklines > m_song.GetTracks()->m_maxtracklen) {
		g_tracklines = m_song.GetTracks()->m_maxtracklen;
	}

	g_song_x     = SONG_X;
	g_songlines = 5;
	if (g_tracks4_8 == 4 && 	g_active_ti==PARTTRACKS) {
		g_songlines = 10;
	} else if (g_width > 1028) {
		g_song_x = 780;
		g_songlines = (g_height - SONG_Y - SONG_HEADER_H - WIN_BORDER_B) / SONG_LINE_H;
	}
}

bool CRmtView::Resize(int width, int height)
{
	if (width == m_width && height == m_height) return false;
	if (m_width != 0) {
		m_mem_dc.SelectObject((CBitmap*)0);
		m_mem_bitmap.DeleteObject();
		m_mem_dc.DeleteDC();
	}
	m_width = width;
	m_height = height;
	if (m_width != 0) {
		CDC *dc = GetDC();
		m_mem_bitmap.CreateCompatibleBitmap(dc,m_width,m_height);
		m_mem_dc.CreateCompatibleDC(dc);
		m_mem_dc.SelectObject(&m_mem_bitmap);
		g_mem_dc = &m_mem_dc;
		g_width = m_width;
		g_height = m_height;

//		UpdateModule();

		m_mem_dc.FillSolidRect(0,0,m_width,m_height,RGB(0,0,0)); //inicializacni cerne pozadi
		ReleaseDC(dc);
	}
	return true;
}

void CRmtView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	//prikazova radka
	CString cmdl = GetCommandLine();
	CString commandlinefilename="";
	g_prgpath = "";
	g_lastloadpath_songs = g_lastloadpath_instruments = g_lastloadpath_tracks = "";
	if (cmdl!="")
	{
		int i1=0,i2=0;
		GetCommandLineItem(cmdl,i2,i1);	//cesta/nazev.exe
		if (i1!=i2)
		{
			CString exefilename=cmdl.Mid(i2,i1-i2);
			int l=exefilename.ReverseFind('/');
			if (l<0) l=exefilename.ReverseFind('\\');
			if (l>=0)
			{
				g_prgpath = exefilename.Left(l+1);	//vcetne lomitka
			}
		}
		i1++;
		GetCommandLineItem(cmdl,i1,i2);	//parametr
		if (i1!=i2)
		{
			commandlinefilename=cmdl.Mid(i1,i2-i1);
		}
	}



	//
	m_song.SetRMTTitle();

	// default size
	Resize(800, 600);
	CDC *dc=GetDC();
//	m_mem_bitmap.CreateCompatibleBitmap(dc,800,600);
//	m_mem_dc.CreateCompatibleDC(dc);
//	m_mem_dc.SelectObject(&m_mem_bitmap);

	if (m_gfx_bitmap == (HBITMAP)0) {
		m_gfx_bitmap.LoadBitmap(MAKEINTRESOURCE(IDB_GFX));
		m_gfx_dc.CreateCompatibleDC(dc);
		m_gfx_dc.SelectObject(&m_gfx_bitmap);
	}

	g_gfx_dc = &m_gfx_dc;
	g_hwnd = AfxGetApp()->GetMainWnd()->m_hWnd;
	g_viewhwnd = this->m_hWnd;

	m_pen1 = new CPen(PS_SOLID,1,RGB(144,144,144));
	m_penorig = g_mem_dc->SelectObject(m_pen1);

	m_mem_dc.FillSolidRect(0,0,m_width,m_height,RGB(0,0,0)); //inicializacni cerne pozadi

	ReleaseDC(dc);

	//kurzory
	m_cursororig = LoadCursor(NULL,IDC_ARROW);
	m_cursorchanonoff = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORCHANNELONOFF));
	m_cursorenvvolume = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORENVVOLUME));
	m_cursorgoto = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORGOTO));
	m_cursordlg = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORDLG));
	m_cursorsetpos = LoadCursor(AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDC_CURSORSETPOS));

	//klavesnice
	g_shiftkey=g_controlkey=0;
	g_numlock=0;
	
	g_capslock_other=GetCapsLock();
	g_capslock_rmt=0;
	SetCapsLock(0);

	/*
	g_scrolllock_other=GetScrollLock();
	g_scrolllock_rmt=0;
	SetScrollLock(0);
	*/

	//aktualni casti
	g_activepart = PARTTRACKS;		//tracks
	g_active_ti = PARTTRACKS;	//v dolni casti aktivni tracks

	//vsechny kanaly zapnute
	SetChannelOnOff(-1,1);		//vsechny kanaly zapnout

	//KONFIGURACE
	ReadConfig();

	//view prvky
	ChangeViewElements(0); //bez write!

	// PRVOTNI INICIALIZACE POKEYE (NACTENI DLL)
	if (!m_song.InitPokey())
	{
		m_song.DeInitPokey();
		exit(1);	//hlasku vypsal uz InitPokey
	}

	// PRVOTNI INICIALIZACE 6502 (NACTENI DLL)
	if (!Atari6502_Init())
	{
		Atari6502_DeInit();
		exit(1);
	}

	// PRVOTNI NACTENI A INICIALIZACE ATARI RMT RUTINY
	Memory_Clear();
	if (!Atari_LoadRMTRoutines())
	{
		//MessageBox("Can't open Atari RMT ATARI system routines.","Error",MB_ICONERROR);
		MessageBox("Fatal error with RMT ATARI system routines.","Error",MB_ICONERROR);
		exit(1);
	}
	Atari_InitRMTRoutine();


	g_screenupdate=1;	//prvni vykresleni

	m_timeranalyzer=0;
	m_timeranalyzer = SetTimer(1,20,NULL);

	//Zobrazi ABOUT dialog kdyz neni Pokey nebo 6502
	if (!m_song.GetPokey()->GetRenderSound() || !g_is6502)
	{
		AfxGetApp()->GetMainWnd()->PostMessage(WM_COMMAND,ID_APP_ABOUT,0);
	}

	//Inicializuje MIDI
	m_midi.MidiInit();
	m_midi.MidiOn();

	//Pal nebo NTSC
	m_song.ChangeTimer((g_ntsc)? 17 : 20);

	//Jestli byl tracker spusten s parametrem, pak nacte prislusny soubor
	if (commandlinefilename)
	{
		m_song.FileOpen((LPCTSTR)commandlinefilename);
	}

	//PostMessage(WM_COMMAND,ID_FILE_NEW); //aby se pak vyvolal new dialog
}

int CRmtView::MouseAction(CPoint point,UINT mousebutt,short wheelzDelta=0)
{
	int i;
	int px,py;

	CRect rec(g_song_x+SONG_LEFT_W,SONG_Y+SONG_HEADER_H,g_song_x+SONG_LEFT_W+g_tracks4_8*3*8-8,SONG_Y+SONG_HEADER_H+g_songlines*SONG_LINE_H);
	if (rec.PtInRect(point))
	{
		//Song
		SetCursor(m_cursorgoto);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.SongCursorGoto(CPoint(point.x-(g_song_x+SONG_LEFT_W),point.y-(SONG_Y+SONG_HEADER_H)));
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta!=0)
		{
			BOOL r=0;
			if (wheelzDelta>0) r=m_song.SongKey(VK_UP,0,0);
			else
			if (wheelzDelta<0) r=m_song.SongKey(VK_DOWN,0,0);
			if (r) SCREENUPDATE;
		}
		return 5;
	}

	// Header
	rec.SetRect(g_song_x+SONG_LEFT_W,SONG_Y,g_song_x+SONG_LEFT_W+g_tracks4_8*3*8-8,SONG_Y+SONG_HEADER_H);
	if (rec.PtInRect(point))
	{
		//nad Songem L1-R4 pro channel on/off/solo/inverze
		i = (point.x + 4 - (g_song_x+SONG_LEFT_W)) / (8*3);
		if (i<0) i=0;
		else
		{	if (i>=g_tracks4_8) i=g_tracks4_8-1; }
		px = i;
		//
		SetCursor(m_cursorchanonoff);
		//
		if (mousebutt & MK_LBUTTON)
		{
			SetChannelOnOff(px,-1);	//inverze
			SCREENUPDATE;
		}
		if (mousebutt & MK_RBUTTON)
		{
			SetChannelSolo(px);		//solo/mute on off
			SCREENUPDATE;
		}
		return 1;
	}

	rec.SetRect(INFO_X,INFO_Y,INFO_X+SONGNAMEMAXLEN*8,INFO_Y+16);
	if (rec.PtInRect(point))
	{
		//Song name
		SetCursor(m_cursorgoto);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.InfoCursorGotoSongname(point.x-INFO_X);
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(INFO_X+13*8,INFO_Y+16,INFO_X+13*8+7*8,INFO_Y+16*2);
	if (rec.PtInRect(point))
	{
		//Song speed
		SetCursor(m_cursorgoto);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.InfoCursorGotoSpeed(point.x-(INFO_X+13*8));
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(INFO_X+47*8,INFO_Y+3*16,INFO_X+47*8+10*8,INFO_Y+3*16+16);
	if (rec.PtInRect(point))
	{
		//Octave Select Dialog
		SetCursor(m_cursordlg);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.InfoCursorGotoOctaveSelect(point.x,point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta!=0)
		{
			BOOL r=0;
			if (wheelzDelta>0) r=m_song.OctaveUp();
			else
			if (wheelzDelta<0) r=m_song.OctaveDown();
			if (r) SCREENUPDATE;
		}
		return 6;
	}

	rec.SetRect(INFO_X+49*8,INFO_Y+4*16,INFO_X+49*8+9*8,INFO_Y+4*16+16);
	if (rec.PtInRect(point))
	{
		//Volume Select Dialog
		SetCursor(m_cursordlg);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.InfoCursorGotoVolumeSelect(point.x,point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta!=0)
		{
			BOOL r=0;
			if (wheelzDelta>0) r=m_song.VolumeUp();
			else
			if (wheelzDelta<0) r=m_song.VolumeDown();
			if (r) SCREENUPDATE;
		}
		return 6;
	}


	rec.SetRect(INFO_X,INFO_Y+3*16,INFO_X+4*8+INSTRNAMEMAXLEN*8,INFO_Y+3*16+16);
	if (rec.PtInRect(point))
	{
		//Instrument Select Dialog
Instrument_Select_Dialog:
		SetCursor(m_cursordlg);
		//
		if (mousebutt & MK_LBUTTON)
		{
			BOOL r=m_song.InfoCursorGotoInstrumentSelect(point.x,point.y);
			if (r) SCREENUPDATE;
		}
		if (wheelzDelta!=0)
		{
			if (wheelzDelta>0) m_song.ActiveInstrPrev();
			else
			if (wheelzDelta<0) m_song.ActiveInstrNext();
			SCREENUPDATE;
		}
		return 6;
	}

	//DOLNI CASTI

	if (g_active_ti==PARTTRACKS)
	{
		//TrackEdit                   (-12 pridano smerem nahoru)
		rec.SetRect(TRACKS_X+5*8,TRACKS_Y-12,TRACKS_X+5*8+g_tracks4_8*8*11,TRACKS_Y+32);
		if (rec.PtInRect(point))
		{
			i = (point.x - (TRACKS_X+5*8)) / (8*11);
			if (i<0) i=0;
			else
			if (i>=g_tracks4_8) i=g_tracks4_8-1;
			px = i;
			//
			SetCursor(m_cursorchanonoff);
			//
			if (mousebutt & MK_LBUTTON)
			{
				SetChannelOnOff(px,-1);	//inverze
				SCREENUPDATE;
			}
			if (mousebutt & MK_RBUTTON)
			{
				SetChannelSolo(px);		//solo/mute on off
				SCREENUPDATE;
			}
			return 1;
		}

		// Track click
		rec.SetRect(TRACKS_X+6*8,TRACKS_Y+TRACKS_HEADER_H,TRACKS_X+3*8+g_tracks4_8*8*11,TRACKS_Y+TRACKS_HEADER_H+g_tracklines*TRACK_LINE_H);
		if (rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				BOOL r=m_song.TrackCursorGoto(CPoint(point.x-(TRACKS_X+6*8),point.y-(TRACKS_Y+TRACKS_HEADER_H)));
				if (r) SCREENUPDATE;
			}
			if (wheelzDelta!=0)
			{
				BOOL r=0;
				if (wheelzDelta>0) r=m_song.TrackKey(VK_UP,0,0);
				else
				if (wheelzDelta<0) r=m_song.TrackKey(VK_DOWN,0,0);
				if (r) SCREENUPDATE;
			}
			return 4;
		}
	}
	else
	if (g_active_ti==PARTINSTRS)
	{
		//InstrEdit

		int ainstr = m_song.GetActiveInstr();
		//VOLUME LEFT (dolni)
		int r= m_song.GetInstruments()->GetInstrArea(ainstr,0,rec);
		if (r && rec.PtInRect(point))
		{
			px = (point.x - rec.left) /8;
			py = 15- ((point.y - rec.top) /4);
			//return 2;
			SetCursor(m_cursorenvvolume);
			//
			if (g_mousebutt & MK_LBUTTON) //porovnava g_mousebutt, aby to fungovalo i pri move
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				m_song.GetInstruments()->SetEnvVolume(ainstr,0,px,py);
				SCREENUPDATE;
			}
			if (g_mousebutt & MK_RBUTTON) //porovnava g_mousebutt, aby to fungovalo i pri move
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				m_song.GetInstruments()->SetEnvVolume(ainstr,0,px,0);
				SCREENUPDATE;
			}
			return 2;
		}

		//VOLUME RIGHT (horni)
		r= m_song.GetInstruments()->GetInstrArea(ainstr,1,rec);
		if (r && rec.PtInRect(point))
		{
			px = (point.x - rec.left) /8;
			py = 15- ((point.y - rec.top) /4);
			//return 3;
			SetCursor(m_cursorenvvolume);
			//
			if (g_mousebutt & MK_LBUTTON) //porovnava g_mousebutt, aby to fungovalo i pri move
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				m_song.GetInstruments()->SetEnvVolume(ainstr,1,px,py);
				SCREENUPDATE;
			}
			if (g_mousebutt & MK_RBUTTON) //porovnava g_mousebutt, aby to fungovalo i pri move
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				m_song.GetInstruments()->SetEnvVolume(ainstr,1,px,0);
				SCREENUPDATE;
			}
			return 3;
		}


		//ENVELOPE PARAMETERS velka tabulka
		r= m_song.GetInstruments()->GetInstrArea(ainstr,2,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),0);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//ENVELOPE PARAMETERS rada cisel pro hlasitost praveho kanalu
		r= m_song.GetInstruments()->GetInstrArea(ainstr,3,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),1);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT TABLE
		r= m_song.GetInstruments()->GetInstrArea(ainstr,4,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),2);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT NAME
		r= m_song.GetInstruments()->GetInstrArea(ainstr,5,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),3);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT PARAMETERS
		r= m_song.GetInstruments()->GetInstrArea(ainstr,6,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorgoto);
			//
			if (mousebutt & MK_LBUTTON)
			{
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),4);
				if (r) SCREENUPDATE;
			}
			return 3;
		}

		//INSTRUMENT SELECT DIALOG
		r= m_song.GetInstruments()->GetInstrArea(ainstr,7,rec);
		if (r && rec.PtInRect(point))
		{
			point.y-=(4*16+8);
			point.x+=64;
			goto Instrument_Select_Dialog;
		}

		//ENVELOPE LEN a GO PARAMETER - delka a smycka pomoci mysi
		r= m_song.GetInstruments()->GetInstrArea(ainstr,8,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorsetpos);
			//
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),5);
			}
			if (mousebutt & MK_RBUTTON)
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),6);
			}
			if (r) SCREENUPDATE;
			return 7;
		}

		//TABLE LEN a GO PARAMETER - delka a smycka pomoci mysi
		r= m_song.GetInstruments()->GetInstrArea(ainstr,9,rec);
		if (r && rec.PtInRect(point))
		{
			SetCursor(m_cursorsetpos);
			//
			r=0;
			if (mousebutt & MK_LBUTTON)
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),7);
			}
			if (mousebutt & MK_RBUTTON)
			{
				m_song.GetUndo()->ChangeInstrument(ainstr,0,UETYPE_INSTRDATA);
				r=m_song.GetInstruments()->CursorGoto(ainstr,CPoint(point.x-rec.left,point.y-rec.top),8);
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
	m_song.GetUndo()->Separator();
	g_mousebutt|=MK_LBUTTON;
	MouseAction(point,MK_LBUTTON);
	CView::OnLButtonDown(nFlags, point);
}

void CRmtView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_song.GetUndo()->Separator();
	g_mousebutt&=~MK_LBUTTON;
	CView::OnLButtonUp(nFlags, point);
}

void CRmtView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	m_song.GetUndo()->Separator();
	OnLButtonDown(nFlags, point);
	OnLButtonUp(nFlags, point);
}

void CRmtView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	m_song.GetUndo()->Separator();
	g_mousebutt|=MK_RBUTTON;
	MouseAction(point,MK_RBUTTON);
	CView::OnRButtonDown(nFlags, point);
}

void CRmtView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	m_song.GetUndo()->Separator();
	g_mousebutt&=~MK_RBUTTON;
	CView::OnRButtonUp(nFlags, point);
}

void CRmtView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	m_song.GetUndo()->Separator();
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
	// TODO: Add your message handler code here and/or call default
	return 1;
	//return CView::OnSetCursor(pWnd, nHitTest, message);
}

/*
void CRmtView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CView::OnSysKeyDown(nChar,nRepCnt,nFlags);
}
*/

void CRmtView::OnSysChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	/*
	char s[128];
	sprintf(s,"nChar=%u nRepCnt=%u nFlags=%u",nChar,nRepCnt,nFlags);
	SetStatusBarText(s);
	Sleep(500);
	*/
	//8,1,8206
	if ( nFlags&0x2000 ) //ALT ?
	{
		//ALT+
		if (nChar==8) //8 = VK_BACKSPACE:
		{
			//MessageBox("aaa","bbb");
			if (g_shiftkey)
			{
				if (m_song.Redo()) //Shift+Alt+Backspace (nechova se idealne - zustava viset Shift)
				{
					SCREENUPDATE;
					return;
				}
			}
			else
			{
				if (m_song.Undo()) //Alt+Backspace
				{
					SCREENUPDATE;
					return;
				}
			}
			//pokud uz Undo nejde, pusti ho dal aby udelal zvuk
		}
		/*
		else
		if (nChar==XXX)
		{
			if (m_song.Redo()) SCREENUPDATE;
			return;
		}
		*/
	}
	CView::OnSysChar( nChar, nRepCnt, nFlags );
}


const int  NChaCode[]={  36,  38,  33, VK_SUBTRACT,  37,  12,  39, VK_ADD,  35,  40,  34,  45};
const char FlaToCha[]={0x67,0x68,0x69,109,0x64,0x65,0x66,107,0x61,0x62,0x63,0x60};

const char layout2[]={VK_F5,VK_F6,VK_F7,VK_F8, VK_F3,VK_F2,VK_F4,VK_ESCAPE};

void CRmtView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{

	/*
	char s[128];
	sprintf(s,"nChar=%u nRepCnt=%u nFlags=%u",nChar,nRepCnt,nFlags);
	SetStatusBarText(s);

	Sleep(500);
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
	return;
	//*/


	UINT vk = nChar;

//	g_shiftkey = (GetAsyncKeyState(VK_SHIFT)!=NULL);
//	g_controlkey = (GetAsyncKeyState(VK_CONTROL)!=NULL);

	if (g_keyboard_layout==1 && vk>=VK_F1 && vk<=VK_F8)
	{
		//Layout 2
		if (vk==VK_F7 && g_controlkey)
		{
			m_song.Play(MPLAY_TRACK,g_shiftkey ^ g_keyboard_playautofollow,1);
			goto KeyDownFinish;
		}
		vk = layout2[vk-VK_F1];
	}


	//*
	{
	 UINT nfb = nFlags & 0x1ff;	//pri autorepeatu se nastavuje v nFlags bit 16384
	 if (nfb>=71 && nfb<=82) //shift + numblock 0-9 + -
	 {
		if ((int)nChar == NChaCode[nfb-71])
		{
			vk=FlaToCha[nfb-71];
//			g_shiftkey=1;	//pridan test pri pousteni shiftu, jestli je to skutecne shift
		}
	 }
	}
	/*/
		char s[128];
		//sprintf(s,"shift=%i control=%i vkey=%i nFlags=%i    ",GetKeyState(VK_LSHIFT),GetKeyState(VK_LCONTROL),vk,nFlags);
		sprintf(s,"shift=%i control=%i vkey=%i nFlags=%i    ",g_shiftkey,g_controlkey,vk,nFlags);
		SetStatusBarText(s);
		//MessageBox(s);
		//TextXY(s,10,10);
		//SCREENUPDATE;
	//	return;
	// */

	switch(vk)
	{
	case VK_ESCAPE:
		m_song.Stop();					//utisi vsechno
		if (g_prove)					//je-li zapnuto, vypne PROVE MODE
		{
			g_prove=0;
			SCREENUPDATE;
		}
		if (g_keyboard_escresetatarisound)
		{
			Atari_InitRMTRoutine(); //reinit RMT rutiny
		}
		break;

		//
	case VK_F1:
		/*
		//DEBUG:
		m_song.GetPokey()->Test();
		break;
		//konec
		*/
		if (g_controlkey)
			m_song.SetBookmark();
		else
			m_song.Play(MPLAY_BOOKMARK,g_shiftkey ^ g_keyboard_playautofollow);		//od bookmarku  (+shift => followplay)	
		/*
		{
			CKeyboardLayoutDlg dlg;
			dlg.DoModal();
		}
		//*/
		// Atari_InstrumentTurnOff(m_song.GetActiveInstr());
		//if (m_timeranalyzer) {	KillTimer(m_timeranalyzer);	m_timeranalyzer = 0; }
		break;
	case VK_MEDIA_STOP:
		m_song.Stop();
		break;

	case VK_MEDIA_PLAY_PAUSE:
	case VK_F2:
		m_song.Play(MPLAY_SONG,g_shiftkey ^ g_keyboard_playautofollow);		//cely song od 0  (+shift => followplay)	
		break;

	case VK_F3:
		m_song.Play(MPLAY_FROM,g_shiftkey ^ g_keyboard_playautofollow);		//od aktualniho mista (+shift => followplay)	
		break;

	case VK_F4:
		if (g_controlkey)
			m_song.Play(MPLAY_BLOCK,g_shiftkey ^ g_keyboard_playautofollow);		//aktualni blok porad dokola (+shift => followplay)
		else
			m_song.Play(MPLAY_TRACK,g_shiftkey ^ g_keyboard_playautofollow);		//aktualni tracky porad dokola (+shift => followplay)	
		break;

		//
	case VK_F5:
		m_song.GetUndo()->Separator();
		OnEmTracks();
		break;

	case VK_F6:
		m_song.GetUndo()->Separator();
		OnEmInstruments();
		break;

	case VK_F7:
		m_song.GetUndo()->Separator();
		OnEmInfo();
		break;

	case VK_F8:
		m_song.GetUndo()->Separator();
		OnEmSong();
		break;

		//

	case VK_F9:							//prove mode
		m_song.GetUndo()->Separator();
		OnProvemode();
		break;

	case VK_F11:						//respect volume
		m_song.GetUndo()->Separator();
		g_respectvolume ^=1;
		SCREENUPDATE;
		break;

		//

	case VK_F12:
		m_song.GetUndo()->Separator();
		if (g_activepart != g_active_ti)
			g_activepart = g_active_ti;
		else
		{
			if (g_activepart==PARTTRACKS)
				g_activepart=g_active_ti=PARTINSTRS;
			else
				g_activepart=g_active_ti=PARTTRACKS;	
		}
		SCREENUPDATE;
		break;

	case VK_SHIFT:
		g_shiftkey = 1;
		goto KeyDownNoUndoCheckPoint;
		break;

	case VK_CONTROL:
		g_controlkey = 1;
		goto KeyDownNoUndoCheckPoint;
		break;

	case VK_NUMLOCK:
		if (g_keyboard_usenumlock) {
			//g_numlock=GetNumLock();
			if ( nFlags !=0 )
			{
				SetNumLock(0);				//Vypina NumLock
				if ( (nFlags & 0x1ff) == 0) break;	//posun o +-1 se dela jen pri RUCNIM stlaceni NumLocku
				int c = g_linesafter;
				if (g_shiftkey)		//SHIFT+NUMLOCK
				{
					c--;
					if (c<0) c=8;
				}
				else 
				{					//NUMLOCK
					c++;
					if (c>8) c=0;
				}
				CMainFrame *mf = ((CMainFrame*)AfxGetMainWnd());
				if (mf)
				{
					g_linesafter = c;	//pokud by se nepodarilo MainFrame, tak ani nemeni g_linesafter
					mf->m_c_linesafter.SetCurSel(g_linesafter);
				}
			}
		}
		break;

	case VK_SCROLL:	//ScrollLock key
		OnPlayfollow();
		break;

	case VK_CANCEL: //Control+Pause = VK_BREAK
		break; //nic

	case VK_PAUSE:
		if (g_shiftkey) m_song.GetPokey()->ReInitSound(); //SHIFT+PAUSE ..celkovy reinit zvuku
		Atari_InitRMTRoutine(); //reinit RMT rutiny
		if (m_song.GetPlayMode()==0) //pouze je-li modul zastaven
		{
			g_playtime=0;
			DrawPlaytimecounter();
		}
		//MessageBeep(-1);
		break;

	case VK_DIVIDE:
		m_song.TrackLeft(1);
		SCREENUPDATE;
		break;
	
	case VK_MULTIPLY:
		m_song.TrackRight(1);
		SCREENUPDATE;
		break;

	case 192:	//VK_BACKQUOTE ( ` opacny apostrof)
		if (g_shiftkey)
		{
			SetChannelOnOff(-1,-1);	//inverze soucasneho stavu vyp/zap		
		}
		else
		if (g_controlkey)
		{
			int ch = m_song.GetActiveColumn(); //aktualni channel
			SetChannelOnOff(ch,-1);			//jeho inverze soucasneho stavu vyp/zap
		}
		else
		{
			int ch = m_song.GetActiveColumn(); //aktualni channel
			SetChannelSolo(ch);
		}
		SCREENUPDATE;
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
			SetChannelOnOff(vk-49,-1);		//invertuje stav kanalu 1-8 (=>zap/vyp)
			SCREENUPDATE;
		}
		else
			goto AllModesDefaultKey;
		break;

	case 57:	//VK_9
		if (g_controlkey && !g_shiftkey)	//CONTROL + 9
		{
			SetChannelOnOff(-1,1);		//vsechny zapnout
			SCREENUPDATE;
		}
		else
			goto AllModesDefaultKey;
		break;

	case 48:	//VK_0
		if (g_controlkey && !g_shiftkey)	//CONTROL + 0
		{
			SetChannelOnOff(-1,0);		//vsechny vypnout
			SCREENUPDATE;
		}
		else
			goto AllModesDefaultKey;
		break;

	case 76:	//VK_L
		if (g_controlkey && !g_shiftkey) //CONTROL+L
		{
			SetStatusBarText("Load...");
			OnFileOpen();
			SetStatusBarText("");
		}
		else
			goto AllModesDefaultKey;
		break;


	case 83:	//VK_S
		if (g_controlkey && !g_shiftkey) //CONTROL+S
		{
			m_song.Stop();
			SetStatusBarText("Save...");
			CString filename=m_song.GetFilename();
			if (g_keyboard_askwhencontrol_s
				&& (filename!="" || m_song.GetFiletype()!=0))
			{
				//ma-li se ptat a bude-li se provadet prepis uz existujiciho
				//(=> nebude "Save as..." dialog)
				CString s;
				s.Format("Save song to file '%s'?",filename);
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

	default:
AllModesDefaultKey:
		int r=0;

		if (g_prove)
		{
			//testovani hry na klavesnici
			r=m_song.ProveKey(vk,g_shiftkey,g_controlkey);
		}
		else
		{
			short capslock = GetKeyState(20);	//VK_CAPS_LOCK
			if (g_shiftkey 
				&& (NoteKey(vk)>=0 || Numblock09Key(vk)>=0 || vk==VK_SPACE)
				&& (!capslock || (g_activepart!=PARTINFO && g_active_ti!=PARTINSTRS))
			   )
			{
				r = m_song.ProveKey(vk,g_shiftkey,g_controlkey);
			}
			else
			{
				switch (g_activepart)
				{
				case PARTINFO:
					r=m_song.InfoKey(vk,g_shiftkey,g_controlkey);
					break;
				case PARTTRACKS:
					r=m_song.TrackKey(vk,g_shiftkey,g_controlkey);
					break;
				case PARTINSTRS:
					r=m_song.InstrKey(vk,g_shiftkey,g_controlkey);
					break;
				case PARTSONG:
					r=m_song.SongKey(vk,g_shiftkey,g_controlkey);
					break;
				}
			}
		}
		if (r)
			SCREENUPDATE;
//		else
//			CView::OnKeyDown(nChar, nRepCnt, nFlags);
	}


	//UndoCheckPoint
	//nedela se je-li to jen shift nebo jen control
	// (ke kteremu teprve prijde klavesa priste)
	//m_song.UndoCheckPoint();

KeyDownNoUndoCheckPoint:
KeyDownFinish:

//	TRACE("Key:%d\n", vk);
	CView::OnKeyDown(nChar, nRepCnt, nFlags);

//	g_shiftkey = (GetAsyncKeyState(VK_SHIFT)!=NULL);
//	g_controlkey = (GetAsyncKeyState(VK_CONTROL)!=NULL);
//	UpdateShiftControlKeys();

/*
	CString s;
	s.Format("NumLock=%u",GetNumLock());
	SetStatusBarText(s);
*/
}

void CRmtView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{

//	g_shiftkey = (GetAsyncKeyState(VK_SHIFT)!=NULL);
//	g_controlkey = (GetAsyncKeyState(VK_CONTROL)!=NULL);

//*
	if (nChar==VK_SHIFT)
	{
		//if ( (nFlags & (4+8+16)) != 4+16) g_shiftkey=0; //porovnani kvuli autogenerovane zprave o pusteni shiftu pri shift+numblock keys (+/-)
		g_shiftkey=0;
	}
	else
	if (nChar==VK_CONTROL) 
	{
		g_controlkey=0;
	}
//	*/

	//	else
	CView::OnKeyUp(nChar, nRepCnt, nFlags);

/*
	CString s;
	s.Format("NumLock=%u",GetNumLock());
	SetStatusBarText(s);
*/
}


void CRmtView::OnFileOpen() 
{
	m_song.FileOpen();
}

extern CRmtApp theApp;

void CRmtView::OnFileOpenRecent(UINT i) 
{	
	int nIndex = i - ID_FILE_MRU_FILE1;
	CString& s = theApp.GetRecentFile(nIndex);
	m_song.FileOpen(s);
}

void CRmtView::OnFileReload() 
{
	m_song.FileReload();
}

void CRmtView::OnUpdateFileReload(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_song.FileCanBeReloaded());
}

void CRmtView::OnFileSave()
{
	m_song.FileSave();
}

void CRmtView::OnFileSaveAs() 
{
	m_song.FileSaveAs();
}

void CRmtView::OnFileNew() 
{
	m_song.FileNew();
	SCREENUPDATE;
}

void CRmtView::OnFileImport() 
{
	m_song.FileImport();
	SCREENUPDATE;
}

void CRmtView::OnFileExportAs() 
{
	m_song.FileExportAs();
}

void CRmtView::OnInstrLoad() 
{
	m_song.FileInstrumentLoad();
	SCREENUPDATE;
}

void CRmtView::OnInstrSave() 
{
	m_song.FileInstrumentSave();	
}

void CRmtView::OnInstrCopy() 
{
	m_song.InstrCopy();	
}

void CRmtView::OnInstrPaste() 
{
	m_song.InstrPaste();
	SCREENUPDATE;
}

void CRmtView::OnInstrCut() 
{
	m_song.GetUndo()->ChangeInstrument(m_song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	m_song.InstrCut();
	SCREENUPDATE;
}

void CRmtView::OnInstrDelete() 
{
	m_song.GetUndo()->ChangeInstrument(m_song.GetActiveInstr(),0,UETYPE_INSTRDATA,1);
	m_song.InstrDelete();
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeLRenvelopesonly() 
{
	m_song.InstrPaste(1); //L/R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeRenvelopeonly() 
{
	m_song.InstrPaste(2); //R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeLenvelopeonly() 
{
	m_song.InstrPaste(3); //L
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialEnvelopeparametersonly() 
{
	m_song.InstrPaste(4); //ENVELOPE PARS
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialTableonly() 
{
	m_song.InstrPaste(5); //TABLE
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeenvandenvelopeparsonly() 
{
	m_song.InstrPaste(6); //VOL+ENV
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialInsertvolenvsandenvparstocurpos() 
{
	m_song.InstrPaste(7); //VOL+ENV TO CURPOS
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumeltorenvelopeonly() 
{
	m_song.InstrPaste(8); //volume L to R
	SCREENUPDATE;
}

void CRmtView::OnInstrumentPastespecialVolumertolenvelopeonly() 
{
	m_song.InstrPaste(9); //volume R to L
	SCREENUPDATE;
}

//update
void CRmtView::OnUpdateInstrumentPastespecialInsertvolenvsandenvparstocurpos(CCmdUI* pCmdUI) 
{
	//to curpos
	int i = m_song.GetActiveInstr();
	TInstrument* ai = &m_song.GetInstruments()->m_instr[i];
	pCmdUI->Enable(g_activepart==PARTINSTRS && (ai->act==2)); //kdyz je na editaci envelope
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
	m_song.TrackCopy();
}

void CRmtView::OnTrackPaste() 
{
	m_song.GetUndo()->ChangeTrack(m_song.SongGetActiveTrack(),m_song.GetActiveLine(),UETYPE_TRACKDATA);
	m_song.TrackPaste();
	SCREENUPDATE;
}

void CRmtView::OnTrackCut() 
{
	m_song.GetUndo()->ChangeTrack(m_song.SongGetActiveTrack(),m_song.GetActiveLine(),UETYPE_TRACKDATA);
	m_song.TrackCut();
	SCREENUPDATE;
}

void CRmtView::OnTrackDelete() 
{
	m_song.GetUndo()->ChangeTrack(m_song.SongGetActiveTrack(),m_song.GetActiveLine(),UETYPE_TRACKDATA);
	m_song.TrackDelete();
	SCREENUPDATE;
}

void CRmtView::OnUpdateTrackCopy(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (m_song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackPaste(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (m_song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (m_song.SongGetActiveTrack()>=0));
}

void CRmtView::OnUpdateTrackDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((g_activepart!=PARTINSTRS) && (m_song.SongGetActiveTrack()>=0));
}

/*
void CRmtView::OnInstrumentRandominstrument() 
{
	m_song.GetInstruments()->RandomInstrument(m_song.GetActiveInstr()); //Random
	SCREENUPDATE;
}

void CRmtView::OnUpdateInstrumentRandominstrument(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_activepart==PARTINSTRS);	
}
*/

void CRmtView::OnTrackLoad() 
{
	m_song.FileTrackLoad();
	SCREENUPDATE;
}

void CRmtView::OnTrackSave() 
{
	m_song.FileTrackSave();
	SCREENUPDATE;
}

void CRmtView::OnUpdateTrackLoad(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_song.SongGetActiveTrack()>=0);
}

void CRmtView::OnUpdateTrackSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_song.SongGetActiveTrack()>=0);
}

void CRmtView::OnSongCopyline() 
{
	m_song.SongCopyLine();	
}

void CRmtView::OnSongPasteline() 
{
	m_song.SongPasteLine();
	SCREENUPDATE;
}

void CRmtView::OnSongClearline() 
{
	m_song.SongClearLine();
	SCREENUPDATE;
}

void CRmtView::OnSongDeleteactualline() 
{
	m_song.SongDeleteLine(m_song.SongGetActiveLine());
	SCREENUPDATE;
}

void CRmtView::OnSongInsertnewemptyline() 
{
	m_song.SongInsertLine(m_song.SongGetActiveLine());
	SCREENUPDATE;
}

void CRmtView::OnSongInsertnewlinewithunusedtracks() 
{
	int line = m_song.SongGetActiveLine();
	m_song.SongPrepareNewLine(line);
	m_song.SongSetActiveLine(line);
	SCREENUPDATE;
}

void CRmtView::OnSongInsertcopyorcloneofsonglines() 
{
	int line = m_song.SongGetActiveLine();
	m_song.SongInsertCopyOrCloneOfSongLines(line);
	m_song.SongSetActiveLine(line);
	SCREENUPDATE;
}

void CRmtView::OnSongPutnewemptyunusedtrack() 
{
	m_song.SongPutnewemptyunusedtrack();
	SCREENUPDATE;
}

void CRmtView::OnSongMaketracksduplicate() 
{
	m_song.SongMaketracksduplicate();
	SCREENUPDATE;	
}

void CRmtView::OnUpdateSongMaketracksduplicate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_song.SongGetActiveTrack()>=0);
}

//---

void CRmtView::OnPlay0() 
{
	m_song.Play(MPLAY_BOOKMARK,m_song.m_followplay);	//od bookmarku
}

void CRmtView::OnPlay1() 
{
	m_song.Play(MPLAY_SONG,m_song.m_followplay);		//cely song od 0  - s respektovanim followplay
}

void CRmtView::OnPlay2() 
{
	m_song.Play(MPLAY_FROM,m_song.m_followplay);		//od aktualniho mista  - s respektovanim followplay
}

void CRmtView::OnPlay3() 
{
	m_song.Play(MPLAY_TRACK,m_song.m_followplay);		//aktualni tracky porad dokola  - s respektovanim followplay
}


void CRmtView::OnPlaystop() 
{
	m_song.Stop();					//utisi vsechno	
}

void CRmtView::OnPlayfollow() 
{
	m_song.m_followplay ^= 1;
}

void CRmtView::OnEmTracks() 
{
	g_activepart=g_active_ti=PARTTRACKS;	//tracks
	SCREENUPDATE;
}

void CRmtView::OnEmInstruments() 
{
	g_activepart=g_active_ti=PARTINSTRS;	//instrs
	g_trackcl.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnEmInfo() 
{
	g_activepart=PARTINFO;		//info
	g_trackcl.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnEmSong() 
{
	g_activepart=PARTSONG;		//song
	g_trackcl.BlockDeselect();
	SCREENUPDATE;
}

void CRmtView::OnUpdatePlay0(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	int ch= m_song.IsBookmark();
	pCmdUI->Enable(ch);
	ch= (m_song.GetPlayMode()==MPLAY_BOOKMARK);
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlay1(CCmdUI* pCmdUI) 
{
	int ch= (m_song.GetPlayMode()==1)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlay2(CCmdUI* pCmdUI) 
{
	int ch= (m_song.GetPlayMode()==2)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlay3(CCmdUI* pCmdUI) 
{
	int ch= (m_song.GetPlayMode()==3)? 1 : 0;
	pCmdUI->SetCheck(ch);
}

void CRmtView::OnUpdatePlayfollow(CCmdUI* pCmdUI) 
{
	int ch= (m_song.m_followplay)? 1 : 0;
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
	if (g_prove==0)
		g_prove=1;
	else
	{
		if (g_prove==1 && g_tracks4_8>4)	//PROVE 2 jde jen pro 8 tracku
			g_prove=2;
		else
			g_prove=0;
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
	//
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
	g_trackcl.BlockNoteTransposition(m_song.GetActiveInstr(),1);
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockNoteup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockNotedown() 
{
	g_trackcl.BlockNoteTransposition(m_song.GetActiveInstr(),-1);
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockNotedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockVolumeup() 
{
	g_trackcl.BlockVolumeChange(m_song.GetActiveInstr(),1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockVolumeup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockVolumedown() 
{
	g_trackcl.BlockVolumeChange(m_song.GetActiveInstr(),-1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockVolumedown(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}


void CRmtView::OnBlockInstrleft() 
{
	g_trackcl.BlockInstrumentChange(m_song.GetActiveInstr(),-1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockInstrleft(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockInstrright() 
{
	g_trackcl.BlockInstrumentChange(m_song.GetActiveInstr(),1);	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockInstrright(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockInstrall() 
{
	g_trackcl.BlockAllOnOff();	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockInstrall(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_trackcl.m_all);
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockBackup() 
{
	m_song.GetUndo()->ChangeTrack(m_song.SongGetActiveTrack(),m_song.GetActiveLine(),UETYPE_TRACKDATA,1);
	g_trackcl.BlockRestoreFromBackup();	
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockBackup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnBlockPlay() 
{
	m_song.Play(MPLAY_BLOCK,m_song.m_followplay);		//vybrany blok porad dokola  - s respektovanim followplay
}

void CRmtView::OnUpdateBlockPlay(CCmdUI* pCmdUI) 
{
	int ch= (m_song.GetPlayMode()==4)? 1 : 0;
	pCmdUI->SetCheck(ch);
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
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
	if (m_midi.IsOn()) 
		m_midi.MidiOff();
	else
		m_midi.MidiOn();
}

void CRmtView::OnUpdateMidionoff(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_midi.IsOn());	
}

void CRmtView::OnBlockCopy() 
{
	m_song.TrackKey(67,0,1);	//Ctrl+C
	SCREENUPDATE;
}

void CRmtView::OnBlockCut() 
{
	m_song.TrackKey(88,0,1);	//Ctrl+X
	SCREENUPDATE;
}

void CRmtView::OnBlockDelete() 
{
	m_song.TrackKey(VK_DELETE,0,1);	//Del
	SCREENUPDATE;
}

void CRmtView::OnBlockPaste() 
{
	//m_song.TrackKey(86,0,1);	//Ctrl+V
	m_song.BlockPaste();	//paste normal
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialMergewithcurrentcontent() 
{
	m_song.BlockPaste(1);	//paste special - merge
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialVolumevaluesonly() 
{
	m_song.BlockPaste(2);	//paste special - volumes only
	SCREENUPDATE;
}

void CRmtView::OnBlockPastespecialSpeedvaluesonly() 
{
	m_song.BlockPaste(3);	//paste special - speeds only
	SCREENUPDATE;
}

void CRmtView::OnBlockExchange() 
{
	m_song.TrackKey(69,0,1);	//Ctrl+E
	SCREENUPDATE;
}

void CRmtView::OnBlockEffect() 
{
	m_song.TrackKey(70,0,1);	//Ctrl+F
	SCREENUPDATE;
}

void CRmtView::OnBlockSelectall() 
{
	m_song.TrackKey(65,0,1);	//Ctrl+A
	SCREENUPDATE;
}

void CRmtView::OnUpdateBlockCut(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());
}

void CRmtView::OnUpdateBlockDelete(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());	
}

void CRmtView::OnUpdateBlockEffect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());	
}

void CRmtView::OnUpdateBlockExchange(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(g_trackcl.IsBlockSelected());	
}

void CRmtView::OnTrackClearallduplicatedtracks() 
{
	m_song.Stop();
	int r=MessageBox("Are you sure to clear all duplicated tracks and adjust song?","Clear all duplicated tracks",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
	m_song.GetUndo()->ChangeSong(0,0,UETYPE_SONGDATA,1);

	int clearedtracks;
	clearedtracks = m_song.SongClearDuplicatedTracks();

	CString s;
	s.Format("Deleted %i duplicated tracks.",clearedtracks);
	MessageBox((LPCTSTR)s,"Clear all duplicated tracks",MB_OK);
	SCREENUPDATE;
}


void CRmtView::OnTrackClearalltracksunusedinsong() 
{
	m_song.Stop();
	int r=MessageBox("Are you sure to delete all tracks unused in song?","Clear all unused tracks",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL);

	int clearedtracks;
	clearedtracks = m_song.SongClearUnusedTracks();

	CString s;
	s.Format("Deleted %i tracks unused in song.",clearedtracks);
	MessageBox((LPCTSTR)s,"Clear all unused tracks",MB_OK);
	SCREENUPDATE;
}

void CRmtView::OnTrackAlltrackscleanup() 
{
	//Smazat vsechny tracky
	m_song.Stop();

	int r=MessageBox("WARNING:\nReally cleanup all the tracks?","All tracks cleanup",MB_YESNO | MB_ICONWARNING);
	if (r==IDYES)
	{
		m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL);
		m_song.GetTracks()->InitTracks();
		SCREENUPDATE;
	}
}

void CRmtView::OnUpdateTrackSearchandbuildloop(CCmdUI* pCmdUI) 
{
	int track=m_song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) && (m_song.GetTracks()->GetGoLine(track)<0) );
}

void CRmtView::OnTrackSearchandbuildloop() 
{
	m_song.Stop();

	int track=m_song.SongGetActiveTrack();
	if (track>=0)
	{
		m_song.GetUndo()->ChangeTrack(track,m_song.GetActiveLine(),UETYPE_TRACKDATA);
		int res=m_song.GetTracks()->TrackBuildLoop(track);
		if (res) SCREENUPDATE;
	}
}

void CRmtView::OnUpdateTrackExpandloop(CCmdUI* pCmdUI) 
{
	int track=m_song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) && (m_song.GetTracks()->GetGoLine(track)>=0) );
}

void CRmtView::OnTrackExpandloop() 
{
	m_song.Stop();

	int track=m_song.SongGetActiveTrack();
	if (track>=0)
	{
		m_song.GetUndo()->ChangeTrack(track,m_song.GetActiveLine(),UETYPE_TRACKDATA);
		int res=m_song.GetTracks()->TrackExpandLoop(track);
		if (res) SCREENUPDATE;
	}
}

void CRmtView::OnUpdateTrackInfoaboutusingofactualtrack(CCmdUI* pCmdUI) 
{
	int track=m_song.SongGetActiveTrack();
	pCmdUI->Enable( (track>=0) );
}

void CRmtView::OnTrackInfoaboutusingofactualtrack() 
{
	m_song.TrackInfo(m_song.SongGetActiveTrack());
}

void CRmtView::OnInstrumentInfo() 
{
	m_song.InstrInfo(m_song.GetActiveInstr());
}

void CRmtView::OnInstrumentChange() 
{
	int r=m_song.InstrChange(m_song.GetActiveInstr());
	if (r) SCREENUPDATE;
}

void CRmtView::OnInstrumentClearallunusedinstruments() 
{
	m_song.Stop();
	int r=MessageBox("Are you sure to delete all instruments unused in any tracks?","Clear unused instruments",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeInstrument(0,0,UETYPE_INSTRSALL);

	int clearedinstrs = m_song.ClearAllInstrumentsUnusedInAnyTrack();
	CString s;
	s.Format("Deleted %i unused instruments.",clearedinstrs);
	MessageBox((LPCTSTR)s,"Clear unused instruments",MB_OK);
	SCREENUPDATE;
}

void CRmtView::OnInstrAllinstrumentscleanup() 
{
	//Smazat vsechny instrumenty
	m_song.Stop();

	int r=MessageBox("WARNING:\nReally cleanup all the instruments?","All instruments cleanup",MB_YESNO | MB_ICONWARNING);
	if (r==IDYES)
	{
		m_song.GetUndo()->ChangeInstrument(0,0,UETYPE_INSTRSALL);

		m_song.GetInstruments()->InitInstruments();
		SCREENUPDATE;
	}
}

void CRmtView::OnSongTracksorderchange() 
{
	m_song.Stop();

	m_song.TracksOrderChange();
	SCREENUPDATE;
}

void CRmtView::OnUpdateSongSongswitch4_8(CCmdUI* pCmdUI) 
{
	pCmdUI->SetText((g_tracks4_8<=4)? "Song switch to 8 tracks" : "Song switch to mono 4 tracks");
}

void CRmtView::OnSongSongswitch4_8() 
{
	m_song.Stop();

	m_song.Songswitch4_8((g_tracks4_8<=4)? 8 : 4);
	SCREENUPDATE;
}

void CRmtView::OnSongSongchangemaximallengthoftracks() 
{
	m_song.Stop();

	int ma=m_song.GetEffectiveMaxtracklen();

	CChangeMaxtracklenDlg dlg;
	dlg.m_info.Format("Current value: %i\nComputed effective value for current song: %i",m_song.GetTracks()->m_maxtracklen,ma);
	dlg.m_maxtracklen=ma;
	if (dlg.DoModal()==IDOK)
	{
		//Undo
		m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL);
		//
		ma=dlg.m_maxtracklen;
		m_song.ChangeMaxtracklen(ma);
		SCREENUPDATE;
	}
}


void CRmtView::OnSongSearchandrebuildloopsinalltracks() 
{
	m_song.Stop();	//stop music

	int r=MessageBox("Are you sure to search and rebuild wise loops in all tracks?","Search and rebuild loops",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL);

	//nejdriv rozbali vsechny dosavadni loopy
	int tracksmodified=0,loopsexpanded=0;
	m_song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	//a ted to vsechno prohleda a vytvori loopy znovu
	int optitracks=0,optibeats=0;
	m_song.TracksAllBuildLoops(optitracks,optibeats);
	CString s;
	s.Format("Found and rebuilt loops in %i tracks (%i beats/lines).",optitracks,optibeats);
	MessageBox((LPCTSTR)s,"Search and rebuild loops",MB_OK);
	SCREENUPDATE;
}

void CRmtView::OnSongExpandloopsinalltracks() 
{
	m_song.Stop();	//stop music

	int r=MessageBox("Are you sure to expand loops in all tracks?","Expand loops",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL);

	int tracksmodified=0,loopsexpanded=0;
	m_song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	CString s;
	s.Format("Found and expanded loops in %i tracks (%i beats/lines).",tracksmodified,loopsexpanded);
	MessageBox((LPCTSTR)s,"Expand loops",MB_OK);
	SCREENUPDATE;
}

void CRmtView::OnSongSizeoptimization() 
{
	//ALL size optimalizations
	m_song.Stop();	//stop music

	int r=MessageBox("Are you sure to delete all tracks and instruments unused in song,\ntruncate unused tracks parts and rebuild wise tracks loops,\ndelete all duplicated tracks, renumber all tracks and instruments\nand change maximal tracks length to effective computed value?","All size optimizations",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (r!=IDYES) return;

	m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
	m_song.GetUndo()->ChangeInstrument(0,0,UETYPE_INSTRSALL,-1);
	m_song.GetUndo()->ChangeSong(0,0,UETYPE_SONGDATA,1);

	int chmaxtl=0;
	//nejdriv rozbali vsechny dosavadni loopy
	int tracksmodified=0,loopsexpanded=0;
	m_song.TracksAllExpandLoops(tracksmodified,loopsexpanded);
	//zjisti si originalni a efektivni delku maxtracklen a pripadne zkrati
	int maxtracklen = m_song.GetTracks()->m_maxtracklen;
	int effemaxtracklen = m_song.GetEffectiveMaxtracklen();
	if (effemaxtracklen<maxtracklen)
	{
		m_song.ChangeMaxtracklen(effemaxtracklen);
		chmaxtl=1;
	}
	//ted to zaloopuje znovu
	int optitracks=0,optibeats=0;
	m_song.TracksAllBuildLoops(optitracks,optibeats);
	//a AZ NAKONEC odmaze nepouzite tracky a jejich casti
	int clearedtracks=0,truncatedtracks=0,truncatedbeats=0;
	m_song.SongClearUnusedTracksAndParts(clearedtracks,truncatedtracks,truncatedbeats);

	//a teprve ted (po odklizeni nepouzitych tracku) odstrani nepouzite instrumenty
	int clearedinstruments;
	clearedinstruments = m_song.ClearAllInstrumentsUnusedInAnyTrack();

	//a ted odmaze zdvojene tracky a zkoriguje jejich vyskyty v songu
	//(mohly vzniknout predchozimi upravami)
	int duplicatedtracks;
	duplicatedtracks = m_song.SongClearDuplicatedTracks();

	//ted jeste precisluje tracky (aby se odstranily pripadne mezery)
	m_song.RenumberAllTracks(1);

	//a ted jeste precisluje instrumenty (aby se odstranily pripadne mezery)
	m_song.RenumberAllInstruments(1);

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
	m_song.Stop();

	CRenumberTracksDlg dlg;
	if (dlg.DoModal()==IDOK)
	{
		//schova tracky a song
		m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
		m_song.GetUndo()->ChangeSong(0,0,UETYPE_SONGDATA,1);
		m_song.RenumberAllTracks( dlg.m_radio );	//type=1...order by songcolumns, type=2...order by songlines
		SCREENUPDATE;
	}
}

void CRmtView::OnInstrumentRenumberallinstruments() 
{
	m_song.Stop();

	CRenumberInstrumentsDlg dlg;
	if (dlg.DoModal()==IDOK)
	{
		//schova instrumenty a tracky
		m_song.GetUndo()->ChangeInstrument(0,0,UETYPE_INSTRSALL,-1);
		m_song.GetUndo()->ChangeTrack(0,0,UETYPE_TRACKSALL,1);
		m_song.RenumberAllInstruments( dlg.m_radio );	//type=1...remove gaps, 2=order by using in tracks, type=3...order by instrument names
		SCREENUPDATE;
	}
}

BOOL CRmtView::GetNumLock()
{
	BYTE keys[256];
	GetKeyboardState(keys);
	return (keys[VK_NUMLOCK]!=0);
}

/*
typedef struct _OSVERSIONINFO{ 
    DWORD dwOSVersionInfoSize; 
    DWORD dwMajorVersion; 
    DWORD dwMinorVersion; 
    DWORD dwBuildNumber; 
    DWORD dwPlatformId; 
    TCHAR szCSDVersion[ 128 ]; 
};
*/

void CRmtView::SetNumLock(BOOL onoff)
{
	if (!m_setnumlockfunction) return;
	BOOL state=GetNumLock();
	if (state != onoff) SetLockKeys(VK_NUMLOCK,onoff);
}

void CRmtView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	// TODO: Add your message handler code here
	if (g_keyboard_usenumlock) {
		g_numlock=GetNumLock();
		SetNumLock(0);
	}

	/*
	g_capslock_other=GetCapsLock();
	SetCapsLock(g_capslock_rmt);
	g_scrolllock_other=GetScrollLock();
	SetScrollLock(g_scrolllock_rmt);
	*/

/*
	BYTE keys[256];
	GetKeyboardState(keys);
	
	g_shiftkey = (keys[VK_SHIFT]!=0);
	g_controlkey = (keys[VK_CONTROL]!=0);
//*/
	g_shiftkey = g_controlkey = 0;

//	g_shiftkey = (GetAsyncKeyState(VK_SHIFT)!=NULL);
//	g_controlkey = (GetAsyncKeyState(VK_CONTROL)!=NULL);

	g_focus=1;	//hlavni okno ma focus
}

void CRmtView::OnKillFocus(CWnd* pNewWnd) 
{

	CView::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
//	Beep(300,200);	
	if (g_keyboard_usenumlock) {	
		SetNumLock(g_numlock);
	}

	/*
	g_capslock_rmt=GetCapsLock();
	SetCapsLock(g_capslock_other);
	g_scrolllock_rmt=GetScrollLock();
	SetScrollLock(g_scrolllock_other);
	*/

	g_focus=0;	//hlavni okno nema focus
}


BOOL CRmtView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	CRect rec;
	//AfxGetApp()->GetMainWnd()->GetWindowRect(&rec);
	//this->GetWindowRect(&rec);
	::GetWindowRect(g_viewhwnd,&rec);
	CPoint np(pt-rec.TopLeft());
	MouseAction(np,0,zDelta);

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


void CRmtView::OnUndoUndo() 
{
	if (m_song.Undo()) SCREENUPDATE;
}

void CRmtView::OnUpdateUndoUndo(CCmdUI* pCmdUI) 
{
	int u = m_song.UndoGetUndoSteps();
	if (u>0)
	{
		pCmdUI->Enable(1);
		CString s;
		s.Format("&Undo (%u)\tAlt+Backspace",u);
		pCmdUI->SetText(s);
	}
	else
	{
		pCmdUI->Enable(0);
		pCmdUI->SetText("&Undo\tAlt+Backspace");
	}
}

void CRmtView::OnUndoRedo() 
{
	if (m_song.Redo()) SCREENUPDATE;
}

void CRmtView::OnUpdateUndoRedo(CCmdUI* pCmdUI) 
{
	int u = m_song.UndoGetRedoSteps();
	if (u>0)
	{
		pCmdUI->Enable(1);
		CString s;
		s.Format("&Redo (%u)",u);
		pCmdUI->SetText(s);
	}
	else
	{
		pCmdUI->Enable(0);
		pCmdUI->SetText("&Redo");
	}
}

void CRmtView::OnUndoClearundoredo() 
{
	m_song.GetUndo()->Clear();
}

void CRmtView::OnUpdateUndoClearundoredo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_song.UndoGetUndoSteps() || m_song.UndoGetRedoSteps());
}

/*
//ID_APP_EXIT:   Exit the application.
//CWinApp::OnAppExit handles this command by sending a WM_CLOSE message to the application's main window. The standard shutting down of the application (prompting for dirty files and so on) is handled by the CFrameWnd implementation.
//Customization of this command handler is not recommended. Overriding CWinApp::SaveAllModified or the CFrameWnd closing logic is recommended.

void CRmtView::OnAppExit() 
{
	//if ( m_song.WarnUnsavedChanges() ) return; //nechce to
	CView::OnClose();
}
*/

void CRmtView::OnWantExit() //vola se z menu File/Exit ID_WANTEXIT misto puvodniho ID_APP_EXIT
{
	if (g_keyboard_usenumlock) {

		SetNumLock(g_numlock);
		m_setnumlockfunction=0;
	}
	//SetCapsLock(g_capslock_other);
	//SetScrollLock(g_scrolllock_other);
	if ( m_song.WarnUnsavedChanges() )
	{
		if (g_keyboard_usenumlock) {
			m_setnumlockfunction=1;
			g_numlock=GetNumLock();
			SetNumLock(0);
		}
		//SetCapsLock(g_capslock_rmt);
		//SetScrollLock(g_scrolllock_rmt);
		return; //nema byt exit
	}
	//ma byt exit
	//m_setnumlockfunction necha 0 aby uz nikdo stav numlocku nemenil
	m_song.Stop();
	g_closeapplication=1;
	AfxGetApp()->GetMainWnd()->PostMessage(WM_CLOSE,0,0);
}

void CRmtView::OnTrackCursorgotothespeedcolumn() 
{
	if (m_song.CursorToSpeedColumn()) SCREENUPDATE;
}

void CRmtView::OnUpdateTrackCursorgotothespeedcolumn(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(	g_activepart==PARTTRACKS && m_song.SongGetActiveTrack()>=0 );
}

