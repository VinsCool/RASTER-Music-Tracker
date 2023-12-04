// RMT Module V2 Format Prototype
// By VinsCool, 2023
//
// TODO: Move the Legacy Import code to IO_Song.cpp or similar, in order to get most of the CModule functions cleared from unrelated stuff
// TODO: Move most of the Editor Functions to CSong or similar for the same reason
// TODO: Create an enum for the majority of the things defined below, it might make future revisions a lot easier...

#pragma once

#include "General.h"
#include "global.h"


// ----------------------------------------------------------------------------
// Module Header definition
//

#define MODULE_VERSION					0											// Module Version number, the highest value is always assumed to be the most recent
#define MODULE_IDENTIFIER				"RMTE"										// "Raster Music Tracker Extended" Module Identifier
#define MODULE_REGION					g_ntsc										// 0 for PAL, 1 for NTSC, anything else is also assumed to be NTSC
#define MODULE_CHANNEL_COUNT			g_tracks4_8									// 4 for Mono, 8 for Stereo, add more for whatever setup that could be used
#define MODULE_BASE_TUNING				g_baseTuning								// Default A-4 Tuning
#define MODULE_BASE_NOTE				g_baseNote									// Default Base Note (A-)
#define MODULE_BASE_OCTAVE				g_baseOctave								// Default Base Octave (0)
#define MODULE_TEMPERAMENT				g_baseTemperament							// Default Tuning Temperament
#define MODULE_PRIMARY_HIGHLIGHT		g_trackLinePrimaryHighlight					// Pattern Primary Highlight
#define MODULE_SECONDARY_HIGHLIGHT		g_trackLineSecondaryHighlight				// Pattern Secondary Highlight
#define MODULE_LINE_NUMBERING_MODE		g_tracklinealtnumbering						// Row numbering mode
#define MODULE_LINE_STEP				g_linesafter								// Line Step between cursor movements
#define MODULE_DISPLAY_FLAT_NOTES		g_displayflatnotes							// Display accidentals as Flat instead of Sharp
#define MODULE_DISPLAY_GERMAN_NOTATION	g_usegermannotation							// Display notes using the German Notation
#define MODULE_SCALING_PERCENTAGE		g_scaling_percentage						// Display scaling percentage
#define MODULE_DEFAULT_NAME				"Noname Module"
#define MODULE_DEFAULT_AUTHOR			"Unknown"
#define MODULE_DEFAULT_COPYRIGHT		"2023"
#define MODULE_DEFAULT_SUBTUNE_NAME		"Noname Subtune"
#define MODULE_DEFAULT_SUBTUNE			0											// Default Active Subtune
#define MODULE_DEFAULT_INSTRUMENT		0											// Default Active Instrument
#define MODULE_DEFAULT_PATTERN_LENGTH	64											// Default Pattern Length
#define MODULE_DEFAULT_SONG_LENGTH		1											// Default Song Length
#define MODULE_DEFAULT_SONG_SPEED		6											// Default Song Speed
#define MODULE_DEFAULT_INSTRUMENT_SPEED	1											// Default Instrument Speed
#define MODULE_DEFAULT_CHANNEL_COUNT	4											// Default Channel Count
#define MODULE_DEFAULT_SOUNDCHIP_COUNT	POKEY1
#define MODULE_SONG_NAME_MAX			64											// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64											// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64											// Maximum length of Copyright info
#define MODULE_PADDING					32											// Padding bytes for the Module file format specifications
#define ENVELOPE_PADDING				4											// Padding bytes for the Envelope parameters used in Instruments


// ----------------------------------------------------------------------------
// Song, Pattern and Instrument definition
//

#define SUBTUNE_NAME_MAX			64												// Maximum length of Subtune name
#define SUBTUNE_COUNT				64												// Maximum number of Subtunes
#define SONGLINE_COUNT				256												// Maximum Songline Index
#define PATTERN_COUNT				256												// Maximum Pattern Index
#define ROW_COUNT					256												// Maximum Row Index
#define SONG_SPEED_MAX				256												// Maximum Song Speed
#define INSTRUMENT_SPEED_MAX		16												// Maximum Instrument Speed
#define CHANNEL_COUNT				(POKEY_SOUNDCHIP_COUNT * POKEY_CHANNEL_COUNT)	// Maximum number of Channels in total
#define CH1(x)						((x % POKEY_CHANNEL_COUNT) == CH1)				// Is POKEY Channel 1?
#define CH2(x)						((x % POKEY_CHANNEL_COUNT) == CH2)				// Is POKEY Channel 2?
#define CH3(x)						((x % POKEY_CHANNEL_COUNT) == CH3)				// Is POKEY Channel 3?
#define CH4(x)						((x % POKEY_CHANNEL_COUNT) == CH4)				// Is POKEY Channel 4?
#define NOTE_COUNT					120												// Maximum Note Index, for a total of 10 octaves
#define INSTRUMENT_COUNT			64												// Maximum Instrument Index
#define VOLUME_COUNT				16												// Maximum Volume Index
#define EFFECT_PARAMETER_COUNT		256												// Maximum Effect Parameter $XY Index
#define INSTRUMENT_NAME_MAX			64												// Maximum length of Instrument name
#define ENVELOPE_STEP_COUNT			256												// Maximum Envelope Index


// ----------------------------------------------------------------------------
// RMTE Module Enums for Subtune, Pattern, Row, etc
//

// Maximum number of Channels per POKEY soundchip defined by Count
typedef enum pokeyChannel_t : BYTE
{
	CH1 = 0,
	CH2,
	CH3,
	CH4,
	POKEY_CHANNEL_COUNT,
} TPokeyChannel;

// Maximum number of POKEY soundchips defined by Count
typedef enum pokeySoundchip_t : BYTE
{
	POKEY1 = 0,
	POKEY2,
	POKEY3,
	POKEY4,
	POKEY_SOUNDCHIP_COUNT,
} TPokeySoundchip;

// Maximum number of Effect Commands in Pattern defined by Count
typedef enum effectCommandColumn_t : BYTE
{
	CMD1 = 0,
	CMD2,
	CMD3,
	CMD4,
	PATTERN_EFFECT_COUNT,
} TEffectCommandColumn;

// Valid Note Index ranges from 0 to the value of NOTE_COUNT
// Additional entries must be inserted at the end before NOTE_INDEX_MAX
typedef enum patternNote_t : BYTE
{
	NOTE_EMPTY = NOTE_COUNT,
	NOTE_OFF,
	NOTE_RELEASE,
	NOTE_RETRIGGER,
	NOTE_INDEX_MAX,
} TPatternNote;

// Valid Instrument Index ranges from 0 to the value of INSTRUMENT_COUNT
// Additional entries must be inserted at the end before INSTRUMENT_INDEX_MAX
typedef enum patternInstrument_t : BYTE
{
	INSTRUMENT_EMPTY = INSTRUMENT_COUNT,
	INSTRUMENT_INDEX_MAX,
} TPatternInstrument;

// Valid Volume Index ranges from 0 to the value of VOLUME_COUNT
// Additional entries must be inserted at the end before VOLUME_INDEX_MAX
typedef enum patternVolume_t : BYTE
{
	VOLUME_EMPTY = VOLUME_COUNT,
	VOLUME_INDEX_MAX,
} TPatternVolume;

// Pattern Effect Commands, there is no definitive count so it's structured slightly differently
// Additional entries must be inserted at the end before PE_INDEX_MAX
typedef enum patternEffectCommand_t : BYTE
{
	PE_EMPTY = EMPTY,
	PE_ARPEGGIO,
	PE_PITCH_UP,
	PE_PITCH_DOWN,
	PE_PORTAMENTO,
	PE_VIBRATO,
	PE_VOLUME_FADE,
	PE_GOTO_SONGLINE,
	PE_END_PATTERN,
	PE_SET_SPEED,
	PE_SET_FINETUNE,
	PE_SET_DELAY,
	PE_INDEX_MAX,
} TPatternEffectCommand;

// Instrument Effect Commands, there is no definitive count so it's structured slightly differently
// Additional entries must be inserted at the end before IE_INDEX_MAX
typedef enum instrumentEffectCommand_t : BYTE
{
	IE_EMPTY = EMPTY,		// Not actually needed, using Transpose set to 0x00 would do the same, and save 1 CMD slot, but let's just use this for simplicity
	IE_TRANSPOSE,			// Technically speaking, this is an Arpeggio Command that is both Relative and Volatile, equivalent to the Legacy RMT CMD0
	IE_PITCH_UP,			// Override Freq Shift Parameter and Freq Shift Delay
	IE_PITCH_DOWN,			// Override Freq Shift Parameter and Freq Shift Delay
	IE_PORTAMENTO,			// Override Portamento Parameter, equivalent to the Legacy RMT CMD5
	IE_VIBRATO,				// Override Vibrato Parameter and Vibrato Delay
	IE_VOLUME_FADE,			// Override Volume Fade Parameter and Volume Fade Delay
	IE_FINETUNE,			// Technically speaking, this is a Finetune Command that is both Relative and Volatile, equivalent to the Legacy RMT CMD2
	IE_AUTOFILTER,			// Override AutoFilter Parameter, which may be either Absolute or Additive, equivalent to the Legacy RMT CMD6
	IE_SET_FREQ_MSB,		// Set Freq MSB, Absolute and Volatile, 16-bit mode only, ignored otherwise, equivalent to the Legacy RMT CMD1
	IE_SET_FREQ_LSB,		// Set Freq LSB, Absolute and Volatile, equivalent to the Legacy RMT CMD1
	IE_FREQ_SHIFT,			// Freq Shift, Additive, equivalent to the Legacy RMT CMD4
	IE_NOTE_SHIFT,			// Note Shift, Additive, equivalent to the Legacy RMT CMD3
	IE_INDEX_MAX,
} TInstrumentEffectCommand;

// Effect Command Parameter preset values
typedef enum effectParameter_t : BYTE
{
	EFFECT_PARAMETER_MIN = 0x00,
	EFFECT_PARAMETER_MED = 0x80,
	EFFECT_PARAMETER_MAX = 0xFF,
} TEffectParameter;


// ----------------------------------------------------------------------------
// RMTE Module Structs for Subtune, Pattern, Row, etc
//

// Effect Command Data, 1 byte for the Identifier, and 1 byte for the Parameter, nothing too complicated
struct TEffect
{
	BYTE command : 5;						// Command Identifier, bits are not representative of anything, yet
	BYTE parameter;							// Command Parameter, values ranging from 0 to 255 may be used
};

// Row Data, used within the Pattern data, designed to be easy to manage, following a Row by Row approach
struct TRow
{
	BYTE note : 7;							// Note Index, as well as Pattern Commands such as Stop, Release, Retrigger, etc
	BYTE instrument : 7;					// Instrument Index, bits are not representative of anything, yet
	BYTE volume : 5;						// Volume Index, bits are not representative of anything, yet
	TEffect effect[PATTERN_EFFECT_COUNT];	// Effect Command, toggled from the Active Effect Columns in Track Channels
};

// Pattern Data, indexed by the TRow Struct
struct TPattern
{
	TRow row[ROW_COUNT];					// Row data is contained withn its associated Pattern index
};

// Channel Index, used for indexing the Songline and Pattern data, similar to the CSong Class
struct TChannel
{
	union
	{
		struct
		{
			BYTE channelVolume : 4;			// Channel Volume, currently placeholder bits, not sure how that would work out...
			BYTE effectCount : 2;			// Number of Effect Commands enabled per Track Channel, overriden by the next bit possibly...?
			bool isEffectEnabled : 1;		// Channel is using Effect Commands? Placeholder bit for now
			bool isMuted : 1;				// Channel is muted? Placeholder bit for now
		};
		BYTE parameters[MODULE_PADDING];	// Parameter bytes, with padding reserved for future format revisions, mainly for backwards compatibility
	};
	BYTE songline[SONGLINE_COUNT];			// Pattern Index for each songline within the Track Channel
	TPattern pattern[PATTERN_COUNT];		// Pattern Data for the Track Channel
};

// Subtune Index, used for indexing all of the Module data, indexed by the TIndex Struct
struct TSubtune
{
	char name[SUBTUNE_NAME_MAX + 1];		// Subtune Name
	union
	{
		struct
		{
			BYTE songLength;				// Song Length, in Songlines
			BYTE patternLength;				// Pattern Length, in Rows
			BYTE songSpeed;					// Song Speed, in Frames per Row
			BYTE instrumentSpeed : 4;		// Instrument Speed, in Frames per VBI
			BYTE channelCount : 4;			// Number of Channels used in Subtune
		};
		BYTE parameters[MODULE_PADDING];	// Parameter bytes, with padding reserved for future format revisions, mainly for backwards compatibility
	};
	TChannel channel[CHANNEL_COUNT];		// Channel Index assigned to the Subtune
};

// Redundant...?
struct TSubtuneIndex
{
	TSubtune* subtune[SUBTUNE_COUNT];
};

// Row data encoding, a bit set would mean there is empty data for an element
// This is a very rough implementation of the DUMB Music Driver Module format encoding
// That format is very barebone, but it might do a good enough job, hopefully
// 
// Reminder: Bitwise order goes from 0 to 7, the leftmost bits are the last entries added in the Struct!
// If I wanted to specifically check Bit 7 for a parameter, I must remember that it will be at the bottom, not the top!
//
struct TRowEncoding
{
	union
	{
		struct
		{
			bool isEmptyCmd4 : 1;			// Empty CMD4? Skip next 2 bytes
			bool isEmptyCmd3 : 1;			// Empty CMD3? Skip next 2 bytes
			bool isEmptyCmd2 : 1;			// Empty CMD2? Skip next 2 bytes
			bool isEmptyCmd1 : 1;			// Empty CMD1? Skip next 2 bytes
			bool isEmptyVolume : 1;			// Empty Volume? Skip next byte
			bool isEmptyInstrument : 1;		// Empty Instrument? Skip next byte
			bool isEmptyNote : 1;			// Empty Note? Skip next byte
			bool isEndOfPattern : 1;		// Pattern Terminator?, Skip next byte and set Infinite Pause Length
		};
		BYTE parameters;					// Bitwise parameters union
	};
	BYTE pauseLength;						// Skip next 0-255 Rows, overridden by the Pattern Terminator Bit
};

// ----------------------------------------------------------------------------
// RMTE Module Structs for Instrument, Envelope, Table, etc
//

// Instrument Envelope Macro, used to specify which Envelope Index is associated to the Instrument and how it is used
struct TMacro
{
	union
	{
		struct
		{
			BYTE index : 6;					// Unique Envelopes may be shared between all Instruments, useful for using identical Volume, Timbre, Audctl, etc
			bool isReversed : 1;			// Envelope is played backwards if True, otherwise it will be played in the correct order, placeholder bit for now
			bool isEnabled : 1;				// Envelope is used if True, otherwise it will be skipped, but still displayed on screen for reference
		};
		BYTE parameters;					// Bitwise parameters union
	};
};

// A Macro of Instrument Envelope Macros, what else? This should be pretty self-explanatory :D
struct TEnvelopeMacro
{
	TMacro volume;
	TMacro timbre;
	TMacro audctl;
	TMacro effect;
	TMacro note;
	TMacro freq;
};

// Instrument Envelope parameters, used to define things such as the Envelope Length, Loop Point, Speed, etc
struct TEnvelopeParameter
{
	union
	{
		struct
		{
			BYTE length;					// Envelope Length, in frames
			BYTE loop;						// Envelope Loop point
			BYTE release;					// Envelope Release point
			BYTE speed : 4;					// Envelope Speed, in ticks per frames
			bool isLooped : 1;				// Loop point is used if True, otherwise the Envelope will end after the last Frame was processed
			bool isReleased : 1;			// Release point is used if True, could also be combined to Loop Point, useful for sustaining a Note Release
			bool isAbsolute : 1;			// Absolute Mode is used if True, otherwise Relative Mode is used (Note and Freq Tables only)
			bool isAdditive : 1;			// Additive Mode is used if True, could also be combined to Absolute Mode if desired (Note and Freq Tables only)
		};
		BYTE parameters[ENVELOPE_PADDING];	// Bitwise parameters union, there is probably nothing else to add here but we never know...
	};
};

// Instrument Data, due to the Legacy TInstrument struct still in the codebase, this is temporarily defined as TInstrumentV2
struct TInstrumentV2
{
	char name[INSTRUMENT_NAME_MAX + 1];		// Instrument Name
	union
	{
		struct
		{
			TEnvelopeMacro envelope;		// Envelope Macros associated to the Instrument, taking priority over certain Pattern Effect Commands
			BYTE volumeFade;				// Volume Fadeout parameter, taking priority over EFFECT_VOLUME_FADEOUT for Legacy RMT Instrument compatibility
			BYTE volumeSustain;				// Volume Sustain parameter, taking priority over EFFECT_VOLUME_FADEOUT for Legacy RMT Instrument compatibility
			BYTE volumeDelay;				// Volume Delay parameter, used when VolumeFade is a non-zero parameter, a delay of 0x00 is immediate
			BYTE vibrato;					// Vibrato parameter, taking priority over EFFECT_VIBRATO for Legacy RMT Instrument compatibility
			BYTE vibratoDelay;				// Vibrato Delay parameter, used when Vibrato is a non-zero parameter, a delay of 0x00 is immediate
			BYTE freqShift;					// Freq Shift parameter, taking priority EFFECT_PITCH_UP and EFFECT_PITCH_DOWN for Legacy RMT Instrument compatibility
			BYTE freqShiftDelay;			// Freq Shift Delay parameter, used when FreqShift is a non-zero parameter, a delay of 0x00 is immediate
			BYTE autoFilter;				// AutoFilter parameter, taking priority over EFFECT_AUTOFILTER for Legacy RMT Instrument compatibility
			bool autoFilterMode : 1;		// AutoFilter Mode parameter, Additive Mode if True, otherwise it is Absolute
		};
		BYTE parameters[MODULE_PADDING];	// Parameter bytes, with padding reserved for future format revisions, mainly for backwards compatibility
	};
};

// Instrument Volume Envelope
struct TVolumeEnvelope
{
	union
	{
		struct
		{
			BYTE volumeLeft : 4;			// Left POKEY Volume, for Legacy RMT Instrument Compatibility, used by default for everything otherwise
			BYTE volumeRight : 4;			// Right POKEY Volume, for Legacy RMT Instrument Compatibility
		};
		BYTE parameters;
	};
};

// Instrument Timbre Envelope
struct TTimbreEnvelope
{
	union
	{
		struct
		{
			BYTE waveForm : 4;				// eg: Buzzy, Gritty, etc
			BYTE distortion : 3;			// eg: Pure (0xA0), Poly4 (0xC0), etc
			bool isVolumeOnly : 1;			// Volume Only Mode, an override is plannned for sample/wavetable playback support in the future...
		};
		BYTE parameters;
	};
};

// Instrument AUDCTL Envelope
struct TAudctlEnvelope
{
	union
	{
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
		BYTE parameters;
	};
};

// Instrument Effect(s) Envelope
struct TEffectEnvelope
{
	// Legacy RMT (1.28 and 1.34) have the following Effect Commands:
	// 
	// CMD0 -> Set Note (Relative)
	// CMD1 -> Set Freq (Absolute, 8-bit Parameter only)
	// CMD2 -> Finetune (Relative)
	// CMD3 -> Set Note (Additive)
	// CMD4 -> Set FreqShift (Additive)
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
	//
	// Idea 5: Make a completely different setup that would combine the best of both worlds
	// The format would make use of 4 bytes per Envelope Step, limiting itself to 1/4 of the maximal Envelope length as a compromise
	// 
	// Byte 1 -> Most of the AutoParameter bits, providing combined effects if they are compatible
	// Byte 2 -> Instrument Effect Command Identifier, used for either 2 8-bit Commands, or 1 16-bit Command
	// Byte 3 -> If it is a 16-bit Command, Byte 3 and 4 will be combined and used as the Effect Command Parameter
	// Byte 4 -> Else, Byte 3 and 4 will be distinct 8-bit parameters, both assigned to their respective 8-bit Commands
	// 
	union
	{
		struct
		{
			// Byte 1: Automatic Triggers
			bool autoFilter : 1;			// High Pass Filter, triggered from Channel 1 and/or 2, hijacking Channel 3 and/or 4
			bool auto16Bit : 1;				// 16-bit mode, triggered from Channel 2 and/or 4, hijacking Channel 1 and/or 3
			bool autoReverse16 : 1;			// Reverse 16-bit mode, triggered from Channel 1 and/or 3, hijacking Channel 2 and/or 4
			bool auto179Mhz : 1;			// 1.79Mhz mode, triggered from Channel 1 and/or 3
			bool auto15Khz : 1;				// 15Khz mode, triggered from any Channel, hijacking all Channels not affected by 1.79Mhz mode (16-bit included)
			bool autoPoly9 : 1;				// Poly9 Noise mode, triggered from any Channel, hijacking all Channels using Distortion 0 and 8
			bool autoTwoTone : 1;			// Automatic Two-Tone Filter, triggered from Channel 1, hijacking Channel 2
			bool unused : 1;				// Reserved

			// Byte 2: Effect Command Identifiers
			BYTE command_1 : 4;
			BYTE command_2 : 4;

			// Byte 3 and 4: Effect Command Parameters
			BYTE parameter_1;
			BYTE parameter_2;
		};
		BYTE parameters[4];
	};
};

// Instrument Note Table Envelope
struct TNoteTableEnvelope
{
	union
	{
		struct
		{
			SBYTE noteXY : 6;				// Transpose by +- 32 semitones (Relative/Additive), capped to NOTE_COUNT
			bool isNoteY : 1;				// Add X or Y semitones to noteXY (Relative, only in Note Scheme Mode), capped to NOTE_COUNT
			bool isNoteScheme : 1;			// Note Scheme is paired to PE_ARPEGGIO's XY Parameters, only when it is active in a Channel
		};
		SBYTE noteRelative : 7;				// Transpose by +- 64 semitones (Relative/Additive), capped to NOTE_COUNT
		BYTE noteAbsolute;					// Set Note Index 0-255 (Absolute/Additive), capped to NOTE_COUNT
		BYTE parameters;
	};
};

// Instrument Freq Table Envelope, 2 bytes are used per Envelope step, in order to form the corresponding 8-bit/16-bit Freq values when needed
struct TFreqTableEnvelope
{
	union
	{
		struct
		{
			BYTE freqAbsoluteLo;			// Set Freq LSB Index 0-255 (Absolute/Additive), capped to FREQ_COUNT
			BYTE freqAbsoluteHi;			// Set Freq MSB Index 0-255 (Absolute/Additive), capped to FREQ_COUNT
		};
		struct
		{
			SBYTE freqRelativeLo;			// Transpose by +- 128 Freq LSB units (Relative/Additive), capped to FREQ_COUNT
			SBYTE freqRelativeHi;			// Transpose by +- 128 Freq MSB units (Relative/Additive), capped to FREQ_COUNT
		};
		WORD freqAbsolute;					// Set Freq Index 0-65535 (Absolute/Additive), capped to FREQ_COUNT_16
		SWORD freqRelative;					// Transpose by +- 32768 Freq units (Relative/Additive), capped to FREQ_COUNT_16
		BYTE parameters[2];
	};
};

// Instrument Envelope data, with all the data structures unified, making virtually any Envelope compatible between each others
struct TEnvelope
{
	TEnvelopeParameter parameter;
	union
	{
		TVolumeEnvelope volume[ENVELOPE_STEP_COUNT];
		TTimbreEnvelope timbre[ENVELOPE_STEP_COUNT];
		TAudctlEnvelope audctl[ENVELOPE_STEP_COUNT];
		TEffectEnvelope effect[ENVELOPE_STEP_COUNT / 4];	// Due to Legacy Effect commands, and a lot of new parameters
		TNoteTableEnvelope note[ENVELOPE_STEP_COUNT];
		TFreqTableEnvelope freq[ENVELOPE_STEP_COUNT / 2];	// Due to 16-bit Freq values
	};
};

// Also sort of redundant...? This is justified for the Envelopes alone, at least...
struct TInstrumentIndex
{
	TInstrumentV2* instrument[INSTRUMENT_COUNT];
	TEnvelope* volume[INSTRUMENT_COUNT];
	TEnvelope* timbre[INSTRUMENT_COUNT];
	TEnvelope* audctl[INSTRUMENT_COUNT];
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
// TODO: Using 24bit addressing might be plenty for this now that the data could be compressed somewhat...
typedef struct LoHeader_t
{
	UINT subtuneIndex[SUBTUNE_COUNT];			// Offset to Subtune
	UINT instrumentIndex[INSTRUMENT_COUNT];		// Offset to Instrument
	UINT volumeEnvelope[INSTRUMENT_COUNT];		// Offset to Volume Envelope
	UINT timbreEnvelope[INSTRUMENT_COUNT];		// Offset to Timbre Envelope
	UINT audctlEnvelope[INSTRUMENT_COUNT];		// Offset to AUDCTL Envelope
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

	bool InitialiseEnvelopeParameter(TEnvelopeParameter* pEnvelopeParameter);

	bool CreateVolumeEnvelope(UINT instrument);
	bool DeleteVolumeEnvelope(UINT instrument);
	bool InitialiseVolumeEnvelope(TEnvelope* pEnvelope);

	bool CreateTimbreEnvelope(UINT instrument);
	bool DeleteTimbreEnvelope(UINT instrument);
	bool InitialiseTimbreEnvelope(TEnvelope* pEnvelope);

	bool CreateAudctlEnvelope(UINT instrument);
	bool DeleteAudctlEnvelope(UINT instrument);
	bool InitialiseAudctlEnvelope(TEnvelope* pEnvelope);

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
	const bool IsValidEffectCommandIndex(UINT command) { return command < PE_INDEX_MAX; };
	const bool IsValidEffectParameter(UINT parameter) { return parameter < EFFECT_PARAMETER_COUNT; };
	const bool IsValidCommandColumn(UINT column) { return column < PATTERN_EFFECT_COUNT; };

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

	bool SetNewEmptyPatternInSongline(UINT subtune, UINT channel, UINT songline);
	bool SetNewEmptyPatternInSongline(TSubtune* pSubtune, UINT channel, UINT songline);
	bool SetNewEmptyPatternInSongline(TChannel* pChannel, UINT songline);

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
	TEnvelope* GetEffectEnvelope(UINT instrument);
	TEnvelope* GetNoteTableEnvelope(UINT instrument);
	TEnvelope* GetFreqTableEnvelope(UINT instrument);

	bool SetInstrumentName(UINT instrument, const char* name);
	bool SetInstrumentName(TInstrumentV2* instrument, const char* name);

	//-- Other functions --//

	const char* GetPatternEffectCommandIdentifier(TPatternEffectCommand command);

	const char* GetPatternNoteCommand(TPatternNote note);
	const char* GetPatternNoteIndex(TPatternNote note);
	UINT GetPatternNoteOctave(TPatternNote note);

	const char* GetPatternInstrumentCommand(TPatternInstrument instrument);

	const char* GetPatternVolumeCommand(TPatternVolume volume);

private:
	char m_moduleName[MODULE_SONG_NAME_MAX + 1];
	char m_moduleAuthor[MODULE_AUTHOR_NAME_MAX + 1];
	char m_moduleCopyright[MODULE_COPYRIGHT_INFO_MAX + 1];
	TSubtuneIndex* m_subtuneIndex;
	TInstrumentIndex* m_instrumentIndex;
};
