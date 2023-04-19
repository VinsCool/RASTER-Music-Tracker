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
	if (!m_index) m_index = new TSubtune[MODULE_SUBTUNE_COUNT];
	if (!m_instrument) m_instrument = new TInstrumentV2[PATTERN_INSTRUMENT_COUNT];

	// Set default module parameters
	SetSongName("Noname Song");
	SetSubtuneName(MODULE_DEFAULT_SUBTUNE, "");
	SetActiveSubtune(MODULE_DEFAULT_SUBTUNE);
	SetSubtuneCount(MODULE_SUBTUNE_COUNT);
	SetSongLength(MODULE_SONG_LENGTH);
	SetPatternLength(MODULE_TRACK_LENGTH);
	SetChannelCount(TRACK_CHANNEL_MAX);	//(MODULE_STEREO);	// FIXME: g_tracks4_8 is NOT initialised when it is called for the first time here!
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
	for (int i = 1; i < GetSubtuneCount(); i++)
	{
		CopySubtune(GetSubtuneIndex(GetActiveSubtune()), GetSubtuneIndex(i));
	}

	// Also clear all instruments in the module
	for (int i = 0; i < PATTERN_INSTRUMENT_COUNT; i++)
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
	CString importLog;
	importLog.Format("");

	WORD songlineStep[SONGLINE_MAX];
	memset(songlineStep, INVALID, SONGLINE_MAX);

	BYTE subtuneOffset[SONGLINE_MAX];
	memset(subtuneOffset, 0, SONGLINE_MAX);

	// This will become the final count of decoded Subtunes from the Legacy RMT Module
	BYTE subtuneCount = 0;

	// Create a Temporary Subtune, to make the Import procedure a much easier task
	TSubtune* importSubtune = new TSubtune;

	// Clear the current module data
	InitialiseModule();

	// Copy an empty Subtune to the Temporary Subtune to initialise it
	CopySubtune(GetSubtuneIndex(MODULE_DEFAULT_SUBTUNE), importSubtune);

	// Decode the Legacy RMT Module into the Temporary Subtune, and re-construct the imported data if successful
	if (DecodeLegacyRMT(in, importSubtune, importLog))
	{
		importLog.AppendFormat("Stage 1 - Decoding of Legacy RMT Module:\n\n");
		importLog.AppendFormat("Song Name: \"");
		importLog.AppendFormat(GetSongName());
		importLog.AppendFormat("\"\n");
		importLog.AppendFormat("Song Length: %02X, Pattern Length: %02X, Channels: %01X\n", importSubtune->songLength, importSubtune->patternLength, importSubtune->channelCount);
		importLog.AppendFormat("Song Speed: %02X, Instrument Speed: %02X\n\n", importSubtune->songSpeed, importSubtune->instrumentSpeed);

		importLog.AppendFormat("Stage 2 - Constructing RMTE Module from imported data:\n\n");

		// Process all indexed Songlines until all the Subtunes are identified
		for (int i = 0; i < importSubtune->songLength; i++)
		{
			// If the Indexed Songline was not already processed...
			if (!IsValidSongline(songlineStep[i]))
			{
				// if a new Subtune is found, set the offset to the current Songline Index
				importLog.AppendFormat("Identified: Subtune %02X, in Songline %02X\n", subtuneCount, i);
				subtuneOffset[subtuneCount] = i;
				subtuneCount++;

				// From here, analyse the next Songlines until a loop is detected
				for (int j = i; j < importSubtune->songLength; j++)
				{
					// If the Songline Step offset is Valid, a loop was completed, there is nothing else to do here
					if (IsValidSongline(songlineStep[j]))
					{
						importLog.AppendFormat("Loop Point found in Songline %02X\n", j - 1);
						break;
					}

					// Set the Songline Step to the value of i to clearly indicate it belongs to the same Subtune
					songlineStep[j] = i;

					// If a Goto Songline Command is found, the Songline Step will be set to the Destination Songline
					if (importSubtune->channel[CH1].songline[j] == 0xFE)
					{
						j = importSubtune->channel[CH2].songline[j];

						// Also set the Songline Step at the offset of Destination Songline, so it won't be referenced again
						songlineStep[j] = i;
					}
				}
			}
		}

		// Set the Subtune count to the number of individual Subtunes identified
		SetSubtuneCount(subtuneCount);

		// Clear the current Subtune Index, then create a new one matching the number of Subtunes to be imported
		delete m_index;
		m_index = new TSubtune[subtuneCount];

		importLog.AppendFormat("Confidently detected %i unique Subtune(s).\n\n", subtuneCount);
		importLog.AppendFormat("Stage 3 - Optimising Subtunes with compatibility tweaks:\n\n");

		// Copy all of the imported patterns to every Channels, so they all share identical data
		for (int i = 0; i < importSubtune->channelCount; i++)
		{
			// The Songline Index won't be overwritten in the process, since we will need it in its current form!
			for (int j = 0; j < TRACK_PATTERN_MAX; j++)
				CopyPattern(&importSubtune->channel[0].pattern[j], &importSubtune->channel[i].pattern[j]);

			// Set the Active Effect Command Columns for each channels
			importSubtune->effectCommandCount[i] = i == CH1 ? 2 : 1;
		}

		// Re-construct all of individual Subtunes that were detected
		for (int i = 0; i < GetSubtuneCount(); i++)
		{
			BYTE offset = subtuneOffset[i];

			// Copy the data previously imported from the Temporary Subtune into the Active Subtune
			SetActiveSubtune(i);
			CopySubtune(importSubtune, GetSubtuneIndex(i));

			// This will be used once again for detecting loop points in Subtunes
			memset(songlineStep, INVALID, SONGLINE_MAX);

			// Re-construct every Songlines used by the Subtune, until the loop point is found
			for (int j = 0; j < SONGLINE_MAX; j++)
			{
				// If the Songline Step offset is Valid, a loop was completed, and the Songlength will be set here
				if (IsValidSongline(songlineStep[offset]))
				{
					SetSongLength(j - 1);
					break;
				}

				// Set the Songline Step to the value of j to match the reconstructed Songline Index
				songlineStep[offset] = j;

				// If a Goto Songline Command is found, the data from Destination Songline will be copied to the next Songline
				if (importSubtune->channel[CH1].songline[offset] == 0xFE)
				{
					offset = importSubtune->channel[CH2].songline[offset];

					// Also set the Songline Step at the offset of Destination Songline, so it won't be referenced again
					songlineStep[offset] = j;
				}

				// Fetch the Patterns from the backup Songline Index first
				for (int k = 0; k < GetChannelCount(); k++)
					SetPatternInSongline(k, j, importSubtune->channel[k].songline[offset]);

				// Otherwise, the Songline offset will increment by 1 for the next Songline
				offset++;
			}

			// Re-arrange all Patterns to make them unique entries for every Songline, so editing them will not overwrite anything intended to be used differently
			RenumberIndexedPatterns();

			// In order to merge all of the Bxx and Dxx Commands, find all Dxx Commands that were used, and move them to Channel 1, unless a Bxx Command was already used there
			for (int j = 0; j < GetSongLength(); j++)
			{
				// If the Shortest Pattern Length is below actual Pattern Length, a Dxx Command was already used somewhere, and must be replaced
				if (GetShortestPatternLength(j) < GetPatternLength())
					SetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2, PATTERN_EFFECT_DXX);

				// Set the final Goto Songline Command Bxx to the Songline found at the loop point
				if (j == GetSongLength() - 1)
					SetPatternRowCommand(CH1, GetPatternInSongline(CH1, j), GetShortestPatternLength(j) - 1, CMD2, PATTERN_EFFECT_BXX | (songlineStep[offset] - 1));

				// Skip CH1, since it was already processed above
				for (int k = CH2; k < GetChannelCount(); k++)
				{
					// All Pattern Rows will be edited, regardless of their contents
					for (int l = 0; l < TRACK_ROW_MAX; l++)
					{
						// The Fxx Commands are perfectly fine as they are, so the Effect Column 1 is also skipped
						for (int m = CMD2; m < PATTERN_ACTIVE_EFFECT_MAX; m++)
							SetPatternRowCommand(k, GetPatternInSongline(k, j), l, m, PATTERN_EFFECT_EMPTY);
					}
				}
			}

			// Finally, apply the Size Optimisations, the Subtune should have been reconstructed successfully!
			AllSizeOptimisations();
			importLog.AppendFormat("Reconstructed: Subtune %02X\n", GetActiveSubtune());
			importLog.AppendFormat("Song Length: %02X, Pattern Length: %02X, Channels: %01X\n", GetSongLength(), GetPatternLength(), GetChannelCount());
			importLog.AppendFormat("Song Speed: %02X, Instrument Speed: %02X\n", GetSongSpeed(), GetInstrumentSpeed());
			importLog.AppendFormat("Loop Point found in Songline %02X\n\n", songlineStep[offset] - 1);
		}

		// Set the Active Subtune to the Default parameter, once the Legacy RMT Import procedure was completed
		SetActiveSubtune(MODULE_DEFAULT_SUBTUNE);

		// Final number of Subtunes that were imported
		importLog.AppendFormat("Processed: %i Subtune(s) with All Size Optimisations.\n\n", GetSubtuneCount());
	}

	// Delete the Temporary Subtune once it is no longer needed
	delete importSubtune;

	// Spawn a messagebox with the statistics collected during the Legacy RMT Module import procedure
	MessageBox(g_hwnd, importLog, "Import Legacy RMT", MB_ICONINFORMATION);
}

// Decode Legacy RMT Module Data, Return True if successful
// If an invalid parameter was found, settle for a compromise using the default values
// The import procedure will be aborted if there is no suitable recovery, however!
bool CModule::DecodeLegacyRMT(std::ifstream& in, TSubtune* subtune, CString& log)
{
	// Make sure the Subtune is not a Null pointer
	if (!subtune)
		return false;

	WORD fromAddr, toAddr;

	BYTE mem[65536];
	memset(mem, 0, 65536);

	BYTE instrumentLoaded[PATTERN_INSTRUMENT_COUNT];
	memset(instrumentLoaded, 0, PATTERN_INSTRUMENT_COUNT);

	// Load the first RMT Module Block, in order to get all of the Pattern, Songline and Instrument Data
	if (!LoadBinaryBlock(in, mem, fromAddr, toAddr))
	{
		log.AppendFormat("An error was returned from LoadBinaryBlock().\n");
		log.AppendFormat("This may be caused by an invalid file, or corrupted data.\n\n");
		return false;
	}

	// 1st, 2nd and 3rd byte: "RMT" identifier
	char* identifier = (char*)mem + fromAddr;

	// 4th byte: # of channels (4 or 8)
	subtune->channelCount = mem[fromAddr + 3] & 0x0F;

	// 5th byte: track length
	subtune->patternLength = mem[fromAddr + 4];

	// 6th byte: song speed
	subtune->songSpeed = mem[fromAddr + 5];

	// 7th byte: Instrument speed
	subtune->instrumentSpeed = mem[fromAddr + 6];

	// 8th byte: RMT format version (only needed for V0 Instruments, could be discarded otherwise)
	BYTE version = mem[fromAddr + 7];

	// Check that the header starts with "RMT", any mismatch will flag the entire file as invalid, regardless of its contents
	if (strncmp(identifier, "RMT", 3) != 0)
	{
		identifier[3] = 0;	// Append a Null terminator so only 3 bytes could be output
		log.AppendFormat("Invalid identifier from file header.\n");
		log.AppendFormat("Expected \"RMT\", but found \"");
		log.AppendFormat(identifier);
		log.AppendFormat("\" instead.\n\n");
		return false;
	}

	// Invalid Channel Count
	if (subtune->channelCount != 4 && subtune->channelCount != 8)
	{
		subtune->channelCount = MODULE_STEREO;
		log.AppendFormat("Warning: Invalid number of Channels, 4 or 8 were expected\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", subtune->channelCount);
	}

	// Invalid Song Speed
	if (subtune->songSpeed < 1)
	{
		subtune->songSpeed = MODULE_SONG_SPEED;
		log.AppendFormat("Warning: Song Speed could not be 0.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", subtune->songSpeed);
	}

	// Invalid Instrument Speed
	if (subtune->instrumentSpeed < 1 || subtune->instrumentSpeed > 8)
	{
		subtune->instrumentSpeed = MODULE_VBI_SPEED;
		log.AppendFormat("Warning: Instrument Speed could only be set between 1 and 8 inclusive.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", subtune->instrumentSpeed);
	}

	// Invalid Legacy RMT Format Version
	if (version > RMTFORMATVERSION)
	{
		version = RMTFORMATVERSION;
		log.AppendFormat("Warning: Invalid RMT Format Version detected.\n");
		log.AppendFormat("Version %i will be assumed by default.\n\n", version);
	}

	// Import all the Legacy RMT Patterns
	if (!ImportLegacyPatterns(subtune, mem, fromAddr))
	{
		log.AppendFormat("Error: Corrupted or Invalid Pattern Data!\n\n");
		return false;
	}

	// Import all the Legacy RMT Songlines
	if (!ImportLegacySonglines(subtune, mem, fromAddr, toAddr))
	{
		log.AppendFormat("Error: Corrupted or Invalid Songline Data!\n\n");
		return false;
	}

	// Import all the Legacy RMT Instruments
	if (!ImportLegacyInstruments(subtune, mem, fromAddr, version, instrumentLoaded))
	{
		log.AppendFormat("Error: Corrupted or Invalid Instrument Data!\n\n");
		return false;
	}

	// Load the second RMT Module Block, in order to get the Song and Instrument Names
	if (!LoadBinaryBlock(in, mem, fromAddr, toAddr))
	{
		log.AppendFormat("Stripped RMT Module detected.\nThe default Song and Instrument Names will be used instead.\n\n");

		// Decoding of Legacy RMT Module should have been successful, even without the second Module Block
		return true;
	}

	// Get the Song Name pointer
	BYTE* ptrName = mem + fromAddr;

	// Copy the Song Name
	BYTE ch = SetSongName((const char*)ptrName);

	// Get the Instrument Name address, which is directly after the Song Name
	WORD addrInstrumentNames = fromAddr + ch;

	// Process all Instruments
	for (int i = 0; i < PATTERN_INSTRUMENT_COUNT; i++)
	{
		// Skip the Instrument if it was not loaded
		if (!instrumentLoaded[i])
			continue;

		// Get the Instrument Name pointer
		ptrName = mem + addrInstrumentNames;

		// Copy the Name to the indexed Instrument
		ch = SetInstrumentName(i, (const char*)ptrName);

		// Offset the Instrument Name address for the next one
		addrInstrumentNames += ch;
	}

	// Decoding of Legacy RMT Module should have been successful
	return true;
}

// Import Legacy RMT Pattern Data, Return True if successful
bool CModule::ImportLegacyPatterns(TSubtune* subtune, BYTE* sourceMemory, WORD sourceAddress)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!subtune || !sourceMemory)
		return false;

	// Get the pointers used for decoding the Legacy RMT Pattern data
	WORD ptrPatternsLow = sourceMemory[sourceAddress + 10] + (sourceMemory[sourceAddress + 11] << 8);
	WORD ptrPatternsHigh = sourceMemory[sourceAddress + 12] + (sourceMemory[sourceAddress + 13] << 8);
	WORD ptrEnd = sourceMemory[sourceAddress + 14] + (sourceMemory[sourceAddress + 15] << 8);

	// Number of Patterns to decode
	WORD patternCount = ptrPatternsHigh - ptrPatternsLow;

	// Abort the import procedure if the number of Patterns detected is invalid
	if (!IsValidPattern(patternCount))
		return false;

	// Decode all Patterns
	for (int i = 0; i < patternCount; i++)
	{
		// Pattern data pointers are split over two tables, low and high bytes, each indexed by the Pattern number itself
		WORD ptrOnePattern = sourceMemory[ptrPatternsLow + i] + (sourceMemory[ptrPatternsHigh + i] << 8);

		// If the pointer is Null, it is an empty Pattern, and must be skipped
		if (!ptrOnePattern)
			continue;

		// Pattern Index number to process, in order to copy data used by all indexed Songlines
		BYTE pattern = i;

		// Identify the end of the Pattern by comparing the starting address of the next Pattern, or the address to End if all Patterns were processed
		WORD ptrPatternEnd = 0;

		for (int j = i; j < patternCount; j++)
		{
			// Get the address used by the next indexed Pattern, or the address to End if there is no additional Patterns to process 
			if ((ptrPatternEnd = (j + 1 == patternCount) ? ptrEnd : sourceMemory[ptrPatternsLow + j + 1] + (sourceMemory[ptrPatternsHigh + j + 1] << 8)))
				break;

			// If the next Pattern is empty, skip over it and continue seeking for the address until something is found
			i++;
		}

		WORD patternLength = ptrPatternEnd - ptrOnePattern;

		// Invalid data, the Legacy RMT Pattern cannot be larger than 256 bytes!
		if (patternLength > 256)
			return false;

		BYTE* memPattern = sourceMemory + ptrOnePattern;

		// Get the pointer to Pattern data used by the Subtune
		TPattern* t = &subtune->channel[0].pattern[pattern];

		WORD src = 0;
		WORD gotoIndex = INVALID;
		WORD smartLoop = INVALID;

		BYTE data, count;
		BYTE line = 0;

		// Minimal Pattern Length that could be used for a smart loop
		if (patternLength >= 2)
		{
			// There is a smart loop at the end of the Pattern
			if (memPattern[patternLength - 2] == 0xBF)
				gotoIndex = memPattern[patternLength - 1];
		}

		// Fetch the Pattern data for as long as the are bytes remaining to be processed
		while (src < patternLength)
		{
			// Jump to gotoIndex => Set the Smart Loop offset here
			if (src == gotoIndex)
				smartLoop = line;

			// Data to process at current Pattern offset
			data = memPattern[src] & 0x3F;

			switch (data)
			{
			case 0x3D:	// Have Volume only on this row
				t->row[line].volume = ((memPattern[src + 1] & 0x03) << 2) | ((memPattern[src] & 0xC0) >> 6);
				src += 2;	// 2 bytes were processed
				line++;	// 1 row was processed
				break;

			case 0x3E:	// Pause or empty row
				count = memPattern[src] & 0xC0;
				if (!count)
				{
					// If the Pause is 0, the number of Rows to skip is in the next byte
					if (memPattern[src + 1] == 0)
					{
						// Infinite pause => Set Pattern End here
						src = patternLength;
						break;
					}
					line += memPattern[src + 1];	// Number of Rows to skip
					src += 2;	// 2 bytes were processed
				}
				else
				{
					line += (count >> 6);	// Upper 2 bits directly specify a pause between 1 to 3 rows
					src++;	// 1 byte was processed
				}
				break;

			case 0x3F:	// Speed, smart loop, or end
				count = memPattern[src] & 0xC0;
				if (!count)
				{
					// Speed, set Fxx command
					t->row[line].cmd0 = 0x0F00 | memPattern[src + 1];
					src += 2;	// 2 bytes were processed
				}
				if (count == 0x80)
				{
					// Smart loop, no extra data to process
					src = patternLength;
				}
				if (count == 0xC0)
				{
					// End of Pattern, set a Dxx command here, no extra data to process
					t->row[line - 1].cmd1 = 0x0D00;
					src = patternLength;
				}
				break;

			default:	// Note, Instrument and Volume data on this Row
				t->row[line].note = data;
				t->row[line].instrument = ((memPattern[src + 1] & 0xfc) >> 2);
				t->row[line].volume = ((memPattern[src + 1] & 0x03) << 2) | ((memPattern[src] & 0xc0) >> 6);
				src += 2;	// 2 bytes were processed
				line++;	// 1 row was processed
			}
		}

		// The Pattern must to be "expanded" in order to be compatible, an equivalent for Smart Loop does not yet exist for the RMTE format
		if (IsValidRow(smartLoop))
		{
			for (int j = 0; line + j < subtune->patternLength; j++)
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

	// Legacy RMT Patterns should have been imported successfully
	return true;
}

// Import Legacy RMT Songline Data, Return True if successful
bool CModule::ImportLegacySonglines(TSubtune* subtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!subtune || !sourceMemory)
		return false;

	// Variables for processing songline data
	WORD line = 0, src = 0;
	BYTE channel = 0;

	// Get the pointers used for decoding the Legacy RMT Songline data
	WORD ptrSong = sourceMemory[sourceAddress + 14] + (sourceMemory[sourceAddress + 15] << 8);
	BYTE* memSong = sourceMemory + ptrSong;

	// Get the Song length, in bytes
	WORD lengthSong = endAddress - ptrSong + 1;

	// Abort the import procedure if the Song length detected is invalid
	if (lengthSong < INVALID || lengthSong > 256 * 8)
		return false;

	// Decode all Songlines
	while (src < lengthSong)
	{
		BYTE data = memSong[src];

		// Process the Pattern index in the songline indexed by the channel number
		switch (data)
		{
		case 0xFE:	// Goto Songline commands are only valid from the first channel, but we know it's never used anywhere else
			subtune->channel[channel].songline[line] = data;
			subtune->channel[channel + 1].songline[line] = memSong[src + 1];	// Set the songline index number in Channel 2
			subtune->channel[channel + 2].songline[line] = INVALID;	// The Goto songline address isn't needed
			subtune->channel[channel + 3].songline[line] = INVALID;	// Set the remaining channels to INVALID
			channel = subtune->channelCount;	// Set the channel index to the channel count to trigger the condition below
			src += subtune->channelCount;	// The number of bytes processed is equal to the number of channels
			break;

		default:	// An empty pattern at 0xFF is also valid for the RMTE format
			subtune->channel[channel].songline[line] = data;
			channel++;	// 1 pattern per channel, for each indexed songline
			src++;	// 1 byte was processed
		}

		// 1 songline was processed when the channel count is equal to the number of channels
		if (channel >= subtune->channelCount)
		{
			channel = 0;	// Reset the channel index
			line++;	// Increment the songline count by 1
		}

		// Break out of the loop if the maximum number of songlines was processed
		if (line >= SONGLINE_MAX)
			break;
	}

	// Set the Songlength to the number of decoded Songlines
	subtune->songLength = (BYTE)line;

	// Legacy RMT Songlines should have been imported successfully
	return true;
}

// Import Legacy RMT Instrument Data, Return True if successful
bool CModule::ImportLegacyInstruments(TSubtune* subtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!subtune || !sourceMemory)
		return false;

	// Get the pointers used for decoding the Legacy RMT Instrument data
	WORD ptrInstruments = sourceMemory[sourceAddress + 8] + (sourceMemory[sourceAddress + 9] << 8);
	WORD ptrEnd = sourceMemory[sourceAddress + 10] + (sourceMemory[sourceAddress + 11] << 8);

	// Number of instruments to decode
	WORD instrumentCount = (ptrEnd - ptrInstruments) / 2;

	// Abort the import procedure if the number of Instruments detected is invalid
	if (!IsValidInstrument(instrumentCount))
		return false;

	// Decode all Instruments, TODO: Add exceptions for V0 Instruments
	for (int i = 0; i < instrumentCount; i++)
	{
		// Get the pointer to Instrument envelope and parameters
		WORD ptrOneInstrument = sourceMemory[ptrInstruments + i * 2] + (sourceMemory[ptrInstruments + i * 2 + 1] << 8);

		// If it is a NULL pointer, in this case, an offset of 0, the instrument is empty, and must be skipped
		if (!ptrOneInstrument)
			continue;

		BYTE* memInstrument = sourceMemory + ptrOneInstrument;

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

		// Instrument was loaded
		isLoaded[i]++;
	}

	// Legacy RMT Instruments should have been imported successfully
	return true;
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
