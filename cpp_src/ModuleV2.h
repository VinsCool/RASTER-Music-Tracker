// RMT Module V2 Format Prototype
// By VinsCool, 2023

#pragma once

#include "General.h"
#include "global.h"

// ----------------------------------------------------------------------------
// Data boundaries constant
//
#define INVALID							-1								// Failsafe value for invalid data
#define BYTE_MIN						0x00							// BYTE Minimum
#define BYTE_MAX						0xFF							// BYTE Maximum
#define WORD_MIN						0x0000							// WORD Minimum
#define WORD_MAX						0xFFFF							// WORD Maximum

// ----------------------------------------------------------------------------
// Module Header definition
//
#define MODULE_VERSION					0								// Module Version number, the highest value is always assumed to be the most recent
#define MODULE_IDENTIFIER				"RMTE"							// Raster Music Tracker Extended, "DUMB" is another potential identifier
#define MODULE_REGION					g_ntsc							// 0 for PAL, 1 for NTSC, anything else is also assumed to be NTSC
#define MODULE_STEREO					g_tracks4_8						// 4 for Mono, 8 for Stereo
#define MODULE_A4_TUNING				g_basetuning					// Default A-4 Tuning
#define MODULE_BASE_NOTE				g_basenote						// Default Base Note (A-)
#define MODULE_TEMPERAMENT				g_temperament					// Default Tuning Temperament
#define MODULE_PRIMARY_HIGHLIGHT		g_trackLinePrimaryHighlight		// Pattern Primary Highlight
#define MODULE_SECONDARY_HIGHLIGHT		g_trackLineSecondaryHighlight	// Pattern Secondary Highlight
#define MODULE_LINE_NUMBERING_MODE		g_tracklinealtnumbering			// Row numbering mode
#define MODULE_LINE_STEP				g_linesafter					// Line Step between cursor movements
#define MODULE_DISPLAY_FLAT_NOTES		g_displayflatnotes				// Display accidentals as Flat instead of Sharp
#define MODULE_DISPLAY_GERMAN_NOTATION	g_usegermannotation				// Display notes using the German Notation
#define MODULE_TRACK_LENGTH				64								// Default Track Length
#define MODULE_SONG_LENGTH				1								// Default Song Length
#define MODULE_SONG_SPEED				6								// Default Song Speed
#define MODULE_VBI_SPEED				1								// Default VBI Speed
#define MODULE_TITLE_NAME_MAX			64								// Maximum length of Song Title
#define MODULE_AUTHOR_NAME_MAX			64								// Maximum length of Author name
#define MODULE_COPYRIGHT_INFO_MAX		64								// Maximum length of Copyright info

// ----------------------------------------------------------------------------
// Song and Track Pattern definition
//
#define SUBTUNE_NAME_MAX				64								// Maximum length of Subtune name
#define SONGLINE_MAX					256								// 0-255 inclusive, Songline index used in Song
#define TRACK_CHANNEL_MAX				8								// 0-7 inclusive, 2 POKEY soundchips, each using 4 Channels, a typical Stereo configuration
#define TRACK_LENGTH_MAX				256								// 0-255 inclusive, Row index used in Pattern
#define TRACK_PATTERN_MAX				256								// 0-255 inclusive, Pattern index used in Song
#define PATTERN_COLUMN_MAX				8								// 0-7 inclusive, Pattern column index used for Note, Instrument, Volume, and Effect Commands
#define PATTERN_ACTIVE_EFFECT_MAX		4								// 0-3 inclusive, Number of Active Effect columns in Track Channel
#define PATTERN_NOTE_MAX				96								// 0-95 inclusive, Note index used in Pattern
#define PATTERN_INSTRUMENT_MAX			64								// 0-63 inclusive, Instrument index used in Pattern
#define PATTERN_VOLUME_MAX				16								// 0-15 inclusive, Volume index used in Pattern
#define PATTERN_EFFECT_MAX				16								// 0-15 inclusive, Effect index used in Pattern

// ----------------------------------------------------------------------------
// Instrument definition
//
#define INSTRUMENT_NAME_MAX				64								// Maximum length of instrument name
#define INSTRUMENT_PARAMETER_MAX		24								// Instrument parameter 0-23, inclusive
#define ENVELOPE_INDEX_MAX				48								// Instrument envelope 0-47, inclusive
#define ENVELOPE_PARAMETER_MAX			8								// Instrument envelope parameter 0-7, inclusive
#define TABLE_INDEX_MAX					32								// Instrument note/freq table 0-31, inclusive

// ----------------------------------------------------------------------------
// Effect Command definition
//
#define EFFECT_PARAMETER_MAX			256								// 0-255 inclusive, Effect $XY Parameter used in Pattern
#define EFFECT_PARAMETER_MIN			0x00							// The $XY Parameter of 0 may be used to disable certain Effect Commands
#define EFFECT_PARAMETER_DEFAULT		0x80							// The $XY Parameter of 128 may be used to disable certain Effect Commands
#define NOTE_OFF						PATTERN_NOTE_MAX				// The Note Command OFF will stop the last played note in the Track Channel
#define NOTE_RELEASE					(PATTERN_NOTE_MAX + 1)			// The Note Command === will release the last played note in the Track Channel
#define NOTE_RETRIGGER					(PATTERN_NOTE_MAX + 2)			// The Note Command ~~~ will retrigger the last played note in the Track Channel

struct Song
{
	// TODO
};

struct Pattern
{
	// TODO
};

struct Row
{
	// TODO
};

struct Instrument
{
	// TODO
};

struct Parameter
{
	// TODO
};
