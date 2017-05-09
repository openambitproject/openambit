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
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stddef.h>
#include <stdint.h>
#include "libambit.h"

enum ambit_commands_e {
    ambit_command_device_info           = 0x0000,
    ambit_command_time                  = 0x0300,
    ambit_command_date                  = 0x0302,
    ambit_command_status                = 0x0306,
    ambit_command_personal_settings     = 0x0b00,
    ambit_command_personal_settings_write = 0x0b01,
    ambit_command_waypoint_count        = 0x0b02,
    ambit_command_waypoint_read         = 0x0b03,
    ambit_command_nav_memory_delete     = 0x0b04,
    ambit_command_waypoint_write        = 0x0b05,
    ambit_command_log_count             = 0x0b06,
    ambit_command_log_head_first        = 0x0b07,
    ambit_command_log_head_peek         = 0x0b08,
    ambit_command_log_head_step         = 0x0b0a,
    ambit_command_log_head              = 0x0b0b,
    ambit_command_gps_orbit_head        = 0x0b15,
    ambit_command_data_write            = 0x0b16,
    ambit_command_log_read              = 0x0b17,
    ambit_command_data_tail_len         = 0x0b18,
    ambit_command_lock_check            = 0x0b19,
    ambit_command_lock_set              = 0x0b1a,
    ambit_command_write_start           = 0x0b1b, // Really!? Just a guess...
    ambit_command_ambit3_get_compact_serial = 0x0b1e,
    ambit_command_ambit3_memory_map     = 0x0b21,
    ambit_command_unknown4              = 0x0b25, // Ambit3 Peak fw 2.0.4
    ambit_command_unknown5              = 0x0b26, // Ambit3 Peak fw 2.0.4
    ambit_command_unknown6              = 0x0b27, // Ambit3 Peak fw 2.0.4
    ambit_command_ambit3_settings       = 0x1100,
    ambit_command_ambit3_settings_write = 0x1101,
    ambit_command_unknown7              = 0x1104, // Ambit3 Peak fw 2.0.4 (probably Read FW)
    ambit_command_ambit3_log_headers    = 0x1200,
    ambit_command_ambit3_log_synced     = 0x1201,
    ambit_command_unknown8              = 0x1202, // Ambit3 Peak fw 2.0.4
};

/**
 * Write command to device
 * \param legacy_format 0=normal, 1=legacy, 2=version 2
 */
int libambit_protocol_command(ambit_object_t *object, uint16_t command, uint8_t *data, size_t datalen, uint8_t **reply_data, size_t *replylen, uint8_t legacy_format);
void libambit_protocol_free(uint8_t *data);

#endif /* __PROTOCOL_H__ */
