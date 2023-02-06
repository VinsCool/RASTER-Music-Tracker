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

#include "stdafx.h"
#include "General.h"

#define bits_literal (1+8)                      // Number of bits for encoding a literal
#define bits_match (1 + bits_moff + bits_mlen)  // Bits for encoding a match
#define max_mlen (min_mlen + (1<<bits_mlen) -1) // Maximum match length
#define max_off (1<<bits_moff)                  // Maximum offset

 // Struct for LZ optimal parsing
struct lzop
{
    const uint8_t* data;                        // The data to compress
    int size;                                   // Data size
    int* bits;                                  // Number of bits needed to code from position
    int* mlen;                                  // Best match length at position (0 == no match);
    int* mpos;                                  // Best match offset at position
};

struct bf
{
    int len;
    uint8_t buf[65536];
    int bnum;
    int bpos;
    int hpos;
    int total;
    unsigned char* out;
};

class CCompressLzss
{
public:
    CCompressLzss();
    int LZSS_SAP(unsigned char* src, int srclen, unsigned char* dst, int optimisations = SAPR_OPTIMISATIONS_AUDC);

private:
    int bits_moff;                              // Number of bits used for OFFSET
    int bits_mlen;                              // Number of bits used for MATCH
    int min_mlen;                               // Minimum match length
    int fmt_literal_first;                      // Always include first literal in the output
    int fmt_pos_start_zero;                     // Match positions start at 0, else start at max
    int* stat_len;                              // Statistics
    int* stat_off;

    void init(struct bf* x);
    void bflush(struct bf* x);
    void add_bit(struct bf* x, int bit);
    void add_byte(struct bf* x, int byte);
    void add_hbyte(struct bf* x, int hbyte);
    int maximum(int a, int b);
    int get_mlen(const uint8_t* a, const uint8_t* b, int max);
    int hsh(const uint8_t* p);
    void lzop_init(struct lzop* lz, const uint8_t* data, int size);
    void lzop_free(struct lzop* lz);
    int match(const uint8_t* data, int pos, int size, int* mpos);
    void lzop_backfill(struct lzop* lz, int last_literal);
    int lzop_last_is_match(const struct lzop* lz);
    int lzop_encode(struct bf* b, const struct lzop* lz, int pos, int lpos);
    void Optimise_AUDC(uint8_t* buf);
    void Optimise_AUDCTL(uint8_t* buf);
    void Optimise_AUDF(uint8_t* buf);
};