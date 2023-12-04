// Computer Keyboard Mapping configuration and Input Handler functions for Raster Music Tracker
// By VinsCool, 2023

#include "stdafx.h"

#include "Keyboard.h"
#include "ModuleV2.h"


CKeyboard::CKeyboard()
{
	SetDefaultKeyBinding();
}

CKeyboard::~CKeyboard()
{
	// Destructor
}

// Set all Action Key Bindings to the default parameters
// Unused or Optional combinations may be left blank on purpose
void CKeyboard::SetDefaultKeyBinding()
{
	// Clear all existing combinations
	memset(&m_keyBinding, 0x00, sizeof(m_keyBinding));

	// Assign the default parameters for all Virtual Key code and Modifier Key flags
	// Values are set in this order:		VK		CTRL		ALT		SHIFT
	m_keyBinding.keyCombo[AB_MOVE_UP] = { VK_UP, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_DOWN] = { VK_DOWN, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_LEFT] = { VK_LEFT, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_RIGHT] = { VK_RIGHT, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_SONGLINE_UP] = { VK_UP, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_SONGLINE_DOWN] = { VK_DOWN, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_CHANNEL_LEFT] = { VK_LEFT, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_CHANNEL_RIGHT] = { VK_RIGHT, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_SUBTUNE_LEFT] = { VK_PAGE_UP, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_SUBTUNE_RIGHT] = { VK_PAGE_DOWN, false, true, false };
	m_keyBinding.keyCombo[AB_MOVE_PATTERN_EDITOR] = { VK_F1, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_INSTRUMENT_EDITOR] = { VK_F2, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_INFO_EDITOR] = { VK_F3, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_SONGLINE_EDITOR] = { VK_F4, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_TAB_LEFT] = { VK_TAB, false, false, true };
	m_keyBinding.keyCombo[AB_MOVE_TAB_RIGHT] = { VK_TAB, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_PAGE_UP] = { VK_PAGE_UP, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_PAGE_DOWN] = { VK_PAGE_DOWN, false, false, false };
	m_keyBinding.keyCombo[AB_FILE_NEW] = { VK_W, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_LOAD] = { VK_O, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_RELOAD] = { VK_R, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_SAVE] = { VK_S, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_IMPORT] = { VK_I, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_EXPORT] = { VK_E, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_COPY] = { VK_C, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_CUT] = { VK_X, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_PASTE] = { VK_V, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_PASTE_MERGE] = { VK_M, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_SELECT_ALL] = { VK_A, true, false, false };
	m_keyBinding.keyCombo[AB_UNDO_UNDO] = { VK_Z, true, false, false };
	m_keyBinding.keyCombo[AB_UNDO_REDO] = { VK_Y, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_CLEAR] = { VK_SPACEBAR, false, false, false };
	m_keyBinding.keyCombo[AB_EDIT_EMPTY] = { VK_BACKSPACE, false, false, false };
	m_keyBinding.keyCombo[AB_EDIT_DELETE] = { VK_DELETE, false, false, false };
	m_keyBinding.keyCombo[AB_EDIT_INSERT] = { VK_INSERT, false, false, false };
	m_keyBinding.keyCombo[AB_EDIT_DUPLICATE] = { VK_D, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_NEW] = { VK_N, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_INCREMENT] = { VK_RIGHT, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_DECREMENT] = { VK_LEFT, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_INCREMENT_MORE] = { VK_UP, true, false, false };
	m_keyBinding.keyCombo[AB_EDIT_DECREMENT_MORE] = { VK_DOWN, true, false, false };
	m_keyBinding.keyCombo[AB_TRANSPOSE_NOTE_UP] = { VK_F2, true, false, false };
	m_keyBinding.keyCombo[AB_TRANSPOSE_NOTE_DOWN] = { VK_F1, true, false, false };
	m_keyBinding.keyCombo[AB_TRANSPOSE_PATTERN_UP] = { VK_F2, true, false, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_PATTERN_DOWN] = { VK_F1, true, false, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_SONGLINE_UP] = { VK_F2, true, true, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_SONGLINE_DOWN] = { VK_F1, true, true, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_NOTE_UP_MORE] = { VK_F4, true, false, false };
	m_keyBinding.keyCombo[AB_TRANSPOSE_NOTE_DOWN_MORE] = { VK_F3, true, false, false };
	m_keyBinding.keyCombo[AB_TRANSPOSE_PATTERN_UP_MORE] = { VK_F4, true, false, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_PATTERN_DOWN_MORE] = { VK_F3, true, false, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_SONGLINE_UP_MORE] = { VK_F4, true, true, true };
	m_keyBinding.keyCombo[AB_TRANSPOSE_SONGLINE_DOWN_MORE] = { VK_F3, true, true, true };
	m_keyBinding.keyCombo[AB_NOTE_OFF] = { VK_OEM_MINUS, false, false, false };
	m_keyBinding.keyCombo[AB_NOTE_RELEASE] = { VK_OEM_PLUS, false, false, false };
	m_keyBinding.keyCombo[AB_NOTE_RETRIGGER] = { VK_OEM_6, false, false, false };
	m_keyBinding.keyCombo[AB_PLAY_START] = { VK_F5, false, false, false };
	m_keyBinding.keyCombo[AB_PLAY_PATTERN] = { VK_F6, false, false, false };
	m_keyBinding.keyCombo[AB_PLAY_CURSOR] = { VK_F7, false, false, false };
	m_keyBinding.keyCombo[AB_PLAY_BOOKMARK] = { VK_F8, false, false, false };
	m_keyBinding.keyCombo[AB_PLAY_STOP] = { VK_ESCAPE, false, false, false };
	m_keyBinding.keyCombo[AB_ACTIVE_OCTAVE_UP] = { VK_MULTIPLY, false, false, false };
	m_keyBinding.keyCombo[AB_ACTIVE_OCTAVE_DOWN] = { VK_DIVIDE, false, false, false };
	m_keyBinding.keyCombo[AB_ACTIVE_VOLUME_UP] = { VK_ADD, false, false, false };
	m_keyBinding.keyCombo[AB_ACTIVE_VOLUME_DOWN] = { VK_SUBTRACT, false, false, false };
	m_keyBinding.keyCombo[AB_ACTIVE_STEP_UP] = { VK_MULTIPLY, false, false, true };
	m_keyBinding.keyCombo[AB_ACTIVE_STEP_DOWN] = { VK_DIVIDE, false, false, true };
	m_keyBinding.keyCombo[AB_ACTIVE_INSTRUMENT_LEFT] = { VK_SUBTRACT, false, false, true };
	m_keyBinding.keyCombo[AB_ACTIVE_INSTRUMENT_RIGHT] = { VK_ADD, false, false, true };
	m_keyBinding.keyCombo[AB_TOGGLE_EDIT_MODE] = { VK_SPACEBAR, true, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_FOLLOW] = { VK_F12, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_REGION] = { VK_F10, false, false, false };

	// Unmapped combinations
	m_keyBinding.keyCombo[AB_TOGGLE_VOLUME_MASK] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_INSTRUMENT_MASK] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_1] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_2] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_3] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_4] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_5] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_6] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_7] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_8] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_9] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_10] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_11] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_12] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_13] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_14] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_15] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_CHANNEL_16] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_ACTIVE_CHANNEL] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_ALL_CHANNELS] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_BOOKMARK_CREATE] = { EMPTY, false, false, false };
	m_keyBinding.keyCombo[AB_BOOKMARK_DELETE] = { EMPTY, false, false, false };

	// Mapped combinations
	m_keyBinding.keyCombo[AB_MISC_INCREMENT_COMMAND_COLUMN] = { VK_RIGHT, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_DECREMENT_COMMAND_COLUMN] = { VK_LEFT, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_INCREMENT_HIGHLIGHT_PRIMARY] = { VK_ADD, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_DECREMENT_HIGHLIGHT_PRIMARY] = { VK_SUBTRACT, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_INCREMENT_HIGHLIGHT_SECONDARY] = { VK_MULTIPLY, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_DECREMENT_HIGHLIGHT_SECONDARY] = { VK_DIVIDE, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_SCALE_UP] = { VK_ADD, true, true, true };
	m_keyBinding.keyCombo[AB_MISC_SCALE_DOWN] = { VK_SUBTRACT, true, true, true };

	// Test combination, do not use!
	m_keyBinding.keyCombo[AB_TEST_COMBO] = { VK_ENTER, true, false, true };
}

// Return the Action to execute from a Key Combination
UINT CKeyboard::GetKeyBindingAction(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift)
{
	// Test all known Combinations
	for (UINT i = 0; i < AB_COUNT; i++)
	{
		TKeyCombo* keyCombo = &m_keyBinding.keyCombo[i];

		// If an Action Key Binding matched the Key Combination, return i as the Action Identifier
		if (keyCombo->keyVirtual == keyVirtual
			&& keyCombo->keyModifier.keyCtrl == keyCtrl
			&& keyCombo->keyModifier.keyAlt == keyAlt
			&& keyCombo->keyModifier.keyShift == keyShift)
			return i;
	}

	// None of the Action Key Bindings matched the Key Combination tested
	return INVALID;
}

// Return the Note key mapped to the chosen Keyboard layout
UINT CKeyboard::GetNoteKey(UINT scanCode, bool keyCtrl, bool keyAlt, bool keyShift)
{
	// First, make sure to isolate the Scan Code from the provided value since there may be additional flags set to it
	scanCode &= 0xFF;

	// Neither of the Modifier keys are supposed to be held down!
	if (!keyCtrl && !keyAlt && !keyShift)
	{
		switch (scanCode)
		{
		case 0x17: return NOTE_C_2;	// 3rd Octave (half of it)
		case 0x0A: return NOTE_CS2;
		case 0x18: return NOTE_D_2;
		case 0x0B: return NOTE_DS2;
		case 0x19: return NOTE_E_2;
		case 0x03: return NOTE_CS1;	// 2nd Octave (Black Keys)
		case 0x04: return NOTE_DS1;
		case 0x06: return NOTE_FS1;
		case 0x07: return NOTE_GS1;
		case 0x08: return NOTE_AS1;
		case 0x10: return NOTE_C_1;	// 2nd Octave (White Keys)
		case 0x11: return NOTE_D_1;
		case 0x12: return NOTE_E_1;
		case 0x13: return NOTE_F_1;
		case 0x14: return NOTE_G_1;
		case 0x15: return NOTE_A_1;
		case 0x16: return NOTE_B_1;
		case 0x33: return NOTE_C_1;	// 2nd Octave (half of it)
		case 0x26: return NOTE_CS1;
		case 0x34: return NOTE_D_1;
		case 0x27: return NOTE_DS1;
		case 0x35: return NOTE_E_1;
		case 0x1F: return NOTE_CS0;	// 1st Octave (Black Keys)
		case 0x20: return NOTE_DS0;
		case 0x22: return NOTE_FS0;
		case 0x23: return NOTE_GS0;
		case 0x24: return NOTE_AS0;
		case 0x2C: return NOTE_C_0;	// 1st Octave (White Keys)
		case 0x2D: return NOTE_D_0;
		case 0x2E: return NOTE_E_0;
		case 0x2F: return NOTE_F_0;
		case 0x30: return NOTE_G_0;
		case 0x31: return NOTE_A_0;
		case 0x32: return NOTE_B_0;
		}
	}

	return INVALID;
}

// Return the Number key mapped to the chosen Keyboard layout
UINT CKeyboard::GetNumberKey(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift)
{
	// Neither of the Modifier keys are supposed to be held down!
	if (!keyCtrl && !keyAlt && !keyShift)
	{
		switch (keyVirtual)
		{
		case VK_0: case VK_NUMPAD0: return 0x0;
		case VK_1: case VK_NUMPAD1: return 0x1;
		case VK_2: case VK_NUMPAD2: return 0x2;
		case VK_3: case VK_NUMPAD3: return 0x3;
		case VK_4: case VK_NUMPAD4: return 0x4;
		case VK_5: case VK_NUMPAD5: return 0x5;
		case VK_6: case VK_NUMPAD6: return 0x6;
		case VK_7: case VK_NUMPAD7: return 0x7;
		case VK_8: case VK_NUMPAD8: return 0x8;
		case VK_9: case VK_NUMPAD9: return 0x9;
		case VK_A: return 0xA;
		case VK_B: return 0xB;
		case VK_C: return 0xC;
		case VK_D: return 0xD;
		case VK_E: return 0xE;
		case VK_F: return 0xF;
		}
	}

	return INVALID;
}

// Return the Command key mapped to the chosen Keyboard layout
UINT CKeyboard::GetCommandKey(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift)
{
	// Neither of the Modifier keys are supposed to be held down!
	if (!keyCtrl && !keyAlt && !keyShift)
	{
		switch (keyVirtual)
		{
		case VK_0: return PE_ARPEGGIO;
		case VK_1: return PE_PITCH_UP;
		case VK_2: return PE_PITCH_DOWN;
		case VK_3: return PE_PORTAMENTO;
		case VK_4: return PE_VIBRATO;
		case VK_A: return PE_VOLUME_FADE;
		case VK_B: return PE_GOTO_SONGLINE;
		case VK_D: return PE_END_PATTERN;
		case VK_F: return PE_SET_SPEED;
		case VK_P: return PE_SET_FINETUNE;
		case VK_G: return PE_SET_DELAY;
		}
	}

	return INVALID;
}
