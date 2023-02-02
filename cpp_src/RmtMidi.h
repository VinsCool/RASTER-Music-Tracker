/*
	RMTMIDI.H	
*/

#ifndef __RMTMIDI__
#define __RMTMIDI__

#define MIDIDEVNAMELEN	256

class CRmtMidi
{
public:
	CRmtMidi();
	~CRmtMidi();

	void SetDevice(const char* devname) { strncpy(m_MidiInDeviceName, devname, MIDIDEVNAMELEN); };

	BOOL IsOn()						{ return m_MidiIsOn; };

	int MidiInit();
	int MidiOn();
	void MidiOff();
	int MidiRestart();

	int GetMidiDevId()				{ return m_MidiInDeviceId; }
	char *GetMidiDevName()			{ return m_MidiInDeviceName; }

	// MIDI settings
	BOOL	m_TouchResponse;			// Should MIDI input handle touch response?
	int		m_VolumeOffset;				// Starts from volume 1 by default
	BOOL	m_NoteOff;					// By default, NoteOff is turned off

	int		m_LastNoteOnChannel[16];	// Last recorded notes on each MIDI channel
	int		m_NoteVolumeOnChannel[16];	// Note volume on individual MIDI channels
	int		m_InstrumentOnChannel[16];	// Last set instrument numbers on individual MIDI channels

private:

	BOOL	m_MidiIsOn;

	HMIDIIN	m_MidiInHandle;							// MIDI IN handle
	char	m_MidiInDeviceName[MIDIDEVNAMELEN+1];	// The name of the MIDI IN device
	int		m_MidiInDeviceId;						// Id of the MIDI IN device

	// Not used
	//HMIDIOUT m_hmidiout;		//MIDI OUT handle
	//char m_midioutdevname[MIDIDEVNAMELEN+1]; //the name of the MIDI OUT device
	//int m_midioutid;					//id MIDI OUT device
};

#endif
