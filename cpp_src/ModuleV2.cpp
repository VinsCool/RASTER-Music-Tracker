// RMT Module V2 Format Prototype
// By VinsCool, 2023

#include "StdAfx.h"
#include "ModuleV2.h"

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
