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
	if (!m_instrument) m_instrument = new TInstrument[PATTERN_INSTRUMENT_MAX];

	// Set all data to invalid, which will be assumed as empty	
	for (int i = 0; i < TRACK_CHANNEL_MAX; i++)
	{
		for (int j = 0; j < SONGLINE_MAX; j++)
		{
			m_index[i].songline[j] = INVALID;
			m_index[i].activeEffectCommand = 1;
		}

		for (int j = 0; j < TRACK_PATTERN_MAX; j++)
		{
			for (int k = 0; k < TRACK_LENGTH_MAX; k++)
			{
				m_index[i].pattern[j].row[k].note = INVALID;
				m_index[i].pattern[j].row[k].instrument = INVALID;
				m_index[i].pattern[j].row[k].volume = INVALID;
				m_index[i].pattern[j].row[k].cmd0 = INVALID;
				m_index[i].pattern[j].row[k].cmd1 = INVALID;
				m_index[i].pattern[j].row[k].cmd2 = INVALID;
				m_index[i].pattern[j].row[k].cmd3 = INVALID;
			}
		}
	}

	// Also clear all instruments in the module
	for (int i = 0; i < PATTERN_INSTRUMENT_MAX; i++)
	{
		*m_instrument[i].name = (BYTE)"New Instrument";
		m_instrument[i].envelopeLength= 1;
		m_instrument[i].tableLength = 1;

		for (int j = 0; j < ENVELOPE_INDEX_MAX; j++)
		{
			m_instrument[i].volumeEnvelope[j];
			m_instrument[i].distortionEnvelope[j];
			m_instrument[i].audctlEnvelope[j];
		}

		for (int j = 0; j < INSTRUMENT_TABLE_INDEX_MAX; j++)
		{
			m_instrument[i].noteTable[j];
			m_instrument[i].freqTable[j];
		}
	}

	// Set default module parameters
	m_songLength = MODULE_SONG_LENGTH;
	m_trackLength = MODULE_TRACK_LENGTH;
	m_trackChannelCount = MODULE_STEREO;

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
