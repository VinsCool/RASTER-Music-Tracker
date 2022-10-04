/*
 * Atari SAP-R File Compressor
 * ---------------------------
 *
 * This implementa an optimal LZSS compressor for the SAP-R music files.
 * The compressed files can be played in an Atari using the included
 * assembly programs, depending on the specific parameters.
 *
 * (c) 2020 DMSC
 * Code under MIT license, see LICENSE file.
 * C++ port for Raster Music Tracker by VinsCool, 2022
 */

#pragma once

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include <fstream>	/* needed for Load/SaveBinaryFile */
using namespace std;

#include "getopt.h"
#include "stdafx.h"
#include "libfmemopen.h"	/* needed for using input/output of data through pointers */

extern CString g_prgpath;

static void init(struct bf* x);
static void bflush(struct bf* x);
static void add_bit(struct bf* x, int bit);
static void add_byte(struct bf* x, int byte);
static void add_hbyte(struct bf* x, int hbyte);
static int maximum(int a, int b);
static int get_mlen(const uint8_t* a, const uint8_t* b, int max);
int hsh(const uint8_t* p);
static void lzop_init(struct lzop* lz, const uint8_t* data, int size);
static void lzop_free(struct lzop* lz);
static int match(const uint8_t* data, int pos, int size, int* mpos);
static void lzop_backfill(struct lzop* lz, int last_literal);
static int lzop_last_is_match(const struct lzop* lz);
static int lzop_encode(struct bf* b, const struct lzop* lz, int pos, int lpos);
int LZSS_SAP(unsigned char* src, int srclen);
