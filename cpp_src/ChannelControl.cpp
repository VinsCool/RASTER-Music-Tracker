#include "stdafx.h"
#include "ChannelControl.h"

#include "General.h"

#include "global.h"

// ----------------------------------------------------------------------------
// Channel On/Off helper functions

/// <summary>
/// Turn all or individual sound channels on or off
/// </summary>
/// <param name="ch">-1 = all channels, 0 - 7 = the sound channel</param>
/// <param name="onoff">-1 = invert state, 0 = off, 1 = on</param>
void SetChannelOnOff(int ch, int onoff)
{
	if (ch < 0)
	{
		// All channels
		if (onoff >= 0)
			for (int i = 0; i < SONGTRACKS; i++) g_channelon[i] = onoff;// set the given on/off state
		else
			for (int i = 0; i < SONGTRACKS; i++) g_channelon[i] ^= 1;	// invert the on/off state
	}
	else if (ch < SONGTRACKS)
	{
		// Just that one
		if (onoff >= 0)
			g_channelon[ch] = onoff;	// set the given on/off state
		else
			g_channelon[ch] ^= 1;		// invert the on/off state
	}
}

int GetChannelOnOff(int ch)
{
	return g_channelon[ch];
}

/// <summary>
/// Turn ON only one channel, making sure that all others are turned off
/// </summary>
/// <param name="ch"></param>
void SetChannelSolo(int ch)
{
	int on = GetChannelOnOff(ch);
	if (!on)
	{
		// Target channel is OFF
	Channel_SOLO:
		SetChannelOnOff(-1, 0);	// Turn all off
		SetChannelOnOff(ch, 1);	// and turn ON only the solo channel
	}
	else
	{
		// Target channel is ON
		// If any other channel is on then turn them all off except for the target channel
		for (int i = 0; i < g_tracks4_8; i++)
		{
			if (i != ch && GetChannelOnOff(i)) goto Channel_SOLO;
		}
		// All other channels are off, turn them all on
		SetChannelOnOff(-1, 1);	//turn them all on
	}
}

