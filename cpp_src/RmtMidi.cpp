/*
	MIDI IN support
*/

#include "stdafx.h"
#include "Rmt.h"
#include "RmtDoc.h"
#include "RmtView.h"
#include "MainFrm.h"			//!
#include <mmsystem.h>
#include "RmtMidi.h"
#include "global.h"

extern CSong	g_Song;

////////////// MIDIINPROC //////////////////
void CALLBACK MidiInProc(
    HMIDIIN hMidiIn,	
    UINT wMsg,	
    DWORD dwInstance,	
    DWORD dwParam1,	
    DWORD dwParam2	
   )
{
	// Forward MIDI data and error messages to the global song object
	if (wMsg != MIM_DATA && wMsg != MIM_ERROR) return;
	g_Song.MidiEvent(dwParam1);
}

CRmtMidi::CRmtMidi()
{
	m_MidiIsOn = 0;
	m_MidiInHandle = NULL;
	m_MidiInDeviceId = -1;
	m_MidiInDeviceName[0] = 0;

	m_TouchResponse = 0;
	m_VolumeOffset = 1;
	m_NoteOff = 0;
}

CRmtMidi::~CRmtMidi()
{
	MidiOff();
}

int CRmtMidi::MidiInit()
{
	int wasOnOff = IsOn();

	MidiOff();

	// If there us no device name set then turn MIDI off
	if (m_MidiInDeviceName[0] == 0)
	{
		m_MidiInDeviceId = -1;
		return 1;	//does not want a MIDI device
	}

	MIDIINCAPS micaps;
	int numMidiDevices = midiInGetNumDevs();				// Query how many MIDI devices there are

	for (int i = 0; i < numMidiDevices; i++)
	{
		// Query each MIDI device.
		midiInGetDevCaps(i, &micaps, sizeof(MIDIINCAPS));

		// Check if this is the MIDI device we are looking for
		if (strcmp(m_MidiInDeviceName, micaps.szPname) == 0)
		{
			m_MidiInDeviceId = i;   //found midi in by configfile
			if (wasOnOff) MidiOn();
			return 1;
		}
	}
	// Device was not found.
	// Turn
	m_MidiInDeviceId = -1;

	MessageBox(g_hwnd, CString("Can't init the MIDI IN device\n") + m_MidiInDeviceName, "MIDI IN error", MB_ICONEXCLAMATION);

	m_MidiInDeviceName[0] = 0;

	return 0;
}

int CRmtMidi::MidiOn()
{
	// Init the MIDI channel buffers

	for (int i = 0; i < 16; i++)
	{
		m_LastNoteOnChannel[i] = -1;	// Last pressed keys on each channel
		m_NoteVolumeOnChannel[i] = 0;	// Volume
		m_InstrumentOnChannel[i] = 0;	// Instrument numbers
	}

	if (m_MidiInDeviceId>=0)
	{
		if (IsOn()) MidiOff();
		int status = midiInOpen( &m_MidiInHandle,
					m_MidiInDeviceId,
					(DWORD_PTR) MidiInProc,
					(DWORD_PTR) this,
					CALLBACK_FUNCTION ); 
		if (status != MMSYSERR_NOERROR ) 
		{
			MessageBox(0,"Can't open selected MIDI IN device.","MidiInOpen error", MB_ICONERROR);
			return 0;
		}
		else
		{
			midiInStart(m_MidiInHandle);
			m_MidiIsOn=1;
			return 1;
		}
	}
	return 0;
}

void CRmtMidi::MidiOff()
{
	if (m_MidiInDeviceId>=0) //0 is PC keyboard only
	{
		midiInStop(m_MidiInHandle);
		midiInReset(m_MidiInHandle);
		midiInClose(m_MidiInHandle);
	}
	m_MidiIsOn=0;
}

int CRmtMidi::MidiRestart()
{
	MidiOff();
	return MidiOn();
}

