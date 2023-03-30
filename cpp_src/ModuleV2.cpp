// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"
#include "Atari6502.h"

CModule::CModule()
{
	m_index = NULL;
	m_instrument = NULL;
	InitialiseModule();
}

CModule::~CModule()
{
	ClearModule();
}

void CModule::InitialiseModule()
{
	// Create new module data if it doesn't exist
	if (!m_index) m_index = new TIndex[TRACK_CHANNEL_MAX];
	if (!m_instrument) m_instrument = new TInstrumentV2[PATTERN_INSTRUMENT_MAX];

	// Clear all data, and set default values
	for (int i = 0; i < TRACK_CHANNEL_MAX; i++)
	{
		// Set all indexed Patterns to 0
		for (int j = 0; j < SONGLINE_MAX; j++)
			m_index[i].songline[j] = 0;

		// Set all indexed Rows in Patterns to empty values
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			for (int k = 0; k < TRACK_ROW_MAX; k++)
			{
				m_index[i].pattern[j].row[k].note = PATTERN_NOTE_EMPTY;
				m_index[i].pattern[j].row[k].instrument = PATTERN_INSTRUMENT_EMPTY;
				m_index[i].pattern[j].row[k].volume = PATTERN_VOLUME_EMPTY;
				m_index[i].pattern[j].row[k].cmd0 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd1 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd2 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd3 = PATTERN_EFFECT_EMPTY;
			}
		}

		// By default, only 1 Effect Command is enabled in all Track Channels
		m_index[i].activeEffectCommand = 1;
	}

	// Also clear all instruments in the module
	for (int i = 0; i < PATTERN_INSTRUMENT_MAX; i++)
	{
		strcpy(m_instrument[i].name, "New Instrument");
		m_instrument[i].envelopeLength = 1;
		m_instrument[i].envelopeLoop = 0;
		m_instrument[i].envelopeRelease = 0;
		m_instrument[i].tableLength = 0;
		m_instrument[i].tableLoop = 0;
		m_instrument[i].tableRelease = 0;
		m_instrument[i].tableMode = 0;

		for (int j = 0; j < ENVELOPE_INDEX_MAX; j++)
		{
			m_instrument[i].volumeEnvelope[j] = 0;
			m_instrument[i].distortionEnvelope[j] = 0;
			m_instrument[i].audctlEnvelope[j] = 0;
		}

		for (int j = 0; j < INSTRUMENT_TABLE_INDEX_MAX; j++)
		{
			m_instrument[i].noteTable[j] = 0;
			m_instrument[i].freqTable[j] = 0;
		}
	}

	// Set default module parameters
	strcpy(m_songName, "Noname Song");
	m_songLength = MODULE_SONG_LENGTH;
	m_trackLength = MODULE_TRACK_LENGTH;
	m_channelCount = MODULE_STEREO;

	// Module was initialised
	m_initialised = TRUE;
}

void CModule::ClearModule()
{
	// Don't waste any time, just destroy the module data entirely
	if (m_index) delete m_index;
	if (m_instrument) delete m_instrument;

	// Set the struct pointers to NULL to make sure nothing invalid is accessed
	m_index = NULL;
	m_instrument = NULL;

	// Module was cleared, and must be initialised before it could be used again
	m_initialised = FALSE;
}

void CModule::ImportLegacyRMT(std::ifstream& in)
{
	//BYTE data;
	WORD fromAddr, toAddr;

	BYTE mem[65536];
	memset(mem, 0, 65536);

	if (!LoadBinaryBlock(in, mem, fromAddr, toAddr))
	{
		MessageBox(g_hwnd, "Corrupted RMT module, or incompatible format version.", "Import error", MB_ICONERROR);
		return;
	}

	// Check that the header starts with "RMT"
	if (strncmp((char*)(mem + fromAddr), "RMT", 3) != 0)
	{
		MessageBox(g_hwnd, "Corrupted RMT module, or incompatible format version.", "Import error", MB_ICONERROR);
		return;
	}

	// Clear the current module data
	InitialiseModule();

	// 4th byte: # of channels (4 or 8)
	m_channelCount = mem[fromAddr + 3] & 0x0F;

	// 5th byte: track length
	m_trackLength = mem[fromAddr + 4];

	// 6th byte: song speed
	m_songSpeed = mem[fromAddr + 5];

	// 7th byte: Instrument speed
	m_instrumentSpeed = mem[fromAddr + 6];

	// 8th byte: RMT format version nr.
	//BYTE version = mem[fromAddr + 7];

	// Get various pointers
	WORD ptrInstruments = mem[fromAddr + 8] + (mem[fromAddr + 9] << 8);		// Get ptr to the instuments
	WORD ptrTracksLow = mem[fromAddr + 10] + (mem[fromAddr + 11] << 8);		// Get ptr to tracks table low
	WORD ptrTracksHigh = mem[fromAddr + 12] + (mem[fromAddr + 13] << 8);	// Get ptr to tracks table high
	WORD ptrSong = mem[fromAddr + 14] + (mem[fromAddr + 15] << 8);			// Get ptr to track list (the song)

	// Calculate how long each of the sections are
	WORD numInstruments = (ptrTracksLow - ptrInstruments) / 2;				// Number of instruments
	WORD numTracks = ptrTracksHigh - ptrTracksLow;							// Number of tracks
	WORD lengthSong = toAddr + 1 - ptrSong;									// Number of songlines

	// Decoding of individual instruments
	// Instruments from RMT V0 only have few differences, so decoding its data using the same procedure might be possible(?)
	for (int i = 0; i < numInstruments; i++)
	{
		// Get the pointer to Instrument envelope and parameters
		WORD ptrOneInstrument = mem[ptrInstruments + i * 2] + (mem[ptrInstruments + i * 2 + 1] << 8);

		// If it is a NULL pointer, in this case, an offset of 0, the instrument is empty, and must be skipped
		if (!ptrOneInstrument)
			continue;

		BYTE* memInstrument = mem + ptrOneInstrument;

		// Get the equivalent RMTE Instrument number
		TInstrumentV2* ai = GetInstrument(i);

		// Get the Envelopes, Tables, and other parameters from the original RMT instrument
		BYTE envelopePtr = memInstrument[0];
		BYTE tableLength = memInstrument[0] - 12;
		BYTE tableGoto = memInstrument[1] - 12;
		BYTE envelopeLength = (memInstrument[2] - (memInstrument[0] + 1)) / 3;
		BYTE envelopeGoto = (memInstrument[3] - (memInstrument[0] + 1)) / 3;
		BYTE tableType = memInstrument[4] >> 7;				// Table Type, 0 = Note, 1 = Freq
		//BYTE tableMode = (memInstrument[4] >> 6) & 0x01;	// Table Mode, 0 = Set, 1 = Additive, not supported by the RMTE format
		//BYTE tableSpeed = memInstrument[4] & 0x3F;		// Table Speed, used to offset the equivalent Tables
		BYTE initialAudctl = memInstrument[5];				// AUDCTL, used to initialise the equivalent Envelope
		//BYTE volumeSlide = memInstrument[6];				// Volume Slide, not supported by the RMTE format
		//BYTE volumeMinimum = memInstrument[7] >> 4;		// Volume Minimum, not supported by the RMTE format
		//BYTE vibratoDelay = memInstrument[8];				// Vibrato/Freq Shift Delay, not supported by the RMTE format
		//BYTE vibrato = memInstrument[9] & 0x03;			// Vibrato, not supported by the RMTE format
		//BYTE freqShift = memInstrument[10];				// Freq Shift, not supported by the RMTE format

		// Adjust the Tables length and loop point using the Table Speed parameter
		// Since the size could go as high as 2048 frames, it might be a better idea to simply add a Speed parameter as well...
		//tableLength *= tableSpeed;	// Not decided yet
		//tableGoto *= tableSpeed;	// Not decided yet

		// Set the equivalent data to the RMTE instrument, with respect to boundaries
		ai->tableLength = tableLength < INSTRUMENT_TABLE_INDEX_MAX ? tableLength : 0;	//INVALID	// Not decided yet
		ai->tableLoop = tableGoto <= tableLength ? tableGoto : 0;	//INVALID	// Not decided yet
		ai->envelopeLength = envelopeLength < ENVELOPE_MAX_COLUMNS ? envelopeLength : 0;	//INVALID	// Not decided yet
		ai->envelopeLoop = envelopeGoto > envelopeLength ? envelopeGoto : 0;	//INVALID	// Not decided yet

		// Fill the equivalent RMTE tables based on the tableMode parameter
		for (int j = 0; j <= ai->tableLength; j++)
		{
			ai->noteTable[j] = tableType ? memInstrument[12 + j] : 0;
			ai->freqTable[j] = tableType ? memInstrument[12 + j] : 0;
		}

		// Fill the equivalent RMTE envelopes, which might include some compromises due to the format differences
		for (int j = 0; j <= ai->envelopeLength; j++)
		{
			// Get the 3 bytes used by the original RMT Envelope format
			BYTE envelopeVolume = memInstrument[(memInstrument[0] + 1) + (i * 3)];
			BYTE envelopeCommand = memInstrument[(memInstrument[0] + 1) + (i * 3) + 1];
			BYTE envelopeParameter = memInstrument[(memInstrument[0] + 1) + (i * 3) + 2];
			BYTE envelopeEffectCommand = (envelopeCommand >> 4) & 0x07;

			// The Envelope Effect Command is used for compatibility tweaks, which may or may not provide perfect results
			switch (envelopeEffectCommand)
			{
			case 0x07:	// Overwrite the initialAudctl parameter, for a pseudo AUDCTL envelope when it is used multiple times (Patch16 only)
				initialAudctl = envelopeParameter;
				break;
			}

			// Envelope Volume, only Left POKEY volume is supported by the RMTE format
			ai->volumeEnvelope[j] = envelopeVolume & 0x0F;

			// Envelope Distortion, from 0 to E
			ai->audctlEnvelope[j] = envelopeCommand & 0x0E;

			// Envelope AUDCTL
			ai->audctlEnvelope[j] = initialAudctl;

			// Autofilter, not supported (yet?) by the RMTE format
			//ai->autofilterEnvelope[j] = envelopeCommand >> 7;

			// Instrument Command, not supported (yet?) by the RMTE format
			//ai->commandEnvelope[j] = (envelopeCommand >> 4) & 0x07;

			// Portamento, not supported (and unlikely to be) by the RMTE format
			//ai->portamentoEnvelope[j] = envelopeCommand & 0x01;
		}
	}

	BYTE patternCount = 0;

	// Decoding individual tracks
	for (int i = 0; i < numTracks; i++)
	{
		// Track data pointers are split over two tables, low and high bytes, each indexed by the track number itself
		int ptrOneTrack = mem[ptrTracksLow + i] + (mem[ptrTracksHigh + i] << 8);

		// If it is a NULL pointer, in this case, an offset of 0, the track is empty, and must be skipped
		if (!ptrOneTrack)
			continue;

		// Track index number being processed, needed in order to copy data to all indexed songlines
		int trackNr = i;

		// Identify the end of the track by comparing the starting address of the next track, or the song data pointer if all tracks were processed
		int ptrTrackEnd = 0;

		for (int j = i; j < numTracks; j++)
		{
			// Get the next track's address, unless it is the last track, which will use the Song pointer address due to not having a definite end
			if ((ptrTrackEnd = (j + 1 == numTracks) ? ptrSong : mem[ptrTracksLow + j + 1] + (mem[ptrTracksHigh + j + 1] << 8)))
				break;

			// if the next track is empty, skip over it and continue seeking for the address until something is found
			i++;
		}

		int trackLength = ptrTrackEnd - ptrOneTrack;

		BYTE* memTrack = mem + ptrOneTrack;

		TPattern* t = GetPattern(0, trackNr);

		WORD src = 0;
		WORD gotoIndex = INVALID;
		WORD smartLoop = INVALID;

		BYTE data, count;
		BYTE line = 0;

		// Minimal Track Length for a smart loop
		if (trackLength >= 2)
		{
			// There is a smart loop at the end of the track
			if (memTrack[trackLength - 2] == 0xBF)
				gotoIndex = memTrack[trackLength - 1];
		}

		while (src < trackLength)
		{
			// Jump to gotoIndex => set smart loop to this line
			if (src == gotoIndex)
				smartLoop = line;

			// Data to process at current track offset
			data = memTrack[src] & 0x3F;

			switch (data)
			{
			case 0x3D:	// Have Volume only on this row
				t->row[line].volume = ((memTrack[src + 1] & 0x03) << 2) | ((memTrack[src] & 0xC0) >> 6);
				src += 2;	// 2 bytes were processed
				line++;	// 1 row was processed
				break;

			case 0x3E:	// Pause or empty row
				count = memTrack[src] & 0xC0;
				if (!count)
				{
					// Pause is 0 then the number of rows to skip is in the next byte
					if (!memTrack[src + 1])
					{
						// Infinite pause => end
						src = trackLength;
						break;
					}
					line += memTrack[src + 1];	// Number of rows to skip
					src += 2;	// 2 bytes were processed
				}
				else
				{
					line += (count >> 6);	// Upper 2 bits directly specify a pause between 1 to 3 rows
					src++;	// 1 byte was processed
				}
				break;

			case 0x3F:	// Speed, smart loop, or end
				count = memTrack[src] & 0xC0;
				if (!count)
				{
					// Speed, set Fxx command
					t->row[line].cmd0 = 0x0F00 | memTrack[src + 1];
					src += 2;	// 2 bytes were processed
				}
				if (count == 0x80)
				{
					// Smart loop, no extra data to process
					src = trackLength;
				}
				if (count == 0xC0)
				{
					// End of track, set Dxx command, no extra data to process
					t->row[line - 1].cmd1 = 0x0D00;
					src = trackLength;
				}
				break;

			default:	// Have Note, Instrument and volume data on this line
				t->row[line].note = data;
				t->row[line].instrument = ((memTrack[src + 1] & 0xfc) >> 2);
				t->row[line].volume = ((memTrack[src + 1] & 0x03) << 2) | ((memTrack[src] & 0xc0) >> 6);
				src += 2;	// 2 bytes were processed
				line++;	// 1 row was processed
			}
		}

		// The Pattern must to be "expanded" to be fully compatible since no such thing as smart loop is supported by the RMTE format (yet)
		if (IsValidRow(smartLoop))
		{
			for (int j = 0; line + j < m_trackLength; j++)
			{
				int k = line + j;
				int l = smartLoop + j;
				t->row[k].note = t->row[l].note;
				t->row[k].instrument = t->row[l].instrument;
				t->row[k].volume = t->row[l].volume;
				t->row[k].cmd0 = t->row[l].cmd0;
			}
		}

		// Increment the total count of patterns that were imported for the following blocks below 
		patternCount++;
	}

	// Variables for processing songline data
	BYTE* memSong = mem + ptrSong;
	BYTE channel = 0;
	BYTE line = 0;
	WORD src = 0;

	// This will provide access to the Module Index directly
	TIndex* index = GetIndex();

	// Decoding Songlines Index
	while (src < lengthSong)
	{
		BYTE data = memSong[src];

		// Process the Pattern index in the songline indexed by the channel number
		switch (data)
		{
		case 0xFE:	// Goto Songline commands are only valid from the first channel, but we know it's never used anywhere else
			index[channel].songline[line] = data;
			index[channel + 1].songline[line] = memSong[src + 1];	// Set the songline index number in Channel 2
			index[channel + 2].songline[line] = INVALID;	// The Goto songline address isn't needed
			index[channel + 3].songline[line] = INVALID;	// Set the remaining channels to INVALID
			channel = m_channelCount;	// Set the channel index to the channel count to trigger the condition below
			src += m_channelCount;	// The number of bytes processed is equal to the number of channels
			break;

		default:	// An empty pattern at 0xFF is also valid for the RMTE format
			index[channel].songline[line] = data;
			channel++;	// 1 pattern per channel, for each indexed songline
			src++;	// 1 byte was processed
		}

		// 1 songline was processed when the channel count is equal to the number of channels
		if (channel >= m_channelCount)
		{
			channel = 0;	// Reset the channel index
			line++;	// Increment the songline count by 1
		}
		
		// Break out of the loop if the maximum number of songlines was processed
		if (line > SONGLINE_MAX)
			break;
	}

	// Set the songlength to the number of decoded songlines
	m_songLength = line;


	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Re-organise the Songline and Pattern index due to the differences with the RMTE format
	// TODO: cleanup, split to more simple generic functions that could be useful for other things later...
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	

	// Copy all of the imported patterns to every channels, so they all share identical data
	for (int i = 0; i < m_channelCount; i++)
	{
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			TPattern* source = GetPattern(0, j);
			TPattern* destination = GetPattern(i, j);
			*destination = *source;
		}
	}

	// Attempt to find the Goto Songline Commands and replace them with Bxx commands in the last Row found in the Songline just before it...
	for (int j = 0; j < m_songLength; j++)
	{
		for (int i = 0; i < m_channelCount; i++)
		{
			BYTE pattern = index[i].songline[j];
			
			// Goto Songline Command, valid only in Songlines above 0, and Channel 1
			if (i == 0 && j > 0 && pattern == 0xFE)
			{
				BYTE sourcePattern = index[i].songline[j - 1];
				BYTE destinationPattern = index[i + 1].songline[j];
				//BYTE effectiveTracklength = m_trackLength - 1;
				BYTE effectiveTracklength = GetShortestPatternLength(j - 1);
				bool isUsedAlready = false;

				// Search for patterns that were previously used in Channel 1
				for (int k = 0; k < m_songLength; k++)
				{
					// Ignore the same Songline, and the Songline before it, both the Goto Songline Command and the Pattern itself were loaded from them 
					if (k == j || k == j - 1)
						continue;

					// At least another instance of the same pattern was used, flag it to know it's already used
					if (index[i].songline[k] == sourcePattern)
					{
						isUsedAlready = true;
						break;
					}
				}

				// If the same pattern was used anywhere else for this Channel, duplicate it to a new unused pattern index
				if (isUsedAlready)
				{
					// Duplicate the source pattern to the index of the pattern count as a new pattern index
					TPattern* source = GetPattern(i, sourcePattern);
					TPattern* destination = GetPattern(i, patternCount);
					*destination = *source;

					// Assign the new pattern index to the original Songline Index, effectively replacing it
					index[i].songline[j - 1] = patternCount;

					// Increment the total number of patterns counted once this was done
					patternCount++;
				}

/*
				// Identify the shortest pattern length relative to the ones used in the same Songline
				for (int k = 0; k < m_channelCount; k++)
				{
					// Get the Pattern Index in the Songline before the Goto Songline Command
					BYTE effectivePattern = index[k].songline[j - 1];

					// Check for each Row that could be used within the Effective Tracklength
					for (int l = 0; l < effectiveTracklength; l++)
					{
						// Get the Effect Command Identifier value
						BYTE effectiveCommand = index[k].pattern[effectivePattern].row[l].cmd1 >> 8;

						// If a match is found for the CMD Dxx, set the Effective Tracklength to the current Row Index
						if (effectiveCommand == 0x0D)
						{
							// Only if the Effective Tracklength is not already the shortest common Tracklength, otherwise, it will be ignored
							if (effectiveTracklength >= l)
							{
								effectiveTracklength = l;
								break;
							}
						}
					}
				}
*/

				// Set a Bxx Command, in the Channel 1's Pattern Index, on the Row relative to Effective Tracklength, using the target Songline as its parameter
				sourcePattern = index[i].songline[j - 1];
				index[i].pattern[sourcePattern].row[effectiveTracklength].cmd2 = 0x0B00 | destinationPattern;

				// Finally, set all the Patterns Index found in the Songline from which the Goto Songline was found to Empty, Since they are no longer needed here
				for (int k = 0; k < m_channelCount; k++)
				{
					index[k].songline[j] = INVALID;
				}
			}
		}
	}

	// What could be done right now?

}


// ----------------------------------------------------------------------------
// Functions related to Pattern Data editing, aimed at bulk operations and/or repetitive procedures
// These are broken down into generic functions, to make recycling for other uses fairly easy
//


// Identify the shortest pattern length relative to the other ones used in the same Songline
BYTE CModule::GetShortestPatternLength(BYTE songline)
{
	// Subtract 1 to offset to the last Row Index actually used
	BYTE patternLength = GetTrackLength() - 1;

	// All channels will be processed in order to identify the shortest Pattern
	for (int i = 0; i < GetChannelCount(); i++)
	{
		// Get the Pattern Index used in the Songline first
		BYTE pattern = GetPatternInSongline(i, songline);

		// Check for each Row that could be used within the shortest Pattern
		for (int j = 0; j < patternLength; j++)
		{
			// Check for all Effect Commands used in each Row
			for (int k = 0; k < PATTERN_ACTIVE_EFFECT_MAX; k++)
			{
				// Get the Effect Command Identifier, the Parameter is not needed here
				BYTE command = GetPatternRowCommand(i, pattern, j, k) >> 8;

				// Set the Pattern Length to the current Row Index if a match is found
				if (command == 0x0B || command == 0x0D)
				{
					if (patternLength >= j)
						patternLength = j;
				}
			}
		}
	}

	// The shortest Pattern Length will be returned if successful
	return patternLength;
}
