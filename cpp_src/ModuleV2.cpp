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
	if (!m_index) m_index = new TSubtune[SUBTUNE_MAX];
	if (!m_instrument) m_instrument = new TInstrumentV2[PATTERN_INSTRUMENT_MAX];

	// Set default module parameters
	SetSongName("Noname Song");
	SetSubtuneName(MODULE_DEFAULT_SUBTUNE, "");
	SetActiveSubtune(MODULE_DEFAULT_SUBTUNE);
	SetSubtuneCount(MODULE_SUBTUNE_COUNT);
	SetSongLength(MODULE_SONG_LENGTH);
	SetPatternLength(MODULE_TRACK_LENGTH);
	SetChannelCount(MODULE_STEREO);
	SetSongSpeed(MODULE_SONG_SPEED);
	SetInstrumentSpeed(MODULE_VBI_SPEED);

	// Clear all data, and set default values
	for (int i = 0; i < TRACK_CHANNEL_MAX; i++)
	{
		// Set all indexed Patterns to 0
		for (int j = 0; j < SONGLINE_MAX; j++)
			SetPatternInSongline(i, j, 0);

		// Set all indexed Rows in Patterns to empty values
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
			ClearPattern(GetPattern(i, j));

		// By default, only 1 Effect Command is enabled in all Track Channels
		SetEffectCommandCount(i, 1);
	}

	// Clear all Subtunes using the Active Subtune values
	for (int i = 1; i < SUBTUNE_MAX; i++)
	{
		CopySubtune(GetSubtuneIndex(GetActiveSubtune()), GetSubtuneIndex(i));
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

	// Module was initialised
	SetModuleStatus(TRUE);
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
	SetModuleStatus(FALSE);
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
	SetChannelCount(mem[fromAddr + 3] & 0x0F);

	// 5th byte: track length
	SetPatternLength(mem[fromAddr + 4]);

	// 6th byte: song speed
	SetSongSpeed(mem[fromAddr + 5]);

	// 7th byte: Instrument speed
	SetInstrumentSpeed(mem[fromAddr + 6]);

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
			for (int j = 0; line + j < GetPatternLength(); j++)
			{
				int k = line + j;
				int l = smartLoop + j;
				t->row[k].note = t->row[l].note;
				t->row[k].instrument = t->row[l].instrument;
				t->row[k].volume = t->row[l].volume;
				t->row[k].cmd0 = t->row[l].cmd0;
			}
		}
	}

	// Variables for processing songline data
	BYTE* memSong = mem + ptrSong;
	BYTE channel = 0;
	BYTE line = 0;
	WORD src = 0;

	// This will provide access to the Module Index directly, for the currently Active Subtune
	TIndex* index = GetChannelIndex();

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
			channel = GetChannelCount();	// Set the channel index to the channel count to trigger the condition below
			src += GetChannelCount();	// The number of bytes processed is equal to the number of channels
			break;

		default:	// An empty pattern at 0xFF is also valid for the RMTE format
			index[channel].songline[line] = data;
			channel++;	// 1 pattern per channel, for each indexed songline
			src++;	// 1 byte was processed
		}

		// 1 songline was processed when the channel count is equal to the number of channels
		if (channel >= GetChannelCount())
		{
			channel = 0;	// Reset the channel index
			line++;	// Increment the songline count by 1
		}

		// Break out of the loop if the maximum number of songlines was processed
		if (line > SONGLINE_MAX)
			break;
	}

	// Set the Songlength to the number of decoded songlines
	SetSongLength(line);

	// Copy all of the imported patterns to every Channels, so they all share identical data for the next part
	for (int i = 0; i < GetChannelCount(); i++)
	{
		// The Songline Index won't be overwritten in the process, since we will need it in its current form!
		DuplicatePatternIndex(0, i);
	}

	// Ideally... Right "here" would be the best place to actually "Renumber Patterns", so they become all unique entries
	// Unfortunately, the procedure involved would apply changes to everything, regardless of the data we might want to use
	// The compromise is simply... to not bother with that, because all Patterns will be re-ordered after this part anyway

	// Workaround: due to the Songline 00 data causing some conflicts in the Subtune detection procedure, manually set it before the bigger optimisations
	BYTE startSongline = 0;

	// Search for Goto Songline Commands in Channel 1, and replace them with Bxx commands on the previous Songline
	for (int i = 0; i < GetSongLength(); i++)
	{
		//If a Goto Songline "Pattern" is found, process further
		if (GetPatternInSongline(CH1, i) == 0xFE)
		{
			// Special case for Songline 00: its Destination Songline will be used as the Start Songline, to simulate the effects of the original Goto Songline Command
			if (i == 0)
				startSongline = GetPatternInSongline(CH2, i);
			
			// Otherwise, follow the same procedure for everything else
			else
			{
				// Duplicate the Pattern before editing, just in case it was used multiple times in the Songline Index
				DuplicatePatternInSongline(CH1, i - 1, GetPatternInSongline(CH1, i - 1));

				// Set the Bxx Command on the last Row Index relative to the shortest Pattern length, using the destination Songline as the Command Parameter
				SetPatternRowCommand(CH1, GetPatternInSongline(CH1, i - 1), GetShortestPatternLength(i - 1) - 1, CMD2, PATTERN_EFFECT_BXX | GetPatternInSongline(CH2, i));
			}

			// Finally, set all the Patterns Index found in the Songline from which the Goto Songline was found to Empty, Since they are no longer needed here
			for (int j = 0; j < GetChannelCount(); j++)
			{
				SetPatternInSongline(j, i, INVALID);
			}
		}
	}

	// Since the last Songline is always used as a Goto Songline command, Set the Songlength again to the number of decoded lines, minus 1 this time
	SetSongLength(line - 1);

	// Re-arrange all Patterns to make them unique entries for every Songline, so editing them will not overwrite anything intended to be used differently
	RenumberIndexedPatterns();

	// In order to merge all of the Bxx and Dxx Commands, find all Dxx Commands that were used, and move them to Channel 1, unless a Bxx Command was already used there
	for (int i = 0; i < GetSongLength(); i++)
	{
		// If a Bxx command was already used in Channel 1, don't waste any time here, continue with the next Songline
		if ((GetPatternRowCommand(CH1, GetPatternInSongline(CH1, i), GetShortestPatternLength(i) - 1, CMD2) & 0xFF00) == PATTERN_EFFECT_BXX)
			continue;

		// If the Shortest Pattern Length is below actual Pattern Length, a Dxx Command was already used somewhere, and must be replaced
		if (GetShortestPatternLength(i) < GetPatternLength())
			SetPatternRowCommand(CH1, GetPatternInSongline(CH1, i), GetShortestPatternLength(i) - 1, CMD2, PATTERN_EFFECT_DXX);
	}

	// Clear all Dxx Commands from all Channels, except for Channel 1, which will be used for all of the remaining Bxx and Dxx Commands
	for (int i = 0; i < GetSongLength(); i++)
	{
		// Skip CH1, since it was already processed above
		for (int j = CH2; j < GetChannelCount(); j++)
		{
			// All Pattern Rows will be edited, regardless of their contents
			for (int k = 0; k < TRACK_ROW_MAX; k++)
			{
				// The Fxx Commands are perfectly fine as they are, so the Effect Column 1 is also skipped
				for (int l = CMD2; l < PATTERN_ACTIVE_EFFECT_MAX; l++)
				{
					SetPatternRowCommand(j, GetPatternInSongline(j, i), k, l, PATTERN_EFFECT_EMPTY);
				}
			}
		}
	}

	// Apply preliminary optimisations to the data imported from the Legacy RMT Module
	AllSizeOptimisations();

	// Set the Active Effect Command Columns for each channels
	for (int i = 0; i < GetChannelCount(); i++)
	{
		SetEffectCommandCount(i, i == CH1 ? 2 : 1);
	}

	// Try to identify all the "Valid" Subtunes from the Imported Data
	GetSubtuneFromLegacyRMT(startSongline);

	// Then... not a lot would be left to do here...
	// The entire RMTE Module could virtually be used right away from this point onward...
}


// ----------------------------------------------------------------------------
// Functions related to Pattern Data editing, aimed at bulk operations and/or repetitive procedures
// These are broken down into generic functions, to make recycling for other uses fairly easy
//


// Identify the shortest pattern length relative to the other ones used in the same Songline
BYTE CModule::GetShortestPatternLength(int songline)
{
	// Get the current Pattern Length first
	BYTE patternLength = GetPatternLength();

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
					// Add 1 to match the actual number of Rows per Pattern
					if (patternLength > j + 1)
						patternLength = j + 1;
				}
			}
		}
	}

	// The shortest Pattern Length will be returned if successful
	return patternLength;
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(int channel, int pattern)
{
	return IsUnusedPattern(GetChannelIndex(channel), pattern);
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(TIndex* index, int pattern)
{
	// Make sure the Index is not a Null pointer
	if (!index)
		return false;

	// All Songlines in the Channel Index will be processed
	for (int i = 0; i < GetSongLength(); i++)
	{
		// As soon as a match is found, we know for sure the Pattern is used at least once
		if (index->songline[i] == pattern)
			return false;
	}

	// Otherwise, the Pattern is most likely unused
	return true;
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(int channel, int pattern)
{
	return IsEmptyPattern(GetPattern(channel, pattern));
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(TPattern* pattern)
{
	// Make sure the Pattern is not a Null pointer
	if (!pattern)
		return false;

	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		// If there is a Note, it's not empty
		if (pattern->row[i].note != PATTERN_NOTE_EMPTY)
			return false;

		// If there is an Instrument, it's not empty
		if (pattern->row[i].instrument != PATTERN_INSTRUMENT_EMPTY)
			return false;

		// If there is a Volume, it's not empty
		if (pattern->row[i].volume != PATTERN_VOLUME_EMPTY)
			return false;

		// If there is an Effect Command, it's not empty
		if (pattern->row[i].cmd0 != PATTERN_EFFECT_EMPTY)
			return false;

		if (pattern->row[i].cmd1 != PATTERN_EFFECT_EMPTY)
			return false;

		if (pattern->row[i].cmd2 != PATTERN_EFFECT_EMPTY)
			return false;

		if (pattern->row[i].cmd3 != PATTERN_EFFECT_EMPTY)
			return false;
	}

	// Otherwise, the Pattern is most likely empty
	return true;
}

// Compare 2 Patterns for identical data, Return True if successful
bool CModule::IsIdenticalPattern(TPattern* sourcePattern, TPattern* destinationPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!sourcePattern || !destinationPattern)
		return false;

	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		// If there is a different Note, it's not identical
		if (sourcePattern->row[i].note != destinationPattern->row[i].note)
			return false;

		// If there is a different Instrument, it's not identical
		if (sourcePattern->row[i].instrument != destinationPattern->row[i].instrument)
			return false;

		// If there is a different Volume, it's not identical
		if (sourcePattern->row[i].volume != destinationPattern->row[i].volume)
			return false;

		// If there is a different Effect Command, it's not identical
		if (sourcePattern->row[i].cmd0 != destinationPattern->row[i].cmd0)
			return false;

		if (sourcePattern->row[i].cmd1 != destinationPattern->row[i].cmd1)
			return false;

		if (sourcePattern->row[i].cmd2 != destinationPattern->row[i].cmd2)
			return false;

		if (sourcePattern->row[i].cmd3 != destinationPattern->row[i].cmd3)
			return false;
	}

	// Otherwise, the Pattern is most likely identical
	return true;
}

// Duplicate a Pattern used in a Songline, Return True if successful
bool CModule::DuplicatePatternInSongline(int channel, int songline, int pattern)
{
	// Find the first empty and unused Pattern that is available
	for (int i = 0; i < TRACK_PATTERN_MAX; i++)
	{
		// Ignore the Pattern that is being duplicated
		if (i == pattern)
			continue;

		// If the Pattern is empty and unused, it will be used for the duplication
		if (IsUnusedPattern(channel, i) && IsEmptyPattern(channel, i))
		{
			TPattern* source = GetPattern(channel, pattern);
			TPattern* destination = GetPattern(channel, i);

			// If the Pattern duplication failed, nothing will be changed
			if (!CopyPattern(source, destination))
				break;

			// Replace the Pattern used in the Songline Index with the new one
			SetPatternInSongline(channel, songline, i);

			// Pattern duplication was completed successfully
			return true;
		}
	}

	// Could not create a Pattern duplicate, no data was edited
	return false;
}

// Copy data from source Pattern to destination Pattern, Return True if successful
bool CModule::CopyPattern(TPattern* sourcePattern, TPattern* destinationPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!sourcePattern || !destinationPattern)
		return false;
	
	// Otherwise, copying Pattern data is pretty straightforward
	*destinationPattern = *sourcePattern;

	// Pattern data should have been copied successfully
	return true;
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(int channel, int pattern)
{
	return ClearPattern(GetPattern(channel, pattern));
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(TPattern* destinationPattern)
{
	// Make sure the Pattern is not a Null pointer
	if (!destinationPattern)
		return false;

	// Set all indexed Rows in the Pattern to empty values
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		destinationPattern->row[i].note = PATTERN_NOTE_EMPTY;
		destinationPattern->row[i].instrument = PATTERN_INSTRUMENT_EMPTY;
		destinationPattern->row[i].volume = PATTERN_VOLUME_EMPTY;
		destinationPattern->row[i].cmd0 = PATTERN_EFFECT_EMPTY;
		destinationPattern->row[i].cmd1 = PATTERN_EFFECT_EMPTY;
		destinationPattern->row[i].cmd2 = PATTERN_EFFECT_EMPTY;
		destinationPattern->row[i].cmd3 = PATTERN_EFFECT_EMPTY;
	}

	// Pattern data should have been cleared successfully
	return true;
}

// Copy data from source Index to destination Index, Return True if successful
bool CModule::CopyIndex(TIndex* sourceIndex, TIndex* destinationIndex)
{
	// Make sure both the Indexes from source and destination are not Null pointers
	if (!sourceIndex || !destinationIndex)
		return false;

	// Otherwise, copying Index data is pretty straightforward
	*destinationIndex = *sourceIndex;

	// Index data should have been copied successfully
	return true;
}

// Copy data from source Subtune to destination Subtune, Return True if successful
bool CModule::CopySubtune(TSubtune* sourceSubtune, TSubtune* destinationSubtune)
{
	// Make sure both the Subtunes from source and destination are not Null pointers
	if (!sourceSubtune || !destinationSubtune)
		return false;

	// Otherwise, copying Index data is pretty straightforward
	*destinationSubtune = *sourceSubtune;

	// Subtune data should have been copied successfully
	return true;
}

// Duplicate a Pattern Index from source Channel Index to destination Channel Index, Return True if successful
bool CModule::DuplicatePatternIndex(int sourceIndex, int destinationIndex)
{
	// Process all Patterns within the Index, regardless of them being unused or empty
	for (int i = 0; i < TRACK_PATTERN_MAX; i++)
	{
		TPattern* source = GetPattern(sourceIndex, i);
		TPattern* destination = GetPattern(destinationIndex, i);

		// If the Pattern duplication failed, abort the procedure immediately
		if (!CopyPattern(source, destination))
			return false;
	}

	// Pattern Index should have been copied successfully
	return true;
}

// Find and merge duplicated Patterns, and adjust the Songline Index accordingly
int CModule::MergeDuplicatedPatterns()
{
	int mergedPatterns = 0;

	for (int i = 0; i < GetChannelCount(); i++)
	{
		for (int j = 0; j < GetSongLength(); j++)
		{
			// Get the Pattern from which comparisons will be made
			BYTE reference = GetPatternInSongline(i, j);

			// Compare to every Patterns found in the Channel Index, unused Patterns will be ignored
			for (int k = 0; k < GetSongLength(); k++)
			{
				// Get the Pattern that will be compared to the reference Pattern
				BYTE compared = GetPatternInSongline(i, k);

				// Comparing a Pattern to itself is pointless
				if (compared == reference)
					continue;

				// Get the pointers to Pattern data
				TPattern* source = GetPattern(i, reference);
				TPattern* destination = GetPattern(i, compared);

				// Compare the Patterns, and update the Songline Index if they are identical
				if (IsIdenticalPattern(source, destination))
				{
					SetPatternInSongline(i, k, reference);
					mergedPatterns++;
				}
			}
		}
	}

	// Return the number of merged patterns, if any change was made
	return mergedPatterns;
}

// Renumber all Patterns from first to last Songlines, without optimisations
void CModule::RenumberIndexedPatterns()
{
	// Process all Channels within the Module Index
	for (int i = 0; i < GetChannelCount(); i++)
	{
		// Create a Temporary Index that will be used as a buffer
		TIndex* backupIndex = new TIndex;

		// Get the current Channel Index
		TIndex* channelIndex = GetChannelIndex(i);

		// Copy all Indexed Patterns to single Songline entries, effectively duplicating all Patterns used in multiple Songlines
		for (int j = 0; j < SONGLINE_MAX; j++)
		{
			backupIndex->songline[j] = j;
			CopyPattern(GetIndexedPattern(i, j), &backupIndex->pattern[j]);
		}

		// Copy the re-organised data back to the original Channel Index
		CopyIndex(backupIndex, channelIndex);

		// Delete the Temporary Index once it's no longer needed
		delete backupIndex;
	}
}

// Clear all unused Indexed Patterns
void CModule::ClearUnusedPatterns()
{
	// Process all Channels within the Module Index
	for (int i = 0; i < GetChannelCount(); i++)
	{
		// Search for all unused indexed Patterns
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			// If the Pattern is not used anywhere, it will be deleted
			if (IsUnusedPattern(i, j))
			{
				ClearPattern(GetPattern(i, j));
			}
		}
	}
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
void CModule::ConcatenateIndexedPatterns()
{
	// Process all Channels within the Module Index
	for (int i = 0; i < GetChannelCount(); i++)
	{
		// Create a Temporary Index that will be used as a buffer
		TIndex* backupIndex = new TIndex;

		// Get the current Channel Index
		TIndex* channelIndex = GetChannelIndex(i);

		// Copy the original data to the Temporary Index first
		CopyIndex(channelIndex, backupIndex);

		// Concatenate the Indexed Patterns to remove gaps between them
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			// If a Pattern is used at least once, see if it could also be concatenated further back
			if (!IsUnusedPattern(backupIndex, j))
			{
				// Find the first empty and unused Pattern that is available
				for (int k = 0; k < j; k++)
				{
					TPattern* source = &backupIndex->pattern[j];
					TPattern* destination = &backupIndex->pattern[k];

					// If the Pattern is empty and unused, it will be replaced
					if (IsUnusedPattern(backupIndex, k) && IsEmptyPattern(destination))
					{
						// Copy the Pattern from J to K
						CopyPattern(source, destination);

						// Clear the Pattern from J, since it won't be needed anymore
						ClearPattern(source);

						// Replace the Pattern used in the Songline Index with the new one
						for (int l = 0; l < GetSongLength(); l++)
						{
							if (backupIndex->songline[l] == j)
								backupIndex->songline[l] = k;
						}
					}
				}
			}
		}

		// Copy the re-organised data back to the original Channel Index
		CopyIndex(backupIndex, channelIndex);

		// Delete the Temporary Index once it's no longer needed
		delete backupIndex;
	}
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
void CModule::AllSizeOptimisations()
{
	// First, renumber all Patterns
	RenumberIndexedPatterns();

	// Next, merge all duplicated Patterns
	MergeDuplicatedPatterns();

	// Then, Clear all unused Patterns
	ClearUnusedPatterns();

	// Finally, concatenate all Patterns
	ConcatenateIndexedPatterns();

	// And then...? Most likely a lot more... That's for another day...
}

// Find all individual Subtunes from imported Legacy RMT data, in order to reconstruct all the Subtunes later
void CModule::GetSubtuneFromLegacyRMT(int startSongline)
{
	TSubtune* backupSubtune = new TSubtune;
	CopySubtune(GetSubtuneIndex(MODULE_DEFAULT_SUBTUNE), backupSubtune);

	WORD songlineStep[SONGLINE_MAX];
	memset(songlineStep, INVALID, SONGLINE_MAX);

	BYTE subtuneOffset[SONGLINE_MAX];
	memset(subtuneOffset, 0, SONGLINE_MAX);

	BYTE subtuneCount = 0;

	// Process all indexed Songlines until all Subtunes are identified
	for (int i = startSongline; i < GetSongLength(); i++)
	{
		// If the Indexed Songline was not already processed...
		if (!IsValidSongline(songlineStep[i]))
		{
			// if a new Subtune is found, set the offset to the current Songline Index
			subtuneOffset[subtuneCount] = i;
			subtuneCount++;

			// From here, analyse the next Songlines until a loop is detected
			for (int j = i; j < GetSongLength(); j++)
			{
				// If the Songline Step offset is Valid, a loop was completed, there is nothing else to do here
				if (IsValidSongline(songlineStep[j]))
					break;

				// Set the Songline Step to the value of i to clearly indicate it belongs to the same Subtune
				songlineStep[j] = i;

				// If a Goto Songline Command is found, the Songline Step will be set to the Destination Songline
				if ((GetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2) & 0xFF00) == PATTERN_EFFECT_BXX)
				{
					// The next Songline Step will also set to the value of i, to avoid the ricks of detecting false Subtunes
					songlineStep[j + 1] = i;

					// Update j to match the value of the Destination Songline
					j = GetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2) & 0xFF;

					// The Songline Step will be set once again to the value of i, for the same reasons described above
					songlineStep[j] = i;
				}
			}
		}
	}

	// Re-construct all of individual Subtunes that were detected
	for (int i = 0; i < subtuneCount; i++)
	{
		BYTE offset = subtuneOffset[i];

		CopySubtune(backupSubtune, GetSubtuneIndex(i));

		SetActiveSubtune(i);

		// Set all indexed Patterns to INVALID, to easily keep track of the Empty Songlines
		for (int j = 0; j < SONGLINE_MAX; j++)
			SetPatternInSongline(i, j, INVALID);

		// This will be used once again for detecting loop points in Subtunes
		memset(songlineStep, INVALID, SONGLINE_MAX);

		// Re-construct every Songlines used by the Subtune, until the loop point is found
		for (int j = 0; j < SONGLINE_MAX; j++)
		{
			// If the Songline Step offset is Valid, a loop was completed, and the Songlength will be set here
			if (IsValidSongline(songlineStep[offset]))
			{
				SetSongLength(j);
				break;
			}

			// Set the Songline Step to the value of i to clearly indicate it belongs to the same Subtune
			songlineStep[offset] = i;

			// Fetch the Patterns from the backup Songline Index first
			for (int k = 0; k < GetChannelCount(); k++)
			{
				SetPatternInSongline(k, j, backupSubtune->channel[k].songline[offset]);
			}

			// If a Goto Songline Command is found, the data from Destination Songline will be copied to the next Songline
			if ((GetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2) & 0xFF00) == PATTERN_EFFECT_BXX)
			{
				// Update the Songline offset to the value of the Destination Songline
				offset = GetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2) & 0xFF;

				// Clear the Bxx Command, since it won't be needed anymore, except for the loop point, which will also be adjusted at the end
				//SetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2, PATTERN_EFFECT_DXX);
				continue;
			}

			// Otherwise, the Songline offset will increment by 1 for the next Songline
			offset++;
		}

		// Finally, apply the Size Optimisations, the Subtune should have been reconstructed successfully!
		AllSizeOptimisations();
	}

	// Set the Subtune count and the Active Subtune values once everything was processed
	SetSubtuneCount(subtuneCount);
	SetActiveSubtune(MODULE_DEFAULT_SUBTUNE);

	// Delete the Subtune Backup once it is no longer needed
	delete backupSubtune;
}
