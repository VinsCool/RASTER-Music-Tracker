#include "stdafx.h"

#include "IOHelpers.h"


CString GetFilePath(CString pathandfilename)
{
	//vstup:  c:/neco/nekde/kdovikde\nebo\taky\tohle.ext
	//vystup: c:/neco/nekde/kdovikde\nebo\taky
	//(just from the beginning to the last slash (/ or \)
	CString res;
	int pos = pathandfilename.ReverseFind('/');
	if (pos < 0) pos = pathandfilename.ReverseFind('\\');
	if (pos >= 0)
		res = pathandfilename.Mid(0, pos); //from 0 to pos
	else
		res = "";
	return res;
}

BOOL NextSegment(std::ifstream& in)
{
	char b;
	while (!in.eof())
	{
		in.read((char*)&b, 1);
		if (b == '[') return 1;	//end of segment (beginning of something else)
	}
	return 0;
}

char CharH4(unsigned char b)
{
	BYTE i = b >> 4; 
	return ((BYTE)i + ((i < 10) ? 48 : 55));
}

char CharL4(unsigned char b)
{
	BYTE i = b & 0x0f; 
	return ((BYTE)i + ((i < 10) ? 48 : 55));
}

void Trimstr(char* txt)
{
	char a;
	for (int i = 0; (a = txt[i]); i++)
	{
		if (a == 13 || a == 10)
		{
			txt[i] = 0;
			return;
		}
	}
}

/// <summary>
/// Parse an X character long HEX string into an integer.
/// If the first character is not 0-9A-F then the return value is 01
/// </summary>
/// <param name="txt">HEX string</param>
/// <param name="len">Length of string</param>
/// <returns>Parsed HEX or -1 on failure</returns>
int Hexstr(char* txt, int len)
{
	int r = 0;
	char a;
	int i;
	for (i = 0; (a = txt[i]) && i < len; i++)
	{
		if (a >= '0' && a <= '9')
			r = (r << 4) + (a - '0');
		else if (a >= 'A' && a <= 'F')
			r = (r << 4) + (a - 'A' + 10);
		else
		{
			if (i == 0) r = -1; //nothing
			return r;
		}
	}
	if (i == 0) r = -1; //nothing
	return r;
}

// A very rudimentary CRC32 algorithm designed for data integrity checks
// Useful for the verification of Raster Music Tracker Extended (RMTE) Module files
UINT CRC32(BYTE* data, UINT64 size)
{
	// Initialise the CRC32 with all bits set to 1
	UINT crc = INVALID;
	
	// Run the calculations through the entire data size
	for (UINT64 i = 0; i < size; i++)
	{
		// Fetch a byte from the current data offset
		BYTE bitbyte = data[i];

		// Cycle through all 8 bits within that byte
		for (UINT j = 0; j < 8; j++)
		{
			// If the CRC32 inversion using the byte has bit 0 set, Shift Right 1 bit then invert it using the Magic Number
			if ((crc ^ bitbyte) & 1)
				crc = (crc >> 1) ^ 0xDEADCAFE;

			// Otherwise, simply Shift Right 1 bit from it
			else
				crc >>= 1;

			// Shift Right 1 bit from the byte and process the remaining bits
			bitbyte >>= 1;
		}
	}

	// Invert all bits and return the Checksum
	return crc ^ INVALID;
}
