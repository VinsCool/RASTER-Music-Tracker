#pragma once

#include <fstream>
#include "General.h"

extern CString GetFilePath(CString pathandfilename);

extern BOOL NextSegment(std::ifstream& in);
extern char CharH4(unsigned char b);
extern char CharL4(unsigned char b);

extern void Trimstr(char* txt);
extern int Hexstr(char* txt, int len);

extern UINT CRC32(BYTE* data, UINT64 size);