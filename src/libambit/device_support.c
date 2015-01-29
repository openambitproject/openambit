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
static uint32_t version_number(const uint8_t version[4]);

/*
 * Static variables
 */
static int_known_device_t known_devices[] = {
    { SUUNTO_USB_VENDOR_ID, 0x001c, "Finch", {0x00,0x00,0x00,0x00}, { "Suunto Ambit3 Sport", true, &ambit_device_driver_ambit3, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x001b, "Emu", {0x00,0x00,0x00,0x00}, { "Suunto Ambit3 Peak", true, &ambit_device_driver_ambit3, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x001d, "Greentit", {0x00,0x00,0x00,0x00}, { "Suunto Ambit2 R", true, &ambit_device_driver_ambit, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x01,0x01,0x02,0x00}, { "Suunto Ambit2 S", true, &ambit_device_driver_ambit, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x01,0x01,0x02,0x00}, { "Suunto Ambit2", true, &ambit_device_driver_ambit, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x03,0x00}, { "Suunto Ambit2 S", false, NULL, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x03,0x00}, { "Suunto Ambit2", false, NULL, 0x0400 } },
    { SUUNTO_USB_VENDOR_ID, 0x001a, "Colibri", {0x00,0x02,0x02,0x00}, { "Suunto Ambit2 S (up to 0.2.2)", false, NULL, 0x0200 } },
    { SUUNTO_USB_VENDOR_ID, 0x0019, "Duck", {0x00,0x02,0x02,0x00}, { "Suunto Ambit2 (up to 0.2.2)", false, NULL, 0x0200 } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x02,0x01,0x00,0x00}, { "Suunto Ambit", true, &ambit_device_driver_ambit, 0x0200 } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x09,0x00,0x00}, { "Suunto Ambit", false, NULL, 0x0200 } }, /* First with PMEM 2.0!? */
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x06,0x00,0x00}, { "Suunto Ambit", false, NULL, 0 } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x01,0x01,0x00,0x00}, { "Suunto Ambit", false, NULL, 0 } },
    { SUUNTO_USB_VENDOR_ID, 0x0010, "Bluebird", {0x00,0x00,0x00,0x00}, { "Suunto Ambit", false, NULL, 0 } },
    { 0x0000, 0x0000, NULL, {0x00,0x00,0x00,0x00}, { NULL, false, NULL, 0 } }
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
            (version_number (fw_version) >= version_number (known_devices[i].min_sw_version))) {
            // Found matching entry, reset to this one!
            device = &known_devices[i].public_info;
            break;
        }
    }

    return device;
}

static uint32_t version_number(const uint8_t version[4])
{
    return (  (version[0] << 24)
            | (version[1] << 16)
            | (version[2] <<  0)
            | (version[3] <<  8));
}
