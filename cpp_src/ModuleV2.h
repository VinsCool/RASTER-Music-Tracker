// RMT Module V2 Format Prototype
// By VinsCool, 2023

#pragma once

#include "General.h"
#include "global.h"

// ----------------------------------------------------------------------------
// Data boundaries constant
//
#define INVALID							-1									// Failsafe value for invalid data
//#define BYTE_MIN						0x00								// BYTE Minimum
//#define BYTE_MAX						0xFF								// BYTE Maximum
//#define WORD_MIN						0x0000								// WORD Minimum
//#define WORD_MAX						0xFFFF								// WORD Maximum

// ----------------------------------------------------------------------------
// Module Header definition
//
#define MODULE_VERSION					0									// Module Version number, the highest value is always assumed to be the most recent
#define MODULE_IDENTIFIER				"RMTE"								// Raster Music Tracker Extended, "DUMB" is another potential identifier
#define MODULE_REGION					g_ntsc								// 0 for PAL, 1 for NTSC, anything else is also assumed to be NTSC
#define MODULE_STEREO					g_tracks4_8							// 4 for Mono, 8 for Stereo
#define MODULE_A4_TUNING				g_basetuning						// Default A-4 Tuning
#define MODULE_BASE_NOTE				g_basenote							// Default Base Note (A-)
#define MODULE_TEMPERAMENT				g_temperament						// Default Tuning Temperament
#define MODULE_PRIMARY_HIGHLIGHT		g_trackLinePrimaryHighlight			// Pattern Primary Highlight
#define MODULE_SECONDARY_HIGHLIGHT		g_trackLineSecondaryHighlight		// Pattern Secondary Highlight
#define MODULE_LINE_NUMBERING_MODE		g_tracklinealtnumbering				// Row numbering mode
#define MODULE_LINE_STEP				g_linesafter						// Line Step between cursor movements
#define MODULE_DISPLAY_FLAT_NOTES		g_displayflatnotes					// Display accidentals as Flat instead of Sharp
#define MODULE_DISPLAY_GERMAN_NOTATION	g_usegermannotation					// Display notes using the German Notation
#define MODULE_DEFAULT_SUBTUNE			0									// Default Active Subtune
#define MODULE_SUBTUNE_COUNT			1									// Default Subtune Count
#define MODULE_TRACK_LENGTH				64									// Default Track Length
#define MODULE_SONG_LENGTH				1									// Default Song Length
#define MODULE_SONG_SPEED				6									// Default Song Speed
#define MODULE_VBI_SPEED				1									// Default VBI Speed
#define MODULE_TITLE_NAME_MAX			64									// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64									// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64									// Maximum length of Copyright info

// ----------------------------------------------------------------------------
// Song and Track Pattern definition
//
#define SUBTUNE_NAME_MAX				64									// 0-63 inclusive, Maximum length of Subtune name
#define SUBTUNE_MAX						64									// 0-63 inclusive, Maximum number of Subtunes in a Module file
#define SONGLINE_MAX					256									// 0-255 inclusive, Songline index used in Song
#define TRACK_CHANNEL_MAX				8									// 0-7 inclusive, 2 POKEY soundchips, each using 4 Channels, a typical Stereo configuration
#define TRACK_ROW_MAX					256									// 0-255 inclusive, Row index used in Pattern
#define TRACK_PATTERN_MAX				256									// 0-255 inclusive, Pattern index used in Song

#define CH1								0									// POKEY Channel identifier for Pattern Column 1
#define CH2								1									// POKEY Channel identifier for Pattern Column 2
#define CH3								2									// POKEY Channel identifier for Pattern Column 3
#define CH4								3									// POKEY Channel identifier for Pattern Column 4
#define CH5								4									// POKEY Channel identifier for Pattern Column 5
#define CH6								5									// POKEY Channel identifier for Pattern Column 6
#define CH7								6									// POKEY Channel identifier for Pattern Column 7
#define CH8								7									// POKEY Channel identifier for Pattern Column 8
#define PATTERN_COLUMN_MAX				8									// 0-7 inclusive, Pattern column index used for Note, Instrument, Volume, and Effect Commands

#define PATTERN_ACTIVE_EFFECT_MAX		4									// 0-3 inclusive, Number of Active Effect columns in Track Channel

#define PATTERN_NOTE_COUNT				96									// 0-95 inclusive, Note index used in Pattern
#define PATTERN_NOTE_EMPTY				PATTERN_NOTE_COUNT					// There is no Note in the Pattern Row
#define PATTERN_NOTE_OFF				PATTERN_NOTE_COUNT + 1				// The Note Command OFF will stop the last played note in the Track Channel
#define PATTERN_NOTE_RELEASE			PATTERN_NOTE_COUNT + 2				// The Note Command === will release the last played note in the Track Channel
#define PATTERN_NOTE_RETRIGGER			PATTERN_NOTE_COUNT + 3				// The Note Command ~~~ will retrigger the last played note in the Track Channel
#define PATTERN_NOTE_MAX				PATTERN_NOTE_COUNT + 4				// Total for Note index integrity

#define PATTERN_INSTRUMENT_COUNT		64									// 0-63 inclusive, Instrument index used in Pattern
#define PATTERN_INSTRUMENT_EMPTY		PATTERN_INSTRUMENT_COUNT			// There is no Instrument in the Pattern Row
#define	PATTERN_INSTRUMENT_MAX			PATTERN_INSTRUMENT_COUNT + 1		// Total for Instrument index integrity

#define PATTERN_VOLUME_COUNT			16									// 0-15 inclusive, Volume index used in Pattern
#define PATTERN_VOLUME_EMPTY			PATTERN_VOLUME_COUNT				// There is no Volume in the Pattern Row
#define PATTERN_VOLUME_MAX				PATTERN_VOLUME_COUNT + 1			// Total for Volume index integrity

#define PATTERN_EFFECT_COUNT			16									// 0-15 inclusive, Effect index used in Pattern
#define PATTERN_EFFECT_EMPTY			(PATTERN_EFFECT_COUNT << 8)			// There is no Effect Command in the Pattern Row
#define PATTERN_EFFECT_MAX				((PATTERN_EFFECT_COUNT << 8) + 1)	// Total for Effect Command index integrity

//
#define PATTERN_EFFECT_BXX				(0x0B << 8)							// Effect Command Bxx -> Goto Songline $xx
#define PATTERN_EFFECT_DXX				(0x0D << 8)							// Effect Command Dxx -> End Pattern (no parameter needed)
//

// ----------------------------------------------------------------------------
// Instrument definition
//
#define INSTRUMENT_NAME_MAX				64									// Maximum length of instrument name
#define INSTRUMENT_PARAMETER_MAX		24									// Instrument parameter 0-23, inclusive
#define INSTRUMENT_TABLE_INDEX_MAX		32									// Instrument note/freq table 0-31, inclusive
#define ENVELOPE_INDEX_MAX				48									// Instrument envelope 0-47, inclusive
#define ENVELOPE_PARAMETER_MAX			8									// Instrument envelope parameter 0-7, inclusive

// ----------------------------------------------------------------------------
// Effect Command definition
//
#define EFFECT_PARAMETER_MAX			256									// 0-255 inclusive, Effect $XY Parameter used in Pattern
#define EFFECT_PARAMETER_MIN			0x00								// The $XY Parameter of 0 may be used to disable certain Effect Commands
#define EFFECT_PARAMETER_DEFAULT		0x80								// The $XY Parameter of 128 may be used to disable certain Effect Commands
#define CMD1							0									// Effect Command identifier for Effect Column 1
#define CMD2							1									// Effect Command identifier for Effect Column 2
#define CMD3							2									// Effect Command identifier for Effect Column 3
#define CMD4							3									// Effect Command identifier for Effect Column 4

// ----------------------------------------------------------------------------
// RMTE Module Structs
//

// Row Data, used within the Pattern data, designed to be easy to manage, following a Row by Row approach
struct TRow
{
	BYTE note;										// Note index, as well as Pattern Commands such as Stop, Release, Retrigger, etc
	BYTE instrument;								// Instrument index
	BYTE volume;									// Volume index
	WORD cmd0;										// Effect Command, toggled from the Active Effect Columns in Track Channels
	WORD cmd1;										// All 4 Commands could be used at once in order to run multiple Effects in the same row
	WORD cmd2;										// Certain Effects may not compatible together, however, and could cause priority conflicts
	WORD cmd3;										// Making sure some commands take priority over the others would help working around issues
};

// Pattern Data, indexed by the TRow Struct
struct TPattern
{
	TRow row[TRACK_ROW_MAX];						// Row data is contained withn its associated Pattern index
};

// Channel Index, used for indexing the Songline and Pattern data, similar to the CSong Class
struct TIndex
{
	BYTE songline[SONGLINE_MAX];					// Pattern Index for each songline within the Track Channel
	TPattern pattern[TRACK_PATTERN_MAX];			// Pattern Data for the Track Channel
};

// Subtune Index, used for indexing all of the Module data, indexed by the TIndex Struct
struct TSubtune
{
	char name[SUBTUNE_NAME_MAX];					// Subtune name
	BYTE songLength;								// Song Length, in Songlines
	BYTE patternLength;								// Pattern Length, in Rows
	BYTE channelCount;								// Number of Channels used in Subtune
	BYTE songSpeed;									// Song Speed, in Frames per Row
	BYTE instrumentSpeed;							// Instrument Speed, in Frames per VBI
	BYTE effectCommandCount[TRACK_CHANNEL_MAX];		// Number of Effect Commands enabled per Track Channel
	TIndex channel[TRACK_CHANNEL_MAX];				// Channel Index assigned to the Subtune
};

// Instrument Data, due to the Legacy TInstrument struct, this is temporarily defined as TInstrumentV2
struct TInstrumentV2
{
	char name[INSTRUMENT_NAME_MAX];					// Instrument name
	BYTE envelopeLength;							// Envelope Length, in frames
	BYTE envelopeLoop;								// Envelope Loop point, in frames
	BYTE envelopeRelease;							// Envelope Release point, in frames
	BYTE tableLength;								// Table Length, in frames
	BYTE tableLoop;									// Table Loop point, in frames
	BYTE tableRelease;								// Table Release point, in frames
	BYTE tableMode;									// Table Mode, Absolute or Relative
	BYTE volumeEnvelope[ENVELOPE_INDEX_MAX];		// Volume Envelope
	BYTE distortionEnvelope[ENVELOPE_INDEX_MAX];	// Distortion Envelope
	BYTE audctlEnvelope[ENVELOPE_INDEX_MAX];		// AUDCTL Envelope, may vary between Track Channels
	BYTE noteTable[INSTRUMENT_TABLE_INDEX_MAX];		// Note Table
	BYTE freqTable[INSTRUMENT_TABLE_INDEX_MAX];		// Freq Table
};

// ----------------------------------------------------------------------------
// RMTE Module Class
//
class CModule
{
public:
	CModule();
	~CModule();

	void InitialiseModule();
	void ClearModule();

	void SetModuleStatus(bool status) { m_initialised = status; };

	void ImportLegacyRMT(std::ifstream& in);

	// Booleans for Module index and data integrity
	bool IsModuleInitialised() { return m_initialised; };
	bool IsValidSubtune(int subtune) { return subtune > INVALID && subtune < SUBTUNE_MAX; };
	bool IsValidChannel(int channel) { return channel > INVALID && channel < TRACK_CHANNEL_MAX; };
	bool IsValidSongline(int songline) { return songline > INVALID && songline < SONGLINE_MAX; };
	bool IsValidPattern(int pattern) { return pattern > INVALID && pattern < TRACK_PATTERN_MAX; };
	bool IsValidRow(int row) { return row > INVALID && row < TRACK_ROW_MAX; };
	bool IsValidNote(int note) { return note > INVALID && note < PATTERN_NOTE_MAX; };
	bool IsValidInstrument(int instrument) { return instrument > INVALID && instrument < PATTERN_INSTRUMENT_MAX; };
	bool IsValidVolume(int volume) { return volume > INVALID && volume < PATTERN_VOLUME_MAX; };
	bool IsValidCommand(int command) { return command > INVALID && command < PATTERN_EFFECT_MAX; };
	bool IsValidCommandColumn(int column) { return column > INVALID && column <= PATTERN_ACTIVE_EFFECT_MAX; };
	bool IsValidPatternRowIndex(int channel, int pattern, int row) { return IsValidChannel(channel) && IsValidPattern(pattern) && IsValidRow(row); };

	// Pointers to Module Structs
	TSubtune* GetSubtuneIndex(int subtune) { return IsValidSubtune(subtune) ? &m_index[subtune] : NULL; };
	TIndex* GetChannelIndex() { return GetSubtuneIndex(m_activeSubtune)->channel; };
	TIndex* GetChannelIndex(int channel) { return IsValidChannel(channel) ? &GetSubtuneIndex(m_activeSubtune)->channel[channel] : NULL; };
	BYTE* GetSonglineIndex(int channel) { return IsValidChannel(channel) ? GetSubtuneIndex(m_activeSubtune)->channel[channel].songline : NULL; };
	TPattern* GetPattern(int channel, int pattern) { return IsValidChannel(channel) && IsValidPattern(pattern) ? &GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern] : NULL; };
	TPattern* GetIndexedPattern(int channel, int songline) { return GetPattern(channel, GetPatternInSongline(channel, songline)); };
	TRow* GetRow(int channel, int pattern, int row) { return IsValidPatternRowIndex(channel, pattern, row) ? &GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row] : NULL; };
	TInstrumentV2* GetInstrument(int instrument) { return IsValidInstrument(instrument) ? &m_instrument[instrument] : NULL; };

	// Getters for Pattern data
	const BYTE GetPatternInSongline(int channel, int songline) { return IsValidChannel(channel) && IsValidSongline(songline) ? GetSubtuneIndex(m_activeSubtune)->channel[channel].songline[songline] : INVALID; };
	const BYTE GetPatternRowNote(int channel, int pattern, int row) { return IsValidPatternRowIndex(channel, pattern, row) ? GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].note : INVALID; };
	const BYTE GetPatternRowInstrument(int channel, int pattern, int row) { return IsValidPatternRowIndex(channel, pattern, row) ? GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].instrument : INVALID; };
	const BYTE GetPatternRowVolume(int channel, int pattern, int row) { return IsValidPatternRowIndex(channel, pattern, row) ? GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].volume : INVALID; };
	const WORD GetPatternRowCommand(int channel, int pattern, int row, int column)
	{
		if (IsValidPatternRowIndex(channel, pattern, row))
			switch (column)
			{
			case CMD1: return GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd0;
			case CMD2: return GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd1;
			case CMD3: return GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd2;
			case CMD4: return GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd3;
			}
		return INVALID;
	};

	// Setters for Pattern data
	void SetPatternInSongline(int channel, int songline, int pattern)
	{
		if (IsValidChannel(channel) && IsValidSongline(songline))
			GetSubtuneIndex(m_activeSubtune)->channel[channel].songline[songline] = IsValidPattern(pattern) ? pattern : INVALID;
	};

	void SetPatternRowCommand(int channel, int pattern, int row, int column, WORD effectCommand)
	{
		if (IsValidPatternRowIndex(channel, pattern, row))
			switch (column)
			{
			case CMD1:
				GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd0 = effectCommand;
				break;

			case CMD2:
				GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd1 = effectCommand;
				break;

			case CMD3:
				GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd2 = effectCommand;
				break;

			case CMD4:
				GetSubtuneIndex(m_activeSubtune)->channel[channel].pattern[pattern].row[row].cmd3 = effectCommand;
				break;
			}
	};

	// Getters for Instrument data
	const char* GetInstrumentName(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].name : "INVALID INSTRUMENT"; };
	const BYTE GetInstrumentEnvelopeLength(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].envelopeLength : INVALID; };
	const BYTE GetInstrumentEnvelopeLoop(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].envelopeLoop : INVALID; };
	const BYTE GetInstrumentEnvelopeRelease(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].envelopeRelease : INVALID; };
	const BYTE GetInstrumentTableLength(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].tableLength : INVALID; };
	const BYTE GetInstrumentTableLoop(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].tableLoop : INVALID; };
	const BYTE GetInstrumentTableRelease(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].tableRelease : INVALID; };
	const BYTE GetInstrumentTableMode(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].tableMode : INVALID; };
	const BYTE* GetInstrumentVolumeEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].volumeEnvelope : NULL; };
	const BYTE* GetInstrumentDistortionEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].distortionEnvelope : NULL; };
	const BYTE* GetInstrumentAudctlEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].audctlEnvelope : NULL; };
	const BYTE* GetInstrumentNoteTable(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].noteTable : NULL; };
	const BYTE* GetInstrumentFreqTable(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument].freqTable : NULL; };

	// Getters for Module parameters
	const char* GetSongName() { return m_songName; };
	
	const BYTE GetActiveSubtune() { return m_activeSubtune; };
	const BYTE GetSubtuneCount() { return m_subtuneCount; };

	const char* GetSubtuneName(int subtune) { return GetSubtuneIndex(subtune)->name; };
	const BYTE GetSongLength(int subtune) { return GetSubtuneIndex(subtune)->songLength; };
	const BYTE GetPatternLength(int subtune) { return GetSubtuneIndex(subtune)->patternLength; };
	const BYTE GetChannelCount(int subtune) { return GetSubtuneIndex(subtune)->channelCount; };
	const BYTE GetSongSpeed(int subtune) { return GetSubtuneIndex(subtune)->songSpeed; };
	const BYTE GetInstrumentSpeed(int subtune) { return GetSubtuneIndex(subtune)->instrumentSpeed; };
	const BYTE GetEffectCommandCount(int subtune, int channel) { return IsValidChannel(channel) ? GetSubtuneIndex(subtune)->effectCommandCount[channel] : INVALID; };

	const char* GetSubtuneName() { return GetSubtuneIndex(m_activeSubtune)->name; };
	const BYTE GetSongLength() { return GetSubtuneIndex(m_activeSubtune)->songLength; };
	const BYTE GetPatternLength() { return GetSubtuneIndex(m_activeSubtune)->patternLength; };
	const BYTE GetChannelCount() { return GetSubtuneIndex(m_activeSubtune)->channelCount; };
	const BYTE GetSongSpeed() { return GetSubtuneIndex(m_activeSubtune)->songSpeed; };
	const BYTE GetInstrumentSpeed() { return GetSubtuneIndex(m_activeSubtune)->instrumentSpeed; };
	const BYTE GetEffectCommandCount(int channel) { return IsValidChannel(channel) ? GetSubtuneIndex(m_activeSubtune)->effectCommandCount[channel] : INVALID; };

	// Setters for Module parameters
	void SetSongName(const char* name) { strcpy(m_songName, name); };	// Unsafe?

	void SetActiveSubtune(int subtune) { m_activeSubtune = subtune; };
	void SetSubtuneCount(int count) { m_subtuneCount = count; };

	void SetSubtuneName(int subtune, const char* name) { strcpy(GetSubtuneIndex(subtune)->name, name); };	// Unsafe?
	void SetSongLength(int subtune, int length) { GetSubtuneIndex(subtune)->songLength = length; };
	void SetPatternLength(int subtune, int length) { GetSubtuneIndex(subtune)->patternLength = length; };
	void SetChannelCount(int subtune, int count) { GetSubtuneIndex(subtune)->channelCount = count; };
	void SetSongSpeed(int subtune, int speed) { GetSubtuneIndex(subtune)->songSpeed = speed; };
	void SetInstrumentSpeed(int subtune, int speed) { GetSubtuneIndex(subtune)->instrumentSpeed = speed; };
	void SetEffectCommandCount(int subtune, int channel, int column) { if (IsValidChannel(channel) && IsValidCommandColumn(column)) GetSubtuneIndex(subtune)->effectCommandCount[channel] = column; };

	void SetSubtuneName(const char* name) { strcpy(GetSubtuneIndex(m_activeSubtune)->name, name); };	// Unsafe?
	void SetSongLength(int length) { GetSubtuneIndex(m_activeSubtune)->songLength = length; };
	void SetPatternLength(int length) { GetSubtuneIndex(m_activeSubtune)->patternLength = length; };
	void SetChannelCount(int count) { GetSubtuneIndex(m_activeSubtune)->channelCount = count; };
	void SetSongSpeed(int speed) { GetSubtuneIndex(m_activeSubtune)->songSpeed = speed; };
	void SetInstrumentSpeed(int speed) { GetSubtuneIndex(m_activeSubtune)->instrumentSpeed = speed; };
	void SetEffectCommandCount(int channel, int column) { if (IsValidChannel(channel) && IsValidCommandColumn(column)) GetSubtuneIndex(m_activeSubtune)->effectCommandCount[channel] = column; };

	// Functions related to Pattern Data editing, and other unsorted things added in the process
	BYTE GetShortestPatternLength(int songline);

	bool DuplicatePatternInSongline(int channel, int songline, int pattern);
	bool IsUnusedPattern(int channel, int pattern);
	bool IsUnusedPattern(TIndex* index, int pattern);
	bool IsEmptyPattern(int channel, int pattern);
	bool IsEmptyPattern(TPattern* pattern);
	bool IsIdenticalPattern(TPattern* sourcePattern, TPattern* destinationPattern);
	bool CopyPattern(TPattern* sourcePattern, TPattern* destinationPattern);
	bool ClearPattern(int channel, int pattern);
	bool ClearPattern(TPattern* destinationPattern);
	bool CopyIndex(TIndex* sourceIndex, TIndex* destinationIndex);
	bool CopySubtune(TSubtune* sourceSubtune, TSubtune* destinationSubtune);
	bool DuplicatePatternIndex(int sourceIndex, int destinationIndex);
	int MergeDuplicatedPatterns();
	void RenumberIndexedPatterns();
	void ClearUnusedPatterns();
	void ConcatenateIndexedPatterns();
	void AllSizeOptimisations();
	int GetSubtuneFromLegacyRMT(int startSongline, CString& resultstr);

private:
	TSubtune* m_index;
	TInstrumentV2* m_instrument;

	bool m_initialised;

	char m_songName[MODULE_TITLE_NAME_MAX];

	BYTE m_activeSubtune;
	BYTE m_subtuneCount;
};
