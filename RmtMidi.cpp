/*
	MIDI IN support
*/

#include "stdafx.h"
#include "Rmt.h"

#include "RmtDoc.h"
#include "RmtView.h"

#include "MainFrm.h"			//!

//#include "r_music.h"

#include <mmsystem.h>
//#include "global.h"

#include "RmtMidi.h"


////////////// MIDIINPROC //////////////////
void CALLBACK MidiInProc(
    HMIDIIN hMidiIn,	
    UINT wMsg,	
    DWORD dwInstance,	
    DWORD dwParam1,	
    DWORD dwParam2	
   )
{
	if (wMsg!=MIM_DATA && wMsg!=MIM_ERROR) return;
	//((CMikeplDlg*)dwInstance)->MidiKeybTouch(dwParam1);
	CMainFrame* mf = (CMainFrame*)AfxGetApp()->GetMainWnd();
	if (mf)
	{
		CRmtView* rv = (CRmtView*)mf->GetActiveView();
		if (rv) rv->m_song.MidiEvent(dwParam1);
	}
}

CRmtMidi::CRmtMidi()
{
	m_ison=0;
	m_hmidiin=NULL;
	m_midiinid=-1;
	m_midiindevname[0]=0;
}

CRmtMidi::~CRmtMidi()
{
	MidiOff();
}

int CRmtMidi::MidiInit()
{
	int lastonoff = IsOn();

	MidiOff();

	if (m_midiindevname[0]==0)
	{
		m_midiinid = -1;
		return 1;	//nechce zadne MIDI device
	}

	MIDIINCAPS micaps;
	int mind = midiInGetNumDevs();

	for(int i=0; i<mind; i++)
	{
		midiInGetDevCaps(i,&micaps, sizeof(MIDIINCAPS));
		if (strcmp(m_midiindevname,micaps.szPname)==0)
		{
			m_midiinid = i;   //nasel midi in dle configfile
			//strcpy(m_midiindevname,micaps.szPname);
			if (lastonoff) MidiOn();
			return 1;
		}
	}

	//nenasel
	m_midiinid = -1;

	MessageBox(g_hwnd,CString("Can't init the MIDI IN device\n")+m_midiindevname,"MIDI IN error",MB_ICONEXCLAMATION);

	strcpy(m_midiindevname,"");
	return 0;
}


int CRmtMidi::MidiOn()
{

	for(int i=0; i<16; i++)
	{
		g_midi_notech[i]=-1;	//posledne stlacene klavesy na jednotlivych kanalech
		g_midi_voluch[i]=0;		//hlasitosti
		g_midi_instch[i]=0;		//cisla instrumentu
	}

	if (m_midiinid>=0)
	{
		if (IsOn()) MidiOff();
		int status = midiInOpen( &m_hmidiin,
					m_midiinid,
					(unsigned long) MidiInProc, 
					(unsigned long) this,
					CALLBACK_FUNCTION ); 
		if (status != MMSYSERR_NOERROR ) 
		{
			MessageBox(0,"Can't open selected MIDI IN device.","MidiInOpen error", MB_ICONERROR);
			return 0;
		}
		else
		{
			midiInStart(m_hmidiin);
			m_ison=1;
			return 1;
		}
	}

/*
	if (g_midioutid>=0)
	{
		int status = midiOutOpen( &g_hmidiout,
				g_midioutid,
				NULL, 
				(unsigned long) NULL,
				CALLBACK_NULL ); 
		if (status == MMSYSERR_NOERROR )
		{
			if (g_local)
			{
				unsigned char mv[4];
				mv[0] = 0xb0;	//controller na kanalu 0
				mv[1] = 122;	//local on/off
				mv[2] = 0;		//off
				midiOutShortMsg(g_hmidiout,*(DWORD*)mv);
			}
		}
		else
		{
			strcat(errbf,"Can't open selected MIDI OUT device.");
		}

	}
	else
		strcat(errbf,"MIDI OUT device isn't selected.");

	if (errbf[0]) throw(errbf);
*/
	return 0;
}

void CRmtMidi::MidiOff()
{
	if (m_midiinid>=0) //0 je PC keyboard only
	{
		midiInStop(m_hmidiin);
		midiInReset(m_hmidiin);
		midiInClose(m_hmidiin);
	}

	m_ison=0;

	/*
	if (g_midioutid>=0)
	{
		if (g_local)
		{
			unsigned char mv[4];
			mv[0] = 0xb0;	//controller na kanalu 0
			mv[1] = 122;	//local on/off
			mv[2] = 127;	//on
			midiOutShortMsg(g_hmidiout,*(DWORD*)mv);
		}
		midiOutReset(g_hmidiout);
		midiOutClose(g_hmidiout);
	}
	*/
}

int CRmtMidi::MidiRestart()
{
	MidiOff();
	return MidiOn();
}

