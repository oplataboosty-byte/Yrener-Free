/* stb_image - v2.28 - public domain image loader
   Minimal version for PNG loading
   Original: https://github.com/nothings/stb */

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif

STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF void stbi_image_free(void *retval_from_stbi_load);
STBIDEF const char *stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

#include <stdint.h>

static const char *stbi__g_failure_reason;
STBIDEF const char *stbi_failure_reason(void) { return stbi__g_failure_reason; }
static int stbi__err(const char *str) { stbi__g_failure_reason = str; return 0; }

#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)
#define STBI_ASSERT(x)

typedef struct {
    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original, *img_buffer_original_end;
    int img_x, img_y, img_n;
    int buflen;
    FILE *f;
    stbi_uc buffer_start[128];
    int read_from_callbacks;
    int callback_already_read;
} stbi__context;

static void stbi__refill_buffer(stbi__context *s) {
    if (s->f) {
        int n = (int)fread(s->buffer_start, 1, sizeof(s->buffer_start), s->f);
        s->img_buffer = s->buffer_start;
        s->img_buffer_end = s->buffer_start + n;
    }
}

static stbi_uc stbi__get8(stbi__context *s) {
    if (s->img_buffer < s->img_buffer_end) return *s->img_buffer++;
    if (s->f) { stbi__refill_buffer(s); if (s->img_buffer < s->img_buffer_end) return *s->img_buffer++; }
    return 0;
}

static int stbi__get16be(stbi__context *s) {
    int z = stbi__get8(s);
    return (z << 8) + stbi__get8(s);
}

static uint32_t stbi__get32be(stbi__context *s) {
    uint32_t z = stbi__get16be(s);
    return (z << 16) + stbi__get16be(s);
}

static int stbi__get16le(stbi__context *s) {
    int z = stbi__get8(s);
    return z + (stbi__get8(s) << 8);
}

static uint32_t stbi__get32le(stbi__context *s) {
    uint32_t z = stbi__get16le(s);
    return z + (stbi__get16le(s) << 16);
}

static void stbi__skip(stbi__context *s, int n) {
    if (n <= 0) return;
    if (s->f) fseek(s->f, n, SEEK_CUR);
    else s->img_buffer += n;
}

static int stbi__at_eof(stbi__context *s) {
    if (s->f) return feof(s->f);
    return s->img_buffer >= s->img_buffer_end;
}

static void stbi__start_file(stbi__context *s, FILE *f) {
    memset(s, 0, sizeof(*s));
    s->f = f;
    stbi__refill_buffer(s);
}

static void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, int len) {
    memset(s, 0, sizeof(*s));
    s->img_buffer = s->img_buffer_original = (stbi_uc *)buffer;
    s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)buffer + len;
}

static stbi_uc stbi__compute_y(int r, int g, int b) {
    return (stbi_uc)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static stbi_uc *stbi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y) {
    unsigned int i, j;
    stbi_uc *good;

    if (req_comp == img_n) return data;

    good = (stbi_uc *)STBI_MALLOC(req_comp * x * y);
    if (!good) { STBI_FREE(data); return NULL; }

    for (j = 0; j < y; ++j) {
        unsigned char *src = data + j * x * img_n;
        unsigned char *dest = good + j * x * req_comp;

        for (i = 0; i < x; ++i) {
            switch (img_n * 8 + req_comp) {
                case 1*8+3: dest[0]=dest[1]=dest[2]=src[0]; break;
                case 1*8+4: dest[0]=dest[1]=dest[2]=src[0]; dest[3]=255; break;
                case 2*8+1: dest[0]=src[0]; break;
                case 2*8+3: dest[0]=dest[1]=dest[2]=src[0]; break;
                case 2*8+4: dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1]; break;
                case 3*8+4: dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=255; break;
                case 3*8+1: dest[0]=stbi__compute_y(src[0],src[1],src[2]); break;
                case 3*8+2: dest[0]=stbi__compute_y(src[0],src[1],src[2]); dest[1]=255; break;
                case 4*8+1: dest[0]=stbi__compute_y(src[0],src[1],src[2]); break;
                case 4*8+2: dest[0]=stbi__compute_y(src[0],src[1],src[2]); dest[1]=src[3]; break;
                case 4*8+3: dest[0]=src[0];dest[1]=src[1];dest[2]=src[2]; break;
                default: break;
            }
            src += img_n;
            dest += req_comp;
        }
    }
    STBI_FREE(data);
    return good;
}

// ============ PNG DECODER ============

static int stbi__check_png_header(stbi__context *s) {
    static const stbi_uc png_sig[8] = { 137,80,78,71,13,10,26,10 };
    int i;
    for (i = 0; i < 8; ++i)
        if (stbi__get8(s) != png_sig[i]) return stbi__err("bad png sig");
    return 1;
}

typedef struct {
    uint32_t length;
    uint32_t type;
} stbi__pngchunk;

static stbi__pngchunk stbi__get_chunk_header(stbi__context *s) {
    stbi__pngchunk c;
    c.length = stbi__get32be(s);
    c.type = stbi__get32be(s);
    return c;
}

// Minimal zlib inflate
static int stbi__zexpand(unsigned char **out, int *outsize, int needed) {
    int newsize = *outsize * 2;
    unsigned char *newbuf;
    if (newsize < *outsize + needed) newsize = *outsize + needed;
    newbuf = (unsigned char *)STBI_REALLOC(*out, newsize);
    if (!newbuf) return 0;
    *out = newbuf;
    *outsize = newsize;
    return 1;
}

#define STBI__ZFAST_BITS  9
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

typedef struct {
    stbi_uc *zbuffer, *zbuffer_end;
    int num_bits;
    uint32_t code_buffer;
    unsigned char window[32768];
    int wpos;

    stbi_us fast[1 << STBI__ZFAST_BITS];
    stbi_us firstcode[16];
    int maxcode[17];
    stbi_us firstsymbol[16];
    stbi_uc  size[288];
    stbi_us value[288];

    stbi_us fast2[1 << STBI__ZFAST_BITS];
    stbi_us firstcode2[16];
    int maxcode2[17];
    stbi_us firstsymbol2[16];
    stbi_uc  size2[32];
    stbi_us value2[32];
} stbi__zbuf;

static int stbi__zbuild_huffman(stbi__zbuf *a, stbi_uc *sizelist, int num,
                                stbi_us *fast, stbi_us *firstcode, int *maxcode, stbi_us *firstsymbol, stbi_uc *sizes, stbi_us *values) {
    int i, k = 0;
    int code, next_code[16], sizes_arr[17];
    int s, c, j;

    memset(sizes_arr, 0, sizeof(sizes_arr));
    memset(fast, 0, sizeof(stbi_us) * (1 << STBI__ZFAST_BITS));

    for (i = 0; i < num; ++i) ++sizes_arr[sizelist[i]];
    sizes_arr[0] = 0;

    code = 0;
    for (i = 1; i < 16; ++i) {
        next_code[i] = code;
        firstcode[i] = (stbi_us)code;
        firstsymbol[i] = (stbi_us)k;
        code = code + sizes_arr[i];
        maxcode[i] = code << (16 - i);
        code <<= 1;
        k += sizes_arr[i];
    }
    maxcode[16] = 0x10000;

    for (i = 0; i < num; ++i) {
        s = sizelist[i];
        if (s) {
            c = next_code[s] - firstcode[s] + firstsymbol[s];
            sizes[c] = (stbi_uc)s;
            values[c] = (stbi_us)i;
            if (s <= STBI__ZFAST_BITS) {
                j = (next_code[s] << (STBI__ZFAST_BITS - s));
                while (j < (1 << STBI__ZFAST_BITS)) {
                    fast[j] = (stbi_us)c;
                    j += (1 << (STBI__ZFAST_BITS - s));
                }
            }
            ++next_code[s];
        }
    }
    return 1;
}

static void stbi__fill_bits(stbi__zbuf *z) {
    while (z->num_bits <= 24 && z->zbuffer < z->zbuffer_end) {
        z->code_buffer |= (uint32_t)*z->zbuffer++ << z->num_bits;
        z->num_bits += 8;
    }
}

static int stbi__zhuffman_decode(stbi__zbuf *a, stbi_us *fast, stbi_us *firstcode, int *maxcode, stbi_us *firstsymbol, stbi_uc *sizes, stbi_us *values) {
    int b, s, k;
    if (a->num_bits < 16) stbi__fill_bits(a);
    b = fast[a->code_buffer & STBI__ZFAST_MASK];
    if (b) {
        s = sizes[b];
        a->code_buffer >>= s;
        a->num_bits -= s;
        return values[b];
    }

    k = (int)(a->code_buffer & 0xffff);
    for (s = STBI__ZFAST_BITS + 1; ; ++s)
        if (k < maxcode[s]) break;
    if (s >= 16) return -1;

    b = (k >> (16 - s)) - firstcode[s] + firstsymbol[s];
    a->code_buffer >>= s;
    a->num_bits -= s;
    return values[b];
}

static int stbi__zreceive(stbi__zbuf *z, int n) {
    int k;
    if (z->num_bits < n) stbi__fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}

static int stbi__parse_huffman_block(stbi__zbuf *a, unsigned char **out, int *outsize, int *outpos) {
    static const int length_base[31] = {
            3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,0,0
    };
    static const int length_extra[31] = {
            0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0
    };
    static const int dist_base[32] = {
            1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0
    };
    static const int dist_extra[32] = {
            0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
    };

    int z, len, dist, p, i;
    unsigned char v;

    for (;;) {
        z = stbi__zhuffman_decode(a, a->fast, a->firstcode, a->maxcode, a->firstsymbol, a->size, a->value);
        if (z < 0) return stbi__err("bad huffman code");
        if (z < 256) {
            if (*outpos >= *outsize) {
                if (!stbi__zexpand(out, outsize, 1)) return 0;
            }
            (*out)[(*outpos)++] = (unsigned char)z;
            a->window[a->wpos++] = (unsigned char)z;
            a->wpos &= 32767;
        } else if (z == 256) {
            return 1;
        } else {
            z -= 257;
            len = length_base[z];
            if (length_extra[z]) len += stbi__zreceive(a, length_extra[z]);

            z = stbi__zhuffman_decode(a, a->fast2, a->firstcode2, a->maxcode2, a->firstsymbol2, a->size2, a->value2);
            if (z < 0) return stbi__err("bad huffman code");
            dist = dist_base[z];
            if (dist_extra[z]) dist += stbi__zreceive(a, dist_extra[z]);

            if (*outpos + len > *outsize) {
                if (!stbi__zexpand(out, outsize, len)) return 0;
            }

            p = (a->wpos - dist) & 32767;
            for (i = 0; i < len; ++i) {
                v = a->window[p];
                (*out)[(*outpos)++] = v;
                a->window[a->wpos++] = v;
                a->wpos &= 32767;
                p = (p + 1) & 32767;
            }
        }
    }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a) {
    static const stbi_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
    stbi_uc codelength_sizes[19];
    stbi_uc lencodes[286+32+137];

    stbi_us fast_temp[1 << STBI__ZFAST_BITS];
    stbi_us firstcode_temp[16], firstsymbol_temp[16];
    int maxcode_temp[17];
    stbi_uc size_temp[19];
    stbi_us value_temp[19];

    int hlit, hdist, hclen, i, n, c, cnt;
    stbi_uc fill;

    memset(codelength_sizes, 0, sizeof(codelength_sizes));

    hlit = stbi__zreceive(a, 5) + 257;
    hdist = stbi__zreceive(a, 5) + 1;
    hclen = stbi__zreceive(a, 4) + 4;

    for (i = 0; i < hclen; ++i)
        codelength_sizes[length_dezigzag[i]] = (stbi_uc)stbi__zreceive(a, 3);

    stbi__zbuild_huffman(a, codelength_sizes, 19, fast_temp, firstcode_temp, maxcode_temp, firstsymbol_temp, size_temp, value_temp);

    n = 0;
    while (n < hlit + hdist) {
        c = stbi__zhuffman_decode(a, fast_temp, firstcode_temp, maxcode_temp, firstsymbol_temp, size_temp, value_temp);
        if (c < 0) return stbi__err("bad codelengths");
        if (c < 16) {
            lencodes[n++] = (stbi_uc)c;
        } else {
            fill = 0;
            if (c == 16) { cnt = stbi__zreceive(a, 2) + 3; if (n > 0) fill = lencodes[n-1]; }
            else if (c == 17) { cnt = stbi__zreceive(a, 3) + 3; }
            else { cnt = stbi__zreceive(a, 7) + 11; }
            while (cnt--) lencodes[n++] = fill;
        }
    }

    stbi__zbuild_huffman(a, lencodes, hlit, a->fast, a->firstcode, a->maxcode, a->firstsymbol, a->size, a->value);
    stbi__zbuild_huffman(a, lencodes + hlit, hdist, a->fast2, a->firstcode2, a->maxcode2, a->firstsymbol2, a->size2, a->value2);

    return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a, unsigned char **out, int *outsize, int *outpos) {
    int len, nlen, i;

    a->num_bits &= 7;
    while (a->num_bits > 0) { stbi__zreceive(a, 1); }

    if (a->zbuffer + 4 > a->zbuffer_end) return stbi__err("bad zlib");
    len = a->zbuffer[0] | (a->zbuffer[1] << 8);
    nlen = a->zbuffer[2] | (a->zbuffer[3] << 8);
    a->zbuffer += 4;

    if ((len ^ 0xffff) != nlen) return stbi__err("zlib corrupt");
    if (a->zbuffer + len > a->zbuffer_end) return stbi__err("bad zlib");

    if (*outpos + len > *outsize) {
        if (!stbi__zexpand(out, outsize, len)) return 0;
    }
    memcpy(*out + *outpos, a->zbuffer, len);
    *outpos += len;

    for (i = 0; i < len; ++i) {
        a->window[a->wpos++] = a->zbuffer[i];
        a->wpos &= 32767;
    }
    a->zbuffer += len;

    return 1;
}

static int stbi__parse_zlib(stbi__zbuf *a, unsigned char **out, int *outsize, int *outpos, int parse_header) {
    int final_block, type, cmf, flg, i;
    stbi_uc lencode[288], distcode[32];

    if (parse_header) {
        cmf = stbi__zreceive(a, 8);
        flg = stbi__zreceive(a, 8);
        if ((cmf * 256 + flg) % 31 != 0) return stbi__err("bad zlib header");
        if (flg & 32) return stbi__err("no preset dict");
        if ((cmf & 15) != 8) return stbi__err("bad compression");
    }

    do {
        final_block = stbi__zreceive(a, 1);
        type = stbi__zreceive(a, 2);
        if (type == 0) {
            if (!stbi__parse_uncompressed_block(a, out, outsize, outpos)) return 0;
        } else if (type == 3) {
            return stbi__err("invalid block");
        } else {
            if (type == 1) {
                // Fixed huffman
                for (i = 0; i <= 143; ++i) lencode[i] = 8;
                for (i = 144; i <= 255; ++i) lencode[i] = 9;
                for (i = 256; i <= 279; ++i) lencode[i] = 7;
                for (i = 280; i <= 287; ++i) lencode[i] = 8;
                for (i = 0; i < 32; ++i) distcode[i] = 5;
                stbi__zbuild_huffman(a, lencode, 288, a->fast, a->firstcode, a->maxcode, a->firstsymbol, a->size, a->value);
                stbi__zbuild_huffman(a, distcode, 32, a->fast2, a->firstcode2, a->maxcode2, a->firstsymbol2, a->size2, a->value2);
            } else {
                if (!stbi__compute_huffman_codes(a)) return 0;
            }
            if (!stbi__parse_huffman_block(a, out, outsize, outpos)) return 0;
        }
    } while (!final_block);

    return 1;
}

static unsigned char *stbi__do_zlib(unsigned char *data, int len, int *outlen) {
    stbi__zbuf a;
    int outsize;
    unsigned char *out;
    int outpos;

    memset(&a, 0, sizeof(a));
    a.zbuffer = data;
    a.zbuffer_end = data + len;

    outsize = len * 2;
    out = (unsigned char *)STBI_MALLOC(outsize);
    if (!out) return NULL;

    outpos = 0;
    if (!stbi__parse_zlib(&a, &out, &outsize, &outpos, 1)) {
        STBI_FREE(out);
        return NULL;
    }

    *outlen = outpos;
    return out;
}

static int stbi__paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static int stbi__create_png_image_raw(unsigned char *raw, uint32_t raw_len, int out_n, uint32_t x, uint32_t y, int depth, int color, unsigned char **outimg) {
    uint32_t bytes = (out_n * x * depth + 7) / 8;
    unsigned char *img;
    unsigned char *prior;
    unsigned char *cur;
    unsigned char *rp;
    uint32_t i, j;
    int filter;
    unsigned char raw_byte, left, up, upleft;

    (void)raw_len;
    (void)color;

    img = (unsigned char *)STBI_MALLOC(out_n * x * y);
    if (!img) return 0;

    prior = NULL;
    cur = img;
    rp = raw;

    for (j = 0; j < y; ++j) {
        filter = *rp++;

        for (i = 0; i < x * (uint32_t)out_n; ++i) {
            raw_byte = (i < bytes) ? rp[i] : 0;
            left = (i >= (uint32_t)out_n) ? cur[i - out_n] : 0;
            up = prior ? prior[i] : 0;
            upleft = (prior && i >= (uint32_t)out_n) ? prior[i - out_n] : 0;

            switch (filter) {
                case 0: cur[i] = raw_byte; break;
                case 1: cur[i] = raw_byte + left; break;
                case 2: cur[i] = raw_byte + up; break;
                case 3: cur[i] = raw_byte + ((left + up) >> 1); break;
                case 4: cur[i] = raw_byte + stbi__paeth(left, up, upleft); break;
                default: STBI_FREE(img); return stbi__err("bad filter");
            }
        }
        rp += bytes;
        prior = cur;
        cur += out_n * x;
    }

    *outimg = img;
    return 1;
}

static unsigned char *stbi__do_png(stbi__context *s, int *x, int *y, int *n, int req_comp) {
    unsigned char *idata = NULL;
    unsigned char *out = NULL;
    unsigned char *raw = NULL;
    unsigned char *newdata;
    int idata_len = 0;
    int idata_limit = 0;
    int img_n = 0;
    int depth = 0;
    int color = 0;
    int raw_len = 0;
    uint32_t i;
    stbi__pngchunk c;

    *x = *y = 0;

    if (!stbi__check_png_header(s)) return NULL;

    for (;;) {
        c = stbi__get_chunk_header(s);

        if (c.type == 0x49484452) { // IHDR
            *x = stbi__get32be(s);
            *y = stbi__get32be(s);
            depth = stbi__get8(s);
            color = stbi__get8(s);
            stbi__get8(s); stbi__get8(s); stbi__get8(s); // compression, filter, interlace
            stbi__get32be(s); // crc

            if (depth != 8) {
                stbi__err("bad depth");
                goto fail;
            }
            if (color == 0) img_n = 1;
            else if (color == 2) img_n = 3;
            else if (color == 3) img_n = 1; // palette
            else if (color == 4) img_n = 2;
            else if (color == 6) img_n = 4;
            else {
                stbi__err("bad color");
                goto fail;
            }
        } else if (c.type == 0x49444154) { // IDAT
            if (idata_len + (int)c.length > idata_limit) {
                idata_limit = idata_len + c.length + 4096;
                newdata = (unsigned char *)STBI_REALLOC(idata, idata_limit);
                if (!newdata) {
                    stbi__err("outofmem");
                    goto fail;
                }
                idata = newdata;
            }
            for (i = 0; i < c.length; ++i)
                idata[idata_len++] = stbi__get8(s);
            stbi__get32be(s); // crc
        } else if (c.type == 0x49454E44) { // IEND
            stbi__get32be(s); // crc
            break;
        } else {
            stbi__skip(s, c.length + 4);
        }
    }

    if (!idata) {
        stbi__err("no IDAT");
        goto fail;
    }

    raw = stbi__do_zlib(idata, idata_len, &raw_len);
    STBI_FREE(idata);
    idata = NULL;

    if (!raw) goto fail;

    if (!stbi__create_png_image_raw(raw, raw_len, img_n, *x, *y, depth, color, &out)) {
        STBI_FREE(raw);
        goto fail;
    }
    STBI_FREE(raw);
    raw = NULL;

    *n = img_n;
    if (req_comp && req_comp != img_n)
        out = stbi__convert_format(out, img_n, req_comp, *x, *y);

    return out;

    fail:
    if (idata) STBI_FREE(idata);
    if (out) STBI_FREE(out);
    if (raw) STBI_FREE(raw);
    return NULL;
}

// ============ MAIN API ============

static unsigned char *stbi__load_main(stbi__context *s, int *x, int *y, int *comp, int req_comp) {
    unsigned char *result;

    // Try PNG
    result = stbi__do_png(s, x, y, comp, req_comp);
    if (result) return result;

    return NULL;
}

STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) {
    FILE *f;
    stbi__context s;
    unsigned char *result;

    f = fopen(filename, "rb");
    if (!f) {
        stbi__err("can't open file");
        return NULL;
    }

    stbi__start_file(&s, f);
    result = stbi__load_main(&s, x, y, channels_in_file, desired_channels);

    fclose(f);
    return result;
}

STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels) {
    stbi__context s;
    stbi__start_mem(&s, buffer, len);
    return stbi__load_main(&s, x, y, channels_in_file, desired_channels);
}

STBIDEF void stbi_image_free(void *retval_from_stbi_load) {
    STBI_FREE(retval_from_stbi_load);
}

#endif // STB_IMAGE_IMPLEMENTATION