// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"
#include "Atari6502.h"

CModule::CModule()
{
	memset(m_subtune, NULL, sizeof m_subtune);
	memset(m_instrument, NULL, sizeof m_instrument);
	InitialiseModule();
}

CModule::~CModule()
{
	ClearModule();
}

void CModule::InitialiseModule()
{
	// Set the default Module parameters
	SetSongName("Noname Song");
	SetSongAuthor("Unknown");
	SetSongCopyright("2023");

	// Create 1 empty Subtune, which will be used by default
	CreateSubtune(MODULE_DEFAULT_SUBTUNE);

	// Same thing for the Instrument
	CreateInstrument(MODULE_DEFAULT_INSTRUMENT);
}

void CModule::ClearModule()
{
	// Set the empty Module parameters
	SetSongName("");
	SetSongAuthor("");
	SetSongCopyright("");

	// Delete all Module data and set the associated pointers to NULL
	for (int i = 0; i < SUBTUNE_MAX; i++)
		DeleteSubtune(i);

	for (int i = 0; i < PATTERN_INSTRUMENT_COUNT; i++)
		DeleteInstrument(i);
}

void CModule::CreateSubtune(int subtune)
{
	if (!IsValidSubtune((subtune)))
		return;

	// If there is no Subtune here, create it now and update the Subtune Index accordingly
	if (!m_subtune[subtune])
		m_subtune[subtune] = new TSubtune;

	// A new Subtune must be initialised when it is created
	InitialiseSubtune(m_subtune[subtune]);
}

void CModule::DeleteSubtune(int subtune)
{
	if (!IsValidSubtune((subtune)))
		return;

	// If there is a Subtune here, don't waste any time and delete it without further ado
	if (m_subtune[subtune])
		delete m_subtune[subtune];

	m_subtune[subtune] = NULL;
}

void CModule::InitialiseSubtune(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// Set the default Subtune name TODO(?): Append the Index Number to it
	strncpy_s(pSubtune->name, "Noname Subtune", SUBTUNE_NAME_MAX);

	// Set the default parameters to all the Subtune variables
	pSubtune->songLength = MODULE_SONG_LENGTH;
	pSubtune->patternLength = MODULE_TRACK_LENGTH;
	pSubtune->channelCount = MODULE_STEREO;	//= TRACK_CHANNEL_MAX;
	pSubtune->songSpeed = MODULE_SONG_SPEED;
	pSubtune->instrumentSpeed = MODULE_VBI_SPEED;

	// Clear all data, and set default values
	for (int i = 0; i < TRACK_CHANNEL_MAX; i++)
	{
		// Set all indexed Patterns to 0
		for (int j = 0; j < SONGLINE_MAX; j++)
			pSubtune->channel[i].songline[j] = 0x00;

		// Set all indexed Rows in Patterns to empty values
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
			ClearPattern(&pSubtune->channel[i].pattern[j]);

		// By default, only 1 Effect Command is enabled in all Track Channels
		pSubtune->channel[i].effectCount = 0x01;
	}
}

void CModule::CreateInstrument(int instrument)
{
	if (!IsValidInstrument((instrument)))
		return;

	// If there is no Instrument here, create it now and update the Instrument Index accordingly
	if (!m_instrument[instrument])
		m_instrument[instrument] = new TInstrumentV2;

	// A new Instrument must be initialised when it is created
	InitialiseInstrument(m_instrument[instrument]);
}

void CModule::DeleteInstrument(int instrument)
{
	if (!IsValidInstrument((instrument)))
		return;

	// If there is an Instrument here, don't waste any time and delete it without further ado
	if (m_instrument[instrument])
		delete m_instrument[instrument];

	m_instrument[instrument] = NULL;
}

void CModule::InitialiseInstrument(TInstrumentV2* pInstrument)
{
	if (!pInstrument)
		return;

	// Set the default Instrument name TODO(?): Append the Index Number to it
	strncpy_s(pInstrument->name, "New Instrument", INSTRUMENT_NAME_MAX);

	// Set the default parameters to all the Instrument variables
	pInstrument->volumeFade = 0x00;
	pInstrument->volumeSustain = 0x00;
	pInstrument->vibrato = 0x00;
	pInstrument->freqShift = 0x00;
	pInstrument->delay = 0x00;

	pInstrument->envelopeMacro.mode = 0x00;
	pInstrument->envelopeMacro.length = 0x01;
	pInstrument->envelopeMacro.loop = 0x00;
	pInstrument->envelopeMacro.release = 0x00;
	pInstrument->envelopeMacro.speed = 0x00;

	pInstrument->tableMacro.mode = 0x00;
	pInstrument->tableMacro.length = 0x00;
	pInstrument->tableMacro.loop = 0x00;
	pInstrument->tableMacro.release = 0x00;
	pInstrument->tableMacro.speed = 0x00;

	// Set Envelopes to Empty
	for (int i = 0; i < ENVELOPE_INDEX_MAX; i++)
	{
		pInstrument->envelopeMacro.envelope[i].volume = 0x00;
		pInstrument->envelopeMacro.envelope[i].timbre = 0x00;
		pInstrument->envelopeMacro.envelope[i].audctl = 0x00;
		pInstrument->envelopeMacro.envelope[i].trigger.autoFilter = false;
		pInstrument->envelopeMacro.envelope[i].trigger.auto16Bit = false;
		pInstrument->envelopeMacro.envelope[i].trigger.autoReverse16 = false;
		pInstrument->envelopeMacro.envelope[i].trigger.auto179Mhz = false;
		pInstrument->envelopeMacro.envelope[i].trigger.auto15Khz = false;
		pInstrument->envelopeMacro.envelope[i].trigger.autoPoly9 = false;
		pInstrument->envelopeMacro.envelope[i].trigger.autoTwoTone = false;
		pInstrument->envelopeMacro.envelope[i].trigger.autoToggle = false;
		pInstrument->envelopeMacro.envelope[i].effect.command = 0x00;
		pInstrument->envelopeMacro.envelope[i].effect.parameter = 0x00;
	}

	// Set Tables to Empty
	for (int i = 0; i < INSTRUMENT_TABLE_INDEX_MAX; i++)
	{
		pInstrument->tableMacro.table[i].note = 0x00;
		pInstrument->tableMacro.table[i].freq = 0x00;
	}
}

bool CModule::ImportLegacyRMT(std::ifstream& in)
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
	InitialiseSubtune(importSubtune);

	// Clear the current module data
	ClearModule();
	InitialiseModule();

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

		importLog.AppendFormat("Confidently detected %i unique Subtune(s).\n\n", subtuneCount);
		importLog.AppendFormat("Stage 3 - Optimising Subtunes with compatibility tweaks:\n\n");

		// Copy all of the imported patterns to every Channels, so they all share identical data
		for (int i = 0; i < importSubtune->channelCount; i++)
		{
			// The Songline Index won't be overwritten in the process, since we will need it in its current form!
			for (int j = 0; j < TRACK_PATTERN_MAX; j++)
				CopyPattern(&importSubtune->channel[CH1].pattern[j], &importSubtune->channel[i].pattern[j]);

			// Set the Active Effect Command Columns to the same number for each channels
			importSubtune->channel[i].effectCount = 2;
		}

		// Re-construct all of individual Subtunes that were detected
		for (int i = 0; i < subtuneCount; i++)
		{
			BYTE offset = subtuneOffset[i];

			// Copy the data previously imported from the Temporary Subtune into the Active Subtune
			CreateSubtune(i);
			CopySubtune(importSubtune, GetSubtune(i));

			// This will be used once again for detecting loop points in Subtunes
			memset(songlineStep, INVALID, SONGLINE_MAX);

			// Re-construct every Songlines used by the Subtune, until the loop point is found
			for (int j = 0; j < SONGLINE_MAX; j++)
			{
				// If the Songline Step offset is Valid, a loop was completed, and the Songlength will be set here
				if (IsValidSongline(songlineStep[offset]))
				{
					SetSongLength(i, j - 1);
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
				for (int k = 0; k < GetChannelCount(i); k++)
					SetPatternInSongline(i, k, j, importSubtune->channel[k].songline[offset]);

				// Otherwise, the Songline offset will increment by 1 for the next Songline
				offset++;
			}

			// Re-arrange all Patterns to make them unique entries for every Songline, so editing them will not overwrite anything intended to be used differently
			RenumberIndexedPatterns(i);

			// In order to merge all of the Bxx and Dxx Commands, find all Dxx Commands that were used, and move them to Channel 1, unless a Bxx Command was already used there
			for (int j = 0; j < GetSongLength(i); j++)
			{
				// If the Shortest Pattern Length is below actual Pattern Length, a Dxx Command was already used somewhere, and must be replaced
				if (GetShortestPatternLength(i, j) < GetPatternLength(i))
					SetPatternRowCommand(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, EFFECT_COMMAND_DXX, EFFECT_PARAMETER_MIN);

				// Set the final Goto Songline Command Bxx to the Songline found at the loop point
				if (j == GetSongLength(i) - 1)
					SetPatternRowCommand(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, EFFECT_COMMAND_BXX, songlineStep[offset] - 1);

				// Skip CH1, since it was already processed above
				for (int k = CH2; k < GetChannelCount(i); k++)
				{
					// All Pattern Rows will be edited, regardless of their contents
					for (int l = 0; l < TRACK_ROW_MAX; l++)
					{
						// The Fxx Commands are perfectly fine as they are, so the Effect Column 1 is also skipped
						for (int m = CMD2; m < PATTERN_ACTIVE_EFFECT_MAX; m++)
							SetPatternRowCommand(i, k, GetPatternInSongline(i, k, j), l, m, PATTERN_EFFECT_EMPTY, EFFECT_PARAMETER_MIN);
					}
				}
			}

			// Set the final count of Active Effect Command Columns for each channels once they're all processed
			for (int j = 0; j < GetChannelCount(i); j++)
				SetEffectCommandCount(i, j, j == CH1 ? 2 : 1);

			// Finally, apply the Size Optimisations, the Subtune should have been reconstructed successfully!
			AllSizeOptimisations(i);
			importLog.AppendFormat("Reconstructed: Subtune %02X\n", i);
			importLog.AppendFormat("Song Length: %02X, Pattern Length: %02X, Channels: %01X\n", GetSongLength(i), GetPatternLength(i), GetChannelCount(i));
			importLog.AppendFormat("Song Speed: %02X, Instrument Speed: %02X\n", GetSongSpeed(i), GetInstrumentSpeed(i));
			importLog.AppendFormat("Loop Point found in Songline %02X\n\n", songlineStep[offset] - 1);
		}

		// Workaround: Due to the way RMT was originally designed, the "Global" number of channels must be set here as well
		g_tracks4_8 = GetChannelCount(MODULE_DEFAULT_SUBTUNE);

		// Final number of Subtunes that were imported
		importLog.AppendFormat("Processed: %i Subtune(s) with All Size Optimisations.\n\n", GetSubtuneCount());
	}

	// Delete the Temporary Subtune once it is no longer needed
	delete importSubtune;

	// Spawn a messagebox with the statistics collected during the Legacy RMT Module import procedure
	MessageBox(g_hwnd, importLog, "Import Legacy RMT", MB_ICONINFORMATION);

	return true;
}

// Decode Legacy RMT Module Data, Return True if successful
// If an invalid parameter was found, settle for a compromise using the default values
// The import procedure will be aborted if there is no suitable recovery, however!
bool CModule::DecodeLegacyRMT(std::ifstream& in, TSubtune* pSubtune, CString& log)
{
	// Make sure the Subtune is not a Null pointer
	if (!pSubtune)
		return false;

	CString s;

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
	pSubtune->channelCount = mem[fromAddr + 3] & 0x0F;

	// 5th byte: track length
	pSubtune->patternLength = mem[fromAddr + 4];

	// 6th byte: song speed
	pSubtune->songSpeed = mem[fromAddr + 5];

	// 7th byte: Instrument speed
	pSubtune->instrumentSpeed = mem[fromAddr + 6];

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
	if (pSubtune->channelCount != 4 && pSubtune->channelCount != 8)
	{
		pSubtune->channelCount = MODULE_STEREO;
		log.AppendFormat("Warning: Invalid number of Channels, 4 or 8 were expected\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", pSubtune->channelCount);
	}

	// Invalid Song Speed
	if (pSubtune->songSpeed < 1)
	{
		pSubtune->songSpeed = MODULE_SONG_SPEED;
		log.AppendFormat("Warning: Song Speed could not be 0.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", pSubtune->songSpeed);
	}

	// Invalid Instrument Speed
	if (pSubtune->instrumentSpeed < 1 || pSubtune->instrumentSpeed > 8)
	{
		pSubtune->instrumentSpeed = MODULE_VBI_SPEED;
		log.AppendFormat("Warning: Instrument Speed could only be set between 1 and 8 inclusive.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", pSubtune->instrumentSpeed);
	}

	// Invalid Legacy RMT Format Version
	if (version > RMTFORMATVERSION)
	{
		version = RMTFORMATVERSION;
		log.AppendFormat("Warning: Invalid RMT Format Version detected.\n");
		log.AppendFormat("Version %i will be assumed by default.\n\n", version);
	}

	// Import all the Legacy RMT Patterns
	if (!ImportLegacyPatterns(pSubtune, mem, fromAddr))
	{
		log.AppendFormat("Error: Corrupted or Invalid Pattern Data!\n\n");
		return false;
	}

	// Import all the Legacy RMT Songlines
	if (!ImportLegacySonglines(pSubtune, mem, fromAddr, toAddr))
	{
		log.AppendFormat("Error: Corrupted or Invalid Songline Data!\n\n");
		return false;
	}

	// Import all the Legacy RMT Instruments
	if (!ImportLegacyInstruments(pSubtune, mem, fromAddr, version, instrumentLoaded))
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
	BYTE ch;

	s.Format("");

	// Copy the Song Name
	for (ch = 0; ch < MODULE_SONG_NAME_MAX && ptrName[ch]; ch++)
		s.AppendFormat("%c", ptrName[ch]);

	SetSongName(s);

	// Get the Instrument Name address, which is directly after the Song Name
	WORD addrInstrumentNames = fromAddr + ch + 1;

	// Process all Instruments
	for (int i = 0; i < PATTERN_INSTRUMENT_COUNT; i++)
	{
		// Skip the Instrument if it was not loaded
		if (!instrumentLoaded[i])
			continue;

		// Get the Instrument Name pointer
		ptrName = mem + addrInstrumentNames;

		s.Format("");

		// Copy the Name to the indexed Instrument
		for (ch = 0; ch < INSTRUMENT_NAME_MAX && ptrName[ch]; ch++)
			s.AppendFormat("%c", ptrName[ch]);

		SetInstrumentName(i, s);

		// Offset the Instrument Name address for the next one
		addrInstrumentNames += ch + 1;
	}

	// Decoding of Legacy RMT Module should have been successful
	return true;
}

// Import Legacy RMT Pattern Data, Return True if successful
bool CModule::ImportLegacyPatterns(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!pSubtune || !sourceMemory)
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
		TPattern* t = &pSubtune->channel[0].pattern[pattern];

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
					t->row[line].effect[CMD1].command = EFFECT_COMMAND_FXX;
					t->row[line].effect[CMD1].parameter = memPattern[src + 1];
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
					t->row[line - 1].effect[CMD2].command = EFFECT_COMMAND_DXX;
					t->row[line - 1].effect[CMD2].parameter = EFFECT_PARAMETER_MIN;
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
			for (int j = 0; line + j < pSubtune->patternLength; j++)
			{
				int k = line + j;
				int l = smartLoop + j;
				t->row[k].note = t->row[l].note;
				t->row[k].instrument = t->row[l].instrument;
				t->row[k].volume = t->row[l].volume;
				t->row[k].effect[CMD1].command = t->row[l].effect[CMD1].command;
				t->row[k].effect[CMD1].parameter = t->row[l].effect[CMD1].parameter;
			}
		}
	}

	// Legacy RMT Patterns should have been imported successfully
	return true;
}

// Import Legacy RMT Songline Data, Return True if successful
bool CModule::ImportLegacySonglines(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!pSubtune || !sourceMemory)
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
			pSubtune->channel[channel].songline[line] = data;
			pSubtune->channel[channel + 1].songline[line] = memSong[src + 1];	// Set the songline index number in Channel 2
			pSubtune->channel[channel + 2].songline[line] = INVALID;	// The Goto songline address isn't needed
			pSubtune->channel[channel + 3].songline[line] = INVALID;	// Set the remaining channels to INVALID
			channel = pSubtune->channelCount;	// Set the channel index to the channel count to trigger the condition below
			src += pSubtune->channelCount;	// The number of bytes processed is equal to the number of channels
			break;

		default:	// An empty pattern at 0xFF is also valid for the RMTE format
			pSubtune->channel[channel].songline[line] = data;
			channel++;	// 1 pattern per channel, for each indexed songline
			src++;	// 1 byte was processed
		}

		// 1 songline was processed when the channel count is equal to the number of channels
		if (channel >= pSubtune->channelCount)
		{
			channel = 0;	// Reset the channel index
			line++;	// Increment the songline count by 1
		}

		// Break out of the loop if the maximum number of songlines was processed
		if (line >= SONGLINE_MAX)
			break;
	}

	// Set the Songlength to the number of decoded Songlines
	pSubtune->songLength = (BYTE)line;

	// Legacy RMT Songlines should have been imported successfully
	return true;
}

// Import Legacy RMT Instrument Data, Return True if successful
bool CModule::ImportLegacyInstruments(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded)
{
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!pSubtune || !sourceMemory)
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

		// Initialise the equivalent RMTE Instrument, then get the pointer to it for the next step
		CreateInstrument(i);
		TInstrumentV2* pInstrument = GetInstrument(i);

		// Get the Envelopes, Tables, and other parameters from the original RMT instrument data
		BYTE* memInstrument = sourceMemory + ptrOneInstrument;
		BYTE envelopePtr = memInstrument[0];							// Pointer to Instrument Envelope
		BYTE tablePtr = 12;												// Pointer to Instrument Table
		BYTE tableType = memInstrument[4] >> 7;							// Table Type, 0 = Note, 1 = Freq
		BYTE initialAudctl = memInstrument[5];							// AUDCTL, used to initialise the equivalent Envelope

		// Set the equivalent data to the RMTE instrument, with respect to boundaries
		pInstrument->tableMacro.length = envelopePtr - tablePtr;
		pInstrument->tableMacro.loop = memInstrument[1] - tablePtr;
		pInstrument->envelopeMacro.length = (memInstrument[2] - envelopePtr + 1) / 3;
		pInstrument->envelopeMacro.loop = (memInstrument[3] - envelopePtr + 1) / 3;
		pInstrument->tableMacro.mode = (memInstrument[4] >> 6) & 0x01;	// Table Mode, 0 = Set, 1 = Additive
		pInstrument->tableMacro.speed = memInstrument[4] & 0x3F;		// Table Speed, used to offset the equivalent Tables
		pInstrument->volumeFade = memInstrument[6];						// Volume Slide
		pInstrument->volumeSustain = memInstrument[7] >> 4;				// Volume Minimum
		pInstrument->delay = memInstrument[8];							// Vibrato/Freq Shift Delay
		pInstrument->vibrato = memInstrument[9] & 0x03;					// Vibrato
		pInstrument->freqShift = memInstrument[10];						// Freq Shift

		if (pInstrument->tableMacro.length > INSTRUMENT_TABLE_INDEX_MAX)
			pInstrument->tableMacro.length = 0x00;

		if (pInstrument->tableMacro.loop > pInstrument->tableMacro.length)
			pInstrument->tableMacro.loop = 0x00;

		if (pInstrument->envelopeMacro.length > ENVELOPE_INDEX_MAX)
			pInstrument->envelopeMacro.length = 0x00;

		if (pInstrument->envelopeMacro.loop > pInstrument->envelopeMacro.length)
			pInstrument->envelopeMacro.loop = 0x00;

		// Fill the equivalent RMTE tables based on the tableMode parameter
		for (int j = 0; j <= pInstrument->tableMacro.length; j++)
		{
			pInstrument->tableMacro.table[j].note = !tableType ? memInstrument[tablePtr + j] : 0x00;
			pInstrument->tableMacro.table[j].freq = tableType ? memInstrument[tablePtr + j] : 0x00;
		}

		// Fill the equivalent RMTE envelopes, which might include some compromises due to the format differences
		for (int j = 0; j <= pInstrument->envelopeMacro.length; j++)
		{
			// Get the 3 bytes used by the original RMT Envelope format
			BYTE envelopeVolume = memInstrument[envelopePtr + 1 + (j * 3)];
			BYTE envelopeCommand = memInstrument[envelopePtr + 1 + (j * 3) + 1];
			BYTE envelopeParameter = memInstrument[envelopePtr + 1 + (j * 3) + 2];

			// Envelope Effect Command, from 0 to 7
			BYTE envelopeEffectCommand = (envelopeCommand >> 4) & 0x07;

			// The Envelope Effect Command is used for compatibility tweaks, which may or may not provide perfect results
			switch (envelopeEffectCommand)
			{
			case 0x07:	// Overwrite the initialAudctl parameter, for a pseudo AUDCTL envelope when it is used multiple times (Patch16 only)
				if (envelopeParameter < 0xFD)
				{
					initialAudctl = envelopeParameter;
					envelopeParameter = envelopeEffectCommand = 0x00;
				}
				break;
			}

			// Extended RMT Command Envelope, with compatibility tweaks as a compromise
			pInstrument->envelopeMacro.envelope[j].effect.command = envelopeEffectCommand;
			pInstrument->envelopeMacro.envelope[j].effect.parameter = envelopeParameter;

			// Envelope Distortion, from 0 to E, in steps of 2
			BYTE distortion = envelopeCommand & 0x0E;

			// The original "Auto16Bit" trigger ;)
			if (distortion == 0x06)
				pInstrument->envelopeMacro.envelope[j].trigger.auto16Bit = true;

			switch (distortion)
			{
			case 0x00: distortion = TIMBRE_PINK_NOISE; break;
			case 0x02: distortion = TIMBRE_BELL; break;
			case 0x04: distortion = TIMBRE_SMOOTH_4; break;
			case 0x08: distortion = TIMBRE_WHITE_NOISE; break;
			case 0x06:
			case 0x0A: distortion = TIMBRE_PURE; break;
			case 0x0C: distortion = TIMBRE_BUZZY_C; break;
			case 0x0E: distortion = TIMBRE_GRITTY_C; break;
			}
			
			// Distortion Envelope, converted the equivalent Timbre parameter
			pInstrument->envelopeMacro.envelope[j].timbre = distortion;

			// Envelope Volume, only Left POKEY volume is supported by the RMTE format
			pInstrument->envelopeMacro.envelope[j].volume = envelopeVolume & 0x0F;

			// Envelope AUDCTL
			pInstrument->envelopeMacro.envelope[j].audctl = initialAudctl;

			// AutoFilter Trigger
			pInstrument->envelopeMacro.envelope[j].trigger.autoFilter = envelopeCommand >> 7;

			// Portamento is not supported by the RMTE format, a Pattern Effect Command could be set where the Portamento is expected as a compromise
			//pInstrument->envelopeMacro.envelope[j].portamento = envelopeCommand & 0x01;
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

TChannel* CModule::GetChannel(BYTE subtune, BYTE channel)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune && IsValidChannel(channel))
		return &pSubtune->channel[channel];

	return NULL;
}

TPattern* CModule::GetPattern(BYTE subtune, BYTE channel, BYTE pattern)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		return &pChannel->pattern[pattern];

	return NULL;
}

TPattern* CModule::GetIndexedPattern(BYTE subtune, BYTE channel, BYTE songline)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		return GetPattern(subtune, channel, pChannel->songline[songline]);

	return NULL;
}

TRow* CModule::GetRow(BYTE subtune, BYTE channel, BYTE pattern, BYTE row)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune && IsValidChannel(channel))
		return &pSubtune->channel[channel].pattern[pattern].row[row];

	return NULL;
}

const BYTE CModule::GetPatternInSongline(BYTE subtune, BYTE channel, BYTE songline)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		return pChannel->songline[songline];

	return INVALID;
}

const BYTE CModule::GetPatternRowNote(BYTE subtune, BYTE channel, BYTE pattern, BYTE row)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		return pRow->note;

	return INVALID;
}

const BYTE CModule::GetPatternRowInstrument(BYTE subtune, BYTE channel, BYTE pattern, BYTE row)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		return pRow->instrument;

	return INVALID;
}

const BYTE CModule::GetPatternRowVolume(BYTE subtune, BYTE channel, BYTE pattern, BYTE row)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		return pRow->volume;

	return INVALID;
}

const BYTE CModule::GetPatternRowEffectCommand(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow && IsValidCommandColumn(column))
		return pRow->effect[column].command;

	return INVALID;
}

const BYTE CModule::GetPatternRowEffectParameter(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow && IsValidCommandColumn(column))
		return pRow->effect[column].parameter;

	return INVALID;	// Or maybe 0?
}


void CModule::SetPatternInSongline(BYTE subtune, BYTE channel, BYTE songline, BYTE pattern)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		pChannel->songline[songline] = pattern;
}

void CModule::SetPatternRowNote(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE note)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		pRow->note = note;
}

void CModule::SetPatternRowInstrument(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE instrument)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		pRow->note = instrument;
}

void CModule::SetPatternRowVolume(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE volume)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow)
		pRow->volume = volume;
}

void CModule::SetPatternRowCommand(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column, BYTE effectCommand, BYTE effectParameter)
{
	TRow* pRow = GetRow(subtune, channel, pattern, row);

	if (pRow && IsValidCommandColumn(column))
	{
		pRow->effect[column].command = effectCommand;
		pRow->effect[column].parameter = effectParameter;
	}
}


const char* CModule::GetSubtuneName(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->name;

	return NULL;
}

const BYTE CModule::GetSongLength(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->songLength;

	return NULL;
}

const BYTE CModule::GetPatternLength(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->patternLength;

	return NULL;
}

const BYTE CModule::GetChannelCount(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->channelCount;

	return NULL;
}

const BYTE CModule::GetSongSpeed(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->songSpeed;

	return NULL;
}

const BYTE CModule::GetInstrumentSpeed(BYTE subtune)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		return pSubtune->instrumentSpeed;

	return NULL;
}

const BYTE CModule::GetEffectCommandCount(BYTE subtune, BYTE channel)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		return pChannel->effectCount;

	return NULL;
}


void CModule::SetSubtuneName(BYTE subtune, const char* name)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		strncpy_s(pSubtune->name, name, SUBTUNE_NAME_MAX);
}

void CModule::SetSongLength(BYTE subtune, BYTE length)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		pSubtune->songLength = length;
}

void CModule::SetPatternLength(BYTE subtune, BYTE length)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		pSubtune->patternLength = length;
}

void CModule::SetChannelCount(BYTE subtune, BYTE count)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune && IsValidChannel(count))
		pSubtune->channelCount = count;
}

void CModule::SetSongSpeed(BYTE subtune, BYTE speed)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		pSubtune->songSpeed = speed;
}

void CModule::SetInstrumentSpeed(BYTE subtune, BYTE speed)
{
	TSubtune* pSubtune = GetSubtune(subtune);

	if (pSubtune)
		pSubtune->instrumentSpeed = speed;
}

void CModule::SetEffectCommandCount(BYTE subtune, BYTE channel, BYTE column)
{
	TChannel* pChannel = GetChannel(subtune, channel);

	if (pChannel)
		pChannel->effectCount = column;
}


const BYTE CModule::GetSubtuneCount()
{
	BYTE count = 0;

	for (int i = 0; i < SUBTUNE_MAX; i++)
		count += m_subtune[i] != NULL;

	return count;
}


const char* CModule::GetInstrumentName(int instrument)
{
	TInstrumentV2* pInstrument = GetInstrument(instrument);

	if (pInstrument)
		return pInstrument->name;

	return NULL;
}

void CModule::SetInstrumentName(int instrument, const char* name)
{
	TInstrumentV2* pInstrument = GetInstrument(instrument);

	if (pInstrument)
		strncpy_s(pInstrument->name, name, INSTRUMENT_NAME_MAX);
}


// Identify the shortest pattern length relative to the other ones used in the same Songline
// TODO: Move to CSong
BYTE CModule::GetShortestPatternLength(int subtune, int songline)
{
	return GetShortestPatternLength(GetSubtune(subtune), songline);
}

// Identify the shortest pattern length relative to the other ones used in the same Songline
// TODO: Move to CSong
BYTE CModule::GetShortestPatternLength(TSubtune* pSubtune, int songline)
{
	// Make sure the Subtune is not a Null pointer
	if (!pSubtune)
		return 0;

	// Get the current Pattern Length first
	BYTE patternLength = pSubtune->patternLength;

	// All channels will be processed in order to identify the shortest Pattern
	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		// Get the Pattern Index used in the Songline first
		BYTE pattern = pSubtune->channel[i].songline[songline];

		// Check for each Row that could be used within the shortest Pattern
		for (int j = 0; j < patternLength; j++)
		{
			// Check for all Effect Commands used in each Row
			for (int k = 0; k < pSubtune->channel[i].effectCount; k++)
			{
				// Get the Effect Command Identifier, the Parameter is not needed here
				BYTE command = pSubtune->channel[i].pattern[pattern].row[j].effect[k].command;

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
bool CModule::IsUnusedPattern(int subtune, int channel, int pattern)
{
	return IsUnusedPattern(GetChannel(subtune, channel), pattern);
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(TChannel* pChannel, int pattern)
{
	// Make sure the Index is not a Null pointer
	if (!pChannel)
		return false;

	// All Songlines in the Channel Index will be processed
	for (int i = 0; i < SONGLINE_MAX; i++)
	{
		// As soon as a match is found, we know for sure the Pattern is used at least once
		if (pChannel->songline[i] == pattern)
			return false;
	}

	// Otherwise, the Pattern is most likely unused
	return true;
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(int subtune, int channel, int pattern)
{
	return IsEmptyPattern(GetPattern(subtune, channel, pattern));
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(TPattern* pPattern)
{
	// Make sure the Pattern is not a Null pointer
	if (!pPattern)
		return false;

	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		// If there is a Note, it's not empty
		if (pPattern->row[i].note != PATTERN_NOTE_EMPTY)
			return false;

		// If there is an Instrument, it's not empty
		if (pPattern->row[i].instrument != PATTERN_INSTRUMENT_EMPTY)
			return false;

		// If there is a Volume, it's not empty
		if (pPattern->row[i].volume != PATTERN_VOLUME_EMPTY)
			return false;

		// If there is an Effect Command, it's not empty
		for (int j = 0; j < PATTERN_ACTIVE_EFFECT_MAX; j++)
		{
			// Only the Identifier is checked, since the Parameter cannot be used alone
			if (pPattern->row[i].effect[j].command != PATTERN_EFFECT_EMPTY)
				return false;
		}
	}

	// Otherwise, the Pattern is most likely empty
	return true;
}

// Compare 2 Patterns for identical data, Return True if successful
bool CModule::IsIdenticalPattern(TPattern* pFromPattern, TPattern* pToPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!pFromPattern || !pToPattern)
		return false;

	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		// If there is a different Note, it's not identical
		if (pFromPattern->row[i].note != pToPattern->row[i].note)
			return false;

		// If there is a different Instrument, it's not identical
		if (pFromPattern->row[i].instrument != pToPattern->row[i].instrument)
			return false;

		// If there is a different Volume, it's not identical
		if (pFromPattern->row[i].volume != pToPattern->row[i].volume)
			return false;

		// If there is a different Effect Command, it's not identical
		for (int j = 0; j < PATTERN_ACTIVE_EFFECT_MAX; j++)
		{
			if (pFromPattern->row[i].effect[j].command != pToPattern->row[i].effect[j].command)
				return false;

			if (pFromPattern->row[i].effect[j].parameter != pToPattern->row[i].effect[j].parameter)
				return false;
		}
	}

	// Otherwise, the Pattern is most likely identical
	return true;
}

// Duplicate a Pattern used in a Songline, Return True if successful
bool CModule::DuplicatePatternInSongline(int subtune, int channel, int songline, int pattern)
{
	// Find the first empty and unused Pattern that is available
	for (int i = 0; i < TRACK_PATTERN_MAX; i++)
	{
		// Ignore the Pattern that is being duplicated
		if (i == pattern)
			continue;

		// If the Pattern is empty and unused, it will be used for the duplication
		if (IsUnusedPattern(subtune, channel, i) && IsEmptyPattern(subtune, channel, i))
		{
			// Replace the Pattern used in the Songline Index with the new one as well
			if (CopyPattern(GetPattern(subtune, channel, pattern), GetPattern(subtune, channel, i)))
			{
				SetPatternInSongline(subtune, channel, songline, i);
				return true;
			}
		}
	}

	// Could not create a Pattern duplicate, no data was edited
	return false;
}

// Copy data from source Pattern to destination Pattern, Return True if successful
bool CModule::CopyPattern(TPattern* pFromPattern, TPattern* pToPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!pFromPattern || !pToPattern)
		return false;
	
	// Otherwise, copying Pattern data is pretty straightforward
	*pToPattern = *pFromPattern;

	// Pattern data should have been copied successfully
	return true;
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(int subtune, int channel, int pattern)
{
	return ClearPattern(GetPattern(subtune, channel, pattern));
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(TPattern* pPattern)
{
	// Make sure the Pattern is not a Null pointer
	if (!pPattern)
		return false;

	// Set all indexed Rows in the Pattern to empty values
	for (int i = 0; i < TRACK_ROW_MAX; i++)
	{
		pPattern->row[i].note = PATTERN_NOTE_EMPTY;
		pPattern->row[i].instrument = PATTERN_INSTRUMENT_EMPTY;
		pPattern->row[i].volume = PATTERN_VOLUME_EMPTY;

		for (int j = 0; j < PATTERN_ACTIVE_EFFECT_MAX; j++)
		{
			pPattern->row[i].effect[j].command = PATTERN_EFFECT_EMPTY;
			pPattern->row[i].effect[j].parameter = EFFECT_PARAMETER_MIN;
		}
	}

	// Pattern data should have been cleared successfully
	return true;
}

// Copy data from source Index to destination Index, Return True if successful
bool CModule::CopyChannel(TChannel* pFromChannel, TChannel* pToChannel)
{
	// Make sure both the Indexes from source and destination are not Null pointers
	if (!pFromChannel || !pToChannel)
		return false;

	// Otherwise, copying Index data is pretty straightforward
	*pToChannel = *pFromChannel;

	// Index data should have been copied successfully
	return true;
}

// Copy data from source Subtune to destination Subtune, Return True if successful
bool CModule::CopySubtune(TSubtune* pFromSubtune, TSubtune* pToSubtune)
{
	// Make sure both the Subtunes from source and destination are not Null pointers
	if (!pFromSubtune || !pToSubtune)
		return false;

	// Otherwise, copying Index data is pretty straightforward
	*pToSubtune = *pFromSubtune;

	// Subtune data should have been copied successfully
	return true;
}

// Duplicate a Channel Index in a Subtune, Return True if successful
bool CModule::DuplicateChannelIndex(int subtune, int sourceIndex, int destinationIndex)
{
	// If the duplication failed, nothing will be changed
	if (!CopyChannel(GetChannel(subtune, sourceIndex), GetChannel(subtune, destinationIndex)))
		return false;

	// Duplication should have been done successfully
	return true;
}

// Find and merge duplicated Patterns, and adjust the Songline Index accordingly
// TODO: Move to CSong
void CModule::MergeDuplicatedPatterns(int subtune)
{
	MergeDuplicatedPatterns(GetSubtune(subtune));
}

// Find and merge duplicated Patterns, and adjust the Songline Index accordingly
// TODO: Move to CSong
void CModule::MergeDuplicatedPatterns(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		for (int j = 0; j < pSubtune->songLength; j++)
		{
			// Get the Pattern from which comparisons will be made
			BYTE reference = pSubtune->channel[i].songline[j];

			// Compare to every Patterns found in the Channel Index, unused Patterns will be ignored
			for (int k = 0; k < pSubtune->songLength; k++)
			{
				// Get the Pattern that will be compared to the reference Pattern
				BYTE compared = pSubtune->channel[i].songline[k];

				// Comparing a Pattern to itself is pointless
				if (compared == reference)
					continue;

				// Compare the Patterns, and update the Songline Index if they are identical
				if (IsIdenticalPattern(&pSubtune->channel[i].pattern[reference], &pSubtune->channel[i].pattern[compared]))
					pSubtune->channel[i].songline[k] = reference;
			}
		}
	}
}

// Renumber all Patterns from first to last Songlines, without optimisations
// TODO: Move to CSong
void CModule::RenumberIndexedPatterns(int subtune)
{
	RenumberIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, without optimisations
// TODO: Move to CSong
void CModule::RenumberIndexedPatterns(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// Create a Temporary Index that will be used as a buffer
	TChannel* backupIndex = new TChannel;

	// Process all Channels within the Module Index
	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		// Get the current Channel Index
		TChannel* channelIndex = &pSubtune->channel[i];

		// Copy the original data to the Temporary Index first
		CopyChannel(channelIndex, backupIndex);

		// Copy all Indexed Patterns to single Songline entries, effectively duplicating all Patterns used in multiple Songlines
		for (int j = 0; j < SONGLINE_MAX; j++)
		{
			BYTE pattern = channelIndex->songline[j];
			CopyPattern(&channelIndex->pattern[pattern], &backupIndex->pattern[j]);
			backupIndex->songline[j] = j;
		}

		// Copy the re-organised data back to the original Channel Index
		CopyChannel(backupIndex, channelIndex);
	}

	// Delete the Temporary Index once it's no longer needed
	delete backupIndex;
}

// Clear all unused Indexed Patterns
// TODO: Move to CSong
void CModule::ClearUnusedPatterns(int subtune)
{
	ClearUnusedPatterns(GetSubtune(subtune));
}

// Clear all unused Indexed Patterns
// TODO: Move to CSong
void CModule::ClearUnusedPatterns(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// Process all Channels within the Module Index
	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		// Search for all unused indexed Patterns
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			// If the Pattern is not used anywhere, it will be deleted
			if (IsUnusedPattern(&pSubtune->channel[i], j))
				ClearPattern(&pSubtune->channel[i].pattern[j]);
		}
	}
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
// TODO: Move to CSong
void CModule::ConcatenateIndexedPatterns(int subtune)
{
	ConcatenateIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
// TODO: Move to CSong
void CModule::ConcatenateIndexedPatterns(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// Create a Temporary Index that will be used as a buffer
	TChannel* backupIndex = new TChannel;

	// Process all Channels within the Module Index
	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		// Get the current Channel Index
		TChannel* channelIndex = &pSubtune->channel[i];

		// Copy the original data to the Temporary Index first
		CopyChannel(channelIndex, backupIndex);

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
						for (int l = 0; l < pSubtune->songLength; l++)
						{
							if (backupIndex->songline[l] == j)
								backupIndex->songline[l] = k;
						}
					}
				}
			}
		}

		// Copy the re-organised data back to the original Channel Index
		CopyChannel(backupIndex, channelIndex);
	}

	// Delete the Temporary Index once it's no longer needed
	delete backupIndex;
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
// TODO: Move to CSong
void CModule::AllSizeOptimisations(int subtune)
{
	AllSizeOptimisations(GetSubtune(subtune));
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
// TODO: Move to CSong
void CModule::AllSizeOptimisations(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// First, renumber all Patterns
	RenumberIndexedPatterns(pSubtune);

	// Next, merge all duplicated Patterns
	MergeDuplicatedPatterns(pSubtune);

	// Then, Clear all unused Patterns
	ClearUnusedPatterns(pSubtune);

	// Finally, concatenate all Patterns
	ConcatenateIndexedPatterns(pSubtune);

	// And then...? Most likely a lot more... That's for another day...
}
