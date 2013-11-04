#include "config.h"

#include <glib.h>
#include <epan/conversation.h>
#include <epan/expert.h>
#include <epan/packet.h>
#include <epan/reassemble.h>
#include <epan/dissectors/packet-usb.h>

typedef struct ambit_reassembly_entry {
    guint8 valid;
    guint32 command;
    guint32 start_frame;
    guint32 frame_count;
    guint32 size;
    unsigned char *data;
} ambit_reassembly_entry_t;

typedef struct ambit_protocol_type {
    guint32 command;
    char *name;
    gint (*dissector)(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
} ambit_protocol_type_t;

static const ambit_protocol_type_t *find_subdissector(guint32 command);
static gint dissect_ambit_date_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_date_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_time_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_time_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_status_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_status_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_device_info_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_device_info_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_personal_settings_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_personal_settings_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_unwind_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_unwind_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_next_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_next_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_data_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_data_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);

/* protocols and header fields */
#define D_AMBIT_USBID 0x3f
static int proto_ambit = -1;
static int hf_ambit_usbid = -1;
static int hf_ambit_usblength = -1;
static int hf_ambit_msgpart = -1;
static int hf_ambit_msglength = -1;
static int hf_ambit_msgsegs = -1;
static int hf_ambit_msgseg = -1;
static int hf_ambit_msgheaderchksum = -1;
static int hf_ambit_requestcmd = -1;
static int hf_ambit_pktseqno = -1;
static int hf_ambit_pktlen = -1;
static int hf_ambit_payloadchksum = -1;

static int hf_ambit_unknown = -1;

static int hf_ambit_date = -1;
static int hf_ambit_time = -1;
static int hf_ambit_charge = -1;
static int hf_ambit_model = -1;
static int hf_ambit_serial = -1;
static int hf_ambit_fw_version = -1;
static int hf_ambit_hw_version = -1;
static int hf_ambit_personal_compass_declination = -1;
static int hf_ambit_personal_map_orientation = -1;
static int hf_ambit_personal_date_format = -1;
static int hf_ambit_personal_time_format = -1;
static int hf_ambit_personal_coordinate_system = -1;
static int hf_ambit_personal_unit_system = -1;
static int hf_ambit_personal_alarm_enable = -1;
static int hf_ambit_personal_alarm_time = -1;
static int hf_ambit_personal_dual_time = -1;
static int hf_ambit_personal_gps_sync_time = -1;
static int hf_ambit_personal_weight = -1;
static int hf_ambit_personal_max_hb = -1;
static int hf_ambit_personal_rest_hb = -1;
static int hf_ambit_personal_birthyear = -1;
static int hf_ambit_personal_length = -1;
static int hf_ambit_personal_sex = -1;
static int hf_ambit_personal_fitness_level = -1;
static int hf_ambit_personal_alti_baro_fused_alti = -1;
static int hf_ambit_personal_alti_baro_profile = -1;
static int hf_ambit_personal_backlight_brightness = -1;
static int hf_ambit_personal_backlight_mode = -1;
static int hf_ambit_personal_display_contrast = -1;
static int hf_ambit_personal_invert_display = -1;
static int hf_ambit_personal_lock_sports_mode = -1;
static int hf_ambit_personal_lock_time_mode = -1;
static int hf_ambit_personal_tones = -1;

static int hf_ambit_log_header_seq = -1;
static int hf_ambit_log_header_length = -1;
static int hf_ambit_log_header_sample_desc = -1;
static int hf_ambit_log_header_date = -1;
static int hf_ambit_log_header_time = -1;
static int hf_ambit_log_header_duration = -1;
static int hf_ambit_log_header_ascent = -1;
static int hf_ambit_log_header_descent = -1;
static int hf_ambit_log_header_ascent_time = -1;
static int hf_ambit_log_header_descent_time = -1;
static int hf_ambit_log_header_recovery = -1;
static int hf_ambit_log_header_avg_speed = -1;
static int hf_ambit_log_header_max_speed = -1;
static int hf_ambit_log_header_altitude_max = -1;
static int hf_ambit_log_header_altitude_min = -1;
static int hf_ambit_log_header_hr_avg = -1;
static int hf_ambit_log_header_hr_max = -1;
static int hf_ambit_log_header_peak_effect = -1;
static int hf_ambit_log_header_activity_type = -1;
static int hf_ambit_log_header_activity_name = -1;
static int hf_ambit_log_header_hr_min = -1;
static int hf_ambit_log_header_distance = -1;
static int hf_ambit_log_header_sample_count = -1;
static int hf_ambit_log_header_energy = -1;
static int hf_ambit_log_header_speed_max_time = -1;
static int hf_ambit_log_header_alt_max_time = -1;
static int hf_ambit_log_header_alt_min_time = -1;
static int hf_ambit_log_header_hr_max_time = -1;
static int hf_ambit_log_header_hr_min_time = -1;
static int hf_ambit_log_header_temp_max_time = -1;
static int hf_ambit_log_header_temp_min_time = -1;
static int hf_ambit_log_header_time_first_fix = -1;
static int hf_ambit_log_header_battery_start = -1;
static int hf_ambit_log_header_battery_stop = -1;
static int hf_ambit_log_header_distance_before_calib = -1;

static int hf_ambit_log_header_more = -1;

static int hf_ambit_log_data_address = -1;
static int hf_ambit_log_data_length = -1;
static int hf_ambit_log_data_sof = -1;
static int hf_ambit_log_data_first_addr = -1;
static int hf_ambit_log_data_last_addr = -1;
static int hf_ambit_log_data_entry_count = -1;
static int hf_ambit_log_data_next_free_addr = -1;
static int hf_ambit_log_data_next_addr = -1;
static int hf_ambit_log_data_prev_addr = -1;

static gint ett_ambit = -1;
static gint ett_ambit_data = -1;

static expert_field ei_ambit_undecoded= EI_INIT;

static ambit_reassembly_entry_t *reassembly_entries = NULL;
static guint32 reassembly_entries_alloc = 0;

static const value_string msgpart_index_vals[] = {
    { 0x5d, "First part" },
    { 0x5e, "Following part" },
    { 0, NULL }
};

static const value_string log_header_more_vals[] = {
    { 0x00000400, "More entries" },
    { 0x00000c00, "No more entries" },
    { 0, NULL }
};

static const ambit_protocol_type_t subdissectors[] = {
    { 0x03000500, "Set time", dissect_ambit_time_write },
    { 0x03000a00, "Time reply", dissect_ambit_time_reply },
    { 0x03020500, "Set date", dissect_ambit_date_write },
    { 0x03020a00, "Date reply", dissect_ambit_date_reply },
    { 0x03060500, "Get device status", dissect_ambit_status_get },
    { 0x03060a00, "Device status reply", dissect_ambit_status_reply },
    { 0x00000100, "Get device info", dissect_ambit_device_info_get },
    { 0x00020200, "Device info reply", dissect_ambit_device_info_reply },
    { 0x0b000500, "Get personal settings", dissect_ambit_personal_settings_get },
    { 0x0b000a00, "Personal settings reply", dissect_ambit_personal_settings_reply },
    { 0x0b010500, "Write personal settings", dissect_ambit_personal_settings_reply },
    { 0x0b010a00, "Personal settings write reply", dissect_ambit_personal_settings_get },
    { 0x0b070500, "Log header unwind request", dissect_ambit_log_header_unwind_get },
    { 0x0b070a00, "Log header unwind reply", dissect_ambit_log_header_unwind_reply },
    { 0x0b080500, "Log header next request", dissect_ambit_log_header_next_get },
    { 0x0b080a00, "Log header next reply", dissect_ambit_log_header_next_reply },
    { 0x0b0b0500, "Get log header", dissect_ambit_log_header_get },
    { 0x0b0b0a00, "Log header reply", dissect_ambit_log_header_reply },
    { 0x0b170500, "Get log data", dissect_ambit_log_data_get },
    { 0x0b170a00, "Log data reply", dissect_ambit_log_data_reply },
    { 0, NULL, NULL }
};

static const ambit_protocol_type_t *find_subdissector(guint32 command)
{
    int i = 0;
    const ambit_protocol_type_t *entry = &subdissectors[i++];

    while (entry->command != 0) {
        if (entry->command == command) {
            return entry;
        }
        entry = &subdissectors[i++];
    }

    return NULL;
}

static void dissect_ambit_add_unknown(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gint offset, gint len)
{
    proto_item *unknown_item = NULL;
    unknown_item = proto_tree_add_item(tree, hf_ambit_unknown, tvb, offset, len, ENC_LITTLE_ENDIAN);
    expert_add_info(pinfo, unknown_item, &ei_ambit_undecoded);
}

static gint dissect_ambit_date_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint16 year = tvb_get_letohs(tvb, 0);
    guint8 month = tvb_get_guint8(tvb, 2);
    guint8 day = tvb_get_guint8(tvb, 3);
    proto_tree_add_string_format_value(tree, hf_ambit_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
    offset += 4;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
    offset += 4;
}

static gint dissect_ambit_date_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_time_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint8 hour = tvb_get_guint8(tvb, 4);
    guint8 minute = tvb_get_guint8(tvb, 5);
    guint16 seconds = tvb_get_letohs(tvb, 6) / 1000;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
    offset += 4;
    proto_tree_add_string_format_value(tree, hf_ambit_time, tvb, offset, 4, "Time", "%02d:%02d:%02d", hour, minute, seconds);
    offset += 4;
}

static gint dissect_ambit_time_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_status_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_status_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_charge, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
}

static gint dissect_ambit_device_info_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_device_info_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint8 fw1,fw2,fw3;
    guint8 hw1,hw2;
    guint16 hw3;
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_model, tvb, offset, 16, ENC_LITTLE_ENDIAN);
    offset += 16;
    proto_tree_add_item(tree, hf_ambit_serial, tvb, offset, 16, ENC_LITTLE_ENDIAN);
    offset += 16;
    fw1 = tvb_get_guint8(tvb, offset);
    fw2 = tvb_get_guint8(tvb, offset+1);
    fw3 = tvb_get_guint8(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_fw_version, tvb, offset, 3, "FW version", "%d.%d.%d", fw1, fw2, fw3);
    offset += 3;
    offset += 1;
    hw1 = tvb_get_guint8(tvb, offset);
    hw2 = tvb_get_guint8(tvb, offset+1);
    hw3 = tvb_get_letohs(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_hw_version, tvb, offset, 4, "HW version", "%d.%d.%d", hw1, hw2, hw3);
    offset += 4;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 8);
    offset += 8;
}

static gint dissect_ambit_personal_settings_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_personal_settings_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 1;
    proto_tree_add_item(tree, hf_ambit_personal_lock_sports_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 2;
    proto_tree_add_item(tree, hf_ambit_personal_lock_time_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 4;
    guint8 declination_direction = tvb_get_guint8(tvb, offset);
    guint8 declination_deg = tvb_get_guint8(tvb, offset + 1);
    if (declination_direction == 0) {
        proto_tree_add_string_format_value(tree, hf_ambit_personal_compass_declination, tvb, offset, 2, "Compass declination", "None");
    }
    else {
        proto_tree_add_string_format_value(tree, hf_ambit_personal_compass_declination, tvb, offset, 2, "Compass declination", "%s %f", declination_direction == 1 ? "E" : "W", 0.5*declination_deg);
    }
    offset = 8;
    proto_tree_add_item(tree, hf_ambit_personal_unit_system, tvb, offset, 8, ENC_LITTLE_ENDIAN);
    offset = 19;
    proto_tree_add_item(tree, hf_ambit_personal_coordinate_system, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 21;
    proto_tree_add_item(tree, hf_ambit_personal_map_orientation, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 24;
    proto_tree_add_item(tree, hf_ambit_personal_gps_sync_time, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 25;
    proto_tree_add_item(tree, hf_ambit_personal_time_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 26;
    guint8 alarm_hour = tvb_get_guint8(tvb, offset);
    guint8 alarm_minute = tvb_get_guint8(tvb, offset + 1);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_alarm_time, tvb, offset, 2, "Alarm time", "%d:%d", alarm_hour, alarm_minute);
    offset = 28;
    proto_tree_add_item(tree, hf_ambit_personal_alarm_enable, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 31;
    guint8 dual_hour = tvb_get_guint8(tvb, offset);
    guint8 dual_minute = tvb_get_guint8(tvb, offset + 1);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_dual_time, tvb, offset, 2, "Dual time", "%d:%d", dual_hour, dual_minute);
    offset = 36;
    proto_tree_add_item(tree, hf_ambit_personal_date_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 40;
    proto_tree_add_item(tree, hf_ambit_personal_tones, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 44;
    proto_tree_add_item(tree, hf_ambit_personal_backlight_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 45;
    proto_tree_add_item(tree, hf_ambit_personal_backlight_brightness, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 46;
    proto_tree_add_item(tree, hf_ambit_personal_display_contrast, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 47;
    proto_tree_add_item(tree, hf_ambit_personal_invert_display, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 48;
    guint16 weight = tvb_get_letohs(tvb, offset);
    proto_tree_add_float(tree, hf_ambit_personal_weight, tvb, offset, 2, ((float)weight)/100);
    offset = 50;
    proto_tree_add_item(tree, hf_ambit_personal_birthyear, tvb, offset, 2, ENC_LITTLE_ENDIAN); 
    offset = 52;
    proto_tree_add_item(tree, hf_ambit_personal_max_hb, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 53;
    proto_tree_add_item(tree, hf_ambit_personal_rest_hb, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 54;
    proto_tree_add_item(tree, hf_ambit_personal_fitness_level, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 55;
    guint8 sex = tvb_get_guint8(tvb, offset);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_sex, tvb, offset, 1, "Is male", "%s", sex == 1 ? "true" : "false");
    offset = 56;
    proto_tree_add_item(tree, hf_ambit_personal_length, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 60;
    proto_tree_add_item(tree, hf_ambit_personal_alti_baro_profile, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset = 62;
    proto_tree_add_item(tree, hf_ambit_personal_alti_baro_fused_alti, tvb, offset, 1, ENC_LITTLE_ENDIAN);
}

static gint dissect_ambit_log_header_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint16 msg_id = tvb_get_letohs(tvb, 2);
    guint32 datalen = tvb_get_letohl(tvb, 4);

    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_log_header_seq, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_log_header_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (msg_id == 1) {
        proto_tree_add_item(tree, hf_ambit_log_header_sample_desc, tvb, offset, datalen, ENC_LITTLE_ENDIAN);
    }
    else if (msg_id == 2) {
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
        offset += 1;
        guint16 year = tvb_get_letohs(tvb, offset);
        guint8 month = tvb_get_guint8(tvb, offset + 2);
        guint8 day = tvb_get_guint8(tvb, offset + 3);
        proto_tree_add_string_format_value(tree, hf_ambit_log_header_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
        offset += 4;
        guint8 hour = tvb_get_guint8(tvb, offset);
        guint8 minute = tvb_get_guint8(tvb, offset + 1);
        guint16 seconds = tvb_get_guint8(tvb, offset + 1);
        proto_tree_add_string_format_value(tree, hf_ambit_log_header_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
        offset += 3;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 5);
        offset += 5;
        proto_tree_add_item(tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_ascent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_descent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_avg_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_altitude_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_altitude_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_activity_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_activity_name, tvb, offset, 16, ENC_LITTLE_ENDIAN);
        offset += 16;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_min, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 5);
        offset += 5;
        proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_sample_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_speed_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_alt_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_alt_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_temp_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_temp_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 8);
        offset += 8;
        proto_tree_add_item(tree, hf_ambit_log_header_time_first_fix, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_battery_start, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_battery_stop, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_distance_before_calib, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
    }
}

static gint dissect_ambit_log_header_unwind_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_unwind_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_tree_add_item(tree, hf_ambit_log_header_more, tvb, 0, 4, ENC_LITTLE_ENDIAN);
}

static gint dissect_ambit_log_header_next_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_next_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_tree_add_item(tree, hf_ambit_log_header_more, tvb, 0, 4, ENC_LITTLE_ENDIAN);
}

static gint dissect_ambit_log_data_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_log_data_address, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_ambit_log_data_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
}

static gint dissect_ambit_log_data_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint32 address = tvb_get_letohl(tvb, 0);
    guint32 length = tvb_get_letohl(tvb, 4);
    gint pmem_offset = -1;
    guint16 header_1_len;

    proto_tree_add_item(tree, hf_ambit_log_data_address, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_ambit_log_data_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    if (address == 0x000f4240) {
        /* Known start address, then we know a bit about the header... */
        proto_tree_add_item(tree, hf_ambit_log_data_last_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_first_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_entry_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_next_free_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
    }

    // Loop through data to find PEM start
    int i;
    guint8 a,b,c,d;
    for(i=0; i<length; i++) {
        if((a = tvb_get_guint8(tvb, i)) == 'P') {
            b = tvb_get_guint8(tvb, i+1);
            c = tvb_get_guint8(tvb, i+2);
            d = tvb_get_guint8(tvb, i+3);

            if (b == 'M' && c == 'E' && d == 'M') {
                offset = i;
                proto_tree_add_item(tree, hf_ambit_log_data_sof, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_data_next_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_data_prev_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 2 >= length) break;
                header_1_len = tvb_get_letohs(tvb, offset);
                proto_tree_add_item(tree, hf_ambit_log_header_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + header_1_len >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_sample_desc, tvb, offset, header_1_len, ENC_LITTLE_ENDIAN);
                offset += header_1_len;
                if (offset + 2 >= length) break;
                header_1_len = tvb_get_letohs(tvb, offset);
                proto_tree_add_item(tree, hf_ambit_log_header_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;

                if (offset + 1 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
                offset += 1;
                if (offset + 4 >= length) break;
                guint16 year = tvb_get_letohs(tvb, offset);
                guint8 month = tvb_get_guint8(tvb, offset + 2);
                guint8 day = tvb_get_guint8(tvb, offset + 3);
                proto_tree_add_string_format_value(tree, hf_ambit_log_header_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
                offset += 4;
                if (offset + 3 >= length) break;
                guint8 hour = tvb_get_guint8(tvb, offset);
                guint8 minute = tvb_get_guint8(tvb, offset + 1);
                guint16 seconds = tvb_get_guint8(tvb, offset + 1);
                proto_tree_add_string_format_value(tree, hf_ambit_log_header_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
                offset += 3;
                if (offset + 5 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 5);
                offset += 5;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_ascent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_descent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_avg_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_altitude_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_altitude_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_activity_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 16 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_activity_name, tvb, offset, 16, ENC_LITTLE_ENDIAN);
                offset += 16;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_hr_min, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 5 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 5);
                offset += 5;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_sample_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_speed_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_alt_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_alt_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_hr_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_hr_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_temp_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_temp_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
                if (offset + 8 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 8);
                offset += 8;
                if (offset + 2 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_time_first_fix, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_battery_start, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 1 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_battery_stop, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                if (offset + 4 >= length) break;
                dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
                offset += 4;
                if (offset + 4 >= length) break;
                proto_tree_add_item(tree, hf_ambit_log_header_distance_before_calib, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;

                i = offset;
            }
        }
    }

            static int hf_ambit_log_data_sof = -1;
static int hf_ambit_log_data_first_addr = -1;
static int hf_ambit_log_data_next_free_addr = -1;
static int hf_ambit_log_data_next_addr = -1;
static int hf_ambit_log_data_prev_addr = -1;
}

static gint
dissect_ambit(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint8 usbid = tvb_get_guint8(tvb, 0);
    guint8 len = tvb_get_guint8(tvb, 1) + 2;
    guint8 msg_part = tvb_get_guint8(tvb, 2);
    guint8 data_header_len = tvb_get_guint8(tvb, 3);
    guint16 msg_count = tvb_get_letohs(tvb, 4);
    guint32 command = tvb_get_ntohl(tvb, 8);
    guint32 pkt_len = tvb_get_letohl(tvb, 16);
    tvbuff_t *new_tvb = NULL, *next_tvb = NULL;
    static guint32 fragments_start_frame;
    static guint16 fragments_offset;
    static guint16 fragments_data_len;
    int data_len = 0;
    const ambit_protocol_type_t *subdissector;

    if (usbid == D_AMBIT_USBID) {
        if (msg_part == 0x5d) {
            data_len = data_header_len - 12;
        }
        else {
            data_len = data_header_len;
        }

        while (pinfo->fd->num + 1 > reassembly_entries_alloc) {
            reassembly_entries = g_realloc(reassembly_entries, sizeof(ambit_reassembly_entry_t)*(reassembly_entries_alloc + 10000));
            memset(reassembly_entries+reassembly_entries_alloc, 0, sizeof(ambit_reassembly_entry_t)*10000);
            reassembly_entries_alloc += 10000;
        }
        if (!pinfo->fd->flags.visited) {
            if (msg_part == 0x5d && msg_count > 1) {
                reassembly_entries[pinfo->fd->num].valid = 2;
                reassembly_entries[pinfo->fd->num].command = command;
                reassembly_entries[pinfo->fd->num].start_frame = pinfo->fd->num;
                reassembly_entries[pinfo->fd->num].frame_count = msg_count;
                reassembly_entries[pinfo->fd->num].size = pkt_len;
                reassembly_entries[pinfo->fd->num].data = (unsigned char*)g_malloc(pkt_len);
                fragments_start_frame = pinfo->fd->num;
                fragments_offset = data_len;
                fragments_data_len = pkt_len;

                tvb_memcpy(tvb, reassembly_entries[pinfo->fd->num].data, 20, data_len);
            }
            else if (msg_part == 0x5e) {
                if (fragments_data_len >= fragments_offset + data_len) {
                    //fragments_start_frame = pinfo->fd->num-msg_count;
                    reassembly_entries[pinfo->fd->num].valid = 2;
                    reassembly_entries[pinfo->fd->num].command = reassembly_entries[fragments_start_frame].command;
                    reassembly_entries[pinfo->fd->num].start_frame = fragments_start_frame;
                    reassembly_entries[pinfo->fd->num].frame_count = reassembly_entries[fragments_start_frame].frame_count;
                    reassembly_entries[pinfo->fd->num].size = fragments_data_len;
                    reassembly_entries[pinfo->fd->num].data = reassembly_entries[fragments_start_frame].data;
                    tvb_memcpy(tvb, &(reassembly_entries[fragments_start_frame].data[fragments_offset]), 8, data_len);
                    fragments_offset += data_len;
                }
                else {
                }
            }
            else {
                reassembly_entries[pinfo->fd->num].valid = 1;
                reassembly_entries[pinfo->fd->num].command = command;
            }
        }

        if (tree) { /* we are being asked for details */
            proto_item *ti = NULL;
            proto_tree *ambit_tree = NULL;
            proto_item *data_ti = NULL;
            proto_tree *data_tree = NULL;

            col_set_str(pinfo->cinfo, COL_PROTOCOL, "Ambit");

            ti = proto_tree_add_item(tree, proto_ambit, tvb, 0, len, ENC_NA);
            ambit_tree = proto_item_add_subtree(ti, ett_ambit);
            offset = 2;
            proto_tree_add_item(ambit_tree, hf_ambit_msgpart, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset += 1;
            proto_tree_add_item(ambit_tree, hf_ambit_msglength, tvb, offset, 1, ENC_BIG_ENDIAN);
            offset += 1;
            if (msg_part == 0x5d) {
                proto_tree_add_item(ambit_tree, hf_ambit_msgsegs, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            }
            else if (msg_part == 0x5e) {
                proto_tree_add_item(ambit_tree, hf_ambit_msgseg, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            }
            offset += 2;
            proto_tree_add_item(ambit_tree, hf_ambit_msgheaderchksum, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            if (msg_part == 0x5d) {
                proto_tree_add_item(ambit_tree, hf_ambit_requestcmd, tvb, offset, 4, ENC_BIG_ENDIAN);
                offset += 4;
                offset += 2;
                proto_tree_add_item(ambit_tree, hf_ambit_pktseqno, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                proto_tree_add_item(ambit_tree, hf_ambit_pktlen, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
            }

            if (reassembly_entries[pinfo->fd->num].valid == 1) {
                new_tvb = tvb_new_subset_remaining(tvb, offset);
                pkt_len = data_len;
                col_add_fstr(pinfo->cinfo, COL_INFO, "");
            }
            else if (reassembly_entries[pinfo->fd->num].start_frame + reassembly_entries[pinfo->fd->num].frame_count - 1 == pinfo->fd->num) {
                new_tvb = tvb_new_real_data(reassembly_entries[pinfo->fd->num].data, reassembly_entries[pinfo->fd->num].size, reassembly_entries[pinfo->fd->num].size);
                //tvb_set_child_real_data_tvbuff(tvb, new_tvb);
                add_new_data_source(pinfo, new_tvb, "Reassembled");
                pkt_len = reassembly_entries[pinfo->fd->num].size;

                col_add_fstr(pinfo->cinfo, COL_INFO, " (#%u of #%u) Reassembled", pinfo->fd->num-reassembly_entries[pinfo->fd->num].start_frame + 1, reassembly_entries[pinfo->fd->num].frame_count);
            }
            else {
                col_add_fstr(pinfo->cinfo, COL_INFO, " (#%u of #%u)", pinfo->fd->num-reassembly_entries[pinfo->fd->num].start_frame + 1, reassembly_entries[pinfo->fd->num].frame_count);
            }

            subdissector = find_subdissector(reassembly_entries[pinfo->fd->num].command);
            if (subdissector != NULL) {
                col_prepend_fstr(pinfo->cinfo, COL_INFO, "%s", subdissector->name);
            }

            if (new_tvb != NULL) {
                data_ti = proto_tree_add_item(ambit_tree, proto_ambit, new_tvb, 0, pkt_len, ENC_NA);
                data_tree = proto_item_add_subtree(ambit_tree, ett_ambit_data);
                //proto_tree_add_text(ambit_tree, new_tvb, offset, data_len, "Payload");

                if (subdissector != NULL) {
                    subdissector->dissector(new_tvb, pinfo, data_tree, data);
                }
            }

            offset += data_len;
            proto_tree_add_item(ambit_tree, hf_ambit_payloadchksum, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
        }

        return len;
    }

    return 0;
}

void
proto_register_ambit(void)
{
    static hf_register_info hf[] = {
        { &hf_ambit_unknown,
          { "Unknown", "ambit.unknown", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_usbid,
          { "USB-ID", "ambit.usbid", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_usblength,
          { "USB length", "ambit.usblen", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_msgpart,
          { "msg part", "ambit.msgpart", FT_UINT8, BASE_HEX, VALS(msgpart_index_vals), 0, NULL, HFILL } },
        { &hf_ambit_msglength,
          { "msg length", "ambit.msglen", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_msgsegs,
          { "msg parts", "ambit.msgsegs", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_msgseg,
          { "msg part", "ambit.msgseg", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_msgheaderchksum,
          { "msg header checksum", "ambit.msgheaderchksum", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_requestcmd,
          { "Command", "ambit.command", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_pktseqno,
          { "Packet sequence no", "ambit.pktseqno", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_pktlen,
          { "Packet length", "ambit.pktlen", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_payloadchksum,
          { "Paypload checksum", "ambit.payloadchksum", FT_UINT16, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_date,
          { "Date", "ambit.date", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_time,
          { "Time", "ambit.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_charge,
          { "Charge", "ambit.charge", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_model,
          { "Model", "ambit.model", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_serial,
          { "Serial", "ambit.serial", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_fw_version,
          { "FW version", "ambit.fwversion", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_hw_version,
          { "HW version", "ambit.hwversion", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_compass_declination,
          { "Compass declination", "ambit.personal.compass_declination", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_map_orientation,
          { "Map orientation", "ambit.personal.map_orientation", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_weight,
          { "Weight", "ambit.personal.weight", FT_FLOAT, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_date_format,
          { "Date format", "ambit.personal.date_format", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_time_format,
          { "Time format", "ambit.personal.time_format", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_coordinate_system,
          { "Coordinate system", "ambit.personal.coordinate_system", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_unit_system,
          { "Units", "ambit.personal.units", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_personal_alarm_enable,
          { "Alarm enable", "ambit.personal.alarm_enable", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_alarm_time,
          { "Alarm time", "ambit.personal.alarm_time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_dual_time,
          { "Dual time", "ambit.personal.dual_time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_gps_sync_time,
          { "Sync time w. GPS", "ambit.personal.gps_sync_time", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_alti_baro_fused_alti,
          { "Fused altitude", "ambit.personal.fused_alti", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_alti_baro_profile,
          { "Baro profile", "ambit.personal.baro_profile", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_max_hb,
          { "Max heart beat", "ambit.personal.maxhb", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_rest_hb,
          { "Rest heart rate", "ambit.personal.resthb", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_birthyear,
          { "Birthyear", "ambit.personal.birthyear", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_personal_length,
          { "Length", "ambit.personal.length", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_sex,
          { "IsMale", "ambit.personal.ismale", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_fitness_level,
          { "Fitness level", "ambit.personal.fitness_level", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_backlight_brightness,
          { "Backlight brightness", "ambit.personal.backlight_brightness", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_backlight_mode,
          { "Backlight mode", "ambit.personal.backlight_mode", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_display_contrast,
          { "Display contrast", "ambit.personal.display_contrast", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_invert_display,
          { "Invert display", "ambit.personal.invert_display", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_lock_sports_mode,
          { "Button lock, sports", "ambit.personal.lock_sportsmode", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_lock_time_mode,
          {  "Button lock, time", "ambit.personal.lock_timemode", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_tones,
          { "Tones", "ambit.personal.tones", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_header_seq,
          { "Header part", "ambit.log_header.header_part", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_length,
          { "Length", "ambit.log_header.length", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_sample_desc,
          { "Samples description???", "ambit.log_header.samples_desc", FT_BYTES, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_date,
          { "Date", "ambit.log_header.date", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_time,
          { "Time", "ambit.log_header.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_duration,
          { "Duration (1/10 sec)", "ambit.log_header.duration", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_ascent,
          { "Ascent", "ambit.log_header.ascent", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_descent,
          { "Descent", "ambit.log_header.descent", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_ascent_time,
          { "Ascent time (sec)", "ambit.log_header.ascent_time", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_descent_time,
          { "Descent time (sec)", "ambit.log_header.descent_time", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_recovery,
          { "Recovery time (minutes)", "ambit.log_header.recovery", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_avg_speed,
          { "Avg speed (km/h * 100)", "ambit.log_header.avg_speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_max_speed,
          { "Max speed (km/h * 100)", "ambit.log_header.max_speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_altitude_max,
          { "Altitude max", "ambit.log_header.alt_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_altitude_min,
          { "Altitude min", "ambit.log_header.alt_min", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_avg,
          { "Heartrate avg", "ambit.log_header.hr_avg", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_max,
          { "Heartrate max", "ambit.log_header.hr_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_peak_effect,
          { "Peak training effect (x10)", "ambit.log_header.peak_effect", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_activity_type,
          { "Activity type", "ambit.log_header.activity_type", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_activity_name,
          { "Activity name", "ambit.log_header.activity_name", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_min,
          { "Heartrate min", "ambit.log_header.hr_min", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_distance,
          { "Distance (m)", "ambit.log_header.distance", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_sample_count,
          { "Sample count", "ambit.log_header.sample_count", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_energy,
          { "Energy consumption (kcal)", "ambit.log_header.energy_consumption", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_speed_max_time,
          { "Time max speed (ms)", "ambit.log_header.speed_maxtime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_alt_max_time,
          { "Time max altitude (ms)", "ambit.log_header.alt_maxtime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_alt_min_time,
          { "Time min altitude (ms)", "ambit.log_header.alt_mintime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_max_time,
          { "Time max HR (ms)", "ambit.log_header.hr_maxtime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_min_time,
          { "Time min HR (ms)", "ambit.log_header.hr_mintime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_temp_max_time,
          { "Time max temperature (ms)", "ambit.log_header.temp_maxtime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_temp_min_time,
          { "Time min temperature (ms)", "ambit.log_header.temp_mintime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_time_first_fix,
          { "Time of first fix", "ambit.log_header.time_first_fix", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_battery_start,
          { "Battery at start", "ambit.log_header.battery_start", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_battery_stop,
          { "Battery at end", "ambit.log_header.battery_stop", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_distance_before_calib,
          { "Distance before calibration", "ambit.log_header.distance_before_calib", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_header_more,
          { "More values", "ambit.log_header.more", FT_UINT32, BASE_HEX, VALS(log_header_more_vals), 0, NULL, HFILL } },

        { &hf_ambit_log_data_address,
          { "Address", "ambit.log_data.address", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_length,
          { "Length", "ambit.log_data.length", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_sof,
          { "Start of entry", "ambit.log_data.start_of_entry", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_first_addr,
          { "First entry address", "ambit.log_data.first_addr", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_last_addr,
          { "Last entry address", "ambit.log_data.last_addr", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_entry_count,
          { "Number of entries", "ambit.log_data.entry_count", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_next_free_addr,
          { "Next free address", "ambit.log_data.next_free_addr", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_next_addr,
          { "Next entry address", "ambit.log_data.next_addr", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_data_prev_addr,
          { "Previous entry address", "ambit.log_data.prev_addr", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
    };

    static gint *ett[] = {
        &ett_ambit,
        &ett_ambit_data,
    };

    static ei_register_info ei[] = {
        { &ei_ambit_undecoded, { "ambit.undecoded", PI_UNDECODED, PI_WARN, "Not dissected yet", EXPFILL }},
    };

    expert_module_t *expert_ambit;

    proto_ambit = proto_register_protocol (
        "Suunto Ambit USB Protocol",
        "Ambit",
        "ambit"
        );

    proto_register_field_array(proto_ambit, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    expert_ambit = expert_register_protocol(proto_ambit);
    expert_register_field_array(expert_ambit, ei, array_length(ei));
    //register_dissector("ambit", dissect_ambit, proto_ambit);
}

void
proto_reg_handoff_ambit(void)
{
    static dissector_handle_t ambit_handle;

    //ambit_handle = find_dissector("ambit");
    ambit_handle = new_create_dissector_handle(dissect_ambit, proto_ambit);
    dissector_add_uint("usb.interrupt", IF_CLASS_UNKNOWN, ambit_handle);
    dissector_add_uint("usb.interrupt", IF_CLASS_HID, ambit_handle);
}
