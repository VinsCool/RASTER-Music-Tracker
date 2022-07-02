/*
	RMTMIDI.H	
*/

#ifndef __RMTMIDI__
#define __RMTMIDI__

#define MIDIDEVNAMELEN	256

extern int g_midi_notech[16];
extern int g_midi_voluch[16];
extern int g_midi_instch[16];

extern int g_focus;

class CRmtMidi
{
public:
	CRmtMidi();
	~CRmtMidi();

	void SetDevice(char* devname)	{ strncpy(m_midiindevname,devname,MIDIDEVNAMELEN); };

	BOOL IsOn()			{ return m_ison; };

	int MidiInit();
	int MidiOn();
	void MidiOff();
	int MidiRestart();

	int GetMidiDevId()	{ return m_midiinid; };
	char *GetMidiDevName() { return m_midiindevname; };

private:

	BOOL m_ison;

	HMIDIIN m_hmidiin;			//MIDI IN handle
	//HMIDIOUT m_hmidiout;		//MIDI OUT handle
	char m_midiindevname[MIDIDEVNAMELEN+1];	//the name of the MIDI IN device
	//char m_midioutdevname[MIDIDEVNAMELEN+1]; //the name of the MIDI OUT device
	int m_midiinid;						//id MIDI IN device
	//int m_midioutid;					//id MIDI OUT device
};

#endif
