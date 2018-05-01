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
#include "device_support.h"
#include "device_driver.h"

#include <string.h>
#include <stdio.h>

/*
 * Local definitions
 */
#define SUUNTO_USB_VENDOR_ID 0x1493

typedef struct int_known_device_s {
    uint16_t vid;
    uint16_t pid;
    char *model;
    uint8_t min_sw_version[4];
    ambit_known_device_t public_info;
} int_known_device_t;

/*
 * Static functions
 */

/*
 * Static variables
 */
static int_known_device_t known_devices[] = {
    { SUUNTO_USB_VENDOR_ID, 0x002d, "Loon", {0x01,0x00,0x04,0x0}, { "Suunto Traverse Alpha", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x002c, "Kaka", {0x01,0x00,0x1b,0x00}, { "Suunto Ambit3 Vertical", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x002b, "Jabiru", {0x01,0x00,0x04,0x00}, { "Suunto Traverse", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001e, "Ibisbill", {0x00,0x00,0x00,0x00}, { "Suunto Ambit3 Run", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001c, "Finch", {0x00,0x00,0x00,0x00}, { "Suunto Ambit3 Sport", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001b, "Emu", {0x00,0x00,0x00,0x00}, { "Suunto Ambit3 Peak", true, &ambit_device_driver_ambit3, 0x0400, {0x02,0x04,0x59,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001d, "Greentit", {0x00,0x00,0x00,0x00}, { "Suunto Ambit2 R", true, &ambit_device_driver_ambit, 0x0400, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x01,0x01,0x02,0x00}, { "Suunto Ambit2 S", true, &ambit_device_driver_ambit, 0x0400, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x01,0x01,0x02,0x00}, { "Suunto Ambit2", true, &ambit_device_driver_ambit, 0x0400, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x03,0x00}, { "Suunto Ambit2 S", false, NULL, 0x0400, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x03,0x00}, { "Suunto Ambit2", false, NULL, 0x0400, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x02,0x00}, { "Suunto Ambit2 S (up to 0.2.2)", false, NULL, 0x0200, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x02,0x00}, { "Suunto Ambit2 (up to 0.2.2)", false, NULL, 0x0200, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x02,0x01,0x00,0x00}, { "Suunto Ambit", true, &ambit_device_driver_ambit, 0x0200, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x09,0x00,0x00}, { "Suunto Ambit", false, NULL, 0x0200, {0x02,0x00,0x2d,0x00} } }, /* First with PMEM 2.0, {0x02,0x00,0x2d,0x00} !? */
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x06,0x00,0x00}, { "Suunto Ambit", false, NULL, 0, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x01,0x00,0x00}, { "Suunto Ambit", false, NULL, 0, {0x02,0x00,0x2d,0x00} } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x00,0x00,0x00,0x00}, { "Suunto Ambit", false, NULL, 0, {0x02,0x00,0x2d,0x00} } },
    { 0x0000, 0x0000, NULL, {0x00,0x00,0x00,0x00}, { NULL, false, NULL, 0, {0x00,0x00,0x00,0x00} } }
};

bool libambit_device_support_known(uint16_t vendor_id, uint16_t product_id)
{
    int i;

    for (i=0; i<sizeof(known_devices)/sizeof(known_devices[0]); i++) {
        if (vendor_id == known_devices[i].vid && product_id == known_devices[i].pid) {
            // Found at least one row with the correct device
            return true;
        }
    }

    return false;
}

const ambit_known_device_t *libambit_device_support_find(uint16_t vendor_id, uint16_t product_id, const char *model, const uint8_t *fw_version)
{
    ambit_known_device_t *device = NULL;
    int i;

    for (i=0; i<sizeof(known_devices)/sizeof(known_devices[0]); i++) {
        if (vendor_id == known_devices[i].vid &&
            product_id == known_devices[i].pid &&
            strcmp(model, known_devices[i].model) == 0 &&
            (libambit_fw_version_number (fw_version) >= libambit_fw_version_number (known_devices[i].min_sw_version))) {
            // Found matching entry, reset to this one!
            device = &known_devices[i].public_info;
            break;
        }
    }

    return device;
}

const uint8_t *libambit_device_komposti(uint16_t vendor_id, uint16_t product_id, uint8_t next)
{
    int i;
    for (i=0; i<sizeof(known_devices)/sizeof(known_devices[0]); i++) {
        if(vendor_id == known_devices[i].vid &&
            product_id == known_devices[i].pid) {
                if(next == 0) {
                    printf("libambit_komposti: %x %x %x %x\n", known_devices[i].public_info.komposti_version[0], known_devices[i].public_info.komposti_version[1], known_devices[i].public_info.komposti_version[2], known_devices[i].public_info.komposti_version[3]);
                    return known_devices[i].public_info.komposti_version;
                } else {
                    --next;
                }
        }
    }

    return NULL;
}

uint32_t libambit_fw_version_number(const uint8_t version[4])
{
    return (  (version[0] << 24)
            | (version[1] << 16)
            | (version[2] <<  0)
            | (version[3] <<  8));
}
