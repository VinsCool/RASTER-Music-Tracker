// Computer Keyboard Mapping configuration and Input Handler functions for Raster Music Tracker
// By VinsCool, 2023

#include "stdafx.h"

#include "Keyboard.h"
#include "ModuleV2.h"


CKeyboard::CKeyboard()
{
	SetDefaultKeyBinding();
	CString layout = "A";
	GetKeyboardLayoutName((LPSTR&)layout);
	MessageBoxA(g_hwnd, layout, "test", MB_ICONINFORMATION);
}

CKeyboard::~CKeyboard()
{
	// Destructor
}

void CKeyboard::SetDefaultKeyBinding()
{
	// Clear all existing combinations
	memset(&m_keyBinding, 0x00, sizeof(m_keyBinding));

	// Assign Virtual Key code and Modifier Key flags
	// Values are set in this order:		VK		CTRL	ALT		SHIFT
	//
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
	m_keyBinding.keyCombo[AB_MOVE_HIGHLIGHT_PRIMARY_UP] = { VK_PAGE_UP, false, false, false };
	m_keyBinding.keyCombo[AB_MOVE_HIGHLIGHT_PRIMARY_DOWN] = { VK_PAGE_DOWN, false, false, false };
	m_keyBinding.keyCombo[AB_FILE_NEW] = { VK_W, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_LOAD] = { VK_O, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_RELOAD] = { VK_R, true, false, false };
	m_keyBinding.keyCombo[AB_FILE_SAVE] = { VK_S, true, false, false };
	// AB_FILE_IMPORT, AB_FILE_EXPORT,
	m_keyBinding.keyCombo[AB_CLIPBOARD_COPY] = { VK_C, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_CUT] = { VK_X, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_PASTE] = { VK_V, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_PASTE_MERGE] = { VK_M, true, false, false };
	m_keyBinding.keyCombo[AB_CLIPBOARD_SELECT_ALL] = { VK_A, true, false, false };
	// AB_CLIPBOARD_DESELECT,
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
	m_keyBinding.keyCombo[AB_ACTIVE_INSTRUMENT_LEFT] = { VK_LEFT, false, false, true };
	m_keyBinding.keyCombo[AB_ACTIVE_INSTRUMENT_RIGHT] = { VK_RIGHT, false, false, true };
	m_keyBinding.keyCombo[AB_TOGGLE_EDIT_MODE] = { VK_SPACEBAR, true, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_FOLLOW] = { VK_F12, false, false, false };
	m_keyBinding.keyCombo[AB_TOGGLE_REGION] = { VK_F10, false, false, false };
	// AB_TOGGLE_VOLUME_MASK, AB_TOGGLE_INSTRUMENT_MASK,
	// AB_TOGGLE_CHANNEL_1, AB_TOGGLE_CHANNEL_2, AB_TOGGLE_CHANNEL_3, AB_TOGGLE_CHANNEL_4, AB_TOGGLE_CHANNEL_5, AB_TOGGLE_CHANNEL_6, AB_TOGGLE_CHANNEL_7, AB_TOGGLE_CHANNEL_8,
	// AB_TOGGLE_CHANNEL_9, AB_TOGGLE_CHANNEL_10, AB_TOGGLE_CHANNEL_11, AB_TOGGLE_CHANNEL_12, AB_TOGGLE_CHANNEL_13, AB_TOGGLE_CHANNEL_14, AB_TOGGLE_CHANNEL_15, AB_TOGGLE_CHANNEL_16,
	// AB_TOGGLE_ACTIVE_CHANNEL, AB_TOGGLE_ALL_CHANNELS,
	// AB_BOOKMARK_CREATE, AB_BOOKMARK_DELETE,
	m_keyBinding.keyCombo[AB_MISC_INCREMENT_COMMAND_COLUMN] = { VK_RIGHT, false, true, true };
	m_keyBinding.keyCombo[AB_MISC_DECREMENT_COMMAND_COLUMN] = { VK_LEFT, false, true, true };
	// AB_MISC_INCREMENT_HIGHLIGHT_PRIMARY, AB_MISC_DECREMENT_HIGHLIGHT_PRIMARY,
	// AB_MISC_INCREMENT_HIGHLIGHT_SECONDARY, AB_MISC_DECREMENT_HIGHLIGHT_SECONDARY,
	// AB_MISC_SCALE_UP, AB_MISC_SCALE_DOWN,
	//
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
// Surely, there is a much better method I could use here...
UINT CKeyboard::GetNoteKey(UINT keyVirtual)
{
	UINT noteKey = INVALID;
	UINT octave = 0;

	if (g_keyboard_layout == KB_LAYOUT_QWERTY)
	{
		switch (keyVirtual)
		{
		case VK_I: octave++;
		case VK_Q:
		case VK_OEM_COMMA: octave++;
		case VK_Z: noteKey = NOTE_C;
			break;

		case VK_9: octave++;
		case VK_2:
		case VK_L: octave++;
		case VK_S: noteKey = NOTE_C_SHARP;
			break;

		case VK_O: octave++;
		case VK_W:
		case VK_OEM_PERIOD: octave++;
		case VK_X: noteKey = NOTE_D;
			break;

		case VK_0: octave++;
		case VK_3:
		case VK_OEM_1: octave++;
		case VK_D: noteKey = NOTE_D_SHARP;
			break;

		case VK_P: octave++;
		case VK_E:
		case VK_OEM_2: octave++;
		case VK_C: noteKey = NOTE_E;
			break;

		case VK_R: octave++;
		case VK_V: noteKey = NOTE_F;
			break;

		case VK_5: octave++;
		case VK_G: noteKey = NOTE_F_SHARP;
			break;

		case VK_T: octave++;
		case VK_B: noteKey = NOTE_G;
			break;

		case VK_6: octave++;
		case VK_H: noteKey = NOTE_G_SHARP;
			break;

		case VK_Y: octave++;
		case VK_N: noteKey = NOTE_A;
			break;

		case VK_7: octave++;
		case VK_J: noteKey = NOTE_A_SHARP;
			break;

		case VK_U: octave++;
		case VK_M: noteKey = NOTE_B;
			break;
		}
	}

	if (noteKey != INVALID)
		noteKey += (octave * NOTE_OCTAVE);

	return noteKey;
}

// Return the Number key mapped to the chosen Keyboard layout
UINT CKeyboard::GetNumberKey(UINT keyVirtual)
{
	UINT numberKey = INVALID;

	switch (keyVirtual)
	{
	case VK_0:
	case VK_NUMPAD0:
		numberKey = 0x0;
		break;

	case VK_1:
	case VK_NUMPAD1:
		numberKey = 0x1;
		break;

	case VK_2:
	case VK_NUMPAD2:
		numberKey = 0x2;
		break;

	case VK_3:
	case VK_NUMPAD3:
		numberKey = 0x3;
		break;

	case VK_4:
	case VK_NUMPAD4:
		numberKey = 0x4;
		break;

	case VK_5:
	case VK_NUMPAD5:
		numberKey = 0x5;
		break;

	case VK_6:
	case VK_NUMPAD6:
		numberKey = 0x6;
		break;

	case VK_7:
	case VK_NUMPAD7:
		numberKey = 0x7;
		break;

	case VK_8:
	case VK_NUMPAD8:
		numberKey = 0x8;
		break;

	case VK_9:
	case VK_NUMPAD9:
		numberKey = 0x9;
		break;

	case VK_A:
		numberKey = 0xA;
		break;

	case VK_B:
		numberKey = 0xB;
		break;

	case VK_C:
		numberKey = 0xC;
		break;

	case VK_D:
		numberKey = 0xD;
		break;

	case VK_E:
		numberKey = 0xE;
		break;

	case VK_F:
		numberKey = 0xF;
		break;
	}

	return numberKey;
}

// Return the Number key mapped to the chosen Keyboard layout
UINT CKeyboard::GetCommandKey(UINT keyVirtual)
{
	UINT commandKey = INVALID;

	switch (keyVirtual)
	{

	case VK_0:
		commandKey = PE_ARPEGGIO;
		break;

	case VK_1:
		commandKey = PE_PITCH_UP;
		break;

	case VK_2:
		commandKey = PE_PITCH_DOWN;
		break;

	case VK_3:
		commandKey = PE_PORTAMENTO;
		break;

	case VK_4:
		commandKey = PE_VIBRATO;
		break;

	case VK_A:
		commandKey = PE_VOLUME_FADE;
		break;

	case VK_B:
		commandKey = PE_GOTO_SONGLINE;
		break;

	case VK_D:
		commandKey = PE_END_PATTERN;
		break;

	case VK_F:
		commandKey = PE_SET_SPEED;
		break;

	case VK_P:
		commandKey = PE_SET_FINETUNE;
		break;

	case VK_G:
		commandKey = PE_SET_DELAY;
		break;
	}

	return commandKey;
}

/*
// QWERTY Keyboard layout 
const unsigned char keynotes_QWERTY[256] = 
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x1B, 0xFF, 0x0D, 0x0F, 0xFF, 0x12, 0x14, 0x16, 0xFF, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0x07, 0x04, 0x03, 0x10, 0xFF, 0x06, 0x08, 0x18, 0x0A, 0xFF, 0x0D, 0x0B, 0x09, 0x1A,
	0x1C, 0x0C, 0x11, 0x01, 0x13, 0x17, 0x05, 0x0E, 0x02, 0x15, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x1E, 0x0C, 0xFF, 0x0E, 0x10, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0xFF, 0x1F, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// AZERTY Keyboard layout 
const unsigned char keynotes_AZERTY[256] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x1B, 0xFF, 0x0D, 0x0F, 0xFF, 0x12, 0x14, 0x16, 0xFF, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0x0C, 0x07, 0x04, 0x03, 0x10, 0xFF, 0x06, 0x08, 0x18, 0x0A, 0xFF, 0x0D, 0x0F, 0x09, 0x1A,
	0x1C, 0xFF, 0x11, 0x01, 0x13, 0x17, 0x05, 0x00, 0x02, 0x15, 0x0E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x1E, 0x0B, 0xFF, 0x0C, 0x0E,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0xFF, 0x10,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

const char keynumbs[256] =
{
	//0
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	//64
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	//128
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	//192
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

const char keynumblock09[256] =
{
	//0
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	//64
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	//128
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	//192
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

// TODO: Replace with a different system, there are many issues with accidental values being input!
char NoteKey(int vk)
{
	switch (g_keyboard_layout)
	{
	case KEYBOARD_QWERTY:
		return keynotes_QWERTY[vk];

	case KEYBOARD_AZERTY:
		return keynotes_AZERTY[vk];

	default:
		return -1;	// return INVALID;
	}
}

char NumbKey(int vk)
{
	return keynumbs[vk];
}

char Numblock09Key(int vk)
{
	return keynumblock09[vk];
}
*/
