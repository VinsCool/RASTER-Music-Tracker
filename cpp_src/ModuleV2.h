// RMT Module V2 Format Prototype
// By VinsCool, 2023
//
// TODO: Move the Legacy Import code to IO_Song.cpp or similar, in order to get most of the CModule functions cleared from unrelated stuff
// TODO: Move most of the Editor Functions to CSong or similar for the same reason

#pragma once

#include "General.h"
#include "global.h"

// ----------------------------------------------------------------------------
// Data boundaries constant
//

#define INVALID							-1									// Failsafe value for invalid data


// ----------------------------------------------------------------------------
// Module Header definition
//

#define MODULE_VERSION					0									// Module Version number, the highest value is always assumed to be the most recent
#define MODULE_IDENTIFIER				"RMTE"								// Raster Music Tracker Extended, "DUMB" is another potential identifier
#define MODULE_REGION					g_ntsc								// 0 for PAL, 1 for NTSC, anything else is also assumed to be NTSC
#define MODULE_STEREO					g_tracks4_8							// 4 for Mono, 8 for Stereo
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
#define MODULE_SUBTUNE_COUNT			1									// Default Subtune Count
#define MODULE_TRACK_LENGTH				64									// Default Track Length
#define MODULE_SONG_LENGTH				1									// Default Song Length
#define MODULE_SONG_SPEED				6									// Default Song Speed
#define MODULE_VBI_SPEED				1									// Default VBI Speed
#define MODULE_SONG_NAME_MAX			64									// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64									// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64									// Maximum length of Copyright info
#define MODULE_PATTERN_EMPTY			0xFF								// Empty Pattern in Module if encountered
#define MODULE_ROW_EMPTY				0xFE								// Empty Row in Module if encountered


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

#define PATTERN_NOTE_COUNT				120									// 0-119 inclusive, Note index used in Pattern, for a total of 10 octaves
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
#define PATTERN_EFFECT_EMPTY			PATTERN_EFFECT_COUNT				// There is no Effect Command in the Pattern Row
#define PATTERN_EFFECT_MAX				PATTERN_EFFECT_COUNT + 1			// Total for Effect Command index integrity


// ----------------------------------------------------------------------------
// Instrument definition
//

#define INSTRUMENT_NAME_MAX				64									// Maximum length of Instrument name
#define INSTRUMENT_TABLE_MAX			256									// Instrument Note/Freq Table 0-255, inclusive
#define INSTRUMENT_ENVELOPE_MAX			256									// Instrument Envelope 0-255, inclusive
#define INSTRUMENT_EFFECT_MAX			16									// Instrument Envelope Effect Command 0-15, inclusive


// ----------------------------------------------------------------------------
// Effect Command definition
//

#define EFFECT_PARAMETER_MAX			0xFF								// 0-255 inclusive, Effect $XY Parameter used in Pattern
#define EFFECT_PARAMETER_MIN			0x00								// The $XY Parameter of 0 may be used to disable certain Effect Commands
#define EFFECT_PARAMETER_DEFAULT		0x80								// The $XY Parameter of 128 may be used to disable certain Effect Commands

#define EFFECT_COMMAND_BXX				0x0B								// Effect Command Bxx -> Goto Songline $xx
#define EFFECT_COMMAND_DXX				0x0D								// Effect Command Dxx -> End Pattern, no parameter needed(?)
#define EFFECT_COMMAND_FXX				0x0F								// Effect Command Fxx -> Set Song Speed $xx

#define CMD1							0									// Effect Command identifier for Effect Column 1
#define CMD2							1									// Effect Command identifier for Effect Column 2
#define CMD3							2									// Effect Command identifier for Effect Column 3
#define CMD4							3									// Effect Command identifier for Effect Column 4


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
	BYTE note;										// Note Index, as well as Pattern Commands such as Stop, Release, Retrigger, etc
	BYTE instrument;								// Instrument Index
	BYTE volume;									// Volume Index
	TEffect effect[PATTERN_ACTIVE_EFFECT_MAX];		// Effect Command, toggled from the Active Effect Columns in Track Channels
};

// Pattern Data, indexed by the TRow Struct
struct TPattern
{
	TRow row[TRACK_ROW_MAX];						// Row data is contained withn its associated Pattern index
};

// Channel Index, used for indexing the Songline and Pattern data, similar to the CSong Class
struct TChannel
{
	BYTE effectCount;								// Number of Effect Commands enabled per Track Channel
	BYTE songline[SONGLINE_MAX];					// Pattern Index for each songline within the Track Channel
	TPattern pattern[TRACK_PATTERN_MAX];			// Pattern Data for the Track Channel
};

// Subtune Index, used for indexing all of the Module data, indexed by the TIndex Struct
struct TSubtune
{
	char name[SUBTUNE_NAME_MAX + 1];				// Subtune Name
	BYTE songLength;								// Song Length, in Songlines
	BYTE patternLength;								// Pattern Length, in Rows
	BYTE channelCount;								// Number of Channels used in Subtune
	BYTE songSpeed;									// Song Speed, in Frames per Row
	BYTE instrumentSpeed;							// Instrument Speed, in Frames per VBI
	TChannel channel[TRACK_CHANNEL_MAX];			// Channel Index assigned to the Subtune
};


// ----------------------------------------------------------------------------
// RMTE Module Structs for Instrument, Envelope, Table, etc
//

// Instrument AUDCTL/SKCTL Automatic Trigger bits, useful considering each POKEY channel featuring unique properties
struct TAutoMode
{
	bool autoFilter;								// High Pass Filter, triggered from Channel 1 and/or 2, hijacking Channel 3 and/or 4
	bool auto16Bit;									// 16-bit mode, triggered from Channel 2 and/or 4, hijacking Channel 1 and/or 3
	bool autoReverse16;								// Reverse 16-bit mode, triggered from Channel 1 and/or 3, hijacking Channel 2 and/or 4
	bool auto179Mhz;								// 1.79Mhz mode, triggered from Channel 1 and/or 3
	bool auto15Khz;									// 15Khz mode, triggered from any Channel, hijacking all Channels not affected by 1.79Mhz mode (16-bit included)
	bool autoPoly9;									// Poly9 Noise mode, triggered from any Channel, hijacking all Channels using Distortion 0 and 8
	bool autoTwoTone;								// Automatic Two-Tone Filter, triggered from Channel 1, hijacking Channel 2
	bool autoToggle;								// Automatic Toggle, for each ones of the possible combination, effectively overriding the AUDCTL Envelope(?)
};

// Instrument Envelope, used for most of the Instrument functionalities such as Volume, Timbre, AUDCTL, etc
struct TEnvelope
{
	BYTE volume;									// Envelope Volume
	BYTE timbre;									// Envelope Timbre (Distortion)
	BYTE audctl;									// Envelope AUDCTL (Absolute)
	TAutoMode trigger;								// Envelope AUDCTL/SKCTL (Automatic)
	TEffect effect;									// Extended RMT Instrument Effect Commands (for Legacy RMT Instrument compatibility)
};

// Instrument Table, used for most of the Instrument functionalities relative to Note and Freq offsets
struct TTable
{
	BYTE note;										// Table Note
	BYTE freq;										// Table Freq
};

// Instrument Envelope Macro
struct TEnvelopeMacro
{
	BYTE mode;										// Envelope Mode(?)
	BYTE length;									// Envelope Length, in frames
	BYTE loop;										// Envelope Loop point, in frames
	BYTE release;									// Envelope Release point, in frames
	BYTE speed;										// Envelope Speed, in frames
	TEnvelope envelope[INSTRUMENT_ENVELOPE_MAX];	// Envelope Data
};

// Instrument Table Macro
struct TTableMacro
{
	BYTE mode;										// Table Mode, Absolute or Relative, Additive or Set, any combination thereof
	BYTE length;									// Table Length, in frames
	BYTE loop;										// Table Loop point, in frames
	BYTE release;									// Table Release point, in frames
	BYTE speed;										// Table Speed, in frames
	TTable table[INSTRUMENT_TABLE_MAX];				// Table Data
};

// Instrument Data, due to the Legacy TInstrument struct still in the codebase, this is temporarily defined as TInstrumentV2
struct TInstrumentV2
{
	char name[INSTRUMENT_NAME_MAX + 1];				// Instrument Name
	BYTE volumeFade;								// Volume Fade, take priority over Pattern Effect Axx
	BYTE volumeSustain;								// Volume Sustain, Take priority over Pattern Effect Axx
	BYTE vibrato;									// Vibrato trigger, take priority over Pattern Effect 4xx
	BYTE freqShift;									// Freq Shift trigger, take priority over Pattern Effect 1xx and 2xx
	BYTE delay;										// Vibrato and Freq Shift delay, set to 0x01 for no delay, 0x00 to disable
	TEnvelopeMacro envelopeMacro;					// Instrument Envelope Macro
	TTableMacro tableMacro;							// Instrument Table Macro
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
	void InitialiseInstrument(TInstrumentV2* p);

	//-- Legacy RMT Module Import Functions --//

	bool ImportLegacyRMT(std::ifstream& in);
	bool DecodeLegacyRMT(std::ifstream& in, TSubtune* pSubtune, CString& log);
	bool ImportLegacyPatterns(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress);
	bool ImportLegacySonglines(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress);
	bool ImportLegacyInstruments(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded);

	//-- Booleans for Module Index and Data integrity --//

	bool IsValidSubtune(int subtune) { return subtune >= 0 && subtune < SUBTUNE_MAX; };
	bool IsValidChannel(int channel) { return channel >= 0 && channel < TRACK_CHANNEL_MAX; };
	bool IsValidSongline(int songline) { return songline >= 0 && songline < SONGLINE_MAX; };
	bool IsValidPattern(int pattern) { return pattern >= 0 && pattern < TRACK_PATTERN_MAX; };
	bool IsValidRow(int row) { return row >= 0 && row < TRACK_ROW_MAX; };
	bool IsValidNote(int note) { return note >= 0 && note < PATTERN_NOTE_MAX; };
	bool IsValidInstrument(int instrument) { return instrument >= 0 && instrument < PATTERN_INSTRUMENT_MAX; };
	bool IsValidVolume(int volume) { return volume >= 0 && volume < PATTERN_VOLUME_MAX; };
	bool IsValidCommand(int command) { return command >= 0 && command < PATTERN_EFFECT_MAX; };
	bool IsValidCommandColumn(int column) { return column >= 0 && column < PATTERN_ACTIVE_EFFECT_MAX; };

	//-- Pointers to Module Data --//

	TSubtune* GetSubtune(BYTE subtune) { return IsValidSubtune(subtune) ? m_subtune[subtune] : NULL; };
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

	TInstrumentV2* GetInstrument(int instrument) { return IsValidInstrument(instrument) ? m_instrument[instrument] : NULL; };
	const char* GetInstrumentName(int instrument);
	void SetInstrumentName(int instrument, const char* name);

/*
	const char* GetInstrumentName(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->name : "INVALID INSTRUMENT"; };
	const BYTE GetInstrumentEnvelopeLength(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->envelopeLength : INVALID; };
	const BYTE GetInstrumentEnvelopeLoop(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->envelopeLoop : INVALID; };
	const BYTE GetInstrumentEnvelopeRelease(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->envelopeRelease : INVALID; };
	const BYTE GetInstrumentEnvelopeSpeed(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->envelopeSpeed : INVALID; };
	const BYTE GetInstrumentTableLength(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->tableLength : INVALID; };
	const BYTE GetInstrumentTableLoop(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->tableLoop : INVALID; };
	const BYTE GetInstrumentTableRelease(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->tableRelease : INVALID; };
	const BYTE GetInstrumentTableMode(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->tableMode : INVALID; };
	const BYTE GetInstrumentTableSpeed(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->tableSpeed : INVALID; };
	const BYTE* GetInstrumentVolumeEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->volumeEnvelope : NULL; };
	const BYTE* GetInstrumentDistortionEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->distortionEnvelope : NULL; };
	const BYTE* GetInstrumentAudctlEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->audctlEnvelope : NULL; };
	const BYTE* GetInstrumentCommandEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->commandEnvelope : NULL; };
	const WORD* GetInstrumentParameterEnvelope(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->parameterEnvelope : NULL; };
	const BYTE* GetInstrumentNoteTable(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->noteTable : NULL; };
	const BYTE* GetInstrumentFreqTable(int instrument) { return IsValidInstrument(instrument) ? m_instrumentIndex[instrument]->freqTable : NULL; };
*/

private:
	char m_songName[MODULE_SONG_NAME_MAX + 1];
	char m_songAuthor[MODULE_AUTHOR_NAME_MAX + 1];
	char m_songCopyright[MODULE_COPYRIGHT_INFO_MAX + 1];
	TSubtune* m_subtune[SUBTUNE_MAX];
	TInstrumentV2* m_instrument[PATTERN_INSTRUMENT_COUNT];
};
