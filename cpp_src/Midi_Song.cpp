#include "StdAfx.h"
#include <fstream>

#include "GuiHelpers.h"
#include "Song.h"

// MFC interface code
#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"
#include "EffectsDlg.h"
#include "MainFrm.h"


#include "Atari6502.h"
#include "XPokey.h"
#include "IOHelpers.h"

#include "Instruments.h"
#include "Clipboard.h"


#include "global.h"

#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"
#include "RmtMidi.h"


extern CInstruments	g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CXPokey g_Pokey;
extern CRmtMidi g_Midi;

void CSong::MidiEvent(DWORD dwParam)
{
	unsigned char chn, cmd, pr1, pr2;
	unsigned char* mv = (unsigned char*)&dwParam;
	cmd = mv[0] & 0xf0;
	chn = mv[0] & 0x0f;
	pr1 = mv[1];
	pr2 = mv[2];
	if (cmd == 0x80 && chn != 15 && chn != 9) { cmd = 0x90; pr2 = 0; }	//key off, as long as the MIDI channel isn't 15 or 10 to avoid conflicts
	else
		if (cmd == 0xf0)
		{
			if (mv[0] == 0xff)
			{
				//System Reset
			MIDISystemReset:
				Atari_InitRMTRoutine(); //reinit RMT routines
				for (int i = 1; i < 16; i++)	//from 1, because it is MULTITIMBRAL 2-16
				{
					g_Midi.m_LastNoteOnChannel[i] = -1;	//last pressed keys on each channel
					g_Midi.m_NoteVolumeOnChannel[i] = 0;		//volume
					g_Midi.m_InstrumentOnChannel[i] = 0;		//instrument numbers
				}
			}
			return; //END
		}

	if (chn > 0 && chn < 9)
	{
		//2-10 channels (chn = 1-9) are used for multitimbral L1-L4, R1-R4
		int atc = (chn - 1) % g_tracks4_8;	//atari track 0-7 (resp. 0-3 in mono)
		int note, vol;
		if (cmd == 0x90)
		{
			if (chn == 9)
			{
				//channel 10 (chn=9) ...drums channel
				if (pr2 > 0) //"note on" any non-zero volume
				{
					// planned
					// make a record of all g_midi _.... ch [0 to g_tracks4_8] into the track and move the line one step lower
				}
			}
			else
			{
				//channel 2-9 (chn=1-8)
				note = pr1 - 36;
				if (note >= 0 && note < NOTESNUM)
				{
					if (pr2 != 0 || (pr2 == 0 && note == g_Midi.m_LastNoteOnChannel[1 + atc]))
					{
						vol = pr2 / 8;
					NoteOFF:
						g_Midi.m_LastNoteOnChannel[1 + atc] = note;
						g_Midi.m_NoteVolumeOnChannel[1 + atc] = vol;
						int ins = g_Midi.m_InstrumentOnChannel[chn];
						SetPlayPressedTonesTNIV(atc, note, ins, vol);
					}
				}
			}
		}
		else
			if (cmd == 0xc0)
			{
				if (pr1 >= 0 && pr1 < INSTRSNUM)
				{
					g_Midi.m_InstrumentOnChannel[1 + atc] = pr1;
				}
			}
			else
				if (cmd == 0xb0)
				{
					if (pr1 == 123)
					{
						//All notes OFF
						note = -1;
						vol = 0;
						goto NoteOFF;
					}
					else
						if (pr1 == 121)
						{
							//Reset All Controls
							goto MIDISystemReset;
						}
				}
		return; //END
	}

	if (!g_RmtHasFocus && !g_prove) return;	//when it has no focus and is not in prove mode, the MIDI input will be ignored, to avoid overwriting patterns accidentally

	//test input from my own MIDI controller. CH15 for most events input, and CH9 specifically for the drumpad buttons, used for certain shortcuts triggered with MIDI NOTE ON events
	if (chn == 15 || chn == 9)
	{
		//command buttons, while this would technically work from any MIDI channel, it is specifically mapped for CH15 in order to avoid conflicing code, as a temporary workaround
		if (cmd == 0xB0 && chn == 15)	//control change and key pressed
		{
			int o = (m_ch_offset) ? 2 : 0;
			switch (pr1)
			{
				case 1:		//Modulation wheel
					m_mod_wheel = (pr2 - 64) / 8;
					break;

				case 7:		//volume slider
					m_vol_slider = pr2 / 8;
					if (m_vol_slider == 0) m_vol_slider++;
					if (m_vol_slider > 15) m_vol_slider = 15;
					m_volume = m_vol_slider;
					break;

				case 115:	//LOOP key
					if (!pr2) break;	//no key press
					Play(MPLAY_TRACK, m_followplay, 0);
					break;

				case 116:	//STOP key
					if (!pr2) break;	//no key press
					Stop();
					break;

				case 117:	//PLAY key
					if (!pr2) break;	//no key press
					Play(MPLAY_SONG, m_followplay, 0);
					break;

				case 118:	//REC key
					if (!pr2) break;	//no key press
					//todo: call CRMTView Class functions directly instead of redundancy copypasta
					// PROVE_EDIT_MODE -> PROVE_JAM_MONO_MODE
					// PROVE_JAM_MONO_MODE -> PROVE_JAM_STEREO_MODE
					//					   -> PROVE_MIDI_CH15_MODE
					// PROVE_MIDI_CH15_MODE -> PROVE_EDIT_MODE
					if (g_prove == PROVE_EDIT_MODE) g_prove = PROVE_JAM_MONO_MODE;
					else if (g_prove == PROVE_MIDI_CH15_MODE) g_prove = PROVE_EDIT_MODE;			//disable the special MIDI test mode immediately
					else
					{
						if (g_prove == PROVE_JAM_MONO_MODE && g_tracks4_8 > 4)	//PROVE 2 only works for 8 tracks
							g_prove = PROVE_JAM_STEREO_MODE;
						else
							g_prove = PROVE_MIDI_CH15_MODE;						//special mode exclusive to MIDI CH15
					}
					break;

				case 123:
					if (!pr2) break;	//no key press
					Stop();
					//Atari_InitRMTRoutine();
					goto MIDISystemReset;
					break;

					//SPECIAL MIDI CH15 MODE
					if (g_prove == PROVE_MIDI_CH15_MODE)
					{
				case 71: //Knob C1, AUDF0/AUDF2 upper 4 bits
					//g_atarimem[0xD200] &= 0x0F;
					//g_atarimem[0xD200] |= pr2 << 4;
					g_atarimem[0x3178 + o] &= 0x0F;
					g_atarimem[0x3178 + o] |= pr2 << 4;

					//g_pokey.MemToPokey(g_tracks4_8);
					break;

				case 72: //Knob C2, AUDF1/AUDF3 upper 4 bits
					//g_atarimem[0xD202] &= 0x0F;
					//g_atarimem[0xD202] |= pr2 << 4;
					g_atarimem[0x3179 + o] &= 0x0F;
					g_atarimem[0x3179 + o] |= pr2 << 4;
					break;

				case 73: //Knob C3, AUDC0/AUDC2 volume
					//g_atarimem[0xD201] &= 0xF0;
					//g_atarimem[0xD201] |= pr2;
					g_atarimem[0x3180 + o] &= 0xF0;
					g_atarimem[0x3180 + o] |= pr2;
					break;

				case 74: //Knob C4, AUDC1/AUDC3 volume
					//g_atarimem[0xD203] &= 0xF0;
					//g_atarimem[0xD203] |= pr2;
					g_atarimem[0x3181 + o] &= 0xF0;
					g_atarimem[0x3181 + o] |= pr2;
					break;

				case 75: //Knob C5, AUDF0/AUDF2 lower 4 bits
					//g_atarimem[0xD200] &= 0xF0;
					//g_atarimem[0xD200] |= pr2;
					g_atarimem[0x3178 + o] &= 0xF0;
					g_atarimem[0x3178 + o] |= pr2;
					break;

				case 76: //Knob C6, AUDF1/AUDF3 lower 4 bits
					//g_atarimem[0xD202] &= 0xF0;
					//g_atarimem[0xD202] |= pr2;
					g_atarimem[0x3179 + o] &= 0xF0;
					g_atarimem[0x3179 + o] |= pr2;
					break;

				case 77: //Knob C7, AUDC0/AUDC2 distortion
					//g_atarimem[0xD201] &= 0x0F;
					//g_atarimem[0xD201] |= (pr2 * 2) << 4;
					g_atarimem[0x3180 + o] &= 0x0F;
					g_atarimem[0x3180 + o] |= (pr2 * 2) << 4;
					break;

				case 78: //Knob C8, AUDC1/AUDC3 distortion
					//g_atarimem[0xD203] &= 0x0F;
					//g_atarimem[0xD203] |= (pr2 * 2) << 4;
					g_atarimem[0x3181 + o] &= 0x0F;
					g_atarimem[0x3181 + o] |= (pr2 * 2) << 4;
					break;
					}

				default:
					//do nothing
					break;
			}
			return;	//finished, everything else will be ignored, unless it's using a different MIDI channel
		}

		if (cmd == 0x90 && chn == 9 && g_prove == PROVE_MIDI_CH15_MODE)
		{	//drumpads used to control the POKEY registers
			switch (pr1)
			{
				case 60:	//drumpad 1, toggle High Pass Filter in ch1+3
					if (!pr2) break;	//no key press
					g_atarimem[0x3C69] ^= 0x04;
					break;

				case 62:	//drumpad 2, toggle High Pass Filter in ch2+4
					if (!pr2) break;	//no key press
					g_atarimem[0x3C69] ^= 0x02;
					break;

				case 66:	//drumpad 3, toggle 1.79mHz mode in the respective channels
					if (!pr2) break;	//no key press
					g_atarimem[0x3C69] ^= (m_ch_offset) ? 0x20 : 0x40;
					break;

				case 70:	//drumpad 4, toggle Join 16-bit mode in the respective channels
					if (!pr2) break;	//no key press
					g_atarimem[0x3C69] ^= (m_ch_offset) ? 0x08 : 0x10;
					break;

				case 74:	//drumpad 5, select the POKEY channels 1 and 2 or 3 and 4
					if (!pr2) break;	//no key press
					if (m_ch_offset) m_ch_offset = 0;
					else  m_ch_offset = 1;
					break;

				case 69:	//drumpad 6, reset all AUDCTL and SKCTL bits
					if (!pr2) break;	//no key press
					g_atarimem[0x3CD3] = 0x03;	//SKCTL
					g_atarimem[0x3C69] = 0x00;	//AUDCTL
					break;

				case 75:	//drumpad 7, toggle Two-Tone filter
					if (!pr2) break;	//no key press
					if (g_atarimem[0x3CD3] == 0x03) g_atarimem[0x3CD3] = 0x8B;
					else g_atarimem[0x3CD3] = 0x03;
					break;

				case 73:	//drumpad 8, toggle 15kHz mode
					if (!pr2) break;	//no key press
					g_atarimem[0x3C69] ^= 0x01;
					break;

				default:
					//do nothing
					break;
			}
			return;
		}

		if (chn == 9) return;	//we do not want any of those MIDI events outside of the drumpads!!!

		//default notes input event, which is mostly copied from the CH0 code. This is a very terrible approach, and will eventually be replaced (see above)
		if (chn == 15 && g_prove != PROVE_MIDI_CH15_MODE)
		{

			int atc = m_heldkeys % g_tracks4_8;	//atari track 0-7 (resp. 0-3 in mono)

			if (cmd == 0x80) //key off
			{
				//key off
				int note = pr1 - 36 + m_mod_wheel;		//from the 3rd octave + modulation wheel offset
				//if (g_Midi.m_LastNoteOnChannel[atc] == note) //last key pressed on this midi channel
				//{
				m_heldkeys--;
				g_Midi.m_NoteVolumeOnChannel[atc] = 0;		//volume
				g_Midi.m_LastNoteOnChannel[atc] = -1;
				g_Midi.m_InstrumentOnChannel[atc] = m_activeinstr;		//instrument numbers
			//}
				if (m_heldkeys < 0) m_heldkeys = 0;
				return;
			}

			if (cmd == 0x90)
			{
				//key on
				int note = pr1 - 36 + m_mod_wheel;		//from the 3rd octave + modulation wheel offset
				int vol;

				//if (g_Midi.m_LastNoteOnChannel[atc] == note) //last key pressed on this midi channel
				m_heldkeys++;

				if (pr2 == 0)
				{
					if (!g_Midi.m_NoteOff) return;	//note off is not recognized
					vol = 0;			//keyoff
				}
				else
					if (g_Midi.m_TouchResponse)
					{
						vol = g_Midi.m_VolumeOffset + pr2 / 8;	//dynamics
						if (vol == 0) vol++;		//vol=1
						else
							if (vol > 15) vol = 15;

						m_volume = vol;
					}
					else
					{
						vol = m_volume;
					}

				if (note >= 0 && note < NOTESNUM)		//only within this range
				{
					if (g_activepart != PART_TRACKS || g_prove) goto Prove_midi_test;	//play notes but do not record them if the active screen is not TRACKS, or if any other PROVE combo is detected

					if (vol > 0)
					{
						//volume > 0 => write note
						//Quantization
						if (m_play && m_followplay && (m_speeda < (m_speed / 2)))
						{
							m_quantization_note = note;
							m_quantization_instr = m_activeinstr;
							m_quantization_vol = vol;
							g_Midi.m_LastNoteOnChannel[atc] = note; //see below
							g_Midi.m_NoteVolumeOnChannel[atc] = vol;		//volume
							g_Midi.m_InstrumentOnChannel[atc] = m_activeinstr;		//instrument numbers

						}	//end Q
						else
							if (TrackSetNoteInstrVol(note, m_activeinstr, vol))
							{
								BLOCKDESELECT;
								g_Midi.m_LastNoteOnChannel[atc] = note; //last key pressed on this midi channel
								g_Midi.m_NoteVolumeOnChannel[atc] = vol;		//volume
								g_Midi.m_InstrumentOnChannel[atc] = m_activeinstr;		//instrument numbers
								if (g_respectvolume)
								{
									int v = TrackGetVol();
									if (v >= 0 && v <= MAXVOLUME) vol = v;
								}
								goto NextLine_midi_test;
							}
					}
					else
					{
						//volume = 0 => noteOff => delete note and write only volume 0
						if (g_Midi.m_LastNoteOnChannel[atc] == note) //is it really the last one pressed?
						{
							if (m_play && m_followplay && (m_speed < (m_speed / 2)))
							{
								m_quantization_note = -2;
							}
							else
								if (TrackSetNoteActualInstrVol(-1) && TrackSetVol(0))
									goto NextLine_midi_test;
						}
					}

					if (0) //inside jumps only through goto
					{
					NextLine_midi_test:
						if (!(m_play && m_followplay)) TrackDown(g_linesafter);	//scrolls only when there is no followplay
					Prove_midi_test:
						//SetPlayPressedTonesTNIV(m_trackactivecol, note, m_activeinstr, vol);
						SetPlayPressedTonesTNIV(atc, note, m_activeinstr, vol);
						//	if ((g_prove == PROVE_JAM_STEREO_MODE || g_controlkey) && g_tracks4_8 > 4)
						//	{	//with control or in prove2 => stereo test
						//		SetPlayPressedTonesTNIV((m_trackactivecol + 4) & 0x07, note, m_activeinstr, vol);
						//	}
					}
				}
			}

		} ////
		else //notes (soon...)
		{
			int note = pr1 /* - 36 + m_mod_wheel */;			//direct MIDI note mapping, for easier tests, else the older comment applies -> //from the 3rd octave + modulation wheel offset
			int vol = m_volume;									//direct volume value taken from the one of active instrument in memory, controlled by the volume slider
			int track = 0;										//m_trackactivecol is the active channel to map, so for tests simply moving the cursor should do the trick

			char midi_audctl = 0x00;							//AUDCTL without any special effect, default 64khz clock
			char midi_audc = 0x00;								//AUDC, for the Distortion and Volume
			char midi_audf = 0x00;								//AUDF, for the frequency 

			if (note > 63) return;								//crossing the boundary of the older table, so let's ignore it for now

			if (cmd == 0xc0)
			{
				m_midi_distortion = (pr1 % 8) * 2;
				return;
			}

			//MIDI NOTE OFF events
			if (cmd == 0x80)
			{
				m_heldkeys--;
				if (m_heldkeys < 0) m_heldkeys = 0;				//if by any mean the count is desynced, force it to be 0
				track = (m_trackactivecol + m_heldkeys) % 4;	//offset to the previous channel
				for (int i = 0; i < 4; i++)
				{
					if (note == g_Midi.m_LastNoteOnChannel[i])
					{
						track = i;	//if there is a match the correct channel will be used 
						g_Midi.m_LastNoteOnChannel[track] = -1;						//note
						g_Midi.m_NoteVolumeOnChannel[track] = 0;						//volume
						g_Midi.m_InstrumentOnChannel[track] = 0;						//instrument numbers
						break;
					}
				}
			}

			//MIDI NOTE ON events
			if (cmd == 0x90)
			{
				track = (m_trackactivecol + m_heldkeys) % 4;
				m_heldkeys++;
				for (int i = 0; i < 4; i++)
				{
					if (g_Midi.m_LastNoteOnChannel[i] == -1)
					{
						track = i;	//if there is a match the first empty channel found will be used
						g_Midi.m_LastNoteOnChannel[track] = note;					//note
						g_Midi.m_NoteVolumeOnChannel[track] = vol;						//volume
						g_Midi.m_InstrumentOnChannel[track] = m_midi_distortion;		//instrument numbers to set the Distortion lol
						break;
					}
				}
			}

			//TESTING HARDCODED DATA, THIS MUST NOT BE THE WAY TO GO!
			//COMMENT THIS ENTIRE BLOCK OUT ONCE A PROPER INPUT HANDLER IS ADDED TO TAKE ALL THE PARAMETERS INTO ACCOUNT
			//

			//midi_audf = g_atarimem[0xB100 + note];		//Distortion A 64khz frequency directly loaded from the generated table in memory
			midi_audc |= g_Midi.m_InstrumentOnChannel[track] << 4;			//force Distortion based on instrument to AUDC
			midi_audctl = g_atarimem[0x3C69];

			bool CLOCK_15 = midi_audctl & 0x01;
			bool HPF_CH24 = midi_audctl & 0x02;
			bool HPF_CH13 = midi_audctl & 0x04;
			bool JOIN_34 = midi_audctl & 0x08;
			bool JOIN_12 = midi_audctl & 0x10;
			bool CH3_179 = midi_audctl & 0x20;
			bool CH1_179 = midi_audctl & 0x40;
			bool POLY9 = midi_audctl & 0x80;
			//bool TWO_TONE = (skctl == 0x8B) ? 1 : 0;

			//combined modes for some special output...
			bool JOIN_16BIT = ((JOIN_12 && CH1_179 && (track == 1 || track == 5)) || (JOIN_34 && CH3_179 && (track == 3 || track == 7))) ? 1 : 0;
			bool CLOCK_179 = ((CH1_179 && (track == 0 || track == 4)) || (CH3_179 && (track == 2 || track == 6))) ? 1 : 0;
			if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode

			if (CH1_179 && CH3_179)
			{
				//force only valid 1.79mhz channels even if the current track doesn't support it, if both are enabled but not in the right channel
				if (track > 0 && track < 2) track = 2;
				else if (track > 2) track = 0;
				CLOCK_179 = 1;
			}

			//what is the distortion? must be known to set the right note table
			switch (midi_audc & 0xF0)
			{
				case 0x00:
					goto case_default;
					break;

				case 0x20:
				case 0x60:
					if (CLOCK_179)
					{
						midi_audf = g_atarimem[0xB040 + note];
					}
					else if (CLOCK_15)
						goto case_default;
					else
						midi_audf = g_atarimem[0xB000 + note];
					break;

				case 0x40:
					goto case_default;
					break;

				case 0x80:
					goto case_default;
					break;

				case 0xC0:
					if (CLOCK_179)
						midi_audf = g_atarimem[0xB240 + note];
					else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
						midi_audf = g_atarimem[0xB4C0 + note];
					else
						midi_audf = g_atarimem[0xB200 + note];
					break;

				case 0xE0:
					midi_audc = (char)0xC0;	//Distortion C bass E
					if (CLOCK_179)
						midi_audf = g_atarimem[0xB340 + note];
					else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
						midi_audf = g_atarimem[0xB4C0 + note];
					else
						midi_audf = g_atarimem[0xB300 + note];
					break;

				case 0xA0:
				default:
				case_default:
					if (CLOCK_179)
						midi_audf = g_atarimem[0xB140 + note];
					else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
						midi_audf = g_atarimem[0xB480 + note];
					else
						midi_audf = g_atarimem[0xB100 + note];
					break;
			}

			midi_audc |= g_Midi.m_NoteVolumeOnChannel[track];				//also merge the volume into it

			//DIRECT MEMORY WRITE
			//g_atarimem[0x3C69] = midi_audctl;				//AUDCTL address used by SetPokey
			g_atarimem[0x3178 + track] = midi_audf;			//AUDF address + offset used by SetPokey
			g_atarimem[0x3180 + track] = midi_audc;			//AUDC address + offset used by SetPokey

			//
			//END OF HARDCODED TEST
		}
	}

	//The following is performed only on channel 0 (chn = 0), and is the defacto notes input in tracks. 
	//This is also the only mode that is specifically using the old code, and is technically legacy for compatibility reasons.
	if (chn == 0)
	{

		if (cmd == 0x90)
		{
			//key on/off
			int note = pr1 - 36;		//from the 3rd octave
			int vol;
			if (pr2 == 0)
			{
				if (!g_Midi.m_NoteOff) return;	//note off is not recognized
				vol = 0;			//keyoff
			}
			else
				if (g_Midi.m_TouchResponse)
				{
					vol = g_Midi.m_VolumeOffset + pr2 / 8;	//dynamics
					if (vol == 0) vol++;		//vol=1
					else
						if (vol > 15) vol = 15;

					m_volume = vol;
				}
				else
				{
					vol = m_volume;
				}

			if (note >= 0 && note < NOTESNUM)		//only within this range
			{
				if (g_activepart != PART_TRACKS || g_prove) goto Prove_midi;	//play notes but do not record them if the active screen is not TRACKS, or if any other PROVE combo is detected

				if (vol > 0)
				{
					//volume > 0 => write note
					//Quantization
					if (m_play && m_followplay && (m_speeda < (m_speed / 2)))
					{
						m_quantization_note = note;
						m_quantization_instr = m_activeinstr;
						m_quantization_vol = vol;
						g_Midi.m_LastNoteOnChannel[chn] = note; //see below
					}	//end Q
					else
						if (TrackSetNoteInstrVol(note, m_activeinstr, vol))
						{
							BLOCKDESELECT;
							g_Midi.m_LastNoteOnChannel[chn] = note; //last key pressed on this midi channel
							if (g_respectvolume)
							{
								int v = TrackGetVol();
								if (v >= 0 && v <= MAXVOLUME) vol = v;
							}
							goto NextLine_midi;
						}
				}
				else
				{
					//volume = 0 => noteOff => delete note and write only volume 0
					if (g_Midi.m_LastNoteOnChannel[chn] == note) //is it really the last one pressed?
					{
						if (m_play && m_followplay && (m_speed < (m_speed / 2)))
						{
							m_quantization_note = -2;
						}
						else
							if (TrackSetNoteActualInstrVol(-1) && TrackSetVol(0))
								goto NextLine_midi;
					}
				}

				if (0) //inside jumps only through goto
				{
				NextLine_midi:
					if (!(m_play && m_followplay)) TrackDown(g_linesafter);	//scrolls only when there is no followplay
				Prove_midi:
					SetPlayPressedTonesTNIV(m_trackactivecol, note, m_activeinstr, vol);
					if ((g_prove == PROVE_JAM_STEREO_MODE) && g_tracks4_8 > 4)
					{	//with control or in prove2 => stereo test
						SetPlayPressedTonesTNIV((m_trackactivecol + 4) & 0x07, note, m_activeinstr, vol);
					}
				}
			}
		}

	}
	else
		if (cmd == 0xc0)
		{
			//prg change
			if (pr1 >= 0 && pr1 < INSTRSNUM)
			{
				ActiveInstrSet(pr1);
			}
		}
}
