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
#ifndef __DEVICE_SUPPORT_H__
#define __DEVICE_SUPPORT_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct ambit_known_device_s {
    char *name;
    bool supported;
    struct ambit_device_driver_s *driver;
    uint32_t driver_param;
    uint8_t komposti_version[4];
} ambit_known_device_t;

bool libambit_device_support_known(uint16_t vendor_id, uint16_t product_id);
const ambit_known_device_t *libambit_device_support_find(uint16_t vendor_id, uint16_t product_id, const char *model, const uint8_t *fw_version);
const uint8_t *libambit_device_komposti(uint16_t vendor_id, uint16_t product_id, uint8_t next);
uint32_t libambit_fw_version_number(const uint8_t version[4]);

#endif /* __DEVICE_SUPPORT_H__ */
