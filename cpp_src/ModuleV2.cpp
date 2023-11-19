// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"
#include "Atari6502.h"

// Temporary workaround to avoid including whole header shit
extern const char* notesandscales[5][40];

CModule::CModule()
{
	m_subtuneIndex = new TSubtuneIndex();
	m_instrumentIndex = new TInstrumentIndex();
	InitialiseModule();
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
	{
		DeleteInstrument(i);
		DeleteVolumeEnvelope(i);
		DeleteTimbreEnvelope(i);
		DeleteAudctlEnvelope(i);
		DeleteEffectEnvelope(i);
		DeleteNoteTableEnvelope(i);
		DeleteFreqTableEnvelope(i);
	}
}


//--

bool CModule::CreateSubtune(UINT subtune)
{
	if (m_subtuneIndex && IsValidSubtune(subtune))
	{
		// If there is no Subtune here, create it now and update the Subtune Index accordingly
		if (!m_subtuneIndex->subtune[subtune])
			m_subtuneIndex->subtune[subtune] = new TSubtune();
		
		return InitialiseSubtune(m_subtuneIndex->subtune[subtune]);
	}

	return false;
}

bool CModule::DeleteSubtune(UINT subtune)
{
	if (m_subtuneIndex && IsValidSubtune(subtune))
	{
		// If there is a Subtune here, don't waste any time and delete it without further ado
		if (m_subtuneIndex->subtune[subtune])
			delete m_subtuneIndex->subtune[subtune];
			
		m_subtuneIndex->subtune[subtune] = NULL;
		return true;
	}
	
	return false;
}

bool CModule::InitialiseSubtune(TSubtune* pSubtune)
{
	if (!pSubtune)
		return false;

	// Set the default Subtune name
	SetSubtuneName(pSubtune, "");

	// Set the default parameters to all the Subtune variables
	pSubtune->songLength = MODULE_DEFAULT_SONG_LENGTH;
	pSubtune->patternLength = MODULE_DEFAULT_PATTERN_LENGTH;
	pSubtune->channelCount = MODULE_DEFAULT_CHANNEL_COUNT;
	pSubtune->songSpeed = MODULE_DEFAULT_SONG_SPEED;
	pSubtune->instrumentSpeed = MODULE_DEFAULT_INSTRUMENT_SPEED;

	// Delete all Channels with leftover data
	DeleteAllChannels(pSubtune);

	// Subtune was initialised
	return true;
}

bool CModule::DeleteChannel(TSubtune* pSubtune, UINT channel)
{
	return InitialiseChannel(GetChannel(pSubtune, channel));
}

bool CModule::InitialiseChannel(TChannel* pChannel)
{
	if (!pChannel)
		return false;

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

	// Channel was initialised
	return true;
}

bool CModule::DeletePattern(TChannel* pChannel, UINT pattern)
{
	return InitialisePattern(GetPattern(pChannel, pattern));
}

bool CModule::InitialisePattern(TPattern* pPattern)
{
	if (!pPattern)
		return false;

	// Delete all Rows with leftover data
	DeleteAllRows(pPattern);

	// Pattern was initialised
	return true;
}

bool CModule::DeleteRow(TPattern* pPattern, UINT row)
{
	return InitialiseRow(GetRow(pPattern, row));
}

bool CModule::InitialiseRow(TRow* pRow)
{
	if (!pRow)
		return false;

	pRow->note = NOTE_EMPTY;
	pRow->instrument = INSTRUMENT_EMPTY;
	pRow->volume = VOLUME_EMPTY;

	for (int i = 0; i < PATTERN_EFFECT_COUNT; i++)
	{
		pRow->effect[i].command = PE_EMPTY;
		pRow->effect[i].parameter = EFFECT_PARAMETER_MIN;
	}

	// Row was initialised
	return true;
}


//--

bool CModule::CreateInstrument(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Instrument here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->instrument[instrument])
			m_instrumentIndex->instrument[instrument] = new TInstrumentV2();

		return InitialiseInstrument(m_instrumentIndex->instrument[instrument]);
	}

	return false;
}

bool CModule::DeleteInstrument(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Instrument here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->instrument[instrument])
			delete m_instrumentIndex->instrument[instrument];

		m_instrumentIndex->instrument[instrument] = NULL;
		return true;
	}

	return false;
}


bool CModule::InitialiseInstrument(TInstrumentV2* pInstrument)
{
	if (!pInstrument)
		return false;

	// Set the default Instrument name
	SetInstrumentName(pInstrument, "New Instrument");

	pInstrument->volumeFade = 0x00;
	pInstrument->volumeSustain = 0x00;
	pInstrument->volumeDelay = 0x00;
	pInstrument->vibrato = 0x00;
	pInstrument->vibratoDelay = 0x00;
	pInstrument->freqShift = 0x00;
	pInstrument->freqShiftDelay = 0x00;
	pInstrument->autoFilter = 0x00;
	pInstrument->autoFilterMode = false;

	// Set the default Envelope Macro parameters, always disabled for newly created Instruments
	TMacro macro{ 0x00, false, false };
	pInstrument->envelope = { macro, macro, macro, macro, macro, macro };

	// Instrument was initialised
	return true;
}


//--

bool CModule::CreateVolumeEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->volume[instrument])
			m_instrumentIndex->volume[instrument] = new TEnvelope();

		return InitialiseVolumeEnvelope(m_instrumentIndex->volume[instrument]);
	}

	return false;
}

bool CModule::DeleteVolumeEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->volume[instrument])
			delete m_instrumentIndex->volume[instrument];

		m_instrumentIndex->volume[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseVolumeEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TVolumeEnvelope volume{ 0x00 };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT; i++)
		pEnvelope->volume[i] = volume;

	// Volume Envelope was initialised
	return true;
}


//--

bool CModule::CreateTimbreEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->timbre[instrument])
			m_instrumentIndex->timbre[instrument] = new TEnvelope();

		return InitialiseTimbreEnvelope(m_instrumentIndex->timbre[instrument]);
	}

	return false;
}

bool CModule::DeleteTimbreEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->timbre[instrument])
			delete m_instrumentIndex->timbre[instrument];

		m_instrumentIndex->timbre[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseTimbreEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TTimbreEnvelope timbre{ TIMBRE_PURE };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT; i++)
		pEnvelope->timbre[i] = timbre;

	// Timbre Envelope was initialised
	return true;
}


//--

bool CModule::CreateAudctlEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->audctl[instrument])
			m_instrumentIndex->audctl[instrument] = new TEnvelope();

		return InitialiseAudctlEnvelope(m_instrumentIndex->audctl[instrument]);
	}

	return false;
}

bool CModule::DeleteAudctlEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->audctl[instrument])
			delete m_instrumentIndex->audctl[instrument];

		m_instrumentIndex->audctl[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseAudctlEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TAudctlEnvelope audctl{ 0x00 };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT; i++)
		pEnvelope->audctl[i] = audctl;

	// Audctl Envelope was initialised
	return true;
}


//--

bool CModule::CreateEffectEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->effect[instrument])
			m_instrumentIndex->effect[instrument] = new TEnvelope();

		return InitialiseEffectEnvelope(m_instrumentIndex->effect[instrument]);
	}

	return false;
}

bool CModule::DeleteEffectEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->effect[instrument])
			delete m_instrumentIndex->effect[instrument];

		m_instrumentIndex->effect[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseEffectEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TEffectEnvelope effect{ false, false, false, false, false, false, false, false, IE_EMPTY, IE_EMPTY, 0x00, 0x00 };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT / 4; i++)
		pEnvelope->effect[i] = effect;

	// Effect Envelope was initialised
	return true;
}


//--

bool CModule::CreateNoteTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->note[instrument])
			m_instrumentIndex->note[instrument] = new TEnvelope();

		return InitialiseNoteTableEnvelope(m_instrumentIndex->note[instrument]);
	}

	return false;
}

bool CModule::DeleteNoteTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->note[instrument])
			delete m_instrumentIndex->note[instrument];

		m_instrumentIndex->note[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseNoteTableEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TNoteTableEnvelope noteTable{ 0x00 };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT; i++)
		pEnvelope->note[i] = noteTable;

	// Note Table Envelope was initialised
	return true;
}


//--

bool CModule::CreateFreqTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is no Envelope here, create it now and update the Instrument Index accordingly
		if (!m_instrumentIndex->freq[instrument])
			m_instrumentIndex->freq[instrument] = new TEnvelope();

		return InitialiseFreqTableEnvelope(m_instrumentIndex->freq[instrument]);
	}

	return false;
}

bool CModule::DeleteFreqTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
	{
		// If there is an Envelope here, don't waste any time and delete it without further ado
		if (m_instrumentIndex->freq[instrument])
			delete m_instrumentIndex->freq[instrument];

		m_instrumentIndex->freq[instrument] = NULL;
		return true;
	}

	return false;
}

bool CModule::InitialiseFreqTableEnvelope(TEnvelope* pEnvelope)
{
	if (!pEnvelope)
		return false;

	// Set the default Envelope parameters
	TEnvelopeParameter parameter{ 0x01, 0x00, 0x01, 0x01, false, false, false, false };
	TFreqTableEnvelope freqTable{ 0x0000 };

	pEnvelope->parameter = parameter;

	// Set the default Envelope values
	for (int i = 0; i < ENVELOPE_STEP_COUNT / 2; i++)
		pEnvelope->freq[i] = freqTable;

	// Freq Table Envelope was initialised
	return true;
}


//--

bool CModule::ImportLegacyRMT(std::ifstream& in)
{
	CString importLog;
	importLog.Format("");

	UINT songlineStep[SONGLINE_COUNT];
	memset(songlineStep, INVALID, sizeof(songlineStep));

	UINT subtuneOffset[SONGLINE_COUNT];
	memset(subtuneOffset, EMPTY, sizeof(subtuneOffset));

	// This will become the final count of decoded Subtunes from the Legacy RMT Module
	UINT subtuneCount = 0;

	// Create a Temporary Subtune, to make the Import procedure a much easier task
	TSubtune* importSubtune = new TSubtune();
	InitialiseSubtune(importSubtune);

	// Clear the current module data
	InitialiseModule();

	// Decode the Legacy RMT Module into the Temporary Subtune, and re-construct the imported data if successful
	if (DecodeLegacyRMT(in, importSubtune, importLog))
	{
		importLog.AppendFormat("Stage 1 - Decoding of Legacy RMT Module:\n\n");
		importLog.AppendFormat("Song Name: \"");
		importLog.AppendFormat(GetSubtuneName(importSubtune));
		importLog.AppendFormat("\"\n");
		importLog.AppendFormat("Song Length: %02X, Pattern Length: %02X, Channels: %01X\n", GetSongLength(importSubtune), GetPatternLength(importSubtune), GetChannelCount(importSubtune));
		importLog.AppendFormat("Song Speed: %02X, Instrument Speed: %02X\n\n", GetSongSpeed(importSubtune), GetInstrumentSpeed(importSubtune));
		importLog.AppendFormat("Stage 2 - Constructing RMTE Module from imported data:\n\n");

		// Process all indexed Songlines until all the Subtunes are identified
		for (UINT i = 0; i < GetSongLength(importSubtune); i++)
		{
			// If the Indexed Songline was not already processed...
			if (!IsValidSongline(songlineStep[i]))
			{
				// if a new Subtune is found, set the offset to the current Songline Index
				importLog.AppendFormat("Identified: Subtune %02X, in Songline %02X\n", subtuneCount, i);
				subtuneOffset[subtuneCount] = i;
				subtuneCount++;

				// From here, analyse the next Songlines until a loop is detected
				for (UINT j = i; j < GetSongLength(importSubtune); j++)
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
					if (GetPatternInSongline(importSubtune, CH1, j) == 0xFE)
					{
						j = GetPatternInSongline(importSubtune, CH2, j);

						// Also set the Songline Step at the offset of Destination Songline, so it won't be referenced again
						songlineStep[j] = i;
					}
				}
			}
		}

		importLog.AppendFormat("Confidently detected %i unique Subtune(s).\n\n", subtuneCount);
		importLog.AppendFormat("Stage 3 - Optimising Subtunes with compatibility tweaks:\n\n");

		// Copy all of the imported patterns to every Channels, so they all share identical data
		for (UINT i = 0; i < GetChannelCount(importSubtune); i++)
		{
			// The Songline Index won't be overwritten in the process, since we will need it in its current form!
			for (UINT j = 0; j < PATTERN_COUNT; j++)
				CopyPattern(GetPattern(importSubtune, CH1, j), GetPattern(importSubtune, i, j));

			// Set the Active Effect Command Columns to the same number for each channels
			SetEffectCommandCount(importSubtune, i, 2);
		}

		// Re-construct all of individual Subtunes that were detected
		for (UINT i = 0; i < subtuneCount; i++)
		{
			UINT offset = subtuneOffset[i];

			// Copy the data previously imported from the Temporary Subtune into the Active Subtune
			CreateSubtune(i);
			CopySubtune(importSubtune, GetSubtune(i));

			// This will be used once again for detecting loop points in Subtunes
			memset(songlineStep, INVALID, sizeof(songlineStep));

			// Re-construct every Songlines used by the Subtune, until the loop point is found
			for (UINT j = 0; j < SONGLINE_COUNT; j++)
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
				if (GetPatternInSongline(importSubtune, CH1, offset) == 0xFE)
				{
					offset = GetPatternInSongline(importSubtune, CH2, offset);

					// Also set the Songline Step at the offset of Destination Songline, so it won't be referenced again
					songlineStep[offset] = j;
				}
				
				// Fetch the Patterns from the backup Songline Index first
				for (UINT k = 0; k < GetChannelCount(i); k++)
					SetPatternInSongline(i, k, j, GetPatternInSongline(importSubtune, k, offset));

				// Otherwise, the Songline offset will increment by 1 for the next Songline
				offset++;
			}

			// Re-arrange all Patterns to make them unique entries for every Songline, so editing them will not overwrite anything intended to be used differently
			RenumberIndexedPatterns(i);

			// In order to merge all of the Bxx and Dxx Commands, find all Dxx Commands that were used, and move them to Channel 1, unless a Bxx Command was already used there
			for (UINT j = 0; j < GetSongLength(i); j++)
			{
				// If the Shortest Pattern Length is below actual Pattern Length, a Dxx Command was already used somewhere, and must be replaced
				if (GetShortestPatternLength(i, j) < GetPatternLength(i))
				{
					SetPatternRowEffectCommand(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, PE_END_PATTERN);
					SetPatternRowEffectParameter(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, EFFECT_PARAMETER_MIN);
				}

				// Set the final Goto Songline Command Bxx to the Songline found at the loop point
				if (j == GetSongLength(i) - 1)
				{
					SetPatternRowEffectCommand(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, PE_GOTO_SONGLINE);
					SetPatternRowEffectParameter(i, CH1, GetPatternInSongline(i, CH1, j), GetShortestPatternLength(i, j) - 1, CMD2, songlineStep[offset] - 1);
				}

				// Skip CH1, since it was already processed above
				for (UINT k = CH2; k < GetChannelCount(i); k++)
				{
					// All Pattern Rows will be edited, regardless of their contents
					for (UINT l = 0; l < ROW_COUNT; l++)
					{
						// The Fxx Commands are perfectly fine as they are, so the Effect Column 1 is also skipped
						for (UINT m = CMD2; m < PATTERN_EFFECT_COUNT; m++)
						{
							SetPatternRowEffectCommand(i, k, GetPatternInSongline(i, k, j), l, m, PE_EMPTY);
							SetPatternRowEffectParameter(i, k, GetPatternInSongline(i, k, j), l, m, EFFECT_PARAMETER_MIN);
						}
					}
				}
			}

			// Set the final count of Active Effect Command Columns for each channels once they're all processed
			for (UINT j = 0; j < GetChannelCount(i); j++)
				SetEffectCommandCount(i, j, j == CH1 ? 2 : 1);

			// Finally, apply the Size Optimisations, the Subtune should have been reconstructed successfully!
			AllSizeOptimisations(i);
			importLog.AppendFormat("Reconstructed: Subtune %02X\n", i);
			importLog.AppendFormat("Song Length: %02X, Pattern Length: %02X, Channels: %01X\n", GetSongLength(i), GetPatternLength(i), GetChannelCount(i));
			importLog.AppendFormat("Song Speed: %02X, Instrument Speed: %02X\n", GetSongSpeed(i), GetInstrumentSpeed(i));
			importLog.AppendFormat("Loop Point found in Songline %02X\n\n", songlineStep[offset] - 1);
		}

		// Workaround: Due to the way RMT was originally designed, the "Global" number of channels must be set here as well
		g_tracks4_8 = GetChannelCount((UINT)MODULE_DEFAULT_SUBTUNE);

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
	UINT channelCount = mem[fromAddr + 3] & 0x0F;

	// 5th byte: track length
	UINT patternLength = mem[fromAddr + 4];

	// 6th byte: song speed
	UINT songSpeed = mem[fromAddr + 5];

	// 7th byte: Instrument speed
	UINT instrumentSpeed = mem[fromAddr + 6];

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
	if (channelCount != 4 && channelCount != 8)
	{
		channelCount = MODULE_DEFAULT_CHANNEL_COUNT;
		log.AppendFormat("Warning: Invalid number of Channels, 4 or 8 were expected\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", channelCount);
	}

	// Invalid Song Speed
	if (songSpeed < 1)
	{
		songSpeed = MODULE_DEFAULT_SONG_SPEED;
		log.AppendFormat("Warning: Song Speed could not be 0.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", songSpeed);
	}

	// Invalid Instrument Speed
	if (instrumentSpeed < 1 || instrumentSpeed > 8)
	{
		instrumentSpeed = MODULE_DEFAULT_INSTRUMENT_SPEED;
		log.AppendFormat("Warning: Instrument Speed could only be set between 1 and 8 inclusive.\n");
		log.AppendFormat("Default value of %02X will be used instead.\n\n", instrumentSpeed);
	}

	// Invalid Legacy RMT Format Version
	if (version > RMTFORMATVERSION)
	{
		version = RMTFORMATVERSION;
		log.AppendFormat("Warning: Invalid RMT Format Version detected.\n");
		log.AppendFormat("Version %i will be assumed by default.\n\n", version);
	}

	// Apply the Imported parameters into the Subtune before processing further
	SetChannelCount(pSubtune, channelCount);
	SetPatternLength(pSubtune, patternLength);
	SetSongSpeed(pSubtune, songSpeed);
	SetInstrumentSpeed(pSubtune, instrumentSpeed);

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

	SetSubtuneName(pSubtune, s);

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
	UINT patternCount = ptrPatternsHigh - ptrPatternsLow;
	UINT patternLength = GetPatternLength(pSubtune);

	// Abort the import procedure if the number of Patterns detected is invalid
	if (!IsValidPattern(patternCount - 1))
		return false;

	// Decode all Patterns
	for (UINT i = 0; i < patternCount; i++)
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

		for (UINT j = i; j < patternCount; j++)
		{
			// Get the address used by the next indexed Pattern, or the address to End if there is no additional Patterns to process 
			if ((ptrPatternEnd = (j + 1 == patternCount) ? ptrEnd : sourceMemory[ptrPatternsLow + j + 1] + (sourceMemory[ptrPatternsHigh + j + 1] << 8)))
				break;

			// If the next Pattern is empty, skip over it and continue seeking for the address until something is found
			i++;
		}

		UINT patternSize = ptrPatternEnd - ptrOnePattern;

		// Invalid data, the Legacy RMT Pattern cannot be larger than 256 bytes!
		if (patternSize > 256)
			return false;

		// Get the pointer to Pattern data used by the Subtune
		BYTE* memPattern = sourceMemory + ptrOnePattern;

		UINT src = 0;
		UINT gotoIndex = INVALID;
		UINT smartLoop = INVALID;

		UINT count;
		UINT row = 0;

		// Minimal Pattern Length that could be used for a smart loop
		if (patternSize >= 2)
		{
			// There is a smart loop at the end of the Pattern
			if (memPattern[patternSize - 2] == 0xBF)
				gotoIndex = memPattern[patternSize - 1];
		}

		// Fetch the Pattern data for as long as the are bytes remaining to be processed
		while (src < patternSize)
		{
			// Jump to gotoIndex => Set the Smart Loop offset here
			if (src == gotoIndex)
				smartLoop = row;

			// Data to process at current Pattern offset
			BYTE data = memPattern[src] & 0x3F;

			switch (data)
			{
			case 0x3D:	// Have Volume only on this row
				SetPatternRowVolume(pSubtune, CH1, pattern, row, ((memPattern[src + 1] & 0x03) << 2) | ((memPattern[src] & 0xC0) >> 6));
				src += 2;	// 2 bytes were processed
				row++;	// 1 row was processed
				break;

			case 0x3E:	// Pause or empty row
				count = memPattern[src] & 0xC0;
				if (!count)
				{
					// If the Pause is 0, the number of Rows to skip is in the next byte
					if (memPattern[src + 1] == 0)
					{
						// Infinite pause => Set Pattern End here
						src = patternSize;
						break;
					}
					row += memPattern[src + 1];	// Number of Rows to skip
					src += 2;	// 2 bytes were processed
				}
				else
				{
					row += (count >> 6);	// Upper 2 bits directly specify a pause between 1 to 3 rows
					src++;	// 1 byte was processed
				}
				break;

			case 0x3F:	// Speed, smart loop, or end
				count = memPattern[src] & 0xC0;
				if (!count)
				{
					// Speed, set Fxx command
					SetPatternRowEffectCommand(pSubtune, CH1, pattern, row, CMD1, PE_SET_SPEED);
					SetPatternRowEffectParameter(pSubtune, CH1, pattern, row, CMD1, memPattern[src + 1]);
					src += 2;	// 2 bytes were processed
				}
				if (count == 0x80)
				{
					// Smart loop, no extra data to process
					src = patternSize;
				}
				if (count == 0xC0)
				{
					// End of Pattern, set a Dxx command here, no extra data to process
					SetPatternRowEffectCommand(pSubtune, CH1, pattern, row - 1, CMD2, PE_END_PATTERN);
					SetPatternRowEffectParameter(pSubtune, CH1, pattern, row - 1, CMD2, EFFECT_PARAMETER_MIN);
					src = patternSize;
				}
				break;

			default:	// Note, Instrument and Volume data on this Row
				SetPatternRowNote(pSubtune, CH1, pattern, row, data + 12);	// Transpose up by 1 octave, almost everything needs at least 1 octave higher
				SetPatternRowInstrument(pSubtune, CH1, pattern, row, ((memPattern[src + 1] & 0xfc) >> 2));
				SetPatternRowVolume(pSubtune, CH1, pattern, row, ((memPattern[src + 1] & 0x03) << 2) | ((memPattern[src] & 0xc0) >> 6));
				src += 2;	// 2 bytes were processed
				row++;	// 1 row was processed
			}
		}

		// The Pattern must to be "expanded" in order to be compatible, an equivalent for Smart Loop does not yet exist for the RMTE format
		if (IsValidRow(smartLoop))
		{
			for (UINT j = 0; row + j < patternLength; j++)
			{
				UINT k = row + j;
				UINT l = smartLoop + j;
				CopyRow(GetRow(pSubtune, CH1, pattern, l), GetRow(pSubtune, CH1, pattern, k));
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
	UINT channelCount = GetChannelCount(pSubtune);
	UINT songLine = 0, src = 0;
	BYTE channel = 0;

	// Get the pointers used for decoding the Legacy RMT Songline data
	WORD ptrSong = sourceMemory[sourceAddress + 14] + (sourceMemory[sourceAddress + 15] << 8);
	BYTE* memSong = sourceMemory + ptrSong;

	// Get the Song length, in bytes
	WORD songLength = endAddress - ptrSong + 1;

	// Abort the import procedure if the Song length detected is invalid
	if (songLength > 256 * 8)
		return false;

	// Decode all Songlines
	while (src < songLength)
	{
		BYTE data = memSong[src];

		// Process the Pattern index in the songline indexed by the channel number
		switch (data)
		{
		case 0xFE:	// Goto Songline commands are only valid from the first channel, but we know it's never used anywhere else
			SetPatternInSongline(pSubtune, channel, songLine, data);
			SetPatternInSongline(pSubtune, channel + 1, songLine, memSong[src + 1]);
			SetPatternInSongline(pSubtune, channel + 2, songLine, INVALID);
			SetPatternInSongline(pSubtune, channel + 3, songLine, INVALID);
			channel = channelCount;	// Set the channel index to the channel count to trigger the condition below
			src += channelCount;	// The number of bytes processed is equal to the number of channels
			break;

		default:	// An empty pattern at 0xFF is also valid for the RMTE format
			SetPatternInSongline(pSubtune, channel, songLine, data);
			channel++;	// 1 pattern per channel, for each indexed songline
			src++;	// 1 byte was processed
		}

		// 1 songline was processed when the channel count is equal to the number of channels
		if (channel >= channelCount)
		{
			channel = 0;	// Reset the channel index
			songLine++;	// Increment the songline count by 1
		}

		// Break out of the loop if the maximum number of songlines was processed
		if (songLine >= SONGLINE_COUNT)
			break;
	}

	// Set the Songlength to the number of decoded Songlines
	SetSongLength(pSubtune, songLine);

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
	UINT instrumentCount = (ptrEnd - ptrInstruments) / 2;

	// Abort the import procedure if the number of Instruments detected is invalid
	if (!IsValidInstrument(instrumentCount - 1))
		return false;

	// Decode all Instruments, TODO: Add exceptions for V0 Instruments
	for (UINT i = 0; i < instrumentCount; i++)
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
		CreateEffectEnvelope(i);
		CreateNoteTableEnvelope(i);		
		CreateFreqTableEnvelope(i);

		// Get the pointers to all Envelopes and Tables used in the Instrument
		TEnvelope* pVolumeEnvelope = GetVolumeEnvelope(i);
		TEnvelope* pTimbreEnvelope = GetTimbreEnvelope(i);
		TEnvelope* pAudctlEnvelope = GetAudctlEnvelope(i);
		TEnvelope* pEffectEnvelope = GetEffectEnvelope(i);
		TEnvelope* pNoteTableEnvelope = GetNoteTableEnvelope(i);
		TEnvelope* pFreqTableEnvelope = GetFreqTableEnvelope(i);

		// Assign everything in the Instrument Index once the data was initialised
		TMacro macro{ (BYTE)i, true, false };
		pInstrument->envelope = { macro, macro, macro, macro, macro, macro };	//, macro };

		// Get the Envelopes, Tables, and other parameters from the original RMT instrument data
		BYTE* memInstrument = sourceMemory + ptrOneInstrument;
		BYTE envelopePtr = memInstrument[0];							// Pointer to Instrument Envelope
		BYTE tablePtr = 12;												// Pointer to Instrument Table

		// Set the equivalent data to the RMTE instrument, with respect to boundaries
		BYTE tableLength = envelopePtr - tablePtr + 1;

		if (tableLength > 32)
			tableLength = 0x00;

		BYTE tableLoop = memInstrument[1] - tablePtr;

		if (tableLoop > tableLength)
			tableLoop = 0x00;

		BYTE envelopeLength = ((memInstrument[2] - envelopePtr + 1) / 3) + 1;

		if (envelopeLength > 48)
			envelopeLength = 0x00;

		BYTE envelopeLoop = ((memInstrument[3] - envelopePtr + 1) / 3);

		if (envelopeLoop > envelopeLength)
			envelopeLoop = 0x00;

		BYTE envelopeSpeed = 0x01;

		bool tableMode = (memInstrument[4] >> 6) & 0x01;				// Table Mode, 0 = Set, 1 = Additive
		BYTE tableSpeed = (memInstrument[4] & 0x3F) + 1;				// Table Speed, used to offset the equivalent Tables

		bool tableType = memInstrument[4] >> 7;							// Table Type, 0 = Note, 1 = Freq
		BYTE initialAudctl = memInstrument[5];							// AUDCTL, used to initialise the equivalent Envelope
		BYTE initialTimbre = 0x0A;										// RMT 1.34 Distortion 6 uses the Distortion A by default
		bool initialSkctl = false;										// SKCTL, used for the Two-Tone Filter Trigger Envelope

		BYTE delay = memInstrument[8];									// Vibrato/Freq Shift Delay
		BYTE vibrato = memInstrument[9] & 0x03;							// Vibrato
		BYTE freqShift = memInstrument[10];								// Freq Shift

		pInstrument->volumeFade = memInstrument[6];						// Volume Slide
		pInstrument->volumeSustain = memInstrument[7] >> 4;				// Volume Minimum
		pInstrument->volumeDelay = envelopeLength;						// Volume Slide delay, RMT originally processed this at Envelope Loop point

		// Import the Vibrato with adjustments to make sound similar to the original implementation
		// FIXME: Not a proper Vibrato Command conversion, this is a SineVibrato hack, the Pitch itself was used for calculations
		switch (vibrato)
		{
		case 0x01:
			vibrato = 0x0F;
			break;

		case 0x02:
			vibrato = 0x0B;
			break;

		case 0x03:
			vibrato = 0x07;
			break;
		}

		// Overwrite the Delay, Vibrato and Freqshift parameters with updated values if changes were needed
		pInstrument->vibrato = delay && vibrato ? vibrato : 0x00;
		pInstrument->vibratoDelay = delay && vibrato ? delay - 1 : 0x00;
		pInstrument->freqShift = delay && freqShift ? freqShift : 0x00;
		pInstrument->freqShiftDelay = delay && freqShift ? delay - 1 : 0x00;

		// Create the Envelope and Table Parameters
		TEnvelopeParameter envelopeParameter = { envelopeLength, envelopeLoop, envelopeLength, envelopeSpeed, true, false, false, false };
		TEnvelopeParameter tableParameter = { tableLength, tableLoop, tableLength, tableSpeed, true, false, false, tableMode };

		// Apply to the respective Envelopes and Tables
		pVolumeEnvelope->parameter = envelopeParameter;
		pTimbreEnvelope->parameter = envelopeParameter;
		pAudctlEnvelope->parameter = envelopeParameter;
		pEffectEnvelope->parameter = envelopeParameter;
		pNoteTableEnvelope->parameter = tableParameter;
		pFreqTableEnvelope->parameter = tableParameter;

		// Table Type is either Freq or Note, so pick whichever is suitable and fill it accordingly
		for (UINT j = 0; j < tableLength; j++)
		{
			if (tableType)
				pFreqTableEnvelope->freq[j].freqAbsolute = memInstrument[tablePtr + j];
			else
				pNoteTableEnvelope->note[j].noteAbsolute = memInstrument[tablePtr + j];
		}

		// Fill the equivalent RMTE envelopes, which might include some compromises due to the format differences
		for (UINT j = 0; j < envelopeLength; j++)
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

			// AutoFilter flag, used to automatically set the High Pass Filter bit in Channel 1 or 2
			bool autoFilter = envelopeCommand >> 7;

			// Auto16Bit flag, used to automatically set the 16-bit mode in Channel 2 or 4
			bool auto16Bit = false;

			// AutoPortamento flag, used to automatically apply the Portamento parameters during playback
			bool autoPortamento = envelopeCommand & 0x01;

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

				// The CMD7 is unused once these paramaters are converted, so set both the Command and Parameter to 0
				envelopeEffectCommand = IE_EMPTY;
				envelopeParameter = 0x00;
				break;

			case 0x06:
				// Various hacks were used with this command in the RMT 1.34 driver, such as the Distortion 6... Distortion used in 16-bit mode
				if (distortion == 0x06)
				{
					initialTimbre = envelopeParameter & 0x0E;
					envelopeEffectCommand = IE_EMPTY;
					envelopeParameter = 0x00;
				}
				else
					envelopeEffectCommand = IE_AUTOFILTER;
				break;

			case 0x05:
				envelopeEffectCommand = IE_PORTAMENTO;
				break;

			case 0x04:
				envelopeEffectCommand = IE_FREQ_SHIFT;
				break;

			case 0x03:
				envelopeEffectCommand = IE_NOTE_SHIFT;
				break;

			case 0x02:
				envelopeEffectCommand = IE_FINETUNE;
				break;

			case 0x01:
				envelopeEffectCommand = IE_SET_FREQ_LSB;
				break;

			case 0x00:
				envelopeEffectCommand = IE_TRANSPOSE;
				break;
			}

			// See above? Yeah, that, it does the thing, amazing isn't it?
			if (distortion == 0x06)
			{
				// The original "Auto16Bit" trigger ;)
				auto16Bit = true;
				distortion = initialTimbre;
			}

			// To be converted to the equivalent Timbre parameter
			switch (distortion)
			{
			case 0x00:
				distortion = TIMBRE_PINK_NOISE;
				break;

			case 0x02:
				distortion = TIMBRE_BELL;
				break;

			case 0x04:
				distortion = TIMBRE_SMOOTH_4;
				break;

			case 0x08:
				distortion = TIMBRE_WHITE_NOISE;
				break;

			case 0x06:
				// RMT 1.28 would set Distortion C by default, this is just assuming 1.34 behaviour for now

			case 0x0A:
				distortion = TIMBRE_PURE;
				break;

			case 0x0C:
				distortion = TIMBRE_BUZZY_C;
				break;

			case 0x0E:
				distortion = TIMBRE_GRITTY_C;
				break;
			}

			// Envelope Timbre, based on the Distortion parameter
			pTimbreEnvelope->timbre[j].timbreEnvelope = distortion;

			// Envelope Volume
			pVolumeEnvelope->volume[j].volumeLeft = envelopeVolume & 0x0F;
			pVolumeEnvelope->volume[j].volumeRight = envelopeVolume & 0xF0;

			// Set the Volume Only Mode as well if needed
			//pVolumeEnvelope->volume[j].isVolumeOnly = isVolumeOnly;

			// Envelope AUDCTL
			pAudctlEnvelope->audctl[j].audctlEnvelope = initialAudctl;

			// AutoFilter Trigger
			pEffectEnvelope->effect[j].autoFilter = autoFilter;

			// Auto16Bit Trigger
			pEffectEnvelope->effect[j].auto16Bit = auto16Bit;

			// AutoTwoTone Trigger
			pEffectEnvelope->effect[j].autoTwoTone = initialSkctl;

			// Portamento a Pattern Effect Command could be set where the Portamento is expected as a compromise
			//pEffectEnvelope->effect[j].autoPortamento = autoPortamento;

			// Extended RMT Command Envelope, with compatibility tweaks as a compromise
			pEffectEnvelope->effect[j].command_1 = envelopeEffectCommand;
			pEffectEnvelope->effect[j].parameter_1 = envelopeParameter;
			
			//pEffectEnvelope->effect[j].effectCommandLo = envelopeEffectCommand;
			//pEffectEnvelope->effect[j].effectCommandHi = 0x00;
			//pEffectEnvelope->effect[j].is16BitCommand = false;
			//pEffectEnvelope->effect[j].isEffectEnabled = true;
			//pEffectEnvelope->effect[j].effectParameterLo = envelopeParameter;
			//pEffectEnvelope->effect[j].effectParameterHi = 0x00;
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

TSubtune* CModule::GetSubtune(UINT subtune)
{
	if (m_subtuneIndex && IsValidSubtune(subtune))
		return m_subtuneIndex->subtune[subtune];

	return NULL;
}

TChannel* CModule::GetChannel(UINT subtune, UINT channel)
{
	return GetChannel(GetSubtune(subtune), channel);
}

TChannel* CModule::GetChannel(TSubtune* pSubtune, UINT channel)
{
	if (pSubtune && IsValidChannel(channel))
		return &pSubtune->channel[channel];

	return NULL;
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
	if (pChannel && IsValidPattern(pattern))
		return &pChannel->pattern[pattern];

	return NULL;
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
	return GetPattern(pChannel, GetPatternInSongline(pChannel, songline));
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
	if (pPattern && IsValidRow(row))
		return &pPattern->row[row];

	return NULL;
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
			effectCount = PATTERN_EFFECT_COUNT;

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
	if (pChannel)
	{
		// 0 is actually the highest possible value due to base 0 indexing
		if (!IsValidCommandColumn(column))
			column = 0;

		// If the value of 0 is used for whatever reason, assume it was meant to be 1
		else if (column == 0)
			column++;

		pChannel->effectCount = column;
		return true;
	}

	return false;
}


//--

const UINT CModule::GetSubtuneCount()
{
	UINT count = 0;

	for (UINT i = 0; i < SUBTUNE_COUNT; i++)
		count += GetSubtune(i) != NULL;

	return count;
}


//--

TInstrumentV2* CModule::GetInstrument(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->instrument[instrument];

	return NULL;
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


//--

TEnvelope* CModule::GetVolumeEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->volume[instrument];

	return NULL;
}

TEnvelope* CModule::GetTimbreEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->timbre[instrument];

	return NULL;
}

TEnvelope* CModule::GetAudctlEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->audctl[instrument];

	return NULL;
}

TEnvelope* CModule::GetEffectEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->effect[instrument];

	return NULL;
}

TEnvelope* CModule::GetNoteTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->note[instrument];

	return NULL;
}

TEnvelope* CModule::GetFreqTableEnvelope(UINT instrument)
{
	if (m_instrumentIndex && IsValidInstrument(instrument))
		return m_instrumentIndex->freq[instrument];

	return NULL;
}


//--

bool CModule::SetInstrumentName(UINT instrument, const char* name)
{
	return SetInstrumentName(GetInstrument(instrument), name);
}

bool CModule::SetInstrumentName(TInstrumentV2* pInstrument, const char* name)
{
	if (pInstrument)
	{
		strncpy_s(pInstrument->name, name, INSTRUMENT_NAME_MAX);
		return true;
	}

	return false;
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
				if (command == PE_GOTO_SONGLINE || command == PE_END_PATTERN)
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
		for (UINT i = 0; i < SONGLINE_COUNT; i++)
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
		for (int i = 0; i < PATTERN_EFFECT_COUNT; i++)
		{
			// Only the Identifier is checked, since the Parameter cannot be used alone
			if (pRow->effect[i].command != PE_EMPTY)
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
	for (int i = 0; i < PATTERN_EFFECT_COUNT; i++)
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

// Create a new Pattern in a Songline to a new unused position, Return True if successful
bool CModule::SetNewEmptyPatternInSongline(UINT subtune, UINT channel, UINT songline)
{
	return SetNewEmptyPatternInSongline(GetChannel(subtune, channel), songline);
}

// Create a new Pattern in a Songline to a new unused position, Return True if successful
bool CModule::SetNewEmptyPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline)
{
	return SetNewEmptyPatternInSongline(GetChannel(pSubtune, channel), songline);
}

// Create a new Pattern in a Songline to a new unused position, Return True if successful
bool CModule::SetNewEmptyPatternInSongline(TChannel* pChannel, UINT songline)
{
	if (pChannel && IsValidSongline(songline))
	{
		// Find the first empty and unused Pattern that is available
		for (int i = 0; i < PATTERN_COUNT; i++)
		{
			if (IsUnusedPattern(pChannel, i) && IsEmptyPattern(GetPattern(pChannel, i)))
				return SetPatternInSongline(pChannel, songline, i);
		}
	}

	// Could not create a new Pattern, no data was edited
	return false;
}

// Copy data from source Row to destination Row, Return True if successful
bool CModule::CopyRow(TRow* pFromRow, TRow* pToRow)
{
	// Make sure both the Rows from source and destination are not Null pointers
	if (!pFromRow || !pToRow)
		return false;

	pToRow->note = pFromRow->note;
	pToRow->instrument = pFromRow->instrument;
	pToRow->volume = pFromRow->volume;

	for (int i = 0; i < PATTERN_EFFECT_COUNT; i++)
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
	UINT channelCount = GetChannelCount(pSubtune);

	for (UINT i = 0; i < channelCount; i++)
	{
		for (UINT j = 0; j < SONGLINE_COUNT; j++)
		{
			// Get the Pattern from which comparisons will be made
			UINT reference = GetPatternInSongline(pSubtune, i, j);

			// Compare to every Patterns found in the Channel Index, unused Patterns will be ignored
			for (UINT k = 0; k < SONGLINE_COUNT; k++)
			{
				// Get the Pattern that will be compared to the reference Pattern
				UINT compared = GetPatternInSongline(pSubtune, i, k);

				// Comparing a Pattern to itself is pointless
				if (compared == reference)
					continue;

				// Compare the Patterns, if a match is found, update the Songline Index and delete the duplicate Pattern
				if (IsIdenticalPattern(GetPattern(pSubtune, i, reference), GetPattern(pSubtune, i, compared)))
				{
					SetPatternInSongline(pSubtune, i, k, reference);
					DeletePattern(GetChannel(pSubtune, i), compared);
				}
			}
		}
	}
}

// Renumber all Patterns from first to last Songlines, without optimisations
void CModule::RenumberIndexedPatterns(UINT subtune)
{
	RenumberIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, without optimisations
void CModule::RenumberIndexedPatterns(TSubtune* pSubtune)
{
	UINT channelCount = GetChannelCount(pSubtune);

	// Process all Channels within the Subtune Index
	for (UINT i = 0; i < channelCount; i++)
	{
		// Create a Temporary Channel that will be used as a buffer
		TChannel* backupChannel = new TChannel();
		InitialiseChannel(backupChannel);

		// Copy the original data to the Temporary Channel first
		CopyChannel(GetChannel(pSubtune, i), backupChannel);

		// Copy all Indexed Patterns to single Songline entries, effectively duplicating all Patterns used in multiple Songlines
		for (UINT j = 0; j < SONGLINE_COUNT; j++)
		{
			UINT pattern = GetPatternInSongline(GetChannel(pSubtune, i), j);
			CopyPattern(GetPattern(pSubtune, i, pattern), GetPattern(backupChannel, j));
			SetPatternInSongline(backupChannel, j, j);
		}

		// Copy the re-organised data back to the original Channel
		CopyChannel(backupChannel, GetChannel(pSubtune, i));

		// Delete the Temporary Channel once it's no longer needed
		delete backupChannel;
	}
}

// Clear all unused Indexed Patterns
void CModule::ClearUnusedPatterns(UINT subtune)
{
	ClearUnusedPatterns(GetSubtune(subtune));
}

// Clear all unused Indexed Patterns
void CModule::ClearUnusedPatterns(TSubtune* pSubtune)
{
	UINT channelCount = GetChannelCount(pSubtune);

	// Process all Channels within the Module Index
	for (UINT i = 0; i < channelCount; i++)
	{
		// Search for all unused indexed Patterns
		for (UINT j = 0; j < PATTERN_COUNT; j++)
		{
			// If the Pattern is not used anywhere, it will be deleted and removed from the Songline Index
			if (IsUnusedPattern(GetChannel(pSubtune, i), j))
				DeletePattern(GetChannel(pSubtune, i), j);
		}
	}
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
void CModule::ConcatenateIndexedPatterns(UINT subtune)
{
	ConcatenateIndexedPatterns(GetSubtune(subtune));
}

// Renumber all Patterns from first to last Songlines, and optimise them by concatenation
void CModule::ConcatenateIndexedPatterns(TSubtune* pSubtune)
{
	UINT channelCount = GetChannelCount(pSubtune);

	// Process all Channels within the Subtune Index
	for (UINT i = 0; i < channelCount; i++)
	{
		// Create a Temporary Channel that will be used as a buffer
		TChannel* backupChannel = new TChannel();
		InitialiseChannel(backupChannel);

		// Copy the original data to the Temporary Channel first
		CopyChannel(GetChannel(pSubtune, i), backupChannel);
		
		// Concatenate the Indexed Patterns to remove gaps between them
		for (UINT j = 0; j < PATTERN_COUNT; j++)
		{
			// If a Pattern is used at least once, see if it could also be concatenated further back
			if (!IsUnusedPattern(backupChannel, j))
			{
				// Find the first empty and unused Pattern that is available
				for (UINT k = 0; k < j; k++)
				{
					// If the Pattern is empty and unused, it will be replaced
					if (IsUnusedPattern(backupChannel, k) && IsEmptyPattern(GetPattern(backupChannel, k)))
					{
						// Copy the Pattern from J to K
						CopyPattern(GetPattern(backupChannel, j), GetPattern(backupChannel, k));

						// Clear the Pattern from J, since it won't be needed anymore
						DeletePattern(backupChannel, j);

						// Replace the Pattern used in the Songline Index with the new one
						for (UINT l = 0; l < SONGLINE_COUNT; l++)
						{
							if (GetPatternInSongline(backupChannel, l) == j)
								SetPatternInSongline(backupChannel, l, k);
						}
					}
				}
			}
		}

		// Copy the re-organised data back to the original Channel
		CopyChannel(backupChannel, GetChannel(pSubtune, i));

		// Delete the Temporary Channel once it's no longer needed
		delete backupChannel;
	}
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
void CModule::AllSizeOptimisations(UINT subtune)
{
	AllSizeOptimisations(GetSubtune(subtune));
}

// Optimise the RMTE Module, by re-organising everything within the Indexed Structures, in order to remove most of the unused/duplicated data efficiently
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

// Return the Pattern Effect Command Identifier characters
const char* CModule::GetPatternEffectCommandIdentifier(TPatternEffectCommand command)
{
	switch (command)
	{
	case PE_EMPTY:
		return "-";

	case PE_ARPEGGIO:
		return "0";

	case PE_PITCH_UP:
		return "1";

	case PE_PITCH_DOWN:
		return "2";

	case PE_PORTAMENTO:
		return "3";

	case PE_VIBRATO:
		return "4";

	case PE_VOLUME_FADE:
		return "A";

	case PE_GOTO_SONGLINE:
		return "B";

	case PE_END_PATTERN:
		return "D";

	case PE_SET_SPEED:
		return "F";

	case PE_SET_FINETUNE:
		return "P";

	case PE_SET_DELAY:
		return "G";

	default:
		// Unknown or Invalid Pattern Effect Command Identifier
		return "?";
	}
}

const char* CModule::GetPatternNoteCommand(TPatternNote note)
{
	switch (note)
	{
	case NOTE_EMPTY:
		return "---";

	case NOTE_OFF:
		return "OFF";

	case NOTE_RELEASE:
		return "===";

	case NOTE_RETRIGGER:
		return "^^^";

	default:
		// Unknown or Invalid Pattern Note Identifier
		return "???";
	}
}

const char* CModule::GetPatternNoteIndex(TPatternNote note)
{
	BYTE notation = 0;

	if (MODULE_DISPLAY_FLAT_NOTES)
		notation += 1;

	if (MODULE_DISPLAY_GERMAN_NOTATION)
		notation += 2;

	return notesandscales[notation][note % 12];
}

UINT CModule::GetPatternNoteOctave(TPatternNote note)
{
	return note / 12;
}

const char* CModule::GetPatternInstrumentCommand(TPatternInstrument instrument)
{
	switch (instrument)
	{
	case INSTRUMENT_EMPTY:
		return "--";

	default:
		// Unknown or Invalid Pattern Instrument Identifier
		return "??";
	}
}

const char* CModule::GetPatternVolumeCommand(TPatternVolume volume)
{
	switch (volume)
	{
	case VOLUME_EMPTY:
		return "--";

	default:
		// Unknown or Invalid Pattern Volume Identifier
		return "??";
	}
}
