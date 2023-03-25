// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"

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
	if (!m_index) m_index = new TIndex[TRACK_CHANNEL_MAX];
	if (!m_instrument) m_instrument = new TInstrumentV2[PATTERN_INSTRUMENT_MAX];

	// Clear all data, and set default values
	for (int i = 0; i < TRACK_CHANNEL_MAX; i++)
	{
		// Set all indexed Patterns to 0
		for (int j = 0; j < SONGLINE_MAX; j++)
			m_index[i].songline[j] = 0;

		// Set all indexed Rows in Patterns to empty values
		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			for (int k = 0; k < TRACK_ROW_MAX; k++)
			{
				m_index[i].pattern[j].row[k].note = PATTERN_NOTE_EMPTY;
				m_index[i].pattern[j].row[k].instrument = PATTERN_INSTRUMENT_EMPTY;
				m_index[i].pattern[j].row[k].volume = PATTERN_VOLUME_EMPTY;
				m_index[i].pattern[j].row[k].cmd0 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd1 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd2 = PATTERN_EFFECT_EMPTY;
				m_index[i].pattern[j].row[k].cmd3 = PATTERN_EFFECT_EMPTY;
			}
		}

		// By default, only 1 Effect Command is enabled in all Track Channels
		m_index[i].activeEffectCommand = 1;
	}

	// Also clear all instruments in the module
	for (int i = 0; i < PATTERN_INSTRUMENT_MAX; i++)
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

	// Set default module parameters
	m_songLength = MODULE_SONG_LENGTH;
	m_trackLength = MODULE_TRACK_LENGTH;
	m_channelCount = MODULE_STEREO;

	// Module was initialised
	m_initialised = TRUE;
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
	m_initialised = FALSE;
}

void CModule::ImportLegacyRMT()
{
	// TODO
}

/*
// Prototype code for making use of the new format...
	module = new Module;
	module->ClearModule();		// Initialise the module
	module->ImportLegacyRMT();	// Import legacy module if loaded in memory

...

// Process pattern data here, you get the idea
	note = module->Song[songline][channel]->Pattern[row]->Row[0]->Note;

	switch (note)
	{
	case INVALID:	// No data, nothing to do here
		break;

	case NOTE_OFF:	// Note Off command, the Track Channel must be stopped and reset
		ResetChannel(*module, channel);
		break;

	case NOTE_RELEASE:	// Note Release command, the last played Note will be released
		ReleaseNote(*module, note, channel);
		break;

	case NOTE_RETRIGGER:	// Note Retrigger command, the last played Note will be retriggered
		RetriggerNote(*module, note, channel);
		break;

	default:	// Play a new note
		PlayNote(*module, note, channel);
	}

...

// Etc, etc :D

}
*/
