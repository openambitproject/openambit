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
#ifndef __SHA256_H__
#define __SHA256_H__

#include <stddef.h>
#include <stdint.h>

#define SHA256_BLOCK_SIZE (512 / 8)

typedef struct {
    uint8_t  data[SHA256_BLOCK_SIZE];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t h[8];
} sha256_ctx;

void sha256(const uint8_t *data, size_t len, uint8_t *hash);
void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t *hash);

#endif /* __SHA256_H__ */
