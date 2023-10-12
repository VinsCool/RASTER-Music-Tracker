// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"
#include "Atari6502.h"

CModule::CModule()
{
	m_subtuneIndex = new TSubtuneIndex();
	m_instrumentIndex = new TInstrumentIndex();
}

CModule::~CModule()
{
	ClearModule();
	delete m_subtuneIndex;
	delete m_instrumentIndex;
}

void CModule::InitialiseModule()
{
	// Set the default Module parameters
	ClearModule();
	//SetModuleName("Noname Song");
	//SetModuleAuthor("Unknown");
	//SetModuleCopyright("2023");

	// Create 1 empty Subtune, which will be used by default
	CreateSubtune(MODULE_DEFAULT_SUBTUNE);

	// Same thing for the Instrument
	CreateInstrument(MODULE_DEFAULT_INSTRUMENT);
}

void CModule::ClearModule()
{
	// Set the empty Module parameters
	SetModuleName("");
	SetModuleAuthor("");
	SetModuleCopyright("");

	// Delete all Module data and set the associated pointers to NULL
	DeleteAllSubtunes();

	// Delete all Instrument data, including Envelopes and Tables
	DeleteAllInstruments();
}


//--

void CModule::DeleteAllSubtunes()
{
	for (int i = 0; i < SUBTUNE_COUNT; i++)
		DeleteSubtune(i);
}

void CModule::DeleteAllChannels(TSubtune* pSubtune)
{
	for (int i = 0; i < CHANNEL_COUNT; i++)
		DeleteChannel(pSubtune, i);
}

void CModule::DeleteAllPatterns(TChannel* pChannel)
{
	for (int i = 0; i < PATTERN_COUNT; i++)
		DeletePattern(pChannel, i);
}

void CModule::DeleteAllRows(TPattern* pPattern)
{
	for (int i = 0; i < ROW_COUNT; i++)
		DeleteRow(pPattern, i);
}


//--

void CModule::DeleteAllInstruments()
{
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
		DeleteInstrument(i);
}


//--

TSubtune* CModule::CreateSubtune(UINT subtune)
{
	if (!m_subtuneIndex || !IsValidSubtune(subtune))
		return NULL;

	// If there is no Subtune here, create it now and update the Subtune Index accordingly
	if (!m_subtuneIndex->subtune[subtune])
		m_subtuneIndex->subtune[subtune] = CreateSubtune();

	return m_subtuneIndex->subtune[subtune];
}

TSubtune* CModule::CreateSubtune()
{
	TSubtune* pSubtune = new TSubtune();
	InitialiseSubtune(pSubtune);
	return pSubtune;
}

void CModule::DeleteSubtune(UINT subtune)
{
	if (!m_subtuneIndex || !IsValidSubtune(subtune))
		return;

	// If there is a Subtune here, don't waste any time and delete it without further ado
	DeleteSubtune(m_subtuneIndex->subtune[subtune]);
	m_subtuneIndex->subtune[subtune] = NULL;
}

void CModule::DeleteSubtune(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	DeleteAllChannels(pSubtune);
	delete pSubtune;
}

void CModule::InitialiseSubtune(UINT subtune)
{
	InitialiseSubtune(GetSubtune(subtune));
}

void CModule::InitialiseSubtune(TSubtune* pSubtune)
{
	if (!pSubtune)
		return;

	// Set the default Subtune name TODO(?): Append the Index Number to it
	SetSubtuneName(pSubtune, "");
	//strncpy_s(pSubtune->name, "Noname Subtune", SUBTUNE_NAME_MAX);

	// Set the default parameters to all the Subtune variables
	pSubtune->songLength = MODULE_DEFAULT_SONG_LENGTH;
	pSubtune->patternLength = MODULE_DEFAULT_PATTERN_LENGTH;
	pSubtune->channelCount = MODULE_DEFAULT_CHANNEL_COUNT;
	pSubtune->songSpeed = MODULE_DEFAULT_SONG_SPEED;
	pSubtune->instrumentSpeed = MODULE_DEFAULT_VBI_SPEED;

	// Delete all Channels with leftover data
	DeleteAllChannels(pSubtune);

	// Create brand new Channels as defined by the parameters above
	for (int i = 0; i < pSubtune->channelCount; i++)
		CreateChannel(pSubtune, i);
}

TChannel* CModule::CreateChannel(TSubtune* pSubtune, UINT channel)
{
	if (!pSubtune || !IsValidChannel(channel))
		return NULL;

	// If there is no Channel here, create it now and update the Channel Index accordingly
	if (!pSubtune->channel[channel])
		pSubtune->channel[channel] = CreateChannel();

	return pSubtune->channel[channel];
}

TChannel* CModule::CreateChannel()
{
	TChannel* pChannel = new TChannel();
	InitialiseChannel(pChannel);
	return pChannel;
}

void CModule::DeleteChannel(TSubtune* pSubtune, UINT channel)
{
	if (!pSubtune || !IsValidChannel(channel))
		return;

	// If there is a Channel here, don't waste any time and delete it without further ado
	DeleteChannel(pSubtune->channel[channel]);
	pSubtune->channel[channel] = NULL;
}

void CModule::DeleteChannel(TChannel* pChannel)
{
	if (!pChannel)
		return;

	DeleteAllPatterns(pChannel);
	delete pChannel;
}

void CModule::InitialiseChannel(TChannel* pChannel)
{
	if (!pChannel)
		return;

	// By default, only 1 Effect Command is enabled in all Track Channels
	pChannel->isMuted = false;
	pChannel->isEffectEnabled = true;
	pChannel->effectCount = 0x01;
	pChannel->channelVolume = 0x0F;

	// Set all indexed Patterns to 0
	for (int i = 0; i < SONGLINE_COUNT; i++)
		pChannel->songline[i] = 0x00;

	// Delete all Patterns with leftover data
	DeleteAllPatterns(pChannel);

	// Create 1 Empty Pattern associated to the Songline Index
	CreatePattern(pChannel, 0x00);
}

TPattern* CModule::CreatePattern(TChannel* pChannel, UINT pattern)
{
	if (!pChannel || !IsValidPattern(pattern))
		return NULL;

	// If there is no Pattern here, create it now and update the Pattern Index accordingly
	if (!pChannel->pattern[pattern])
		pChannel->pattern[pattern] = CreatePattern();

	return pChannel->pattern[pattern];
}

TPattern* CModule::CreatePattern()
{
	TPattern* pPattern = new TPattern();
	InitialisePattern(pPattern);
	return pPattern;
}

void CModule::DeletePattern(TChannel* pChannel, UINT pattern)
{
	if (!pChannel || !IsValidPattern(pattern))
		return;

	// If there is a Pattern here, don't waste any time and delete it without further ado
	DeletePattern(pChannel->pattern[pattern]);
	pChannel->pattern[pattern] = NULL;
}

void CModule::DeletePattern(TPattern* pPattern)
{
	if (!pPattern)
		return;

	DeleteAllRows(pPattern);
	delete pPattern;
}

void CModule::InitialisePattern(TPattern* pPattern)
{
	if (!pPattern)
		return;

	// Delete all Rows with leftover data
	DeleteAllRows(pPattern);

	// Fill the entire Pattern with brand new Rows
	for (int i = 0; i < ROW_COUNT; i++)
		CreateRow(pPattern, i);
}

TRow* CModule::CreateRow(TPattern* pPattern, UINT row)
{
	if (!pPattern || !IsValidRow(row))
		return NULL;

	// If there is no Row here, create it now and update the Row Index accordingly
	if (!pPattern->row[row])
		pPattern->row[row] = CreateRow();

	return pPattern->row[row];
}

TRow* CModule::CreateRow()
{
	TRow* pRow = new TRow();
	InitialiseRow(pRow);
	return pRow;
}

void CModule::DeleteRow(TPattern* pPattern, UINT row)
{
	if (!pPattern || !IsValidRow(row))
		return;

	// If there is a Row here, don't waste any time and delete it without further ado
	DeleteRow(pPattern->row[row]);
	pPattern->row[row] = NULL;
}

void CModule::DeleteRow(TRow* pRow)
{
	if (!pRow)
		return;

	delete pRow;
}

void CModule::InitialiseRow(TRow* pRow)
{
	if (!pRow)
		return;

	pRow->note = NOTE_EMPTY;
	pRow->instrument = INSTRUMENT_EMPTY;
	pRow->volume = VOLUME_EMPTY;

	for (int i = 0; i < ACTIVE_EFFECT_COUNT; i++)
	{
		pRow->effect[i].command = EFFECT_EMPTY;
		pRow->effect[i].parameter = EFFECT_PARAMETER_MIN;
	}
}

//--

TInstrumentV2* CModule::CreateInstrument(UINT instrument)
{
	if (!m_instrumentIndex || !IsValidInstrument(instrument))
		return NULL;

	// If there is no Instrument here, create it now and update the Instrument Index accordingly
	if (!m_instrumentIndex->instrument[instrument])
		m_instrumentIndex->instrument[instrument] = CreateInstrument();

	return m_instrumentIndex->instrument[instrument];
}

TInstrumentV2* CModule::CreateInstrument()
{
	TInstrumentV2* pInstrument = new TInstrumentV2();
	InitialiseInstrument(pInstrument);
	return pInstrument;
}

void CModule::DeleteInstrument(UINT instrument)
{
	if (!m_instrumentIndex || !IsValidInstrument(instrument))
		return;

	// If there is an Instrument here, don't waste any time and delete it without further ado
	DeleteInstrument(m_instrumentIndex->instrument[instrument]);
	m_instrumentIndex->instrument[instrument] = NULL;
}

void CModule::DeleteInstrument(TInstrumentV2* pInstrument)
{
	if (!pInstrument)
		return;
	
	// Do something here...?
	delete pInstrument;
}

void CModule::InitialiseInstrument(UINT instrument)
{
	InitialiseInstrument(GetInstrument(instrument));
}

void CModule::InitialiseInstrument(TInstrumentV2* pInstrument)
{
	if (!pInstrument)
		return;

	// Set the default Instrument name TODO(?): Append the Index Number to it
	strncpy_s(pInstrument->name, "New Instrument", INSTRUMENT_NAME_MAX);

	pInstrument->volumeFade = 0x00;
	pInstrument->volumeSustain = 0x00;
	pInstrument->vibrato = 0x00;
	pInstrument->freqShift = 0x00;
	pInstrument->delay = 0x00;

	// Set the default Envelope parameters, always disabled for newly created Instruments
	TMacro macro{ 0x00, false, false };

	pInstrument->envelope.volume = macro;
	pInstrument->envelope.timbre = macro;
	pInstrument->envelope.audctl = macro;
	pInstrument->envelope.trigger = macro;
	pInstrument->envelope.effect = macro;
	pInstrument->envelope.note = macro;
	pInstrument->envelope.freq = macro;
}

/*
void CModule::CreateVolumeEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is no Envelope here, create it now and update the Envelope Index accordingly
	if (!m_volumeIndex[envelope])
		m_volumeIndex[envelope] = new TInstrumentEnvelope;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentEnvelope(m_volumeIndex[envelope]);
}

void CModule::DeleteVolumeEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is an Envelope here, don't waste any time and delete it without further ado
	if (m_volumeIndex[envelope])
		delete m_volumeIndex[envelope];

	m_volumeIndex[envelope] = NULL;
}

void CModule::CreateTimbreEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is no Envelope here, create it now and update the Envelope Index accordingly
	if (!m_timbreIndex[envelope])
		m_timbreIndex[envelope] = new TInstrumentEnvelope;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentEnvelope(m_timbreIndex[envelope]);
}

void CModule::DeleteTimbreEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is an Envelope here, don't waste any time and delete it without further ado
	if (m_timbreIndex[envelope])
		delete m_timbreIndex[envelope];

	m_timbreIndex[envelope] = NULL;
}

void CModule::CreateAudctlEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is no Envelope here, create it now and update the Envelope Index accordingly
	if (!m_audctlIndex[envelope])
		m_audctlIndex[envelope] = new TInstrumentEnvelope;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentEnvelope(m_audctlIndex[envelope]);
}

void CModule::DeleteAudctlEnvelope(int envelope)
{
	if (!IsValidInstrument(envelope))
		return;

	// If there is an Envelope here, don't waste any time and delete it without further ado
	if (m_audctlIndex[envelope])
		delete m_audctlIndex[envelope];

	m_audctlIndex[envelope] = NULL;
}

void CModule::InitialiseInstrumentEnvelope(TInstrumentEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return;

	// Clear all values
	memset(pEnvelope, 0x00, sizeof(TInstrumentEnvelope));
}


void CModule::CreateTriggerEnvelope(int trigger)
{
	if (!IsValidInstrument(trigger))
		return;

	// If there is no Envelope here, create it now and update the Envelope Index accordingly
	if (!m_triggerIndex[trigger])
		m_triggerIndex[trigger] = new TInstrumentTrigger;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentTrigger(m_triggerIndex[trigger]);
}

void CModule::DeleteTriggerEnvelope(int trigger)
{
	if (!IsValidInstrument(trigger))
		return;

	// If there is an Envelope here, don't waste any time and delete it without further ado
	if (m_triggerIndex[trigger])
		delete m_triggerIndex[trigger];

	m_triggerIndex[trigger] = NULL;
}

void CModule::InitialiseInstrumentTrigger(TInstrumentTrigger* pTrigger)
{
	if (!pTrigger)
		return;

	// Clear all values
	memset(pTrigger, 0x00, sizeof(TInstrumentTrigger));
}


void CModule::CreateEffectEnvelope(int effect)
{
	if (!IsValidInstrument(effect))
		return;

	// If there is no Envelope here, create it now and update the Envelope Index accordingly
	if (!m_effectIndex[effect])
		m_effectIndex[effect] = new TInstrumentEffect;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentEffect(m_effectIndex[effect]);
}

void CModule::DeleteEffectEnvelope(int effect)
{
	if (!IsValidInstrument(effect))
		return;

	// If there is an Envelope here, don't waste any time and delete it without further ado
	if (m_effectIndex[effect])
		delete m_effectIndex[effect];

	m_effectIndex[effect] = NULL;
}

void CModule::InitialiseInstrumentEffect(TInstrumentEffect* pEffect)
{
	if (!pEffect)
		return;

	// Clear all values
	memset(pEffect, 0x00, sizeof(TInstrumentEffect));
}


void CModule::CreateNoteTable(int table)
{
	if (!IsValidInstrument(table))
		return;

	// If there is no Table here, create it now and update the Table Index accordingly
	if (!m_noteIndex[table])
		m_noteIndex[table] = new TInstrumentTable;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentTable(m_noteIndex[table]);
}

void CModule::DeleteNoteTable(int table)
{
	if (!IsValidInstrument(table))
		return;

	// If there is a Table here, don't waste any time and delete it without further ado
	if (m_noteIndex[table])
		delete m_noteIndex[table];

	m_noteIndex[table] = NULL;
}

void CModule::CreateFreqTable(int table)
{
	if (!IsValidInstrument(table))
		return;

	// If there is no Table here, create it now and update the Table Index accordingly
	if (!m_freqIndex[table])
		m_freqIndex[table] = new TInstrumentTable;

	// A new Envelope must be initialised when it is created
	InitialiseInstrumentTable(m_freqIndex[table]);
}

void CModule::DeleteFreqTable(int table)
{
	if (!IsValidInstrument(table))
		return;

	// If there is a Table here, don't waste any time and delete it without further ado
	if (m_freqIndex[table])
		delete m_freqIndex[table];

	m_freqIndex[table] = NULL;
}

void CModule::InitialiseInstrumentTable(TInstrumentTable* pTable)
{
	if (!pTable)
		return;

	// Clear all values
	memset(pTable, 0x00, sizeof(TInstrumentTable));
}
*/

bool CModule::ImportLegacyRMT(std::ifstream& in)
{
/*
	CString importLog;
	importLog.Format("");

	WORD songlineStep[SONGLINE_COUNT];
	memset(songlineStep, INVALID, SONGLINE_COUNT);

	BYTE subtuneOffset[SONGLINE_COUNT];
	memset(subtuneOffset, 0, SONGLINE_COUNT);

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
			for (int j = 0; j < PATTERN_COUNT; j++)
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
			memset(songlineStep, INVALID, SONGLINE_COUNT);

			// Re-construct every Songlines used by the Subtune, until the loop point is found
			for (int j = 0; j < SONGLINE_COUNT; j++)
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
					for (int l = 0; l < ROW_COUNT; l++)
					{
						// The Fxx Commands are perfectly fine as they are, so the Effect Column 1 is also skipped
						for (int m = CMD2; m < ACTIVE_EFFECT_COUNT; m++)
							SetPatternRowCommand(i, k, GetPatternInSongline(i, k, j), l, m, EFFECT_EMPTY, EFFECT_PARAMETER_MIN);
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
*/
	return true;
}

// Decode Legacy RMT Module Data, Return True if successful
// If an invalid parameter was found, settle for a compromise using the default values
// The import procedure will be aborted if there is no suitable recovery, however!
bool CModule::DecodeLegacyRMT(std::ifstream& in, TSubtune* pSubtune, CString& log)
{
/*
	// Make sure the Subtune is not a Null pointer
	if (!pSubtune)
		return false;

	CString s;

	WORD fromAddr, toAddr;

	BYTE mem[65536];
	memset(mem, 0, 65536);

	BYTE instrumentLoaded[INSTRUMENT_COUNT];
	memset(instrumentLoaded, 0, INSTRUMENT_COUNT);

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
		pSubtune->channelCount = MODULE_CHANNEL_COUNT;
		log.AppendFormat("Warning: Invalid number of Channels, 4 or 8 were expected\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", pSubtune->channelCount);
	}

	// Invalid Song Speed
	if (pSubtune->songSpeed < 1)
	{
		pSubtune->songSpeed = MODULE_DEFAULT_SONG_SPEED;
		log.AppendFormat("Warning: Song Speed could not be 0.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", pSubtune->songSpeed);
	}

	// Invalid Instrument Speed
	if (pSubtune->instrumentSpeed < 1 || pSubtune->instrumentSpeed > 8)
	{
		pSubtune->instrumentSpeed = MODULE_DEFAULT_VBI_SPEED;
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
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
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
*/
	// Decoding of Legacy RMT Module should have been successful
	return true;
}

// Import Legacy RMT Pattern Data, Return True if successful
bool CModule::ImportLegacyPatterns(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress)
{
/*
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
*/
	// Legacy RMT Patterns should have been imported successfully
	return true;
}

// Import Legacy RMT Songline Data, Return True if successful
bool CModule::ImportLegacySonglines(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress)
{
/*
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
		if (line >= SONGLINE_COUNT)
			break;
	}

	// Set the Songlength to the number of decoded Songlines
	pSubtune->songLength = (BYTE)line;
*/
	// Legacy RMT Songlines should have been imported successfully
	return true;
}

// Import Legacy RMT Instrument Data, Return True if successful
bool CModule::ImportLegacyInstruments(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded)
{
/*
	// Make sure both the Subtune and Source Memory are not Null pointers
	if (!pSubtune || !sourceMemory)
		return false;

	// Get the pointers used for decoding the Legacy RMT Instrument data
	WORD ptrInstruments = sourceMemory[sourceAddress + 8] + (sourceMemory[sourceAddress + 9] << 8);
	WORD ptrEnd = sourceMemory[sourceAddress + 10] + (sourceMemory[sourceAddress + 11] << 8);

	// Number of instruments to decode
	WORD instrumentCount = (ptrEnd - ptrInstruments) / 2;

	// Abort the import procedure if the number of Instruments detected is invalid
	if (instrumentCount > INSTRUMENT_COUNT)
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

		// Create all Envelopes and Tables, unique for each Instruments
		CreateVolumeEnvelope(i);
		CreateTimbreEnvelope(i);
		CreateAudctlEnvelope(i);
		CreateTriggerEnvelope(i);
		CreateEffectEnvelope(i);
		CreateNoteTable(i);
		CreateFreqTable(i);

		// Assign everything in the Instrument Index once the data was initialised
		pInstrument->index.volume = i;
		pInstrument->index.timbre = i;
		pInstrument->index.audctl = i;
		pInstrument->index.trigger = i;
		pInstrument->index.effect = i;
		pInstrument->index.note = i;
		pInstrument->index.freq = i;

		// Get the Envelopes, Tables, and other parameters from the original RMT instrument data
		BYTE* memInstrument = sourceMemory + ptrOneInstrument;
		BYTE envelopePtr = memInstrument[0];							// Pointer to Instrument Envelope
		BYTE tablePtr = 12;												// Pointer to Instrument Table

		// Set the equivalent data to the RMTE instrument, with respect to boundaries
		BYTE tableLength = envelopePtr - tablePtr + 1;
		BYTE tableLoop = memInstrument[1] - tablePtr;

		BYTE envelopeLength = ((memInstrument[2] - envelopePtr + 1) / 3) + 1;
		BYTE envelopeLoop = ((memInstrument[3] - envelopePtr + 1) / 3);
		BYTE envelopeSpeed = 0x01;

		BYTE tableMode = (memInstrument[4] >> 6) & 0x01;				// Table Mode, 0 = Set, 1 = Additive
		BYTE tableSpeed = (memInstrument[4] & 0x3F) + 1;				// Table Speed, used to offset the equivalent Tables

		BYTE tableType = memInstrument[4] >> 7;							// Table Type, 0 = Note, 1 = Freq
		BYTE initialAudctl = memInstrument[5];							// AUDCTL, used to initialise the equivalent Envelope
		BYTE initialTimbre = 0x0A;										// RMT 1.34 Distortion 6 uses the Distortion A by default
		bool initialSkctl = false;										// SKCTL, used for the Two-Tone Filter Trigger Envelope

		BYTE delay = memInstrument[8];									// Vibrato/Freq Shift Delay
		BYTE vibrato = memInstrument[9] & 0x03;							// Vibrato
		BYTE freqShift = memInstrument[10];								// Freq Shift

		pInstrument->volumeFade = memInstrument[6];						// Volume Slide
		pInstrument->volumeSustain = memInstrument[7] >> 4;				// Volume Minimum

		// Import the Vibrato with adjustments to make sound similar to the original implementation
		switch (vibrato)
		{
		case 0x01: vibrato = 0x0F; break;
		case 0x02: vibrato = 0x0B; break;
		case 0x03: vibrato = 0x07; break;
		}

		// Overwrite the Delay, Vibrato and Freqshift parameters with updated values if changes were needed
		pInstrument->delay = delay? delay - 1 : 0x00;
		pInstrument->vibrato = delay ? vibrato : 0x00;
		pInstrument->freqShift = delay ? freqShift : 0x00;

		// Fill the equivalent RMTE tables based on the tableType parameter
		if (tableLength > 32)
			tableLength = 0x00;

		if (tableLoop > tableLength)
			tableLoop = 0x00;

		if (envelopeLength > 48)
			envelopeLength = 0x00;

		if (envelopeLoop > envelopeLength)
			envelopeLoop = 0x00;

		// Table Type is either Freq or Note, so pick whichever is suitable for the next part
		TInstrumentTable* pTable = tableType ? m_freqIndex[i] : m_noteIndex[i];

		// Assign the necessary flags and parameters to it
		pTable->flag.isLooped = true;
		pTable->flag.isAbsolute = false;
		pTable->flag.isAdditive = tableMode;
		pTable->flag.isReleased = false;

		pTable->parameter.length = tableLength;
		pTable->parameter.loop = tableLoop;
		pTable->parameter.release = 0x00;
		pTable->parameter.speed = tableSpeed;
		
		// Fill the appropriate Table with these values
		for (int j = 0; j < tableLength; j++)
			pTable->table[j] = memInstrument[tablePtr + j];

		// Enable all envelopes, since everything may be used at once!
		TParameter parameter{ envelopeLength, envelopeLoop, 0x00, envelopeSpeed };
		TFlag flag{ true, false, false, false };

		// Get the pointer to all the Envelopes that will be filled with the imported data
		TInstrumentEnvelope* pVolume = m_volumeIndex[i], * pTimbre = m_timbreIndex[i], * pAudctl = m_audctlIndex[i];
		TInstrumentTrigger* pTrigger = m_triggerIndex[i];
		TInstrumentEffect* pEffect = m_effectIndex[i];

		// Assign the necessary flags and parameters to all of them
		pVolume->parameter = pTimbre->parameter = pAudctl->parameter = pTrigger->parameter = pEffect->parameter = parameter;
		pVolume->flag = pTimbre->flag = pAudctl->flag = pTrigger->flag = pEffect->flag = flag;

		// Fill the equivalent RMTE envelopes, which might include some compromises due to the format differences
		for (int j = 0; j < envelopeLength; j++)
		{
			// Get the 3 bytes used by the original RMT Envelope format
			BYTE envelopeVolume = memInstrument[envelopePtr + 1 + (j * 3)];
			BYTE envelopeCommand = memInstrument[envelopePtr + 1 + (j * 3) + 1];
			BYTE envelopeParameter = memInstrument[envelopePtr + 1 + (j * 3) + 2];

			// Envelope Effect Command, from 0 to 7
			BYTE envelopeEffectCommand = (envelopeCommand >> 4) & 0x07;

			// Envelope Distortion, from 0 to E, in steps of 2
			BYTE distortion = envelopeCommand & 0x0E;

			// Volume Only Mode flag, used for the Volume Envelope when it is set
			bool isVolumeOnly = false;

			// The Envelope Effect Command is used for compatibility tweaks, which may or may not provide perfect results
			switch (envelopeEffectCommand)
			{
			case 0x07:
				// Overwrite the initialAudctl parameter, for a pseudo AUDCTL envelope when it is used multiple times (Patch16 only)
				if (envelopeParameter < 0xFD)
					initialAudctl = envelopeParameter;

				else if (envelopeParameter == 0xFD)
					initialSkctl = false;

				else if (envelopeParameter == 0xFE)
					initialSkctl = true;

				else if (envelopeParameter == 0xFF)
					isVolumeOnly = true;

				envelopeParameter = envelopeEffectCommand = 0x00;
				break;

			case 0x06:
				// Various hacks were used with this command in the RMT 1.34 driver, such as the Distortion 6... Distortion used in 16-bit mode
				if (distortion == 0x06)
				{
					initialTimbre = envelopeParameter & 0x0E;
					envelopeParameter = envelopeEffectCommand = 0x00;
				}
				break;
			}

			// See above? Yeah, that, it does the thing, amazing isn't it?
			if (distortion == 0x06)
			{
				// The original "Auto16Bit" trigger ;)
				pTrigger->trigger[j].auto16Bit = true;
				distortion = initialTimbre;
			}

			// To be converted to the equivalent Timbre parameter
			switch (distortion)
			{
			case 0x00:
				pTimbre->envelope[j] = TIMBRE_PINK_NOISE;
				break;

			case 0x02:
				pTimbre->envelope[j] = TIMBRE_BELL;
				break;

			case 0x04:
				pTimbre->envelope[j] = TIMBRE_SMOOTH_4;
				break;

			case 0x08:
				pTimbre->envelope[j] = TIMBRE_WHITE_NOISE;
				break;

			case 0x06:
				// The original "Auto16Bit" trigger ;)
				//pTrigger->trigger[j].auto16Bit = true;

			case 0x0A:
				pTimbre->envelope[j] = TIMBRE_PURE;
				break;

			case 0x0C:
				pTimbre->envelope[j] = TIMBRE_BUZZY_C;
				break;

			case 0x0E:
				pTimbre->envelope[j] = TIMBRE_GRITTY_C;
				break;
			}

			// Envelope Volume, only Left POKEY volume is supported by the RMTE format
			pVolume->envelope[j] = envelopeVolume & 0x0F;

			// Set the Volume Only Mode as well if needed
			if (isVolumeOnly)
				pVolume->envelope[j] |= 0x10;

			// Envelope AUDCTL
			pAudctl->envelope[j] = initialAudctl;

			// AutoFilter Trigger
			pTrigger->trigger[j].autoFilter = envelopeCommand >> 7;

			// AutoTwoTone Trigger
			pTrigger->trigger[j].autoTwoTone = initialSkctl;

			// Portamento a Pattern Effect Command could be set where the Portamento is expected as a compromise
			pTrigger->trigger[j].autoPortamento = envelopeCommand & 0x01;

			// Extended RMT Command Envelope, with compatibility tweaks as a compromise
			pEffect->effect[j].command = envelopeEffectCommand;
			pEffect->effect[j].parameter = envelopeParameter;
		}

		// Instrument was loaded
		isLoaded[i]++;
	}
*/
	// Legacy RMT Instruments should have been imported successfully
	return true;
}


// ----------------------------------------------------------------------------
// Functions related to Pattern Data editing, aimed at bulk operations and/or repetitive procedures
// These are broken down into generic functions, to make recycling for other uses fairly easy
//

TSubtune* CModule::GetSubtune(UINT subtune)
{
	// A new Subtune will be returned if it didn't already exist
	return CreateSubtune(subtune);
}

TChannel* CModule::GetChannel(UINT subtune, UINT channel)
{
	return GetChannel(GetSubtune(subtune), channel);
}

TChannel* CModule::GetChannel(TSubtune* pSubtune, UINT channel)
{
	// A new Channel will be returned if it didn't already exist
	return CreateChannel(pSubtune, channel);
}

TPattern* CModule::GetPattern(UINT subtune, UINT channel, UINT pattern)
{
	return GetPattern(GetChannel(subtune, channel), pattern);
}

TPattern* CModule::GetPattern(TSubtune* pSubtune, UINT channel, UINT pattern)
{
	return GetPattern(GetChannel(pSubtune, channel), pattern);
}

TPattern* CModule::GetPattern(TChannel* pChannel, UINT pattern)
{
	// A new Pattern will be returned if it didn't already exist
	return CreatePattern(pChannel, pattern);
}

TPattern* CModule::GetIndexedPattern(UINT subtune, UINT channel, UINT songline)
{
	return GetIndexedPattern(GetChannel(subtune, channel), songline);
}

TPattern* CModule::GetIndexedPattern(TSubtune* pSubtune, UINT channel, UINT songline)
{
	return GetIndexedPattern(GetChannel(pSubtune, channel), songline);
}

TPattern* CModule::GetIndexedPattern(TChannel* pChannel, UINT songline)
{
	if (pChannel && IsValidSongline(songline))
		return GetPattern(pChannel, pChannel->songline[songline]);

	return NULL;
}

TRow* CModule::GetRow(UINT subtune, UINT channel, UINT pattern, UINT row)
{
	return GetRow(GetPattern(subtune, channel, pattern), row);
}

TRow* CModule::GetRow(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row)
{
	return GetRow(GetPattern(pSubtune, channel, pattern), row);
}

TRow* CModule::GetRow(TChannel* pChannel, UINT pattern, UINT row)
{
	return GetRow(GetPattern(pChannel, pattern), row);
}

TRow* CModule::GetRow(TPattern* pPattern, UINT row)
{
	// A new Row will be returned if it didn't already exist
	return CreateRow(pPattern, row);
}


//--

const UINT CModule::GetPatternInSongline(UINT subtune, UINT channel, UINT songline)
{
	return GetPatternInSongline(GetChannel(subtune, channel), songline);
}

const UINT CModule::GetPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline)
{
	return GetPatternInSongline(GetChannel(pSubtune, channel), songline);
}

const UINT CModule::GetPatternInSongline(TChannel* pChannel, UINT songline)
{
	if (pChannel && IsValidSongline(songline))
		return pChannel->songline[songline];

	return INVALID;
}

const UINT CModule::GetPatternRowNote(UINT subtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowNote(GetRow(subtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowNote(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowNote(GetRow(pSubtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowNote(TChannel* pChannel, UINT pattern, UINT row)
{
	return GetPatternRowNote(GetRow(pChannel, pattern, row));
}

const UINT CModule::GetPatternRowNote(TPattern* pPattern, UINT row)
{
	return GetPatternRowNote(GetRow(pPattern, row));
}

const UINT CModule::GetPatternRowNote(TRow* pRow)
{
	if (pRow)
		return pRow->note;

	return INVALID;
}

const UINT CModule::GetPatternRowInstrument(UINT subtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowInstrument(GetRow(subtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowInstrument(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowInstrument(GetRow(pSubtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowInstrument(TChannel* pChannel, UINT pattern, UINT row)
{
	return GetPatternRowInstrument(GetRow(pChannel, pattern, row));
}

const UINT CModule::GetPatternRowInstrument(TPattern* pPattern, UINT row)
{
	return GetPatternRowInstrument(GetRow(pPattern, row));
}

const UINT CModule::GetPatternRowInstrument(TRow* pRow)
{
	if (pRow)
		return pRow->instrument;

	return INVALID;
}

const UINT CModule::GetPatternRowVolume(UINT subtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowVolume(GetRow(subtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowVolume(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row)
{
	return GetPatternRowVolume(GetRow(pSubtune, channel, pattern, row));
}

const UINT CModule::GetPatternRowVolume(TChannel* pChannel, UINT pattern, UINT row)
{
	return GetPatternRowVolume(GetRow(pChannel, pattern, row));
}

const UINT CModule::GetPatternRowVolume(TPattern* pPattern, UINT row)
{
	return GetPatternRowVolume(GetRow(pPattern, row));
}

const UINT CModule::GetPatternRowVolume(TRow* pRow)
{
	if (pRow)
		return pRow->volume;

	return INVALID;
}

const UINT CModule::GetPatternRowEffectCommand(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectCommand(GetRow(subtune, channel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectCommand(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectCommand(GetRow(pSubtune, channel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectCommand(TChannel* pChannel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectCommand(GetRow(pChannel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectCommand(TPattern* pPattern, UINT row, UINT column)
{
	return GetPatternRowEffectCommand(GetRow(pPattern, row), column);
}

const UINT CModule::GetPatternRowEffectCommand(TRow* pRow, UINT column)
{
	if (pRow && IsValidCommandColumn(column))
		return pRow->effect[column].command;

	return INVALID;
}

const UINT CModule::GetPatternRowEffectParameter(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectParameter(GetRow(subtune, channel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectParameter(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectParameter(GetRow(pSubtune, channel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectParameter(TChannel* pChannel, UINT pattern, UINT row, UINT column)
{
	return GetPatternRowEffectParameter(GetRow(pChannel, pattern, row), column);
}

const UINT CModule::GetPatternRowEffectParameter(TPattern* pPattern, UINT row, UINT column)
{
	return GetPatternRowEffectParameter(GetRow(pPattern, row), column);
}

const UINT CModule::GetPatternRowEffectParameter(TRow* pRow, UINT column)
{
	if (pRow && IsValidCommandColumn(column))
		return pRow->effect[column].parameter;

	return INVALID;
}


//--

bool CModule::SetPatternInSongline(UINT subtune, UINT channel, UINT songline, UINT pattern)
{
	return SetPatternInSongline(GetChannel(subtune, channel), songline, pattern);
}

bool CModule::SetPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline, UINT pattern)
{
	return SetPatternInSongline(GetChannel(pSubtune, channel), songline, pattern);
}

bool CModule::SetPatternInSongline(TChannel* pChannel, UINT songline, UINT pattern)
{
	if (pChannel && IsValidSongline(songline) && IsValidPattern(pattern))
	{
		pChannel->songline[songline] = pattern;
		return true;
	}

	return false;
}

bool CModule::SetPatternRowNote(UINT subtune, UINT channel, UINT pattern, UINT row, UINT note)
{
	return SetPatternRowNote(GetRow(subtune, channel, pattern, row), note);
}

bool CModule::SetPatternRowNote(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT note)
{
	return SetPatternRowNote(GetRow(pSubtune, channel, pattern, row), note);
}

bool CModule::SetPatternRowNote(TChannel* pChannel, UINT pattern, UINT row, UINT note)
{
	return SetPatternRowNote(GetRow(pChannel, pattern, row), note);
}

bool CModule::SetPatternRowNote(TPattern* pPattern, UINT row, UINT note)
{
	return SetPatternRowNote(GetRow(pPattern, row), note);
}

bool CModule::SetPatternRowNote(TRow* pRow, UINT note)
{
	if (pRow && IsValidNoteIndex(note))
	{
		pRow->note = note;
		return true;
	}

	return false;
}

bool CModule::SetPatternRowInstrument(UINT subtune, UINT channel, UINT pattern, UINT row, UINT instrument)
{
	return SetPatternRowInstrument(GetRow(subtune, channel, pattern, row), instrument);
}

bool CModule::SetPatternRowInstrument(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT instrument)
{
	return SetPatternRowInstrument(GetRow(pSubtune, channel, pattern, row), instrument);
}

bool CModule::SetPatternRowInstrument(TChannel* pChannel, UINT pattern, UINT row, UINT instrument)
{
	return SetPatternRowInstrument(GetRow(pChannel, pattern, row), instrument);
}

bool CModule::SetPatternRowInstrument(TPattern* pPattern, UINT row, UINT instrument)
{
	return SetPatternRowInstrument(GetRow(pPattern, row), instrument);
}

bool CModule::SetPatternRowInstrument(TRow* pRow, UINT instrument)
{
	if (pRow && IsValidInstrumentIndex(instrument))
	{
		pRow->instrument = instrument;
		return true;
	}

	return false;
}

bool CModule::SetPatternRowVolume(UINT subtune, UINT channel, UINT pattern, UINT row, UINT volume)
{
	return SetPatternRowVolume(GetRow(subtune, channel, pattern, row), volume);
}

bool CModule::SetPatternRowVolume(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT volume)
{
	return SetPatternRowVolume(GetRow(pSubtune, channel, pattern, row), volume);
}

bool CModule::SetPatternRowVolume(TChannel* pChannel, UINT pattern, UINT row, UINT volume)
{
	return SetPatternRowVolume(GetRow(pChannel, pattern, row), volume);
}

bool CModule::SetPatternRowVolume(TPattern* pPattern, UINT row, UINT volume)
{
	return SetPatternRowVolume(GetRow(pPattern, row), volume);
}

bool CModule::SetPatternRowVolume(TRow* pRow, UINT volume)
{
	if (pRow && IsValidVolumeIndex(volume))
	{
		pRow->volume = volume;
		return true;
	}

	return false;
}

bool CModule::SetPatternRowEffectCommand(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column, UINT command)
{
	return SetPatternRowEffectCommand(GetRow(subtune, channel, pattern, row), column, command);
}

bool CModule::SetPatternRowEffectCommand(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column, UINT command)
{
	return SetPatternRowEffectCommand(GetRow(pSubtune, channel, pattern, row), column, command);
}

bool CModule::SetPatternRowEffectCommand(TChannel* pChannel, UINT pattern, UINT row, UINT column, UINT command)
{
	return SetPatternRowEffectCommand(GetRow(pChannel, pattern, row), column, command);
}

bool CModule::SetPatternRowEffectCommand(TPattern* pPattern, UINT row, UINT column, UINT command)
{
	return SetPatternRowEffectCommand(GetRow(pPattern, row), column, command);
}

bool CModule::SetPatternRowEffectCommand(TRow* pRow, UINT column, UINT command)
{
	if (pRow && IsValidCommandColumn(column) && IsValidEffectCommandIndex(command))
	{
		pRow->effect[column].command = command;
		return true;
	}

	return false;
}

bool CModule::SetPatternRowEffectParameter(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column, UINT parameter)
{
	return SetPatternRowEffectParameter(GetRow(subtune, channel, pattern, row), column, parameter);
}

bool CModule::SetPatternRowEffectParameter(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column, UINT parameter)
{
	return SetPatternRowEffectParameter(GetRow(pSubtune, channel, pattern, row), column, parameter);
}

bool CModule::SetPatternRowEffectParameter(TChannel* pChannel, UINT pattern, UINT row, UINT column, UINT parameter)
{
	return SetPatternRowEffectParameter(GetRow(pChannel, pattern, row), column, parameter);
}

bool CModule::SetPatternRowEffectParameter(TPattern* pPattern, UINT row, UINT column, UINT parameter)
{
	return SetPatternRowEffectParameter(GetRow(pPattern, row), column, parameter);
}

bool CModule::SetPatternRowEffectParameter(TRow* pRow, UINT column, UINT parameter)
{
	if (pRow && IsValidCommandColumn(column) && IsValidEffectParameter(parameter))
	{
		pRow->effect[column].parameter = parameter;
		return true;
	}

	return false;
}


//--

const char* CModule::GetSubtuneName(UINT subtune)
{
	return GetSubtuneName(GetSubtune(subtune));
}

const char* CModule::GetSubtuneName(TSubtune* pSubtune)
{
	if (pSubtune)
		return pSubtune->name;

	return NULL;
}

const UINT CModule::GetSongLength(UINT subtune)
{
	return GetSongLength(GetSubtune(subtune));
}

const UINT CModule::GetSongLength(TSubtune* pSubtune)
{
	if (pSubtune)
	{
		UINT songLength = pSubtune->songLength;

		// 0 is actually the highest possible value due to base 0 indexing
		if (songLength == 0)
			songLength = SONGLINE_COUNT;

		return songLength;
	}

	return EMPTY;
}

const UINT CModule::GetPatternLength(UINT subtune)
{
	return GetPatternLength(GetSubtune(subtune));
}

const UINT CModule::GetPatternLength(TSubtune* pSubtune)
{
	if (pSubtune)
	{
		UINT patternLength = pSubtune->patternLength;

		// 0 is actually the highest possible value due to base 0 indexing
		if (patternLength == 0)
			patternLength = ROW_COUNT;

		return patternLength;
	}

	return EMPTY;
}

const UINT CModule::GetChannelCount(UINT subtune)
{
	return GetChannelCount(GetSubtune(subtune));
}

const UINT CModule::GetChannelCount(TSubtune* pSubtune)
{
	if (pSubtune)
	{
		UINT channelCount = pSubtune->channelCount;

		// 0 is actually the highest possible value due to base 0 indexing
		if (channelCount == 0)
			channelCount = CHANNEL_COUNT;

		return channelCount;
	}

	return EMPTY;
}

const UINT CModule::GetSongSpeed(UINT subtune)
{
	return GetSongSpeed(GetSubtune(subtune));
}

const UINT CModule::GetSongSpeed(TSubtune* pSubtune)
{
	if (pSubtune)
	{
		UINT songSpeed = pSubtune->songSpeed;

		// 0 is actually the highest possible value due to base 0 indexing
		if (songSpeed == 0)
			songSpeed = SONG_SPEED_MAX;

		return songSpeed;
	}

	return EMPTY;
}

const UINT CModule::GetInstrumentSpeed(UINT subtune)
{
	return GetInstrumentSpeed(GetSubtune(subtune));
}

const UINT CModule::GetInstrumentSpeed(TSubtune* pSubtune)
{
	if (pSubtune)
	{
		UINT instrumentSpeed = pSubtune->instrumentSpeed;

		// 0 is actually the highest possible value due to base 0 indexing
		if (instrumentSpeed == 0)
			instrumentSpeed = INSTRUMENT_SPEED_MAX;

		return instrumentSpeed;
	}

	return EMPTY;
}

const UINT CModule::GetEffectCommandCount(UINT subtune, UINT channel)
{
	return GetEffectCommandCount(GetChannel(subtune, channel));
}

const UINT CModule::GetEffectCommandCount(TSubtune* pSubtune, UINT channel)
{
	return GetEffectCommandCount(GetChannel(pSubtune, channel));
}

const UINT CModule::GetEffectCommandCount(TChannel* pChannel)
{
	if (pChannel)
	{
		UINT effectCount = pChannel->effectCount;

		// 0 is actually the highest possible value due to base 0 indexing
		if (effectCount == 0)
			effectCount = ACTIVE_EFFECT_COUNT;

		return effectCount;
	}

	return EMPTY;
}


//--

bool CModule::SetSubtuneName(UINT subtune, const char* name)
{
	return SetSubtuneName(GetSubtune(subtune), name);
}

bool CModule::SetSubtuneName(TSubtune* pSubtune, const char* name)
{
	if (pSubtune)
	{
		strncpy_s(pSubtune->name, name, SUBTUNE_NAME_MAX);
		return true;
	}

	return false;
}

bool CModule::SetSongLength(UINT subtune, UINT length)
{
	return SetSongLength(GetSubtune(subtune), length);
}

bool CModule::SetSongLength(TSubtune* pSubtune, UINT length)
{
	if (pSubtune)// && length <= SONGLINE_COUNT)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (length >= SONGLINE_COUNT)
			length = 0;

		pSubtune->songLength = length;
		return true;
	}

	return false;
}

bool CModule::SetPatternLength(UINT subtune, UINT length)
{
	return SetPatternLength(GetSubtune(subtune), length);
}

bool CModule::SetPatternLength(TSubtune* pSubtune, UINT length)
{
	if (pSubtune)// && length <= ROW_COUNT)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (length >= ROW_COUNT)
			length = 0;

		pSubtune->patternLength = length;
		return true;
	}

	return false;
}

bool CModule::SetChannelCount(UINT subtune, UINT count)
{
	return SetChannelCount(GetSubtune(subtune), count);
}

bool CModule::SetChannelCount(TSubtune* pSubtune, UINT count)
{
	if (pSubtune)// && count <= CHANNEL_COUNT)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (count >= CHANNEL_COUNT)
			count = 0;

		pSubtune->channelCount = count;
		return true;
	}

	return false;
}

bool CModule::SetSongSpeed(UINT subtune, UINT speed)
{
	return SetSongSpeed(GetSubtune(subtune), speed);
}

bool CModule::SetSongSpeed(TSubtune* pSubtune, UINT speed)
{
	if (pSubtune)// && speed <= SONG_SPEED_MAX)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (speed >= SONG_SPEED_MAX)
			speed = 0;

		pSubtune->songSpeed = speed;
		return true;
	}

	return false;
}

bool CModule::SetInstrumentSpeed(UINT subtune, UINT speed)
{
	return SetInstrumentSpeed(GetSubtune(subtune), speed);
}

bool CModule::SetInstrumentSpeed(TSubtune* pSubtune, UINT speed)
{
	if (pSubtune)// && speed <= INSTRUMENT_SPEED_MAX)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (speed >= INSTRUMENT_SPEED_MAX)
			speed = 0;

		pSubtune->instrumentSpeed = speed;
		return true;
	}

	return false;
}

bool CModule::SetEffectCommandCount(UINT subtune, UINT channel, UINT column)
{
	return SetEffectCommandCount(GetChannel(subtune, channel), column);
}

bool CModule::SetEffectCommandCount(TSubtune* pSubtune, UINT channel, UINT column)
{
	return SetEffectCommandCount(GetChannel(pSubtune, channel), column);
}

bool CModule::SetEffectCommandCount(TChannel* pChannel, UINT column)
{
	if (pChannel)// && column <= ACTIVE_EFFECT_COUNT)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (column >= ACTIVE_EFFECT_COUNT)
			column = 0;

		pChannel->effectCount = column;
		return true;
	}

	return false;
}


//--

const UINT CModule::GetSubtuneCount()
{
	UINT count = 0;

	for (int i = 0; i < SUBTUNE_COUNT; i++)
		count += m_subtuneIndex->subtune[i] != NULL;

	return count;
}


//--

TInstrumentV2* CModule::GetInstrument(UINT instrument)
{
	// A new Instrument will be returned if it didn't already exist
	return CreateInstrument(instrument);
}

const char* CModule::GetInstrumentName(UINT instrument)
{
	return GetInstrumentName(GetInstrument(instrument));
}

const char* CModule::GetInstrumentName(TInstrumentV2* pInstrument)
{
	if (pInstrument)
		return pInstrument->name;

	return NULL;
}

void CModule::SetInstrumentName(UINT instrument, const char* name)
{
	SetInstrumentName(GetInstrument(instrument), name);
}

void CModule::SetInstrumentName(TInstrumentV2* pInstrument, const char* name)
{
	if (pInstrument)
		strncpy_s(pInstrument->name, name, INSTRUMENT_NAME_MAX);
}


//--

// Identify the Shortest Pattern Length relative to the other ones used in the same Songline
const UINT CModule::GetShortestPatternLength(UINT subtune, UINT songline)
{
	return GetShortestPatternLength(GetSubtune(subtune), songline);
}

// Identify the Shortest Pattern Length relative to the other ones used in the same Songline
const UINT CModule::GetShortestPatternLength(TSubtune* pSubtune, UINT songline)
{
	// Get the current Pattern Length first
	UINT patternLength = GetPatternLength(pSubtune);

	// Get the Channel Count
	UINT channelCount = GetChannelCount(pSubtune);

	// All the channels will be processed in order to identify the shortest Pattern length
	for (UINT i = 0; i < channelCount; i++)
	{
		// Get the Pattern currently used in the Songline
		TPattern* pPattern = GetIndexedPattern(pSubtune, i, songline);
		
		// Get the Effect Command Count for this Channel
		UINT effectCount = GetEffectCommandCount(GetChannel(pSubtune, i));

		// Get the Effective Pattern Length for this Channel
		UINT effectiveLength = GetEffectivePatternLength(pPattern, patternLength, effectCount);

		// Set the current Pattern Length to the Effective Pattern Length if it is lower
		if (patternLength > effectiveLength)
			patternLength = effectiveLength;
	}

	// The shortest Pattern Length will be returned if successful
	return patternLength;
}

// Identify the Effective Pattern Length using the provided parameters, the Shortest Pattern Length will be returned accordingly
const UINT CModule::GetEffectivePatternLength(UINT subtune, UINT channel, UINT pattern)
{
	return GetEffectivePatternLength(GetChannel(subtune, channel), pattern, GetPatternLength(subtune));
}

// Identify the Effective Pattern Length using the provided parameters, the Shortest Pattern Length will be returned accordingly
const UINT CModule::GetEffectivePatternLength(TSubtune* pSubtune, UINT channel, UINT pattern)
{
	return GetEffectivePatternLength(GetChannel(pSubtune, channel), pattern, GetPatternLength(pSubtune));
}

// Identify the Effective Pattern Length using the provided parameters, the Shortest Pattern Length will be returned accordingly
const UINT CModule::GetEffectivePatternLength(TChannel* pChannel, UINT pattern, UINT patternLength)
{
	return GetEffectivePatternLength(GetPattern(pChannel, pattern), patternLength, GetEffectCommandCount(pChannel));
}

// Identify the Effective Pattern Length using the provided parameters, the Shortest Pattern Length will be returned accordingly
const UINT CModule::GetEffectivePatternLength(TPattern* pPattern, UINT patternLength, UINT effectCount)
{
	if (pPattern)
	{
		// Check for each Row that could be used within the shortest Pattern
		for (UINT i = 0; i < patternLength; i++)
		{
			// Check for all the Effect Commands used in each Row
			for (UINT j = 0; j < effectCount; j++)
			{
				// Get the Effect Command Identifier, the Parameter is not needed here
				UINT command = GetPatternRowEffectCommand(pPattern, i, j);

				// Set the Pattern Length to the current Row Index if a match is found
				if (command == EFFECT_GOTO_SONGLINE || command == EFFECT_END_PATTERN)
				{
					// Add 1 to match the actual number of Rows per Pattern
					if (patternLength > i + 1)
						patternLength = i + 1;
				}
			}
		}
	}

	// The Effective Pattern Length will be returned if successful
	return patternLength;
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(UINT subtune, UINT channel, UINT pattern)
{
	return IsUnusedPattern(GetChannel(subtune, channel), pattern);
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(TSubtune* pSubtune, UINT channel, UINT pattern)
{
	return IsUnusedPattern(GetChannel(pSubtune, channel), pattern);
}

// Return True if a Pattern is used at least once within a Songline Index
bool CModule::IsUnusedPattern(TChannel* pChannel, UINT pattern)
{
	if (pChannel && IsValidPattern(pattern))
	{
		// All Songlines in the Channel Index will be processed
		for (int i = 0; i < SONGLINE_COUNT; i++)
		{
			// As soon as a match is found, we know for sure the Pattern is used at least once
			if (pChannel->songline[i] == pattern)
				return false;
		}
	}

	// Otherwise, the Pattern is most likely unused
	return true;
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(UINT subtune, UINT channel, UINT pattern)
{
	return IsEmptyPattern(GetPattern(subtune, channel, pattern));
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(TSubtune* pSubtune, UINT channel, UINT pattern)
{
	return IsEmptyPattern(GetPattern(pSubtune, channel, pattern));
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(TChannel* pChannel, UINT pattern)
{
	return IsEmptyPattern(GetPattern(pChannel, pattern));
}

// Return True if a Pattern is Empty
bool CModule::IsEmptyPattern(TPattern* pPattern)
{
	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < ROW_COUNT; i++)
	{
		// If a Row contains any data, it is not empty
		if (!IsEmptyRow(pPattern, i))
			return false;
	}

	// Otherwise, the Pattern is most likely empty
	return true;
}

// Return True if a Row is Empty
bool CModule::IsEmptyRow(UINT subtune, UINT channel, UINT pattern, UINT row)
{
	return IsEmptyRow(GetRow(subtune, channel, pattern, row));
}

// Return True if a Row is Empty
bool CModule::IsEmptyRow(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row)
{
	return IsEmptyRow(GetRow(pSubtune, channel, pattern, row));
}

// Return True if a Row is Empty
bool CModule::IsEmptyRow(TChannel* pChannel, UINT pattern, UINT row)
{
	return IsEmptyRow(GetRow(pChannel, pattern, row));
}

// Return True if a Row is Empty
bool CModule::IsEmptyRow(TPattern* pPattern, UINT row)
{
	return IsEmptyRow(GetRow(pPattern, row));
}

// Return True if a Row is Empty
bool CModule::IsEmptyRow(TRow* pRow)
{
	if (pRow)
	{
		// If there is a Note, it's not empty
		if (pRow->note != NOTE_EMPTY)
			return false;

		// If there is an Instrument, it's not empty
		if (pRow->instrument != INSTRUMENT_EMPTY)
			return false;

		// If there is a Volume, it's not empty
		if (pRow->volume != VOLUME_EMPTY)
			return false;

		// If there is an Effect Command, it's not empty
		for (int i = 0; i < ACTIVE_EFFECT_COUNT; i++)
		{
			// Only the Identifier is checked, since the Parameter cannot be used alone
			if (pRow->effect[i].command != EFFECT_EMPTY)
				return false;
		}
	}

	// Otherwise, the Row is most likely empty
	return true;
}

// Compare 2 Patterns for identical data, Return True if successful
bool CModule::IsIdenticalPattern(TPattern* pFromPattern, TPattern* pToPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!pFromPattern || !pToPattern)
		return false;

	// All Rows in the Pattern Index will be processed
	for (int i = 0; i < ROW_COUNT; i++)
	{
		// If Rows don't match, the Patterns are different
		if (!IsIdenticalRow(GetRow(pFromPattern, i), GetRow(pToPattern, i)))
			return false;
	}

	// Otherwise, the Patterns are most likely identical
	return true;
}

// Compare 2 Rows for identical data, Return True if successful
bool CModule::IsIdenticalRow(TRow* pFromRow, TRow* pToRow)
{
	// Make sure both the Rows from source and destination are not Null pointers
	if (!pFromRow || !pToRow)
		return false;

	// If there is a different Note, it's not identical
	if (pFromRow->note != pToRow->note)
		return false;

	// If there is a different Instrument, it's not identical
	if (pFromRow->instrument != pToRow->instrument)
		return false;

	// If there is a different Volume, it's not identical
	if (pFromRow->volume != pToRow->volume)
		return false;

	// If there is a different Effect Command, it's not identical
	for (int i = 0; i < ACTIVE_EFFECT_COUNT; i++)
	{
		if (pFromRow->effect[i].command != pToRow->effect[i].command)
			return false;

		if (pFromRow->effect[i].parameter != pToRow->effect[i].parameter)
			return false;
	}

	// Otherwise, the Rows are most likely identical
	return true;
}

// Duplicate a Pattern used in a Songline to a new unused position, Return True if successful
bool CModule::DuplicatePatternInSongline(UINT subtune, UINT channel, UINT songline, UINT pattern)
{
	return DuplicatePatternInSongline(GetChannel(subtune, channel), songline, pattern);
}

// Duplicate a Pattern used in a Songline to a new unused position, Return True if successful
bool CModule::DuplicatePatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline, UINT pattern)
{
	return DuplicatePatternInSongline(GetChannel(pSubtune, channel), songline, pattern);
}

// Duplicate a Pattern used in a Songline to a new unused position, Return True if successful
bool CModule::DuplicatePatternInSongline(TChannel* pChannel, UINT songline, UINT pattern)
{
	if (pChannel && IsValidSongline(songline) && IsValidPattern(pattern))
	{
		// Find the first empty and unused Pattern that is available
		for (int i = 0; i < PATTERN_COUNT; i++)
		{
			// Ignore the Pattern that is being duplicated
			if (i == pattern)
				continue;

			// If the Pattern is empty and unused, it will be used for the duplication
			if (IsUnusedPattern(pChannel, i) && IsEmptyPattern(GetPattern(pChannel, i)))
			{
				// Replace the Pattern used in the Songline Index with the new one as well
				if (CopyPattern(GetPattern(pChannel, pattern), GetPattern(pChannel, i)))
				{
					SetPatternInSongline(pChannel, songline, i);
					return true;
				}
			}
		}
	}

	// Could not create a Pattern duplicate, no data was edited
	return false;
}

/*
// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(UINT subtune, UINT channel, UINT pattern)
{
	return ClearPattern(GetPattern(subtune, channel, pattern));
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(TSubtune* pSubtune, UINT channel, UINT pattern)
{
	return ClearPattern(GetPattern(pSubtune, channel, pattern));
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(TChannel* pChannel, UINT pattern)
{
	return ClearPattern(GetPattern(pChannel, pattern));
}

// Clear data from Pattern, Return True if successful
bool CModule::ClearPattern(TPattern* pPattern)
{
	// Make sure the Pattern is not a Null pointer
	if (!pPattern)
		return false;

	// Set all indexed Rows in the Pattern to empty values
	for (int i = 0; i < ROW_COUNT; i++)
		InitialiseRow(pPattern->row[i]);

	// Pattern data should have been cleared successfully
	return true;
}
*/

// Copy data from source Row to destination Row, Return True if successful
bool CModule::CopyRow(TRow* pFromRow, TRow* pToRow)
{
	// Make sure both the Rows from source and destination are not Null pointers
	if (!pFromRow || !pToRow)
		return false;

	pToRow->note = pFromRow->note;
	pToRow->instrument = pFromRow->instrument;
	pToRow->volume = pFromRow->volume;

	for (int i = 0; i < ACTIVE_EFFECT_COUNT; i++)
	{
		pToRow->effect[i].command = pFromRow->effect[i].command;
		pToRow->effect[i].parameter = pFromRow->effect[i].parameter;
	}

	// Row data should have been copied successfully
	return true;
}

// Copy data from source Pattern to destination Pattern, Return True if successful
bool CModule::CopyPattern(TPattern* pFromPattern, TPattern* pToPattern)
{
	// Make sure both the Patterns from source and destination are not Null pointers
	if (!pFromPattern || !pToPattern)
		return false;

	for (int i = 0; i < ROW_COUNT; i++)
	{
		// Something went wrong...? Abort the procedure and return False
		if (!CopyRow(GetRow(pFromPattern, i), GetRow(pToPattern, i)))
			return false;
	}

	// Pattern data should have been copied successfully
	return true;
}

// Copy data from source Channel to destination Channel, Return True if successful
bool CModule::CopyChannel(UINT subtune, UINT fromChannel, UINT toChannel)
{
	return CopyChannel(GetChannel(subtune, fromChannel), GetChannel(subtune, toChannel));
}

// Copy data from source Channel to destination Channel, Return True if successful
bool CModule::CopyChannel(TSubtune* pSubtune, UINT fromChannel, UINT toChannel)
{
	return CopyChannel(GetChannel(pSubtune, fromChannel), GetChannel(pSubtune, toChannel));
}

// Copy data from source Channel to destination Channel, Return True if successful
bool CModule::CopyChannel(TChannel* pFromChannel, TChannel* pToChannel)
{
	// Make sure both the Channels from source and destination are not Null pointers
	if (!pFromChannel || !pToChannel)
		return false;

	pToChannel->isMuted = pFromChannel->isMuted;
	pToChannel->isEffectEnabled = pFromChannel->isEffectEnabled;
	pToChannel->effectCount = pFromChannel->effectCount;
	pToChannel->channelVolume = pFromChannel->channelVolume;

	for (int i = 0; i < SONGLINE_COUNT; i++)
		pToChannel->songline[i] = pFromChannel->songline[i];

	for (int i = 0; i < PATTERN_COUNT; i++)
	{
		// Something went wrong...? Abort the procedure and return False
		if (!CopyPattern(GetPattern(pFromChannel, i), GetPattern(pToChannel, i)))
			return false;
	}

	// Channel data should have been copied successfully
	return true;
}

// Copy data from source Subtune to destination Subtune, Return True if successful
bool CModule::CopySubtune(UINT fromSubtune, UINT toSubtune)
{
	return CopySubtune(GetSubtune(fromSubtune), GetSubtune(toSubtune));
}

// Copy data from source Subtune to destination Subtune, Return True if successful
bool CModule::CopySubtune(TSubtune* pFromSubtune, TSubtune* pToSubtune)
{
	// Make sure both the Subtunes from source and destination are not Null pointers
	if (!pFromSubtune || !pToSubtune)
		return false;

	SetSubtuneName(pToSubtune, pFromSubtune->name);

	pToSubtune->songLength = pFromSubtune->songLength;
	pToSubtune->patternLength = pFromSubtune->patternLength;
	pToSubtune->songSpeed = pFromSubtune->songSpeed;
	pToSubtune->instrumentSpeed = pFromSubtune->instrumentSpeed;
	pToSubtune->channelCount = pFromSubtune->channelCount;

	for (int i = 0; i < CHANNEL_COUNT; i++)
	{
		// Something went wrong...? Abort the procedure and return False
		if (!CopyChannel(GetChannel(pFromSubtune, i), GetChannel(pToSubtune, i)))
			return false;
	}

	// Subtune data should have been copied successfully
	return true;
}

// Find and merge duplicated Patterns, and adjust the Songline Index accordingly
void CModule::MergeDuplicatedPatterns(UINT subtune)
{
	MergeDuplicatedPatterns(GetSubtune(subtune));
}

// Find and merge duplicated Patterns, and adjust the Songline Index accordingly
void CModule::MergeDuplicatedPatterns(TSubtune* pSubtune)
{
/*
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
*/
}

// Renumber all Patterns from first to last Songlines, without optimisations
// TODO: Move to CSong
void CModule::RenumberIndexedPatterns(UINT subtune)
{
	RenumberIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, without optimisations
// TODO: Move to CSong
void CModule::RenumberIndexedPatterns(TSubtune* pSubtune)
{
/*
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
		for (int j = 0; j < SONGLINE_COUNT; j++)
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
*/
}

// Clear all unused Indexed Patterns
// TODO: Move to CSong
void CModule::ClearUnusedPatterns(UINT subtune)
{
	ClearUnusedPatterns(GetSubtune(subtune));
}

// Clear all unused Indexed Patterns
// TODO: Move to CSong
void CModule::ClearUnusedPatterns(TSubtune* pSubtune)
{
/*
	if (!pSubtune)
		return;

	// Process all Channels within the Module Index
	for (int i = 0; i < pSubtune->channelCount; i++)
	{
		// Search for all unused indexed Patterns
		for (int j = 0; j < PATTERN_COUNT; j++)
		{
			// If the Pattern is not used anywhere, it will be deleted
			if (IsUnusedPattern(&pSubtune->channel[i], j, pSubtune->songLength))
				ClearPattern(&pSubtune->channel[i].pattern[j]);
		}
	}
*/
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
// TODO: Move to CSong
void CModule::ConcatenateIndexedPatterns(UINT subtune)
{
	ConcatenateIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
// TODO: Move to CSong
void CModule::ConcatenateIndexedPatterns(TSubtune* pSubtune)
{
/*
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
		for (int j = 0; j < PATTERN_COUNT; j++)
		{
			// If a Pattern is used at least once, see if it could also be concatenated further back
			if (!IsUnusedPattern(backupIndex, j, pSubtune->songLength))
			{
				// Find the first empty and unused Pattern that is available
				for (int k = 0; k < j; k++)
				{
					TPattern* source = &backupIndex->pattern[j];
					TPattern* destination = &backupIndex->pattern[k];

					// If the Pattern is empty and unused, it will be replaced
					if (IsUnusedPattern(backupIndex, k, pSubtune->songLength) && IsEmptyPattern(destination))
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
*/
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
// TODO: Move to CSong
void CModule::AllSizeOptimisations(UINT subtune)
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
