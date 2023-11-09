// Computer Keyboard Mapping configuration and Input Handler functions for Raster Music Tracker
// By VinsCool, 2023

#pragma once

#include "General.h"
#include "global.h"


// ----------------------------------------------------------------------------
// Aliases for Virtual Key definitions, to make them easier to remember
//
#define VK_BACKSPACE	VK_BACK
#define VK_ENTER		VK_RETURN
#define VK_PAGE_UP		VK_PRIOR
#define VK_PAGE_DOWN	VK_NEXT
#define VK_SPACEBAR		VK_SPACE


// ----------------------------------------------------------------------------
// ASCII Virtual Key constants, since they are not defined in WinUser.h
//
typedef enum virtualKey_t : BYTE
{
	VK_0 = 0x30,
	VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
	VK_A = 0x41,
	VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N, VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,
} TVirtualKey;


// ----------------------------------------------------------------------------
// Keyboard layouts used with RMT for Notes input, or really anything related to editing
//
typedef enum keyboardLayout_t : BYTE
{
	KB_LAYOUT_QWERTY = 0,
	KB_LAYOUT_AZERTY,
	// Insert additional layouts here
	KB_LAYOUT_COUNT,
} TKeyboardLayout;


// ----------------------------------------------------------------------------
// Note keys for the Standard Western Notation
//
typedef enum noteKeyStandard_t : BYTE
{
	NOTE_C_0, NOTE_CS0, NOTE_D_0, NOTE_DS0, NOTE_E_0, NOTE_F_0, NOTE_FS0, NOTE_G_0, NOTE_GS0, NOTE_A_0, NOTE_AS0, NOTE_B_0,
	NOTE_C_1, NOTE_CS1, NOTE_D_1, NOTE_DS1, NOTE_E_1, NOTE_F_1, NOTE_FS1, NOTE_G_1, NOTE_GS1, NOTE_A_1, NOTE_AS1, NOTE_B_1,
	NOTE_C_2, NOTE_CS2, NOTE_D_2, NOTE_DS2, NOTE_E_2, NOTE_F_2, NOTE_FS2, NOTE_G_2, NOTE_GS2, NOTE_A_2, NOTE_AS2, NOTE_B_2,
	NOTE_OCTAVE = NOTE_C_2 - NOTE_C_1,
} TNoteKeyStandard;


// ----------------------------------------------------------------------------
// Constants for all actions that could be assigned to a key combination
//
typedef enum keyBindingAction_t
{
	// Movement actions
	AB_MOVE_UP, AB_MOVE_DOWN, AB_MOVE_LEFT, AB_MOVE_RIGHT,
	AB_MOVE_SONGLINE_UP, AB_MOVE_SONGLINE_DOWN,
	AB_MOVE_CHANNEL_LEFT, AB_MOVE_CHANNEL_RIGHT,
	AB_MOVE_SUBTUNE_LEFT, AB_MOVE_SUBTUNE_RIGHT,
	AB_MOVE_PATTERN_EDITOR, AB_MOVE_INSTRUMENT_EDITOR, AB_MOVE_INFO_EDITOR, AB_MOVE_SONGLINE_EDITOR,
	AB_MOVE_TAB_LEFT, AB_MOVE_TAB_RIGHT,
	AB_MOVE_PAGE_UP, AB_MOVE_PAGE_DOWN,

	// File I/O actions
	AB_FILE_NEW, AB_FILE_LOAD, AB_FILE_RELOAD, AB_FILE_SAVE, AB_FILE_IMPORT, AB_FILE_EXPORT,

	// Clipboard actions
	AB_CLIPBOARD_COPY, AB_CLIPBOARD_CUT, AB_CLIPBOARD_PASTE, AB_CLIPBOARD_PASTE_MERGE,
	AB_CLIPBOARD_SELECT_ALL,

	// Undo/Redo actions
	AB_UNDO_UNDO, AB_UNDO_REDO,

	// Editor actions
	AB_EDIT_CLEAR, AB_EDIT_EMPTY, AB_EDIT_DELETE, AB_EDIT_INSERT,
	AB_EDIT_DUPLICATE, AB_EDIT_NEW,
	AB_EDIT_INCREMENT, AB_EDIT_DECREMENT,
	AB_EDIT_INCREMENT_MORE, AB_EDIT_DECREMENT_MORE,

	// Note Transposition actions
	AB_TRANSPOSE_NOTE_UP, AB_TRANSPOSE_NOTE_DOWN,
	AB_TRANSPOSE_PATTERN_UP, AB_TRANSPOSE_PATTERN_DOWN,
	AB_TRANSPOSE_SONGLINE_UP, AB_TRANSPOSE_SONGLINE_DOWN,
	AB_TRANSPOSE_NOTE_UP_MORE, AB_TRANSPOSE_NOTE_DOWN_MORE,
	AB_TRANSPOSE_PATTERN_UP_MORE, AB_TRANSPOSE_PATTERN_DOWN_MORE,
	AB_TRANSPOSE_SONGLINE_UP_MORE, AB_TRANSPOSE_SONGLINE_DOWN_MORE,

	// Note Command actions
	AB_NOTE_OFF, AB_NOTE_RELEASE, AB_NOTE_RETRIGGER,

	// Playback actions
	AB_PLAY_START, AB_PLAY_PATTERN, AB_PLAY_CURSOR, AB_PLAY_BOOKMARK, AB_PLAY_STOP,

	// Active variables actions
	AB_ACTIVE_OCTAVE_UP, AB_ACTIVE_OCTAVE_DOWN,
	AB_ACTIVE_VOLUME_UP, AB_ACTIVE_VOLUME_DOWN,
	AB_ACTIVE_STEP_UP, AB_ACTIVE_STEP_DOWN,
	AB_ACTIVE_INSTRUMENT_LEFT, AB_ACTIVE_INSTRUMENT_RIGHT,

	// Toggle actions
	AB_TOGGLE_EDIT_MODE, AB_TOGGLE_FOLLOW, AB_TOGGLE_REGION, AB_TOGGLE_VOLUME_MASK, AB_TOGGLE_INSTRUMENT_MASK,
	AB_TOGGLE_CHANNEL_1, AB_TOGGLE_CHANNEL_2, AB_TOGGLE_CHANNEL_3, AB_TOGGLE_CHANNEL_4, AB_TOGGLE_CHANNEL_5, AB_TOGGLE_CHANNEL_6, AB_TOGGLE_CHANNEL_7, AB_TOGGLE_CHANNEL_8,
	AB_TOGGLE_CHANNEL_9, AB_TOGGLE_CHANNEL_10, AB_TOGGLE_CHANNEL_11, AB_TOGGLE_CHANNEL_12, AB_TOGGLE_CHANNEL_13, AB_TOGGLE_CHANNEL_14, AB_TOGGLE_CHANNEL_15, AB_TOGGLE_CHANNEL_16,
	AB_TOGGLE_ACTIVE_CHANNEL, AB_TOGGLE_ALL_CHANNELS,

	// Bookmark actions
	AB_BOOKMARK_CREATE, AB_BOOKMARK_DELETE,

	// Misc. actions that don't quite fit a category
	AB_MISC_INCREMENT_COMMAND_COLUMN, AB_MISC_DECREMENT_COMMAND_COLUMN,
	AB_MISC_INCREMENT_HIGHLIGHT_PRIMARY, AB_MISC_DECREMENT_HIGHLIGHT_PRIMARY,
	AB_MISC_INCREMENT_HIGHLIGHT_SECONDARY, AB_MISC_DECREMENT_HIGHLIGHT_SECONDARY,
	AB_MISC_SCALE_UP, AB_MISC_SCALE_DOWN,

	// Used for testing the Keyboard Input Handler functionalities, useless otherwise
	AB_TEST_COMBO,

	// Insert additional actions here
	AB_COUNT,
} TKeyBindingAction;


// ----------------------------------------------------------------------------
// Structs for Key combinations that could be binded to specific actions, designed to be easy to customise
//

typedef struct keyModifier_t
{
	bool keyCtrl;
	bool keyAlt;
	bool keyShift;
} TKeyModifier;

typedef struct keyCombo_t
{
	BYTE keyVirtual;
	TKeyModifier keyModifier;
} TKeyCombo;

typedef struct keyBindingIndex_t
{
	TKeyCombo keyCombo[AB_COUNT];
} TKeyBindingIndex;


// ----------------------------------------------------------------------------
// Computer Keyboard Mapping and Input Handler Class
// For pretty much everything related to the Computer Keyboard functionalities
//
class CKeyboard
{
public:
	CKeyboard();
	~CKeyboard();

public:
	void SetDefaultKeyBinding();

	UINT GetKeyBindingAction(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift);
	UINT GetNoteKey(UINT scanCode, bool keyCtrl, bool keyAlt, bool keyShift);
	UINT GetNumberKey(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift);
	UINT GetCommandKey(UINT keyVirtual, bool keyCtrl, bool keyAlt, bool keyShift);

	// Return the last known state for held Modifier keys
	// The Left Modifier keys are assumed by default, unless it is specified otherwise
	const bool IsPressingAlt(bool isRightModifierKey = false) { return isRightModifierKey ? (GetKeyState(VK_RMENU) & 0x8000) : (GetKeyState(VK_LMENU) & 0x8000); };
	const bool IsPressingCtrl(bool isRightModifierKey = false) { return isRightModifierKey ? (GetKeyState(VK_RCONTROL) & 0x80) : (GetKeyState(VK_LCONTROL) & 0x80); };
	const bool IsPressingShift(bool isRightModifierKey = false) { return isRightModifierKey ? (GetKeyState(VK_RSHIFT) & 0x80) : (GetKeyState(VK_LSHIFT) & 0x80); };

	// Unused, but technically functional if I really wanted to use it for some reason...
	const bool IsPressingAnyKey(UINT keyVirtual) { return (GetKeyState(keyVirtual) & 0x80); };

private:
	TKeyBindingIndex m_keyBinding;
};
