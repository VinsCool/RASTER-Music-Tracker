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

#include "lzss_sap.h"

 ///////////////////////////////////////////////////////
 // Bit encoding functions
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

static void init(struct bf* x)
{
    x->total = 0;
    x->len = 0;
    x->bnum = 0;
    x->bpos = -1;
    x->hpos = -1;
}

static void bflush(struct bf* x)
{
    if (x->len)
        memcpy(x->out + x->total, x->buf, x->len);
    x->total += x->len;
    x->len = 0;
    x->bnum = 0;
    x->bpos = -1;
    x->hpos = -1;
}

static void add_bit(struct bf* x, int bit)
{
    if (x->bpos < 0)
    {
        // Adds a new byte holding bits
        x->bpos = x->len;
        x->bnum = 0;
        x->len++;
        x->buf[x->bpos] = 0;
    }
    if (bit)
        x->buf[x->bpos] |= 1 << x->bnum;
    x->bnum++;
    if (x->bnum == 8)
    {
        x->bpos = -1;
        x->bnum = 0;
    }
}

static void add_byte(struct bf* x, int byte)
{
    x->buf[x->len] = byte;
    x->len++;
}

static void add_hbyte(struct bf* x, int hbyte)
{
    if (x->hpos < 0)
    {
        // Adds a new byte holding half-bytes
        x->hpos = x->len;
        x->len++;
        x->buf[x->hpos] = hbyte & 0x0F;
    }
    else
    {
        // Fixes last h-byte
        x->buf[x->hpos] |= hbyte << 4;
        x->hpos = -1;
    }
}

///////////////////////////////////////////////////////
// LZSS compression functions
static int maximum(int a, int b)
{
    return a > b ? a : b;
}

static int get_mlen(const uint8_t* a, const uint8_t* b, int max)
{
    for (int i = 0; i < max; i++)
        if (a[i] != b[i])
            return i;
    return max;
}

int hsh(const uint8_t* p)
{
    size_t x = (size_t)p;
    return 0xFF & (x ^ (x >> 8) ^ (x >> 16) ^ (x >> 24));
}

static int bits_moff = 4;       // Number of bits used for OFFSET
static int bits_mlen = 4;       // Number of bits used for MATCH
static int min_mlen = 2;        // Minimum match length
static int fmt_literal_first = 0; // Always include first literal in the output
static int fmt_pos_start_zero = 0; // Match positions start at 0, else start at max

#define bits_literal (1+8)      // Number of bits for encoding a literal
#define bits_match (1 + bits_moff + bits_mlen)  // Bits for encoding a match

#define max_mlen (min_mlen + (1<<bits_mlen) -1) // Maximum match length
#define max_off (1<<bits_moff)  // Maximum offset

// Statistics
static int* stat_len;
static int* stat_off;

// Struct for LZ optimal parsing
struct lzop
{
    const uint8_t* data;// The data to compress
    int size;           // Data size
    int* bits;          // Number of bits needed to code from position
    int* mlen;          // Best match length at position (0 == no match);
    int* mpos;          // Best match offset at position
};

static void lzop_init(struct lzop* lz, const uint8_t* data, int size)
{
    lz->data = data;
    lz->size = size;

    lz->bits = (int*)calloc(sizeof(int), size);
    lz->mlen = (int*)calloc(sizeof(int), size);
    lz->mpos = (int*)calloc(sizeof(int), size);
}

static void lzop_free(struct lzop* lz)
{
    free(lz->bits);
    free(lz->mlen);
    free(lz->mpos);
}

// Returns maximal match length (and match position) at pos.
static int match(const uint8_t* data, int pos, int size, int* mpos)
{
    int mxlen = -maximum(-max_mlen, pos - size);
    int mlen = 0;
    for (int i = maximum(pos - max_off, 0); i < pos; i++)
    {
        int ml = get_mlen(data + pos, data + i, mxlen);
        if (ml > mlen)
        {
            mlen = ml;
            *mpos = pos - i;
        }
    }
    return mlen;
}

// Calculate optimal encoding from the end of stream.
// if last_literal is 1, we force the last byte to be encoded as a literal.
static void lzop_backfill(struct lzop* lz, int last_literal)
{
    // If no bytes, nothing to do
    if (!lz->size)
        return;

    if (last_literal)
    {
        // Forced last literal - process one byte less
        lz->mlen[lz->size - 1] = 0;
        lz->size--;
        if (!lz->size)
            return;
    }

    // Init last bits
    lz->bits[lz->size - 1] = bits_literal;

    // Go backwards in file storing best parsing
    for (int pos = lz->size - 2; pos >= 0; pos--)
    {
        // Get best match at this position
        int mp = 0;
        int ml = match(lz->data, pos, lz->size, &mp);

        // Init "no-match" case
        int best = lz->bits[pos + 1] + bits_literal;

        // Check all posible match lengths, store best
        lz->bits[pos] = best;
        lz->mpos[pos] = mp;
        for (int l = ml; l >= min_mlen; l--)
        {
            int b;
            if (pos + l < lz->size)
                b = lz->bits[pos + l] + bits_match;
            else
                b = 0;
            if (b < best)
            {
                best = b;
                lz->bits[pos] = best;
                lz->mlen[pos] = l;
                lz->mpos[pos] = mp;
            }
        }
    }
    // Fixup size again
    if (last_literal)
        lz->size++;
}

// Returns 1 if the coded stream would end in a match
static int lzop_last_is_match(const struct lzop* lz)
{
    int last = 0;
    for (int pos = 0; pos < lz->size; )
    {
        int mlen = lz->mlen[pos];
        if (mlen < min_mlen)
        {
            // Skip over one literal byte
            last = 0;
            pos++;
        }
        else
        {
            // Skip over one match
            pos = pos + mlen;
            last = 1;
        }
    }
    return last;
}

static int lzop_encode(struct bf* b, const struct lzop* lz, int pos, int lpos)
{
    if (pos <= lpos)
        return lpos;

    int mlen = lz->mlen[pos];
    int mpos = lz->mpos[pos];

    // Encode best from filled table
    if (mlen < min_mlen)
    {
        // No match, just encode the byte
        add_bit(b, 1);
        add_byte(b, lz->data[pos]);
        stat_len[0] ++;
        return pos;
    }
    else
    {
        int code_pos = (pos - mpos - (fmt_pos_start_zero ? 1 : 2)) & (max_off - 1);
        int code_len = mlen - min_mlen;

        add_bit(b, 0);
        if (bits_mlen + bits_moff <= 8)
            add_byte(b, (code_pos << bits_mlen) + code_len);
        else if (bits_mlen + bits_moff <= 12)
        {
            add_byte(b, (code_pos << (8 - bits_moff)) + (code_len & ((1 << (8 - bits_moff)) - 1)));
            add_hbyte(b, code_len >> (8 - bits_moff));
        }
        else
        {
            int mb = ((code_len + 1) << bits_moff) + code_pos;
            add_byte(b, mb & 0xFF);
            add_byte(b, mb >> 8);
        }

        stat_len[mlen] ++;
        stat_off[mpos] ++;
        return pos + mlen - 1;
    }
}

// Optimise the AUDC bytes
static void Optimise_AUDC(uint8_t* buf)
{
    for (int i = 0; i < 4; i++)
    {
        int audc = i * 2 + 1;
        int vol = buf[audc] & 0x0F;
        int dist = buf[audc] & 0xF0;

        // RMT will handle both the Proper Volume Only output, and the SAP-R dump patch for the Two-Tone Filter
        if (dist < 0xF0)
        {
            if (vol == 0)
                buf[audc] = 0; // No volume, ignore distortion bits

            else if (dist & 0x20)
                buf[audc] &= 0xBF; // No noise, ignore noise type bit
        }
    }
}

// Optimise the AUDCTL bytes, based on the values of AUDC
static void Optimise_AUDCTL(uint8_t* buf)
{
    // CH1 is mute, disable High Pass Filter in CH1+3
    if (!(buf[1] & 0x0F)) buf[8] &= 0xFB;

    // CH1 is mute and Join1+2 is not set, disable 1.79mhz clock in CH1
    if (!(buf[1] & 0x0F) && !(buf[8] & 0x10)) buf[8] &= 0xBF;

    // Both CH1 and CH2 are mute, disable 16-bit mode
    if (!(buf[1] & 0x0F) && !(buf[3] & 0x0F)) buf[8] &= 0xAF;

    // CH2 is mute, disable High Pass Filter in CH2+4
    if (!(buf[3] & 0x0F)) buf[8] &= 0xFD;

    // CH3 is mute and Join3+4 is not set, disable 1.79mhz clock in CH3, if Filter in CH1+3 is also disabled
    if (!(buf[5] & 0x0F) && !(buf[8] & 0x08) && !(buf[8] & 0x04)) buf[8] &= 0xDF;

    // Both CH3 and CH4 are mute, disable 16-bit mode
    if (!(buf[5] & 0x0F) && !(buf[7] & 0x0F)) buf[8] &= 0xF7;
}

// Optimise the AUDF bytes, based on the values of AUDCTL and AUDC
static void Optimise_AUDF(uint8_t* buf)
{
    for (int i = 0; i < 4; i++)
    {
        int audf = i * 2;
        int audc = audf + 1;
        int vol = buf[audc] & 0x0F;
        int audctl = buf[8];
        bool twotone = ((buf[1] & 0x10) && (buf[1] < 0xF0));

        // Check if there is no volume, and if the AUDCTL actually needs the AUDF
        if (!vol)
        {
            // This is literally a case by case situation, this is painful
            switch (i)
            {
            case 0:
                if (!(audctl & 0x04 || audctl & 0x10 || audctl & 0x40))
                    buf[audf] = 0;
                break;

            case 1:
                if (!(audctl & 0x02 || audctl & 0x10 || twotone))
                    buf[audf] = 0;
                break;

            case 2:
                if (!(audctl & 0x04 || audctl & 0x08 || audctl & 0x20))
                    buf[audf] = 0;
                break;

            case 3:
                if (!(audctl & 0x02 || audctl & 0x08))
                    buf[audf] = 0;
                break;
            }
        }
    }
}

// Hacked up version of main() by VinsCool, stripping out most options that aren't needed for RMT 
int LZSS_SAP(unsigned char* src, int srclen, unsigned char* dst, int optimisations)
{
    struct bf b;
    uint8_t buf[9], * data[9];
    int lpos[9];
    int show_stats = 0;
    int bits_mtotal = bits_moff + bits_mlen;
    int bits_set = 0;
    int force_last_literal = 1;
    int format_version = 0;  // LZSS format version - 0 means last version

    int opt = '6';  //LZ16 always, however this may be changed if needed

    switch (opt)
    {
    case '2':
        bits_moff = 7;
        bits_mlen = 5;
        bits_mtotal = 12;
        bits_set |= 8;
        break;
    case '8':
        bits_moff = 4;
        bits_mlen = 4;
        bits_mtotal = 8;
        bits_set |= 8;
        break;
    case '6':
        bits_moff = 8;
        bits_mlen = 8;
        bits_mtotal = 16;
        min_mlen = 1;
        bits_set |= 8;
        break;
    default:
        break;
    }

    // Set format flags:
    switch (format_version)
    {
    case 1:
        fmt_literal_first = 0;
        fmt_pos_start_zero = 1;
        break;
    default:
        fmt_literal_first = 1;
        fmt_pos_start_zero = 0;
        break;
    }

    // Calculate bits
    switch (bits_set)
    {
    case 0:
    case 1:
    case 4:
    case 5:
        bits_mlen = bits_mtotal - bits_moff;
        break;
    case 2:
    case 6:
        bits_moff = bits_mtotal - bits_mlen;
        break;
    case 3:
    case 8:
        // OK
        break;
    default:
        return 0;
        break;
    }

    // Alloc statistic arrays
    stat_len = (int*)calloc(sizeof(int), max_mlen + 1);
    stat_off = (int*)calloc(sizeof(int), max_off + 1);

    // Max size of each bufer: 128k
    for (int i = 0; i < 9; i++)
    {
        data[i] = (uint8_t*)malloc(128 * 1024);
        lpos[i] = -1;
    }

    // Read all data
    int sz = 0;
    int mem = 0;

    // Buffered bytes are loaded from source memory pointer
    for (sz = 0; mem < srclen && sz < (128 * 1024); sz++)
    {
        // SAP-R frames are processed in groups of 9 bytes, in this order: 
        // AUDF0, AUDC0, AUDF1, AUDC1, AUDF2, AUDC2, AUDF3, AUDC3, AUDCTL
        for (int i = 0; i < 9; i++) { buf[i] = src[mem + i]; }

        // Apply desired optimisations to the buffered bytes
        switch (optimisations)
        {
        case SAPR_OPTIMISATIONS_AUDC:
            Optimise_AUDC(buf);
            break;

        case SAPR_OPTIMISATIONS_AUDCTL:
            Optimise_AUDCTL(buf);
            break;

        case SAPR_OPTIMISATIONS_AUDF:
            Optimise_AUDF(buf);
            break;

        case SAPR_OPTIMISATIONS_AUDC_AUDF:
            Optimise_AUDC(buf);
            Optimise_AUDF(buf);
            break;

        case SAPR_OPTIMISATIONS_AUDCTL_AUDC:
            Optimise_AUDC(buf);
            Optimise_AUDCTL(buf);
            break;

        case SAPR_OPTIMISATIONS_AUDCTL_AUDF:
            Optimise_AUDCTL(buf);
            Optimise_AUDF(buf);
            break;

        case SAPR_OPTIMISATIONS_ALL:
            Optimise_AUDC(buf);
            Optimise_AUDCTL(buf);
            Optimise_AUDF(buf);
            break;
        }

        // Write the processed bytes once the optimisations were applied to them
        for (int i = 0; i < 9; i++) { data[i][sz] = buf[i]; }

        // Adjust the offset for the next buffer chunk
        mem += 9;
    }

    // Set the output to the destination memory pointer 
    b.out = dst;

    // Check for empty streams and warn
    int chn_skip[9];
    init(&b);
    for (int i = 8; i >= 0; i--)
    {
        const uint8_t* p = data[i], s = *p;
        int n = 0;
        for (int j = 0; j < sz; j++)
            if (*p++ != s)
                n++;
        if (i != 0 && !n)
        {
            if (show_stats)
                fprintf(stderr, "Skipping channel #%d, set with $%02x.\n", i, s);
            add_bit(&b, 1);
            chn_skip[i] = 1;
        }
        else
        {
            if (i)
                add_bit(&b, 0);
            chn_skip[i] = 0;
            if (!n)
            {
                fprintf(stderr, "WARNING: stream #%d ", i);
                if (s == 0)
                    fprintf(stderr, "is empty");
                else
                    fprintf(stderr, "contains only $%02X", s);
                fprintf(stderr, ", should not be included in output!\n");
            }
        }
    }
    bflush(&b);

    // Now, we store initial values for all chanels:
    for (int i = 8; i >= 0; i--)
    {
        // In version 1 we only store init byte for the skipped channels
        if (fmt_literal_first || chn_skip[i])
            add_byte(&b, *data[i]);
    }
    bflush(&b);

    // Init LZ states
    struct lzop lz[9];
    for (int i = 0; i < 9; i++)
    {
        if (!chn_skip[i])
        {
            lzop_init(&lz[i], data[i], sz);
            lzop_backfill(&lz[i], 0);
        }
    }

    // Detect if at least one of the streams end in a match:
    int end_not_ok = 1;
    for (int i = 0; i < 9; i++)
    {
        if (!chn_skip[i])
            end_not_ok &= lzop_last_is_match(&lz[i]);
    }

    // If all streams end in a match, we need to fix at least one to end in
    // a literal - just fix stream 0, as this is always encoded:
    if (force_last_literal && end_not_ok)
    {
        fprintf(stderr, "LZSS: fixing up stream #0 to end in a literal\n");
        lzop_backfill(&lz[0], 1);
    }
    else if (end_not_ok)
    {
        fprintf(stderr, "WARNING: stream does not end in a literal.\n");
        fprintf(stderr, "WARNING: this can produce errors at the end of decoding.\n");
    }

    // Compress
    for (int pos = fmt_literal_first ? 1 : 0; pos < sz; pos++)
    {
        for (int i = 8; i >= 0; i--)
        {
            if (!chn_skip[i])
                lpos[i] = lzop_encode(&b, &lz[i], pos, lpos[i]);
        }
    }
    bflush(&b);

    // Show stats
    fprintf(stderr, "LZSS: max offset= %d,\tmax len= %d,\tmatch bits= %d,\t",
        max_off, max_mlen, bits_match - 1);
    fprintf(stderr, "ratio: %5d / %d = %5.2f%%\n", b.total, 9 * sz, (100.0 * b.total) / (9.0 * sz));
    if (show_stats)
    {
        for (int i = 0; i < 9; i++)
        {
            if (!chn_skip[i])
            {
                fprintf(stderr, " Stream #%d: %d bits,\t%5.2f%%,\t%5.2f%% of output\n", i,
                    lz[i].bits[0], (100.0 * lz[i].bits[0]) / (8.0 * sz),
                    (100.0 * lz[i].bits[0]) / (8.0 * b.total));
            }
        }
    }

    if (show_stats > 1)
    {
        fprintf(stderr, "\nvalue\t  POS\t  LEN\n");
        for (int i = 0; i <= maximum(max_mlen, max_off); i++)
        {
            fprintf(stderr, "%2d\t%5d\t%5d\n", i,
                (i <= max_off) ? stat_off[i] : 0,
                (i <= max_mlen) ? stat_len[i] : 0);
        }
    }

    // Free memory
    for (int i = 0; i < 9; i++)
    {
        free(data[i]);
        if (!chn_skip[i])
            lzop_free(&lz[i]);
    }
    free(stat_len);
    free(stat_off);

    // Size of compressed data is returned, for use with the destination memory pointer
    return b.total;
}
