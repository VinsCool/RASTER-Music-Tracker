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
#define MODULE_BASE_TUNING				g_baseTuning						// Default A-4 Tuning
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
#define MODULE_DEFAULT_CHANNEL_COUNT	4									// Default Channel Count
#define MODULE_SONG_NAME_MAX			64									// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64									// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64									// Maximum length of Copyright info
#define MODULE_PATTERN_EMPTY			0xFF								// Empty Pattern in Module if encountered
#define MODULE_ROW_EMPTY				0xFE								// Empty Row in Module if encountered


// ----------------------------------------------------------------------------
// Song and Track Pattern definition
//

#define SUBTUNE_NAME_MAX			64											// 0-63 inclusive, Maximum length of Subtune name
#define SUBTUNE_COUNT				64											// 0-63 inclusive, Maximum number of Subtunes in a Module file
#define SONGLINE_COUNT				256											// 0-255 inclusive, Songline index used in Song
#define PATTERN_COUNT				256											// 0-255 inclusive, Pattern index used in Song
#define ROW_COUNT					256											// 0-255 inclusive, Row index used in Pattern
#define SONG_SPEED_MAX				256											// 0-255 inclusive, Song Speed used during playback
#define INSTRUMENT_SPEED_MAX		16											// 0-15 inclusive, Instrument Speed used during playback
#define POKEY_CHANNEL_COUNT			4											// 0-3 inclusive, each POKEY soundchip could use up to 4 Channels at once
#define POKEY_CHIP_COUNT			4											// 0-3 inclusive, Quad POKEY configuration (theoretical)
#define CHANNEL_COUNT				POKEY_CHIP_COUNT * POKEY_CHANNEL_COUNT		// Total number of Channels, Pattern column index used for Note, Instrument, Volume, and Effect Commands
#define CH1							0											// POKEY Channel identifier for Pattern Column 1
#define CH2							1											// POKEY Channel identifier for Pattern Column 2
#define CH3							2											// POKEY Channel identifier for Pattern Column 3
#define CH4							3											// POKEY Channel identifier for Pattern Column 4
#define _CH1(x)						((x % POKEY_CHANNEL_COUNT) == CH1)			// POKEY Channel identifier for Pattern Column 1
#define _CH2(x)						((x % POKEY_CHANNEL_COUNT) == CH2)			// POKEY Channel identifier for Pattern Column 2
#define _CH3(x)						((x % POKEY_CHANNEL_COUNT) == CH3)			// POKEY Channel identifier for Pattern Column 3
#define _CH4(x)						((x % POKEY_CHANNEL_COUNT) == CH4)			// POKEY Channel identifier for Pattern Column 4
#define NOTE_COUNT					120											// 0-119 inclusive, Note index used in Pattern, for a total of 10 octaves
#define NOTE_EMPTY					NOTE_COUNT									// There is no Note in the Pattern Row
#define NOTE_OFF					NOTE_COUNT + 1								// The Note Command OFF will stop the last played note in the Track Channel
#define NOTE_RELEASE				NOTE_COUNT + 2								// The Note Command === will release the last played note in the Track Channel
#define NOTE_RETRIGGER				NOTE_COUNT + 3								// The Note Command ~~~ will retrigger the last played note in the Track Channel
#define NOTE_INDEX_MAX				NOTE_COUNT + 4								// All the valid Note Commands that could be used in the Pattern Editor
#define INSTRUMENT_COUNT			64											// 0-63 inclusive, Instrument index used in Pattern
#define INSTRUMENT_EMPTY			INSTRUMENT_COUNT							// There is no Instrument in the Pattern Row
#define INSTRUMENT_INDEX_MAX		INSTRUMENT_COUNT + 1						// All the valid Instrument Commands that could be used in the Pattern Editor
#define VOLUME_COUNT				16											// 0-15 inclusive, Volume index used in Pattern
#define VOLUME_EMPTY				VOLUME_COUNT								// There is no Volume in the Pattern Row
#define VOLUME_INDEX_MAX			VOLUME_COUNT + 1							// All the valid Volume Commands that could be used in the Pattern Editor
#define PATTERN_EFFECT_COUNT		16											// 0-15 inclusive, Effect index used in Pattern
#define EFFECT_EMPTY				PATTERN_EFFECT_COUNT						// There is no Effect Command in the Pattern Row
#define PATTERN_EFFECT_INDEX_MAX	PATTERN_EFFECT_COUNT + 1					// All the valid Effect Commands that could be used in the Pattern Editor
#define ACTIVE_EFFECT_COUNT			4											// 0-3 inclusive, Number of Active Effect columns in Track Channel
#define INVALID						-1											// Failsafe value for invalid data
#define EMPTY						0											// Failsafe value for invalid data


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
#define EFFECT_PARAMETER_COUNT		0x100	// Maximum range for the Effect Parameter in the Pattern Editor
#define EFFECT_VIBRATO				0x04	// Effect Command 4xy -> Set Vibrato Depth $x and Vibrato Speed $y
#define EFFECT_PORTAMENTO			0x03	// Effect Command 3xy -> Set Portamento Depth $x and Portamento Speed $y
#define EFFECT_GOTO_SONGLINE		0x0B	// Effect Command Bxx -> Goto Songline $xx
#define EFFECT_END_PATTERN			0x0D	// Effect Command Dxx -> End Pattern, Goto Row $xx
#define EFFECT_SET_SONG_SPEED		0x0F	// Effect Command Fxx -> Set Song Speed $xx
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
	bool isMuted : 1;
	bool isEffectEnabled : 1;
	BYTE effectCount : 2;					// Number of Effect Commands enabled per Track Channel
	BYTE channelVolume : 4;
	BYTE songline[SONGLINE_COUNT];			// Pattern Index for each songline within the Track Channel
	TPattern pattern[PATTERN_COUNT];		// Pattern Data for the Track Channel
};

// Subtune Index, used for indexing all of the Module data, indexed by the TIndex Struct
struct TSubtune
{
	char name[SUBTUNE_NAME_MAX + 1];		// Subtune Name
	BYTE songLength;						// Song Length, in Songlines
	BYTE patternLength;						// Pattern Length, in Rows
	BYTE songSpeed;							// Song Speed, in Frames per Row
	BYTE instrumentSpeed : 4;				// Instrument Speed, in Frames per VBI
	BYTE channelCount : 4;					// Number of Channels used in Subtune
	TChannel channel[CHANNEL_COUNT];		// Channel Index assigned to the Subtune
};

// Redundant...?
struct TSubtuneIndex
{
	TSubtune* subtune[SUBTUNE_COUNT];
};

// ----------------------------------------------------------------------------
// RMTE Module Structs for Instrument, Envelope, Table, etc
//

/*
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
	bool autoPortamento;					// Automatic Portamento, triggered in any Channel, initialised using the CMD5 when encountered
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
*/

/*
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
*/

/*
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
*/

/*
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
*/

// An attempt to organise all the Instrument data into 1 place...
struct TMacro
{
	BYTE index : 6;
	bool isEnabled : 1;
	bool isReversed : 1;
};

struct TEnvelopeMacro
{
	TMacro volume;
	TMacro timbre;
	TMacro audctl;
	//TMacro trigger;
	TMacro effect;
	TMacro note;
	TMacro freq;
};

struct TEnvelopeParameter
{
	BYTE length;							// Length, in frames
	BYTE loop;								// Loop point, in frames
	BYTE release;							// Release point, in frames
	BYTE speed : 4;							// Speed, in frames
	bool isLooped : 1;						// Is it Looping?
	bool isReleased : 1;					// Is it Releasing?
	bool isAbsolute : 1;					// Is it Absolute?
	bool isAdditive : 1;					// Is it Additive?
};

struct TInstrumentV2
{
	char name[INSTRUMENT_NAME_MAX + 1];		// Instrument Name
	BYTE volumeFade;						// Volume Fade, take priority over Pattern Effect Axx
	BYTE volumeSustain;						// Volume Sustain, Take priority over Pattern Effect Axx
	BYTE vibrato;							// Vibrato trigger, take priority over Pattern Effect 4xx
	BYTE freqShift;							// Freq Shift trigger, take priority over Pattern Effect 1xx and 2xx
	BYTE delay;								// Vibrato and Freq Shift delay, set to 0x01 for no delay, 0x00 to disable
	TEnvelopeMacro envelope;				// Instrument Macro Envelope(s) Index and Parameters
};

struct TVolume
{
	BYTE volumeLevel : 4;
	BYTE waveTable : 4;
	BYTE stereoPanning : 7;
	bool isVolumeOnly : 1;
};

struct TTimbre
{
	union
	{
		BYTE timbre;
		struct
		{
			BYTE waveForm : 4;				// eg: Buzzy, Gritty, etc
			BYTE distortion : 3;			// eg: Pure (0xA0), Poly4 (0xC0), etc
			bool isOptimalTuning : 1;		// Use a combination of all possible Waveforms, and output the most in-tune pitch for a given Distortion
		};
	};
};

struct TAudctl
{
	union
	{
		BYTE audctl;
		struct
		{
			bool is15KhzMode : 1;
			bool isHighPassCh24 : 1;
			bool isHighPassCh13 : 1;
			bool isJoinedCh34 : 1;
			bool isJoinedCh12 : 1;
			bool is179MhzCh3 : 1;
			bool is179MhzCh1 : 1;
			bool isPoly9Noise : 1;
		};
	};
};

/*
struct TTrigger
{
	bool autoFilter : 1;					// High Pass Filter, triggered from Channel 1 and/or 2, hijacking Channel 3 and/or 4
	bool auto16Bit : 1;						// 16-bit mode, triggered from Channel 2 and/or 4, hijacking Channel 1 and/or 3
	bool autoReverse16 : 1;					// Reverse 16-bit mode, triggered from Channel 1 and/or 3, hijacking Channel 2 and/or 4
	bool auto179Mhz : 1;					// 1.79Mhz mode, triggered from Channel 1 and/or 3
	bool auto15Khz : 1;						// 15Khz mode, triggered from any Channel, hijacking all Channels not affected by 1.79Mhz mode (16-bit included)
	bool autoPoly9 : 1;						// Poly9 Noise mode, triggered from any Channel, hijacking all Channels using Distortion 0 and 8
	bool autoTwoTone : 1;					// Automatic Two-Tone Filter, triggered from Channel 1, hijacking Channel 2
	bool autoPortamento : 1;				// Automatic Portamento, triggered in any Channel, initialised using the CMD5 when encountered
	bool autoVibrato : 1;
};
*/

struct TEffectEnvelope
{
	union
	{
		// Legacy RMT (1.28 and 1.34) have the following Effect Commands:
		// 
		// CMD0 -> Set Note (Relative)
		// CMD1 -> Set Freq (Absolute, 8-bit Parameter only)
		// CMD2 -> Finetune (Relative)
		// CMD3 -> Set Note (Additive)
		// CMD4 -> Set FreqShift (Relative)
		// CMD5 -> Set Portamento Parameter (Active with Portamento Bit)
		// CMD6 -> Set Autofilter Offset (1.28), Set Auto16bit Distortion and Set Sawtooth Direction (1.34 only)
		// CMD7 -> Set Volume Only (1.28), Set Basenote (also 1.28... never used???), Set AUDCTL and Toggle Two-Tone Filter (1.34 only)
		//
		// TODO: Come up with something that could do much of the same things with fewer bits
		// The new Envelope format added multiple things that made few Effect Commands redundant
		//
		// Idea 1: Keep 4 Commands using 2 bits, use the remaining 6 bits to toggle one of the most useful Automatic commands, discard the least likely to be used/imported
		// Idea 2: Keep all 8 Commands using 3 bits, use them exactly like how the Legacy/Patched RMT driver would call for them, for maximal backwards compatibility
		// Idea 3: Same as Idea 1, but attempt to convert most the CMD0/CMD1/CMD2/CMD3 parameters into NoteTable/FreqTable parameters, merging into existing Tables if possible
		// Idea 4: Attempt to convert Instrument Commands into Pattern Commands, which would be really hard to do, but technically could work to make everything compatible
		struct
		{
			bool autoFilter : 1;					// High Pass Filter, triggered from Channel 1 and/or 2, hijacking Channel 3 and/or 4
			bool auto16Bit : 1;						// 16-bit mode, triggered from Channel 2 and/or 4, hijacking Channel 1 and/or 3
			bool autoReverse16 : 1;					// Reverse 16-bit mode, triggered from Channel 1 and/or 3, hijacking Channel 2 and/or 4
			bool auto179Mhz : 1;					// 1.79Mhz mode, triggered from Channel 1 and/or 3
			bool auto15Khz : 1;						// 15Khz mode, triggered from any Channel, hijacking all Channels not affected by 1.79Mhz mode (16-bit included)
			bool autoPoly9 : 1;						// Poly9 Noise mode, triggered from any Channel, hijacking all Channels using Distortion 0 and 8
			bool autoTwoTone : 1;					// Automatic Two-Tone Filter, triggered from Channel 1, hijacking Channel 2
			bool autoPortamento : 1;				// Automatic Portamento, triggered in any Channel, initialised using the CMD5 when encountered
		};
	};
};

struct TTable
{
	union
	{
		BYTE freq;
		WORD freq16;
		struct
		{
			//BYTE note : 7;
			//bool isNegative : 1;
			BYTE note : 8;
			bool isAbsolute : 1;
			bool isScheme : 1;
		};
	};
};

struct TEnvelope
{
	TEnvelopeParameter parameter;
	union
	{
		TVolume volume[ENVELOPE_STEP_COUNT];
		TTimbre timbre[ENVELOPE_STEP_COUNT];
		TAudctl audctl[ENVELOPE_STEP_COUNT];
		//TTrigger trigger[ENVELOPE_STEP_COUNT];
		//TEffect effect[ENVELOPE_STEP_COUNT];
		TEffectEnvelope effect[ENVELOPE_STEP_COUNT];
		TTable note[ENVELOPE_STEP_COUNT];
		TTable freq[ENVELOPE_STEP_COUNT];
	};
};

// Also sort of redundant...? This is justified for the Envelopes alone, at least...
struct TInstrumentIndex
{
	TInstrumentV2* instrument[INSTRUMENT_COUNT];
	TEnvelope* volume[INSTRUMENT_COUNT];
	TEnvelope* timbre[INSTRUMENT_COUNT];
	TEnvelope* audctl[INSTRUMENT_COUNT];
	//TEnvelope* trigger[INSTRUMENT_COUNT];
	TEnvelope* effect[INSTRUMENT_COUNT];
	TEnvelope* note[INSTRUMENT_COUNT];
	TEnvelope* freq[INSTRUMENT_COUNT];
};

// ----------------------------------------------------------------------------
// RMTE Module Header
//

// High Header, used to identify the Module Version and Parameters
typedef struct HiHeader_t
{
	char identifier[4];						// RMTE
	BYTE version;							// 0 = Prototype, 1+ = Release
	BYTE region;							// 0 = PAL, 1 = NTSC
	BYTE highlightPrimary;
	BYTE highlightSecondary;
	double baseTuning;						// A-4 Tuning in Hz, eg: 440, 432, etc
	BYTE baseNote;							// Base Note used for Transposition, eg: 0 = A-, 3 = C-, etc
	BYTE baseOctave;						// Base Octave used for Transposition, eg: 4 for no transposition
} THiHeader;

// Low Header, used to index Pointers to Module Data, a NULL pointer means no data exists
typedef struct LoHeader_t
{
	UINT subtuneIndex[SUBTUNE_COUNT];			// Offset to Subtune
	UINT instrumentIndex[INSTRUMENT_COUNT];		// Offset to Instrument
	UINT volumeEnvelope[INSTRUMENT_COUNT];		// Offset to Volume Envelope
	UINT timbreEnvelope[INSTRUMENT_COUNT];		// Offset to Timbre Envelope
	UINT audctlEnvelope[INSTRUMENT_COUNT];		// Offset to AUDCTL Envelope
	//UINT triggerEnvelope[INSTRUMENT_COUNT];		// Offset to Trigger Envelope
	UINT effectEnvelope[INSTRUMENT_COUNT];		// Offset to Effect Envelope
	UINT noteTableEnvelope[INSTRUMENT_COUNT];	// Offset to Note Table Envelope
	UINT freqTableEnvelope[INSTRUMENT_COUNT];	// Offset to Freq Table Envelope
} TLoHeader;

// RMTE Module Header
typedef struct ModuleHeader_t
{
	THiHeader hiHeader;
	TLoHeader loHeader;
	char name[64];
	char author[64];
	char copyright[64];
} TModuleHeader;


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

	void DeleteAllSubtunes();
	void DeleteAllChannels(TSubtune* pSubtune);
	void DeleteAllPatterns(TChannel* pChannel);
	void DeleteAllRows(TPattern* pPattern);

	void DeleteAllInstruments();

	bool CreateSubtune(UINT subtune);
	bool DeleteSubtune(UINT subtune);
	bool InitialiseSubtune(TSubtune* pSubtune);

	bool DeleteChannel(TSubtune* pSubtune, UINT channel);
	bool InitialiseChannel(TChannel* pChannel);

	bool DeletePattern(TChannel* pChannel, UINT pattern);
	bool InitialisePattern(TPattern* pPattern);

	bool DeleteRow(TPattern* pPattern, UINT row);
	bool InitialiseRow(TRow* pRow);

	bool CreateInstrument(UINT instrument);
	bool DeleteInstrument(UINT instrument);
	bool InitialiseInstrument(TInstrumentV2* pInstrument);

	bool CreateVolumeEnvelope(UINT instrument);
	bool DeleteVolumeEnvelope(UINT instrument);
	bool InitialiseVolumeEnvelope(TEnvelope* pEnvelope);

	bool CreateTimbreEnvelope(UINT instrument);
	bool DeleteTimbreEnvelope(UINT instrument);
	bool InitialiseTimbreEnvelope(TEnvelope* pEnvelope);

	bool CreateAudctlEnvelope(UINT instrument);
	bool DeleteAudctlEnvelope(UINT instrument);
	bool InitialiseAudctlEnvelope(TEnvelope* pEnvelope);

	//bool CreateTriggerEnvelope(UINT instrument);
	//bool DeleteTriggerEnvelope(UINT instrument);
	//bool InitialiseTriggerEnvelope(TEnvelope* pEnvelope);

	bool CreateEffectEnvelope(UINT instrument);
	bool DeleteEffectEnvelope(UINT instrument);
	bool InitialiseEffectEnvelope(TEnvelope* pEnvelope);

	bool CreateNoteTableEnvelope(UINT instrument);
	bool DeleteNoteTableEnvelope(UINT instrument);
	bool InitialiseNoteTableEnvelope(TEnvelope* pEnvelope);

	bool CreateFreqTableEnvelope(UINT instrument);
	bool DeleteFreqTableEnvelope(UINT instrument);
	bool InitialiseFreqTableEnvelope(TEnvelope* pEnvelope);

	//-- Legacy RMT Module Import Functions --//

	bool ImportLegacyRMT(std::ifstream& in);
	bool DecodeLegacyRMT(std::ifstream& in, TSubtune* pSubtune, CString& log);
	bool ImportLegacyPatterns(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress);
	bool ImportLegacySonglines(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, WORD endAddress);
	bool ImportLegacyInstruments(TSubtune* pSubtune, BYTE* sourceMemory, WORD sourceAddress, BYTE version, BYTE* isLoaded);

	//-- Booleans for Module Index and Data integrity --//

	const bool IsValidSubtune(UINT subtune) { return subtune < SUBTUNE_COUNT; };
	const bool IsValidChannel(UINT channel) { return channel < CHANNEL_COUNT; };
	const bool IsValidSongline(UINT songline) { return songline < SONGLINE_COUNT; };
	const bool IsValidPattern(UINT pattern) { return pattern < PATTERN_COUNT; };
	const bool IsValidRow(UINT row) { return row < ROW_COUNT; };
	const bool IsValidNote(UINT note) { return note < NOTE_COUNT; };
	const bool IsValidNoteIndex(UINT note) { return note < NOTE_INDEX_MAX; };
	const bool IsValidInstrument(UINT instrument) { return instrument < INSTRUMENT_COUNT; };
	const bool IsValidInstrumentIndex(UINT instrument) { return instrument < INSTRUMENT_INDEX_MAX; };
	const bool IsValidVolume(UINT volume) { return volume < VOLUME_COUNT; };
	const bool IsValidVolumeIndex(UINT volume) { return volume < VOLUME_INDEX_MAX; };
	const bool IsValidEffectCommand(UINT command) { return command < PATTERN_EFFECT_COUNT; };
	const bool IsValidEffectCommandIndex(UINT command) { return command < PATTERN_EFFECT_INDEX_MAX; };
	const bool IsValidEffectParameter(UINT parameter) { return parameter < EFFECT_PARAMETER_COUNT; };
	const bool IsValidCommandColumn(UINT column) { return column < ACTIVE_EFFECT_COUNT; };

	//-- Pointers to Module Data --//

	TSubtune* GetSubtune(UINT subtune);

	TChannel* GetChannel(UINT subtune, UINT channel);
	TChannel* GetChannel(TSubtune* pSubtune, UINT channel);

	TPattern* GetPattern(UINT subtune, UINT channel, UINT pattern);
	TPattern* GetPattern(TSubtune* pSubtune, UINT channel, UINT pattern);
	TPattern* GetPattern(TChannel* pChannel, UINT pattern);

	TPattern* GetIndexedPattern(UINT subtune, UINT channel, UINT songline);
	TPattern* GetIndexedPattern(TSubtune* pSubtune, UINT channel, UINT songline);
	TPattern* GetIndexedPattern(TChannel* pChannel, UINT songline);

	TRow* GetRow(UINT subtune, UINT channel, UINT pattern, UINT row);
	TRow* GetRow(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row);
	TRow* GetRow(TChannel* pChannel, UINT pattern, UINT row);
	TRow* GetRow(TPattern* pPattern, UINT row);
	
	//-- Getters and Setters for Pattern and Songline Data --//

	const UINT GetPatternInSongline(UINT subtune, UINT channel, UINT songline);
	const UINT GetPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline);
	const UINT GetPatternInSongline(TChannel* pChannel, UINT songline);

	const UINT GetPatternRowNote(UINT subtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowNote(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowNote(TChannel* pChannel, UINT pattern, UINT row);
	const UINT GetPatternRowNote(TPattern* pPattern, UINT row);
	const UINT GetPatternRowNote(TRow* pRow);

	const UINT GetPatternRowInstrument(UINT subtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowInstrument(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowInstrument(TChannel* pChannel, UINT pattern, UINT row);
	const UINT GetPatternRowInstrument(TPattern* pPattern, UINT row);
	const UINT GetPatternRowInstrument(TRow* pRow);

	const UINT GetPatternRowVolume(UINT subtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowVolume(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row);
	const UINT GetPatternRowVolume(TChannel* pChannel, UINT pattern, UINT row);
	const UINT GetPatternRowVolume(TPattern* pPattern, UINT row);
	const UINT GetPatternRowVolume(TRow* pRow);

	const UINT GetPatternRowEffectCommand(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectCommand(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectCommand(TChannel* pChannel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectCommand(TPattern* pPattern, UINT row, UINT column);
	const UINT GetPatternRowEffectCommand(TRow* pRow, UINT column);

	const UINT GetPatternRowEffectParameter(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectParameter(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectParameter(TChannel* pChannel, UINT pattern, UINT row, UINT column);
	const UINT GetPatternRowEffectParameter(TPattern* pPattern, UINT row, UINT column);
	const UINT GetPatternRowEffectParameter(TRow* pRow, UINT column);

	bool SetPatternInSongline(UINT subtune, UINT channel, UINT songline, UINT pattern);
	bool SetPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline, UINT pattern);
	bool SetPatternInSongline(TChannel* pChannel, UINT songline, UINT pattern);

	bool SetPatternRowNote(UINT subtune, UINT channel, UINT pattern, UINT row, UINT note);
	bool SetPatternRowNote(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT note);
	bool SetPatternRowNote(TChannel* pChannel, UINT pattern, UINT row, UINT note);
	bool SetPatternRowNote(TPattern* pPattern, UINT row, UINT note);
	bool SetPatternRowNote(TRow* pRow, UINT note);

	bool SetPatternRowInstrument(UINT subtune, UINT channel, UINT pattern, UINT row, UINT instrument);
	bool SetPatternRowInstrument(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT instrument);
	bool SetPatternRowInstrument(TChannel* pChannel, UINT pattern, UINT row, UINT instrument);
	bool SetPatternRowInstrument(TPattern* pPattern, UINT row, UINT instrument);
	bool SetPatternRowInstrument(TRow* pRow, UINT instrument);

	bool SetPatternRowVolume(UINT subtune, UINT channel, UINT pattern, UINT row, UINT volume);
	bool SetPatternRowVolume(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT volume);
	bool SetPatternRowVolume(TChannel* pChannel, UINT pattern, UINT row, UINT volume);
	bool SetPatternRowVolume(TPattern* pPattern, UINT row, UINT volume);
	bool SetPatternRowVolume(TRow* pRow, UINT volume);

	bool SetPatternRowEffectCommand(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column, UINT command);
	bool SetPatternRowEffectCommand(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column, UINT command);
	bool SetPatternRowEffectCommand(TChannel* pChannel, UINT pattern, UINT row, UINT column, UINT command);
	bool SetPatternRowEffectCommand(TPattern* pPattern, UINT row, UINT column, UINT command);
	bool SetPatternRowEffectCommand(TRow* pRow, UINT column, UINT command);

	bool SetPatternRowEffectParameter(UINT subtune, UINT channel, UINT pattern, UINT row, UINT column, UINT parameter);
	bool SetPatternRowEffectParameter(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row, UINT column, UINT parameter);
	bool SetPatternRowEffectParameter(TChannel* pChannel, UINT pattern, UINT row, UINT column, UINT parameter);
	bool SetPatternRowEffectParameter(TPattern* pPattern, UINT row, UINT column, UINT parameter);
	bool SetPatternRowEffectParameter(TRow* pRow, UINT column, UINT parameter);

	//-- Getters and Setters for Module Parameters --//

	const char* GetModuleName() { return m_moduleName; };
	const char* GetModuleAuthor() { return m_moduleAuthor; };
	const char* GetModuleCopyright() { return m_moduleCopyright; };

	const char* GetSubtuneName(UINT subtune);
	const char* GetSubtuneName(TSubtune* pSubtune);

	const UINT GetSongLength(UINT subtune);
	const UINT GetSongLength(TSubtune* pSubtune);

	const UINT GetPatternLength(UINT subtune);
	const UINT GetPatternLength(TSubtune* pSubtune);

	const UINT GetChannelCount(UINT subtune);
	const UINT GetChannelCount(TSubtune* pSubtune);

	const UINT GetSongSpeed(UINT subtune);
	const UINT GetSongSpeed(TSubtune* pSubtune);

	const UINT GetInstrumentSpeed(UINT subtune);
	const UINT GetInstrumentSpeed(TSubtune* pSubtune);

	const UINT GetEffectCommandCount(UINT subtune, UINT channel);
	const UINT GetEffectCommandCount(TSubtune* pSubtune, UINT channel);
	const UINT GetEffectCommandCount(TChannel* pChannel);

	bool SetModuleName(const char* name) { strncpy_s(m_moduleName, name, MODULE_SONG_NAME_MAX); return true; };
	bool SetModuleAuthor(const char* author) { strncpy_s(m_moduleAuthor, author, MODULE_AUTHOR_NAME_MAX); return true; };
	bool SetModuleCopyright(const char* copyright) { strncpy_s(m_moduleCopyright, copyright, MODULE_COPYRIGHT_INFO_MAX); return true; };
	
	bool SetSubtuneName(UINT subtune, const char* name);
	bool SetSubtuneName(TSubtune* pSubtune, const char* name);

	bool SetSongLength(UINT subtune, UINT length);
	bool SetSongLength(TSubtune* pSubtune, UINT length);

	bool SetPatternLength(UINT subtune, UINT length);
	bool SetPatternLength(TSubtune* pSubtune, UINT length);

	bool SetChannelCount(UINT subtune, UINT count);
	bool SetChannelCount(TSubtune* pSubtune, UINT count);

	bool SetSongSpeed(UINT subtune, UINT speed);
	bool SetSongSpeed(TSubtune* pSubtune, UINT speed);

	bool SetInstrumentSpeed(UINT subtune, UINT speed);
	bool SetInstrumentSpeed(TSubtune* pSubtune, UINT speed);

	bool SetEffectCommandCount(UINT subtune, UINT channel, UINT column);
	bool SetEffectCommandCount(TSubtune* pSubtune, UINT channel, UINT column);
	bool SetEffectCommandCount(TChannel* pChannel, UINT column);


	//-- RMTE Editor Functions --//

	const UINT GetSubtuneCount();

	const UINT GetShortestPatternLength(UINT subtune, UINT songline);
	const UINT GetShortestPatternLength(TSubtune* pSubtune, UINT songline);

	const UINT GetEffectivePatternLength(UINT subtune, UINT channel, UINT pattern);
	const UINT GetEffectivePatternLength(TSubtune* pSubtune, UINT channel, UINT pattern);
	const UINT GetEffectivePatternLength(TChannel* pChannel, UINT pattern, UINT patternLength);
	const UINT GetEffectivePatternLength(TPattern* pPattern, UINT patternLength, UINT effectCount);

	bool IsUnusedPattern(UINT subtune, UINT channel, UINT pattern);
	bool IsUnusedPattern(TSubtune* pSubtune, UINT channel, UINT pattern);
	bool IsUnusedPattern(TChannel* pChannel, UINT pattern);

	bool IsEmptyPattern(UINT subtune, UINT channel, UINT pattern);
	bool IsEmptyPattern(TSubtune* pSubtune, UINT channel, UINT pattern);
	bool IsEmptyPattern(TChannel* pChannel, UINT pattern);
	bool IsEmptyPattern(TPattern* pPattern);

	bool IsEmptyRow(UINT subtune, UINT channel, UINT pattern, UINT row);
	bool IsEmptyRow(TSubtune* pSubtune, UINT channel, UINT pattern, UINT row);
	bool IsEmptyRow(TChannel* pChannel, UINT pattern, UINT row);
	bool IsEmptyRow(TPattern* pPattern, UINT row);
	bool IsEmptyRow(TRow* pRow);

	bool IsIdenticalPattern(TPattern* pFromPattern, TPattern* pToPattern);

	bool IsIdenticalRow(TRow* pFromRow, TRow* pToRow);

	bool DuplicatePatternInSongline(UINT subtune, UINT channel, UINT songline, UINT pattern);
	bool DuplicatePatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline, UINT pattern);
	bool DuplicatePatternInSongline(TChannel* pChannel, UINT songline, UINT pattern);

	bool CopyRow(TRow* pFromRow, TRow* pToRow);

	bool CopyPattern(TPattern* pFromPattern, TPattern* pToPattern);

	bool CopyChannel(UINT subtune, UINT fromChannel, UINT toChannel);
	bool CopyChannel(TSubtune* pSubtune, UINT fromChannel, UINT toChannel);
	bool CopyChannel(TChannel* pFromChannel, TChannel* pToChannel);

	bool CopySubtune(UINT fromSubtune, UINT toSubtune);
	bool CopySubtune(TSubtune* pFromSubtune, TSubtune* pToSubtune);

	void MergeDuplicatedPatterns(UINT subtune);
	void MergeDuplicatedPatterns(TSubtune* pSubtune);

	void RenumberIndexedPatterns(UINT subtune);
	void RenumberIndexedPatterns(TSubtune* pSubtune);

	void ClearUnusedPatterns(UINT subtune);
	void ClearUnusedPatterns(TSubtune* pSubtune);

	void ConcatenateIndexedPatterns(UINT subtune);
	void ConcatenateIndexedPatterns(TSubtune* pSubtune);

	void AllSizeOptimisations(UINT subtune);
	void AllSizeOptimisations(TSubtune* pSubtune);

	//-- Getters and Setters for Instrument Data --//

	TInstrumentV2* GetInstrument(UINT instrument);
	const char* GetInstrumentName(UINT instrument);
	const char* GetInstrumentName(TInstrumentV2* pInstrument);

	TEnvelope* GetVolumeEnvelope(UINT instrument);
	TEnvelope* GetTimbreEnvelope(UINT instrument);
	TEnvelope* GetAudctlEnvelope(UINT instrument);
	//TEnvelope* GetTriggerEnvelope(UINT instrument);
	TEnvelope* GetEffectEnvelope(UINT instrument);
	TEnvelope* GetNoteTableEnvelope(UINT instrument);
	TEnvelope* GetFreqTableEnvelope(UINT instrument);

	bool SetInstrumentName(UINT instrument, const char* name);
	bool SetInstrumentName(TInstrumentV2* instrument, const char* name);

private:
	char m_moduleName[MODULE_SONG_NAME_MAX + 1];
	char m_moduleAuthor[MODULE_AUTHOR_NAME_MAX + 1];
	char m_moduleCopyright[MODULE_COPYRIGHT_INFO_MAX + 1];
	TSubtuneIndex* m_subtuneIndex;
	TInstrumentIndex* m_instrumentIndex;
};
