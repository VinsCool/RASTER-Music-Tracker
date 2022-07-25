// RmtView.h : interface of the CRmtView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RMTVIEW_H__1709C74D_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
#define AFX_RMTVIEW_H__1709C74D_06D0_11D7_BEB0_00600854AFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "r_music.h"
#include "RmtMidi.h"
#include <chrono>

extern CDC* g_mem_dc;

extern int g_width;
extern int g_height;
extern int g_tracklines;
extern int g_cursoractview;
extern int g_line_y;
extern int g_scaling_percentage;

extern double g_basetuning;
extern int g_basenote;
extern int g_temperament;

//ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
extern double g_UNISON;
extern double g_MIN_2ND;
extern double g_MAJ_2ND;
extern double g_MIN_3RD;
extern double g_MAJ_3RD;
extern double g_PERF_4TH;
extern double g_TRITONE;
extern double g_PERF_5TH;
extern double g_MIN_6TH;
extern double g_MAJ_6TH;
extern double g_MIN_7TH;
extern double g_MAJ_7TH;

//ratio left
extern int g_UNISON_L;
extern int g_MIN_2ND_L;
extern int g_MAJ_2ND_L;
extern int g_MIN_3RD_L;
extern int g_MAJ_3RD_L;
extern int g_PERF_4TH_L;
extern int g_TRITONE_L;
extern int g_PERF_5TH_L;
extern int g_MIN_6TH_L;
extern int g_MAJ_6TH_L;
extern int g_MIN_7TH_L;
extern int g_MAJ_7TH_L;

//ratio right
extern int g_UNISON_R;
extern int g_MIN_2ND_R;
extern int g_MAJ_2ND_R;
extern int g_MIN_3RD_R;
extern int g_MAJ_3RD_R;
extern int g_PERF_4TH_R;
extern int g_TRITONE_R;
extern int g_PERF_5TH_R;
extern int g_MIN_6TH_R;
extern int g_MAJ_6TH_R;
extern int g_MIN_7TH_R;
extern int g_MAJ_7TH_R;

extern CDC* g_gfx_dc;
extern HWND g_hwnd;
extern HWND g_viewhwnd;
extern BOOL g_changes;

extern int g_shiftkey;
extern int g_controlkey;
extern int g_altkey;	//does not work yet

extern int g_tracks4_8;

extern BOOL volatile g_screenupdate;
extern BOOL volatile g_invalidatebytimer;

extern int volatile g_screena;
extern int volatile g_screenwait;

extern int g_activepart;
extern int g_active_ti;	
extern int last_active_ti;	
extern int last_activepart;	

extern uint64_t last_ms;
extern uint64_t last_sec;
extern int real_fps;
extern double last_fps;
extern double avg_fps[120];

extern BOOL is_editing_instr;	//will always return 1 in the Instrument Name area
extern BOOL is_editing_infos;	//will always return 1 in the Song Name area

extern BOOL volatile g_prove;
extern BOOL volatile g_respectvolume;

extern BOOL g_viewmaintoolbar;
extern BOOL g_viewblocktoolbar;
extern BOOL g_viewstatusbar;
extern BOOL g_viewplaytimecounter;
extern BOOL g_viewanalyzer;
extern BOOL g_viewpokeyregs;
extern BOOL g_viewinstractivehelp;

extern long g_playtime;

extern BOOL volatile g_is6502;

extern UINT g_mousebutt;
extern int g_mouselastbutt;
extern int g_mouse_px;
extern int g_mouse_py;

extern int g_tracklinehighlight;
extern BOOL g_tracklinealtnumbering;
extern BOOL g_displayflatnotes;
extern BOOL g_usegermannotation;
extern BOOL g_ntsc;
extern BOOL g_nohwsoundbuffer;

extern int g_keyboard_layout;
extern BOOL g_keyboard_playautofollow;
extern BOOL g_keyboard_swapenter;
extern BOOL g_keyboard_updowncontinue;
extern BOOL g_keyboard_rememberoctavesandvolumes;
extern BOOL g_keyboard_escresetatarisound;
extern BOOL g_keyboard_askwhencontrol_s;

extern BOOL g_midi_tr;
extern int g_midi_volumeoffset;
extern BOOL g_midi_noteoff;

extern void SetStatusBarText(const char* text);
extern void TextXY(char *txt,int x,int y,int c=0);

extern void Memory_Clear();
extern int Atari_LoadRMTRoutines();
extern int Atari_InitRMTRoutine();
extern void Get_Driver_Version();	

extern char CharH4(unsigned char b);
extern char CharL4(unsigned char b);

extern char NoteKey(int vk);
extern char Numblock09Key(int vk);

extern unsigned char g_atarimem[65536];

extern CString g_prgpath;
extern CString g_lastloadpath_songs;
extern CString g_lastloadpath_instruments;
extern CString g_lastloadpath_tracks;

extern CTrackClipboard g_trackcl;

extern int Atari6502_Init();
extern void Atari6502_DeInit();

extern void SetChannelOnOff(int ch,int onoff);
extern void SetChannelSolo(int ch);

extern void Atari_InstrumentTurnOff(int instr);

class CRmtView : public CView
{
protected: // create from serialization only
	CRmtView();
	DECLARE_DYNCREATE(CRmtView)

// Attributes
public:
	CRmtDoc* GetDocument();

	void ReadConfig();
	void WriteConfig();

	BOOL DrawAll();
	void DrawAnalyzer();
	void DrawPlaytimecounter();
	void GetFPS();
	void GetMouseXY(int px, int py, int mousebutt);	//debug function, to get the pointer coordinates
	int MouseAction(CPoint point,UINT mousebutt,short wheelzDelta);
	void ChangeViewElements(BOOL writeconfig=1);

	//used to handle the window size and most dynamic elements related to it
	bool Resize(int width, int height);
	int  m_width;
	int  m_height;

	CBitmap m_mem_bitmap;
	CDC		m_mem_dc;
	CBitmap m_gfx_bitmap;
	CDC		m_gfx_dc;

	CPen*	m_pen1;
	CPen*	m_penorig;

	HCURSOR m_cursororig;
	HCURSOR m_cursorchanonoff;
	HCURSOR m_cursorenvvolume;
	HCURSOR m_cursorgoto;
	HCURSOR m_cursordlg;
	HCURSOR m_cursorsetpos;

	CSong	m_song;
	CRmtMidi	m_midi;

	UINT m_timeranalyzer;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRmtView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnSize(UINT nType, int cx, int cy);	//allows proper window resize and adjust things related to it
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRmtView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRmtView)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSysChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileNew();
	afx_msg void OnFileExportAs();
	afx_msg void OnInstrLoad();
	afx_msg void OnInstrSave();
	afx_msg void OnInstrCopy();
	afx_msg void OnInstrPaste();
	afx_msg void OnInstrCut();
	afx_msg void OnInstrDelete();
	afx_msg void OnTrackDelete();
	afx_msg void OnTrackCopy();
	afx_msg void OnTrackPaste();
	afx_msg void OnTrackCut();
	afx_msg void OnSongCopyline();
	afx_msg void OnSongPasteline();
	afx_msg void OnSongClearline();
	afx_msg void OnPlay1();
	afx_msg void OnPlay2();
	afx_msg void OnPlay3();
	afx_msg void OnPlaystop();
	afx_msg void OnPlayfollow();
	afx_msg void OnEmInfo();
	afx_msg void OnEmInstruments();
	afx_msg void OnEmSong();
	afx_msg void OnEmTracks();
	afx_msg void OnUpdateEmTracks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEmInstruments(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEmInfo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEmSong(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlayfollow(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlay1(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlay2(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlay3(CCmdUI* pCmdUI);
	afx_msg void OnProvemode();
	afx_msg void OnUpdateProvemode(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnViewVolumeanalyzer();
	afx_msg void OnUpdateViewVolumeanalyzer(CCmdUI* pCmdUI);
	afx_msg void OnViewPlaytimecounter();
	afx_msg void OnUpdateViewPlaytimecounter(CCmdUI* pCmdUI);
	afx_msg void OnViewInstrumentactivehelp();
	afx_msg void OnUpdateViewInstrumentactivehelp(CCmdUI* pCmdUI);
	afx_msg void OnViewBlocktoolbar();
	afx_msg void OnUpdateViewBlocktoolbar(CCmdUI* pCmdUI);
	afx_msg void OnBlockNoteup();
	afx_msg void OnUpdateBlockNoteup(CCmdUI* pCmdUI);
	afx_msg void OnBlockVolumedown();
	afx_msg void OnUpdateBlockVolumedown(CCmdUI* pCmdUI);
	afx_msg void OnBlockVolumeup();
	afx_msg void OnUpdateBlockVolumeup(CCmdUI* pCmdUI);
	afx_msg void OnBlockNotedown();
	afx_msg void OnUpdateBlockNotedown(CCmdUI* pCmdUI);
	afx_msg void OnBlockInstrleft();
	afx_msg void OnUpdateBlockInstrleft(CCmdUI* pCmdUI);
	afx_msg void OnBlockInstrright();
	afx_msg void OnUpdateBlockInstrright(CCmdUI* pCmdUI);
	afx_msg void OnBlockInstrall();
	afx_msg void OnUpdateBlockInstrall(CCmdUI* pCmdUI);
	afx_msg void OnBlockBackup();
	afx_msg void OnUpdateBlockBackup(CCmdUI* pCmdUI);
	afx_msg void OnBlockPlay();
	afx_msg void OnUpdateBlockPlay(CCmdUI* pCmdUI);
	afx_msg void OnChan1();
	afx_msg void OnChan2();
	afx_msg void OnChan3();
	afx_msg void OnChan4();
	afx_msg void OnChan5();
	afx_msg void OnChan6();
	afx_msg void OnChan7();
	afx_msg void OnChan8();
	afx_msg void OnUpdateChan1(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan2(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan4(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan5(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan6(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan7(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChan8(CCmdUI* pCmdUI);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnViewPokeyregs();
	afx_msg void OnUpdateViewPokeyregs(CCmdUI* pCmdUI);
	afx_msg void OnMidionoff();
	afx_msg void OnUpdateMidionoff(CCmdUI* pCmdUI);
	afx_msg void OnViewConfiguration();
	afx_msg void OnViewTuning();
	afx_msg void OnBlockCopy();
	afx_msg void OnBlockCut();
	afx_msg void OnBlockDelete();
	afx_msg void OnBlockPaste();
	afx_msg void OnBlockExchange();
	afx_msg void OnBlockEffect();
	afx_msg void OnBlockSelectall();
	afx_msg void OnUpdateBlockCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlockDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlockEffect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBlockExchange(CCmdUI* pCmdUI);
	afx_msg void OnTrackAlltrackscleanup();
	afx_msg void OnInstrAllinstrumentscleanup();
	afx_msg void OnSongDeleteactualline();
	afx_msg void OnSongInsertnewemptyline();
	afx_msg void OnSongInsertnewlinewithunusedtracks();
	afx_msg void OnSongInsertcopyorcloneofsonglines();
	afx_msg void OnFileImport();
	afx_msg void OnUpdateSongSongswitch4_8(CCmdUI* pCmdUI);
	afx_msg void OnSongSongswitch4_8();
	afx_msg void OnSongTracksorderchange();
	afx_msg void OnInstrumentInfo();
	afx_msg void OnInstrumentChange();
	afx_msg void OnTrackSearchandbuildloop();
	afx_msg void OnUpdateTrackSearchandbuildloop(CCmdUI* pCmdUI);
	afx_msg void OnSongSearchandrebuildloopsinalltracks();
	afx_msg void OnTrackExpandloop();
	afx_msg void OnUpdateTrackExpandloop(CCmdUI* pCmdUI);
	afx_msg void OnSongExpandloopsinalltracks();
	afx_msg void OnSongSizeoptimization();
	afx_msg void OnInstrumentClearallunusedinstruments();
	afx_msg void OnTrackClearalltracksunusedinsong();
	afx_msg void OnUpdateTrackInfoaboutusingofactualtrack(CCmdUI* pCmdUI);
	afx_msg void OnTrackInfoaboutusingofactualtrack();
	afx_msg void OnTrackRenumberalltracks();
	afx_msg void OnInstrumentRenumberallinstruments();
	afx_msg void OnUpdateTrackCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackPaste(CCmdUI* pCmdUI);
	afx_msg void OnInstrumentPastespecialVolumeLRenvelopesonly();
	afx_msg void OnInstrumentPastespecialVolumeLenvelopeonly();
	afx_msg void OnInstrumentPastespecialTableonly();
	afx_msg void OnInstrumentPastespecialVolumeRenvelopeonly();
	afx_msg void OnInstrumentPastespecialEnvelopeparametersonly();
	afx_msg void OnUpdateInstrumentPastespecialVolumerenvelopeonly(CCmdUI* pCmdUI);
	afx_msg void OnBlockPastespecialVolumevaluesonly();
	afx_msg void OnBlockPastespecialSpeedvaluesonly();
	afx_msg void OnBlockPastespecialMergewithcurrentcontent();
	afx_msg void OnSongPutnewemptyunusedtrack();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnTrackLoad();
	afx_msg void OnTrackSave();
	afx_msg void OnUpdateTrackLoad(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackSave(CCmdUI* pCmdUI);
	afx_msg void OnTrackClearallduplicatedtracks();
	afx_msg void OnSongMaketracksduplicate();
	afx_msg void OnUpdateSongMaketracksduplicate(CCmdUI* pCmdUI);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnPlay0();
	afx_msg void OnUpdatePlay0(CCmdUI* pCmdUI);
	afx_msg void OnFileReload();
	afx_msg void OnUpdateFileReload(CCmdUI* pCmdUI);
	afx_msg void OnUndoUndo();
	afx_msg void OnUpdateUndoUndo(CCmdUI* pCmdUI);
	afx_msg void OnUndoRedo();
	afx_msg void OnUpdateUndoRedo(CCmdUI* pCmdUI);
	afx_msg void OnUndoClearundoredo();
	afx_msg void OnUpdateUndoClearundoredo(CCmdUI* pCmdUI);
	afx_msg void OnInstrumentPastespecialInsertvolenvsandenvparstocurpos();
	afx_msg void OnUpdateInstrumentPastespecialInsertvolenvsandenvparstocurpos(CCmdUI* pCmdUI);
	afx_msg void OnInstrumentPastespecialVolumeenvandenvelopeparsonly();
	afx_msg void OnInstrumentPastespecialVolumeltorenvelopeonly();
	afx_msg void OnUpdateInstrumentPastespecialVolumeltorenvelopeonly(CCmdUI* pCmdUI);
	afx_msg void OnInstrumentPastespecialVolumertolenvelopeonly();
	afx_msg void OnUpdateInstrumentPastespecialVolumertolenvelopeonly(CCmdUI* pCmdUI);
	afx_msg void OnUpdateInstrumentPastespecialVolumelenvelopeonly(CCmdUI* pCmdUI);
	afx_msg void OnTrackCursorgotothespeedcolumn();
	afx_msg void OnUpdateTrackCursorgotothespeedcolumn(CCmdUI* pCmdUI);
	afx_msg void OnViewToolbar();
	afx_msg void OnUpdateViewToolbar(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusBar();
	afx_msg void OnUpdateViewStatusBar(CCmdUI* pCmdUI);
	afx_msg void OnSongSongchangemaximallengthoftracks();
	//}}AFX_MSG
	afx_msg void OnWantExit();
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in RmtView.cpp
inline CRmtDoc* CRmtView::GetDocument()
   { return (CRmtDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RMTVIEW_H__1709C74D_06D0_11D7_BEB0_00600854AFCA__INCLUDED_)
