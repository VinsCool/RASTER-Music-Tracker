#pragma once

#include <fstream>

extern CString GetFilePath(CString pathandfilename);

extern BOOL NextSegment(std::ifstream& in);
extern char CharH4(unsigned char b);
extern char CharL4(unsigned char b);

extern void Trimstr(char* txt);
extern int Hexstr(char* txt, int len);