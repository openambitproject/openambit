/*
 * (C) Copyright 2014 Emil Ljungdahl
 *
 * This file is part of libambit.
 *
 * libambit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 */
#include <string.h>

#include "sha256.h"

/*
 * Local definitions
 */
#define ROTLEFT(a,b)  (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z)     (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)        (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x)        (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x)       (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x)       (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))


/*
 * Static functions
 */
static void sha256_transform(sha256_ctx *ctx, const uint8_t *data);

/*
 * Static variables
 */
static const uint32_t h[8] = {
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};

static const uint32_t k[SHA256_BLOCK_SIZE] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

/*
 * Public functions
 */
void sha256(const uint8_t *data, size_t len, uint8_t *hash)
{
    sha256_ctx ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

void sha256_init(sha256_ctx *ctx)
{
    size_t i;
    ctx->datalen = 0;
    ctx->bitlen = 0;
    for (i=0; i<8; i++) {
        ctx->h[i] = h[i];
    }
}

void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len)
{
    size_t i;

    for (i=0; i<len; i++) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == SHA256_BLOCK_SIZE) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

void sha256_final(sha256_ctx *ctx, uint8_t *hash)
{
    uint32_t i;

    i = ctx->datalen;

    // Pad data left in buffer
    if (i < 56) {
        // There is room for the appending length in this chunk, pad up until 56 byte
        ctx->data[i++] = 0x80;
        while (i < 56) {
            ctx->data[i++] = 0x00;
        }
    }
    else {
        // No room for length, transform this chunk and use an empty one for length
        ctx->data[i++] = 0x80;
        while (i < 64) {
            ctx->data[i++] = 0x00;
        }
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    // Append total length to last buffer
    ctx->bitlen += ctx->datalen << 3;
    for (i=0; i<8; i++) {
        ctx->data[63-i] = ctx->bitlen >> (8*i);
    }
    sha256_transform(ctx, ctx->data);

    // Get hash (stored as 8 (4-byte) words)
    for (i=0; i<8; i++) {
        hash[(i<<2)]   = ctx->h[i] >> 24;
        hash[(i<<2)+1] = ctx->h[i] >> 16;
        hash[(i<<2)+2] = ctx->h[i] >> 8;
        hash[(i<<2)+3] = ctx->h[i];
    }
}

static void sha256_transform(sha256_ctx *ctx, const uint8_t *data)
{
    size_t i;
    uint32_t m[64];
    uint32_t th[8];
    uint32_t t1, t2;

    for (i=0; i<16; i++) {
        m[i] = (data[(i<<2)] << 24) | (data[(i<<2)+1] << 16) | (data[(i<<2)+2] << 8) | (data[(i<<2)+3]);
    }
    for (i=16; i<64; i++) {
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];
    }
    for (i=0; i<8; i++) {
        th[i] = ctx->h[i];
    }
    for (i=0; i<64; i++) {
        t1 = th[7] + EP1(th[4]) + CH(th[4], th[5], th[6]) + k[i] + m[i];
        t2 = EP0(th[0]) + MAJ(th[0], th[1], th[2]);
        th[7] = th[6];
        th[6] = th[5];
        th[5] = th[4];
        th[4] = th[3] + t1;
        th[3] = th[2];
        th[2] = th[1];
        th[1] = th[0];
        th[0] = t1 + t2;
    }
    for (i=0; i<8; i++) {
        ctx->h[i] += th[i];
    }
}
