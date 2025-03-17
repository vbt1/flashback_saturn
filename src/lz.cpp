#include <stdint.h>
#include <string.h>

#define LZ_MAX_OFFSET 4096  // Réduction de la taille de la fenêtre de recherche
#define HASH_TABLE_SIZE 4096

static unsigned int _LZ_StringCompare(const uint8_t *str1, const uint8_t *str2, unsigned int minlen, unsigned int maxlen) {
    unsigned int len;
    for (len = minlen; (len < maxlen) && (str1[len] == str2[len]); ++len);
    return len;
}

static int _LZ_WriteVarSize(unsigned int x, uint8_t *buf) {
    unsigned int y;
    int num_bytes, i, b;

    y = x >> 3;
    for (num_bytes = 5; num_bytes >= 2; --num_bytes) {
        if (y & 0xfe000000) break;
        y <<= 7;
    }

    for (i = num_bytes - 1; i >= 0; --i) {
        b = (x >> (i * 7)) & 0x0000007f;
        if (i > 0) {
            b |= 0x00000080;
        }
        *buf++ = (uint8_t)b;
    }

    return num_bytes;
}

static int _LZ_ReadVarSize(unsigned int *x, const uint8_t *buf) {
    unsigned int y, b, num_bytes;

    y = 0;
    num_bytes = 0;
    do {
        b = (unsigned int)(*buf++);
        y = (y << 7) | (b & 0x0000007f);
        ++num_bytes;
    } while (b & 0x00000080);

    *x = y;
    return num_bytes;
}

int LZ_Compress(const uint8_t *in, uint8_t *out, unsigned int insize) {
    uint8_t marker, symbol;
    unsigned int inpos, outpos, bytesleft, i;
    unsigned int maxoffset, offset, bestoffset;
    unsigned int maxlength, length, bestlength;
    unsigned int histogram[256] = {0};
    const uint8_t *ptr1, *ptr2;
    unsigned int hash_table[HASH_TABLE_SIZE];
    memset(hash_table, 0, sizeof(hash_table));

    if (insize < 1) {
        return 0;
    }

    for (i = 0; i < insize; ++i) {
        ++histogram[in[i]];
    }

    marker = 0;
    for (i = 1; i < 256; ++i) {
        if (histogram[i] < histogram[marker]) {
            marker = i;
        }
    }

    out[0] = marker;
    inpos = 0;
    outpos = 1;
    bytesleft = insize;

    do {
        maxoffset = (inpos > LZ_MAX_OFFSET) ? LZ_MAX_OFFSET : inpos;
        ptr1 = &in[inpos];

        bestlength = 3;
        bestoffset = 0;

        // Utilisation d'une table de hachage pour accélérer la recherche
        unsigned int hash = (ptr1[0] << 8) | ptr1[1];
        offset = hash_table[hash % HASH_TABLE_SIZE];

        while (offset != 0) {
            ptr2 = &in[inpos - offset];
            if ((ptr1[0] == ptr2[0]) && (ptr1[bestlength] == ptr2[bestlength])) {
                maxlength = (bytesleft < offset) ? bytesleft : offset;
                length = _LZ_StringCompare(ptr1, ptr2, 0, maxlength);
                if (length > bestlength) {
                    bestlength = length;
                    bestoffset = offset;
                }
            }
            offset = hash_table[offset % HASH_TABLE_SIZE];
        }

        if ((bestlength >= 8) ||
            ((bestlength == 4) && (bestoffset <= 0x0000007f)) ||
            ((bestlength == 5) && (bestoffset <= 0x00003fff)) ||
            ((bestlength == 6) && (bestoffset <= 0x001fffff)) ||
            ((bestlength == 7) && (bestoffset <= 0x0fffffff))) {
            out[outpos++] = marker;
            outpos += _LZ_WriteVarSize(bestlength, &out[outpos]);
            outpos += _LZ_WriteVarSize(bestoffset, &out[outpos]);
            inpos += bestlength;
            bytesleft -= bestlength;
        } else {
            symbol = in[inpos++];
            out[outpos++] = symbol;
            if (symbol == marker) {
                out[outpos++] = 0;
            }
            --bytesleft;
        }

        // Mise à jour de la table de hachage
        hash_table[hash % HASH_TABLE_SIZE] = inpos;

    } while (bytesleft > 3);

    while (inpos < insize) {
        if (in[inpos] == marker) {
            out[outpos++] = marker;
            out[outpos++] = 0;
        } else {
            out[outpos++] = in[inpos];
        }
        ++inpos;
    }

    return outpos;
}

void LZ_Uncompress(const uint8_t *in, uint8_t *out, unsigned int insize) {
    uint8_t marker, symbol;
    unsigned int i, inpos, outpos, length, offset;

    if (insize < 1) {
        return;
    }

    marker = in[0];
    inpos = 1;
    outpos = 0;

    do {
        symbol = in[inpos++];
        if (symbol == marker) {
            if (in[inpos] == 0) {
                out[outpos++] = marker;
                ++inpos;
            } else {
                inpos += _LZ_ReadVarSize(&length, &in[inpos]);
                inpos += _LZ_ReadVarSize(&offset, &in[inpos]);
                for (i = 0; i < length; ++i) {
                    out[outpos] = out[outpos - offset];
                    ++outpos;
                }
            }
        } else {
            out[outpos++] = symbol;
        }
    } while (inpos < insize);
}
