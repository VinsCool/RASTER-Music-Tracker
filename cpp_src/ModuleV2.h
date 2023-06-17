// RMT Module V2 Format Prototype
// By VinsCool, 2023
//
// TODO: Move the Legacy Import code to IO_Song.cpp or similar, in order to get most of the CModule functions cleared from unrelated stuff
// TODO: Move most of the Editor Functions to CSong or similar for the same reason

#pragma once

#include "General.h"
#include "global.h"


// ----------------------------------------------------------------------------
// Module Header definition
//

#define MODULE_VERSION					0									// Module Version number, the highest value is always assumed to be the most recent
#define MODULE_IDENTIFIER				"RMTE"								// Raster Music Tracker Extended, "DUMB" is another potential identifier
#define MODULE_REGION					g_ntsc								// 0 for PAL, 1 for NTSC, anything else is also assumed to be NTSC
#define MODULE_CHANNEL_COUNT			g_tracks4_8							// 4 for Mono, 8 for Stereo, add more for whatever setup that could be used
#define MODULE_A4_TUNING				g_baseTuning						// Default A-4 Tuning
#define MODULE_BASE_NOTE				g_baseNote							// Default Base Note (A-)
#define MODULE_BASE_OCTAVE				g_baseOctave						// Default Base Octave (0)
#define MODULE_TEMPERAMENT				g_baseTemperament					// Default Tuning Temperament
#define MODULE_PRIMARY_HIGHLIGHT		g_trackLinePrimaryHighlight			// Pattern Primary Highlight
#define MODULE_SECONDARY_HIGHLIGHT		g_trackLineSecondaryHighlight		// Pattern Secondary Highlight
#define MODULE_LINE_NUMBERING_MODE		g_tracklinealtnumbering				// Row numbering mode
#define MODULE_LINE_STEP				g_linesafter						// Line Step between cursor movements
#define MODULE_DISPLAY_FLAT_NOTES		g_displayflatnotes					// Display accidentals as Flat instead of Sharp
#define MODULE_DISPLAY_GERMAN_NOTATION	g_usegermannotation					// Display notes using the German Notation
#define MODULE_SCALING_PERCENTAGE		g_scaling_percentage				// Display scaling percentage
#define MODULE_DEFAULT_SUBTUNE			0									// Default Active Subtune
#define MODULE_DEFAULT_INSTRUMENT		0									// Default Active Instrument
#define MODULE_DEFAULT_PATTERN_LENGTH	64									// Default Pattern Length
#define MODULE_DEFAULT_SONG_LENGTH		1									// Default Song Length
#define MODULE_DEFAULT_SONG_SPEED		6									// Default Song Speed
#define MODULE_DEFAULT_VBI_SPEED		1									// Default VBI Speed
#define MODULE_SONG_NAME_MAX			64									// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64									// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64									// Maximum length of Copyright info
#define MODULE_PATTERN_EMPTY			0xFF								// Empty Pattern in Module if encountered
#define MODULE_ROW_EMPTY				0xFE								// Empty Row in Module if encountered


// ----------------------------------------------------------------------------
// Song and Track Pattern definition
//

#define SUBTUNE_NAME_MAX		64											// 0-63 inclusive, Maximum length of Subtune name
#define SUBTUNE_COUNT			64											// 0-63 inclusive, Maximum number of Subtunes in a Module file
#define SONGLINE_COUNT			256											// 0-255 inclusive, Songline index used in Song
#define PATTERN_COUNT			256											// 0-255 inclusive, Pattern index used in Song
#define ROW_COUNT				256											// 0-255 inclusive, Row index used in Pattern
#define POKEY_CHANNEL_COUNT		4											// 0-3 inclusive, each POKEY soundchip could use up to 4 Channels at once
#define POKEY_CHIP_COUNT		4											// 0-3 inclusive, Quad POKEY configuration (theoretical)
#define CHANNEL_COUNT			POKEY_CHIP_COUNT * POKEY_CHANNEL_COUNT		// Total number of Channels, Pattern column index used for Note, Instrument, Volume, and Effect Commands
#define CH1						0											// POKEY Channel identifier for Pattern Column 1
#define CH2						1											// POKEY Channel identifier for Pattern Column 2
#define CH3						2											// POKEY Channel identifier for Pattern Column 3
#define CH4						3											// POKEY Channel identifier for Pattern Column 4
#define NOTE_COUNT				120											// 0-119 inclusive, Note index used in Pattern, for a total of 10 octaves
#define NOTE_EMPTY				NOTE_COUNT									// There is no Note in the Pattern Row
#define NOTE_OFF				NOTE_COUNT + 1								// The Note Command OFF will stop the last played note in the Track Channel
#define NOTE_RELEASE			NOTE_COUNT + 2								// The Note Command === will release the last played note in the Track Channel
#define NOTE_RETRIGGER			NOTE_COUNT + 3								// The Note Command ~~~ will retrigger the last played note in the Track Channel
#define INSTRUMENT_COUNT		64											// 0-63 inclusive, Instrument index used in Pattern
#define INSTRUMENT_EMPTY		INSTRUMENT_COUNT							// There is no Instrument in the Pattern Row
#define VOLUME_COUNT			16											// 0-15 inclusive, Volume index used in Pattern
#define VOLUME_EMPTY			VOLUME_COUNT								// There is no Volume in the Pattern Row
#define PATTERN_EFFECT_COUNT	16											// 0-15 inclusive, Effect index used in Pattern
#define EFFECT_EMPTY			PATTERN_EFFECT_COUNT						// There is no Effect Command in the Pattern Row
#define ACTIVE_EFFECT_COUNT		4											// 0-3 inclusive, Number of Active Effect columns in Track Channel
#define INVALID					-1											// Failsafe value for invalid data


// ----------------------------------------------------------------------------
// Instrument definition
//

#define INSTRUMENT_NAME_MAX			64		// Maximum length of Instrument name
#define TABLE_STEP_COUNT			256		// Instrument Note/Freq Table 0-255, inclusive
#define ENVELOPE_STEP_COUNT			256		// Instrument Envelope 0-255, inclusive
#define INSTRUMENT_EFFECT_COUNT		16		// Instrument Envelope Effect Command 0-15, inclusive


// ----------------------------------------------------------------------------
// Effect Command definition
//

#define EFFECT_PARAMETER_MAX		0xFF	// 0-255 inclusive, Effect $XY Parameter used in Pattern
#define EFFECT_PARAMETER_MIN		0x00	// The $XY Parameter of 0 may be used to disable certain Effect Commands
#define EFFECT_PARAMETER_DEFAULT	0x80	// The $XY Parameter of 128 may be used to disable certain Effect Commands
#define EFFECT_COMMAND_BXX			0x0B	// Effect Command Bxx -> Goto Songline $xx
#define EFFECT_COMMAND_DXX			0x0D	// Effect Command Dxx -> End Pattern, no parameter needed(?)
#define EFFECT_COMMAND_FXX			0x0F	// Effect Command Fxx -> Set Song Speed $xx
#define CMD1						0		// Effect Command identifier for Effect Column 1
#define CMD2						1		// Effect Command identifier for Effect Column 2
#define CMD3						2		// Effect Command identifier for Effect Column 3
#define CMD4						3		// Effect Command identifier for Effect Column 4


// ----------------------------------------------------------------------------
// RMTE Module Structs for Subtune, Pattern, Row, etc
//

// Effect Command Data, 1 byte for the Identifier, and 1 byte for the Parameter, nothing too complicated
struct TEffect
{
	BYTE command;
	BYTE parameter;
};

// Row Data, used within the Pattern data, designed to be easy to manage, following a Row by Row approach
struct TRow
{
	BYTE note;								// Note Index, as well as Pattern Commands such as Stop, Release, Retrigger, etc
	BYTE instrument;						// Instrument Index
	BYTE volume;							// Volume Index
	TEffect effect[ACTIVE_EFFECT_COUNT];	// Effect Command, toggled from the Active Effect Columns in Track Channels
};

// Pattern Data, indexed by the TRow Struct
struct TPattern
{
	TRow row[ROW_COUNT];					// Row data is contained withn its associated Pattern index
};

// Channel Index, used for indexing the Songline and Pattern data, similar to the CSong Class
struct TChannel
{
	BYTE effectCount;						// Number of Effect Commands enabled per Track Channel
	BYTE songline[SONGLINE_COUNT];			// Pattern Index for each songline within the Track Channel
	TPattern pattern[PATTERN_COUNT];		// Pattern Data for the Track Channel
};

// Subtune Index, used for indexing all of the Module data, indexed by the TIndex Struct
struct TSubtune
{
	char name[SUBTUNE_NAME_MAX + 1];		// Subtune Name
	BYTE songLength;						// Song Length, in Songlines
	BYTE patternLength;						// Pattern Length, in Rows
	BYTE channelCount;						// Number of Channels used in Subtune
	BYTE songSpeed;							// Song Speed, in Frames per Row
	BYTE instrumentSpeed;					// Instrument Speed, in Frames per VBI
	TChannel channel[CHANNEL_COUNT];		// Channel Index assigned to the Subtune
};


// ----------------------------------------------------------------------------
// RMTE Module Structs for Instrument, Envelope, Table, etc
//

// Instrument AUDCTL/SKCTL Automatic Trigger bits, useful considering each POKEY channel featuring unique properties
struct TAutomatic
{
	bool autoFilter;						// High Pass Filter, triggered from Channel 1 and/or 2, hijacking Channel 3 and/or 4
	bool auto16Bit;							// 16-bit mode, triggered from Channel 2 and/or 4, hijacking Channel 1 and/or 3
	bool autoReverse16;						// Reverse 16-bit mode, triggered from Channel 1 and/or 3, hijacking Channel 2 and/or 4
	bool auto179Mhz;						// 1.79Mhz mode, triggered from Channel 1 and/or 3
	bool auto15Khz;							// 15Khz mode, triggered from any Channel, hijacking all Channels not affected by 1.79Mhz mode (16-bit included)
	bool autoPoly9;							// Poly9 Noise mode, triggered from any Channel, hijacking all Channels using Distortion 0 and 8
	bool autoTwoTone;						// Automatic Two-Tone Filter, triggered from Channel 1, hijacking Channel 2
	bool autoToggle;						// Automatic Toggle, for each ones of the possible combination, effectively overriding the AUDCTL Envelope(?)
};

// Instrument Macro Parameters
struct TParameter
{
	BYTE length;							// Length, in frames
	BYTE loop;								// Loop point, in frames
	BYTE release;							// Release point, in frames
	BYTE speed;								// Speed, in frames
};

// Instrument Macro Flags
struct TFlag
{
	bool isLooped;							// Is it Looping?
	bool isReleased;						// Is it Releasing?
	bool isAbsolute;						// Is it Absolute?
	bool isAdditive;						// Is it Additive?
};

// Instrument Envelope Index, 0-63 inclusive, Bit 7 is toggle for enabled/disabled, Bit 6 is unused for now
struct TMacro
{
	BYTE volume;
	BYTE timbre;
	BYTE audctl;
	BYTE trigger;
	BYTE effect;
	BYTE note;
	BYTE freq;
};

struct TInstrumentEnvelope
{
	TParameter parameter;
	TFlag flag;
	BYTE envelope[ENVELOPE_STEP_COUNT];
};

struct TInstrumentTrigger
{
	TParameter parameter;
	TFlag flag;
	TAutomatic trigger[ENVELOPE_STEP_COUNT];
};

struct TInstrumentEffect
{
	TParameter parameter;
	TFlag flag;
	TEffect effect[ENVELOPE_STEP_COUNT];
};

struct TInstrumentTable
{
	TParameter parameter;
	TFlag flag;
	BYTE table[TABLE_STEP_COUNT];
};

// Instrument Data, due to the Legacy TInstrument struct still in the codebase, this is temporarily defined as TInstrumentV2
struct TInstrumentV2
{
	char name[INSTRUMENT_NAME_MAX + 1];		// Instrument Name
	BYTE volumeFade;						// Volume Fade, take priority over Pattern Effect Axx
	BYTE volumeSustain;						// Volume Sustain, Take priority over Pattern Effect Axx
	BYTE vibrato;							// Vibrato trigger, take priority over Pattern Effect 4xx
	BYTE freqShift;							// Freq Shift trigger, take priority over Pattern Effect 1xx and 2xx
	BYTE delay;								// Vibrato and Freq Shift delay, set to 0x01 for no delay, 0x00 to disable
	TMacro index;							// Instrument Macro Envelope(s) Index and Parameters
};


// ----------------------------------------------------------------------------
// RMTE Module Class
//

class CModule
{
public:
	CModule();
	~CModule();
	
	//-- Module Initialisation Functions --//

	void InitialiseModule();
	void ClearModule();

	void CreateSubtune(int subtune);
	void DeleteSubtune(int subtune);
	void InitialiseSubtune(TSubtune* pSubtune);

	void CreateInstrument(int instrument);
	void DeleteInstrument(int instrument);
	void InitialiseInstrument(TInstrumentV2* pInstrument);

	void CreateVolumeEnvelope(int envelope);
	void DeleteVolumeEnvelope(int envelope);
	void CreateTimbreEnvelope(int envelope);
	void DeleteTimbreEnvelope(int envelope);
	void CreateAudctlEnvelope(int envelope);
	void DeleteAudctlEnvelope(int envelope);
	void InitialiseInstrumentEnvelope(TInstrumentEnvelope* pEnvelope);

	void CreateTriggerEnvelope(int trigger);
	void DeleteTriggerEnvelope(int trigger);
	void InitialiseInstrumentTrigger(TInstrumentTrigger* pTrigger);

	void CreateEffectEnvelope(int effect);
	void DeleteEffectEnvelope(int effect);
	void InitialiseInstrumentEffect(TInstrumentEffect* pEffect);

	void CreateNoteTable(int table);
	void DeleteNoteTable(int table);
	void CreateFreqTable(int table);
	void DeleteFreqTable(int table);
	void InitialiseInstrumentTable(TInstrumentTable* pTable);

	//-- Legacy RMT Module Import Functions --//

	bool ImportLegacyRMT(std::ifstream& in);
	bool DecodeLegacyRMT(std::ifstream& in, TSubtune* pSubtune, CString& log);
	bool ImportLegacyPatterns(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress);
	bool ImportLegacySonglines(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress);
	bool ImportLegacyInstruments(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded);

	//-- Booleans for Module Index and Data integrity --//

	bool IsValidSubtune(int subtune) { return subtune >= 0 && subtune < SUBTUNE_COUNT; };
	bool IsValidChannel(int channel) { return channel >= 0 && channel < CHANNEL_COUNT; };
	bool IsValidSongline(int songline) { return songline >= 0 && songline < SONGLINE_COUNT; };
	bool IsValidPattern(int pattern) { return pattern >= 0 && pattern < PATTERN_COUNT; };
	bool IsValidRow(int row) { return row >= 0 && row < ROW_COUNT; };
	bool IsValidNote(int note) { return note >= 0 && note < NOTE_COUNT; };
	bool IsValidInstrument(int instrument) { return instrument >= 0 && instrument < INSTRUMENT_COUNT; };
	bool IsValidVolume(int volume) { return volume >= 0 && volume < VOLUME_COUNT; };
	bool IsValidCommand(int command) { return command >= 0 && command < PATTERN_EFFECT_COUNT; };
	bool IsValidCommandColumn(int column) { return column >= 0 && column < ACTIVE_EFFECT_COUNT; };

	//-- Pointers to Module Data --//

	TSubtune* GetSubtune(BYTE subtune) { return IsValidSubtune(subtune) ? m_subtuneIndex[subtune] : NULL; };
	TChannel* GetChannel(BYTE subtune, BYTE channel);
	TPattern* GetPattern(BYTE subtune, BYTE channel, BYTE pattern);
	TPattern* GetIndexedPattern(BYTE subtune, BYTE channel, BYTE songline);
	TRow* GetRow(BYTE subtune, BYTE channel, BYTE pattern, BYTE row);
	
	//-- Getters and Setters for Pattern and Songline Data --//

	const BYTE GetPatternInSongline(BYTE subtune, BYTE channel, BYTE songline);
	const BYTE GetPatternRowNote(BYTE subtune, BYTE channel, BYTE pattern, BYTE row);
	const BYTE GetPatternRowInstrument(BYTE subtune, BYTE channel, BYTE pattern, BYTE row);
	const BYTE GetPatternRowVolume(BYTE subtune, BYTE channel, BYTE pattern, BYTE row);
	const BYTE GetPatternRowEffectCommand(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column);
	const BYTE GetPatternRowEffectParameter(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column);

	void SetPatternInSongline(BYTE subtune, BYTE channel, BYTE songline, BYTE pattern);
	void SetPatternRowNote(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE note);
	void SetPatternRowInstrument(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE instrument);
	void SetPatternRowVolume(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE volume);
	void SetPatternRowCommand(BYTE subtune, BYTE channel, BYTE pattern, BYTE row, BYTE column, BYTE effectCommand, BYTE effectParameter);

	//-- Getters and Setters for Module Parameters --//

	const char* GetSongName() { return m_songName; };
	const char* GetSongAuthor() { return m_songAuthor; };
	const char* GetSongCopyright() { return m_songCopyright; };

	const char* GetSubtuneName(BYTE subtune);
	const BYTE GetSongLength(BYTE subtune);
	const BYTE GetPatternLength(BYTE subtune);
	const BYTE GetChannelCount(BYTE subtune);
	const BYTE GetSongSpeed(BYTE subtune);
	const BYTE GetInstrumentSpeed(BYTE subtune);
	const BYTE GetEffectCommandCount(BYTE subtune, BYTE channel);

	void SetSongName(const char* name) { strncpy_s(m_songName, name, MODULE_SONG_NAME_MAX); };
	void SetSongAuthor(const char* author) { strncpy_s(m_songAuthor, author, MODULE_AUTHOR_NAME_MAX); };
	void SetSongCopyright(const char* copyright) { strncpy_s(m_songCopyright, copyright, MODULE_COPYRIGHT_INFO_MAX); };

	void SetSubtuneName(BYTE subtune, const char* name);
	void SetSongLength(BYTE subtune, BYTE length);
	void SetPatternLength(BYTE subtune, BYTE length);
	void SetChannelCount(BYTE subtune, BYTE count);
	void SetSongSpeed(BYTE subtune, BYTE speed);
	void SetInstrumentSpeed(BYTE subtune, BYTE speed);
	void SetEffectCommandCount(BYTE subtune, BYTE channel, BYTE column);

	//-- RMTE Editor Functions --//

	const BYTE GetSubtuneCount();
	BYTE GetShortestPatternLength(int subtune, int songline);
	BYTE GetShortestPatternLength(TSubtune* pSubtune, int songline);
	bool DuplicatePatternInSongline(int subtune, int channel, int songline, int pattern);
	bool IsUnusedPattern(int subtune, int channel, int pattern);
	bool IsUnusedPattern(TChannel* pChannel, int pattern, int songlength);
	bool IsEmptyPattern(int subtune, int channel, int pattern);
	bool IsEmptyPattern(TPattern* pPattern);
	bool IsEmptyRow(int subtune, int channel, int pattern, int row);
	bool IsEmptyRow(TRow* pRow);
	bool IsIdenticalPattern(TPattern* pFromPattern, TPattern* pToPattern);
	bool CopyPattern(TPattern* pFromPattern, TPattern* pToPattern);
	bool ClearPattern(int subtune, int channel, int pattern);
	bool ClearPattern(TPattern* pPattern);
	bool CopyChannel(TChannel* pFromChannel, TChannel* pToChannel);
	bool CopySubtune(TSubtune* pFromSubtune, TSubtune* pToSubtune);
	bool DuplicateChannelIndex(int subtune, int sourceIndex, int destinationIndex);
	void MergeDuplicatedPatterns(int subtune);
	void MergeDuplicatedPatterns(TSubtune* pSubtune);
	void RenumberIndexedPatterns(int subtune);
	void RenumberIndexedPatterns(TSubtune* pSubtune);
	void ClearUnusedPatterns(int subtune);
	void ClearUnusedPatterns(TSubtune* pSubtune);
	void ConcatenateIndexedPatterns(int subtune);
	void ConcatenateIndexedPatterns(TSubtune* pSubtune);
	void AllSizeOptimisations(int subtune);
	void AllSizeOptimisations(TSubtune* pSubtune);

	//-- Getters and Setters for Instrument Data --//

	TInstrumentV2* GetInstrument(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument] : NULL; };
	const char* GetInstrumentName(int instrument);
	void SetInstrumentName(int instrument, const char* name);

	TInstrumentEnvelope* GetVolumeEnvelope(BYTE envelope) { return IsValidInstrument(envelope) ? m_volumeIndex[envelope] : NULL; };
	TInstrumentEnvelope* GetTimbreEnvelope(BYTE envelope) { return IsValidInstrument(envelope) ? m_timbreIndex[envelope] : NULL; };
	TInstrumentEnvelope* GetAudctlEnvelope(BYTE envelope) { return IsValidInstrument(envelope) ? m_audctlIndex[envelope] : NULL; };
	TInstrumentTrigger* GetTriggerEnvelope(BYTE trigger) { return IsValidInstrument(trigger) ? m_triggerIndex[trigger] : NULL; };
	TInstrumentEffect* GetEffectEnvelope(BYTE effect) { return IsValidInstrument(effect) ? m_effectIndex[effect] : NULL; };
	TInstrumentTable* GetNoteTable(BYTE table) { return IsValidInstrument(table) ? m_noteIndex[table] : NULL; };
	TInstrumentTable* GetFreqTable(BYTE table) { return IsValidInstrument(table) ? m_freqIndex[table] : NULL; };

private:
	char m_songName[MODULE_SONG_NAME_MAX + 1];
	char m_songAuthor[MODULE_AUTHOR_NAME_MAX + 1];
	char m_songCopyright[MODULE_COPYRIGHT_INFO_MAX + 1];
	TSubtune* m_subtuneIndex[SUBTUNE_COUNT];
	TInstrumentV2* m_instrumentIndex[INSTRUMENT_COUNT];
	TInstrumentEnvelope* m_volumeIndex[INSTRUMENT_COUNT];
	TInstrumentEnvelope* m_timbreIndex[INSTRUMENT_COUNT];
	TInstrumentEnvelope* m_audctlIndex[INSTRUMENT_COUNT];
	TInstrumentTrigger* m_triggerIndex[INSTRUMENT_COUNT];
	TInstrumentEffect* m_effectIndex[INSTRUMENT_COUNT];
	TInstrumentTable* m_noteIndex[INSTRUMENT_COUNT];
	TInstrumentTable* m_freqIndex[INSTRUMENT_COUNT];
};
