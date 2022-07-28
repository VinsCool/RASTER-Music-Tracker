#pragma once


struct Tshpar
{
	int c;
	int x, y;
	char* name;
	int pand;
	int pmax;
	int pfrom;
	int gup, gdw, gle, gri;
};

extern const Tshpar shpar[NUMBEROFPARS];



struct Tshenv
{
	char ch;
	int pand;
	int padd;
	int psub;
	char* name;
	int xpos;
	int ypos;
};

extern const Tshenv shenv[ENVROWS];
