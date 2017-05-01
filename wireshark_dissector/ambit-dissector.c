#include "config.h"

#include "stdio.h"
#include <glib.h>
#include <epan/conversation.h>
#include <epan/expert.h>
#include <epan/packet.h>
#include <epan/reassemble.h>
#include <epan/dissectors/packet-usb.h>

typedef struct ambit_reassembly_entry {
    guint8 valid;
    guint32 command;
    guint32 frame_index;
    guint32 frame_total;
    guint32 size;
    unsigned char *data;
    struct {
        guint32 entry_size;
        unsigned char *entry;
        guint32 start_frame;
    } log;
    struct {
        guint32 entry_size;
        unsigned char *entry;
        guint32 start_frame;
    } log_header;
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
static gint dissect_ambit_log_header_peek_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_peek_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_step_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_header_step_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_count_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_count_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_data_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_data_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_log_data_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length);
static gint dissect_ambit_log_data_sample(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length, guint32 *sampleno, guint32 *periodic_sample_specifier);
static gint dissect_ambit_lock_status_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_lock_status_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_lock_set(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_lock_set_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_gps_data_peek_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_gps_data_peek_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_data_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_data_write_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit_data_write_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length);
//static uint dissect_ambit_data_write_read_data(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_get_length_to_read(guint32 *offset, guint32 length, guint16 packageLength);
static gint dissect_ambit_data_write_sport_modes(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 address, guint32 length);
static gint dissect_ambit_data_write_apps(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 address, guint32 length);
static uint dissect_ambit_data_write_read_data_0101(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0102(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0105(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0106(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0107(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0108(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_0109(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_010a(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);
static uint dissect_ambit_data_write_read_data_010c(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength);

static gint dissect_ambit3_settings_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit3_settings_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit3_log_headers_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit3_log_headers_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit3_log_headers_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length);
static gint dissect_ambit3_device_compact_serial_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);
static gint dissect_ambit3_device_compact_serial_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);

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
static int hf_ambit_pktformat = -1;
static int hf_ambit_pktseqno = -1;
static int hf_ambit_pktlen = -1;
static int hf_ambit_payloadchksum = -1;

static int hf_ambit_unknown = -1;

static int hf_ambit_date = -1;
static int hf_ambit_time = -1;
static int hf_ambit_charge = -1;
static int hf_ambit_komposti_version = -1;
static int hf_ambit_model = -1;
static int hf_ambit_serial = -1;
static int hf_ambit_compact_serial = -1;
static int hf_ambit_fw_version = -1;
static int hf_ambit_hw_version = -1;
static int hf_ambit_bsl_version = -1;
static int hf_ambit_personal_compass_declination = -1;
static int hf_ambit_personal_map_orientation = -1;
static int hf_ambit_personal_date_format = -1;
static int hf_ambit_personal_time_format = -1;
static int hf_ambit_personal_unit_system = -1;
static int hf_ambit_personal_coordinate_system = -1;
static int hf_ambit_personal_language = -1;
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
static int hf_ambit_log_header_temp_max = -1;
static int hf_ambit_log_header_temp_min = -1;
static int hf_ambit_log_header_distance = -1;
static int hf_ambit_log_header_sample_count = -1;
static int hf_ambit_log_header_energy = -1;
static int hf_ambit_log_header_cadence_max = -1;
static int hf_ambit_log_header_cadence_avg = -1;
static int hf_ambit_log_header_swimming_lengths = -1;
static int hf_ambit_log_header_speed_max_time = -1;
static int hf_ambit_log_header_alt_max_time = -1;
static int hf_ambit_log_header_alt_min_time = -1;
static int hf_ambit_log_header_hr_max_time = -1;
static int hf_ambit_log_header_hr_min_time = -1;
static int hf_ambit_log_header_temp_max_time = -1;
static int hf_ambit_log_header_temp_min_time = -1;
static int hf_ambit_log_header_cadence_max_time = -1;
static int hf_ambit_log_header_swimming_pool_length = -1;
static int hf_ambit_log_header_time_first_fix = -1;
static int hf_ambit_log_header_battery_start = -1;
static int hf_ambit_log_header_battery_stop = -1;
static int hf_ambit_log_header_distance_before_calib = -1;
static int hf_ambit_log_header_synced = -1;

static int hf_ambit_log_header_more = -1;

static int hf_ambit_log_count = -1;
static int hf_ambit_unsynced_log_count = -1;

static int hf_ambit_log_data_addr_frame_ref = -1;
static int hf_ambit_log_data_address = -1;
static int hf_ambit_log_data_length = -1;
static int hf_ambit_log_data_sof = -1;
static int hf_ambit_log_data_first_addr = -1;
static int hf_ambit_log_data_last_addr = -1;
static int hf_ambit_log_data_entry_count = -1;
static int hf_ambit_log_data_next_free_addr = -1;
static int hf_ambit_log_data_next_addr = -1;
static int hf_ambit_log_data_prev_addr = -1;

static int hf_ambit_log_sample_length = -1;
static int hf_ambit_log_sample_peri_spec_count = -1;
static int hf_ambit_log_sample_type = -1;
static int hf_ambit_log_sample_peri_spec_id = -1;
static int hf_ambit_log_sample_peri_spec_offset = -1;
static int hf_ambit_log_sample_peri_spec_length = -1;
static int hf_ambit_log_sample_periodic_distance = -1;
static int hf_ambit_log_sample_periodic_speed = -1;
static int hf_ambit_log_sample_periodic_hr = -1;
static int hf_ambit_log_sample_periodic_time = -1;
static int hf_ambit_log_sample_periodic_altitude = -1;
static int hf_ambit_log_sample_periodic_energy = -1;
static int hf_ambit_log_sample_periodic_temp = -1;
static int hf_ambit_log_sample_periodic_pressure = -1;
static int hf_ambit_log_sample_periodic_vert_speed = -1;

static int hf_ambit_log_other_time_offset = -1;
static int hf_ambit_log_other_type = -1;
static int hf_ambit_log_activity_type = -1;
static int hf_ambit_log_activity_custom_mode_id = -1;
static int hf_ambit_log_time_event_type = -1;
static int hf_ambit_log_time_event_date = -1;
static int hf_ambit_log_time_event_time = -1;
static int hf_ambit_log_time_event_duration = -1;
static int hf_ambit_log_time_event_distance = -1;

static int hf_ambit_log_gps_base_navtype = -1;
static int hf_ambit_log_gps_base_date = -1;
static int hf_ambit_log_gps_base_time = -1;
static int hf_ambit_log_gps_base_latitude = -1;
static int hf_ambit_log_gps_base_longitude = -1;
static int hf_ambit_log_gps_base_altitude = -1;
static int hf_ambit_log_gps_base_speed = -1;
static int hf_ambit_log_gps_base_heading = -1;
static int hf_ambit_log_gps_base_ehpe = -1;
static int hf_ambit_log_gps_base_satelite_no = -1;
static int hf_ambit_log_gps_base_hdop = -1;
static int hf_ambit_log_gps_base_sv = -1;
static int hf_ambit_log_gps_base_state = -1;
static int hf_ambit_log_gps_base_snr = -1;

static int hf_ambit_log_gps_small_time = -1;
static int hf_ambit_log_gps_small_latitude = -1;
static int hf_ambit_log_gps_small_longitude = -1;
static int hf_ambit_log_gps_small_ehpe = -1;
static int hf_ambit_log_gps_small_satelite_no = -1;

static int hf_ambit_log_gps_tiny_time = -1;
static int hf_ambit_log_gps_tiny_latitude = -1;
static int hf_ambit_log_gps_tiny_longitude = -1;

static int hf_ambit_log_distance_source_type = -1;

static int hf_ambit_log_altitude_source_type = -1;
static int hf_ambit_log_altitude_source_altitude_offset = -1;
static int hf_ambit_log_altitude_source_pressure_offset = -1;

static int hf_ambit_log_cadence_source_type = -1;

static int hf_ambit_log_fwinfo_build_date = -1;

static int hf_ambit_log_swimming_turn_distance = -1;
static int hf_ambit_log_swimming_turn_lengths = -1;
static int hf_ambit_log_swimming_turn_lengths_wo_change = -1;
static int hf_ambit_log_swimming_turn_classification0 = -1;
static int hf_ambit_log_swimming_turn_classification1 = -1;
static int hf_ambit_log_swimming_turn_classification2 = -1;
static int hf_ambit_log_swimming_turn_classification3 = -1;
static int hf_ambit_log_swimming_turn_prev_style = -1;

static int hf_ambit_log_delayed_store = -1;

static int hf_ambit_log_ibi = -1;

static int hf_ambit_gps_data_head = -1;

static gint ett_ambit = -1;
static gint ett_ambit_data = -1;
static gint ett_ambit_log_data = -1;
static gint ett_ambit_log_samples = -1;
static gint ett_ambit_log_sample = -1;
static gint ett_ambit3_log_headers = -1;
static gint ett_ambit3_log_header = -1;

static int hf_ambit_write_data_length = -1;
static int hf_ambit_write_data_header = -1;
static int hf_ambit_write_data_activity_id = -1;
static gint hf_ambit_write_data_interval_repetitions = -1;
static gint hf_ambit_write_data_show_navigation = -1;
static gint hf_ambit_write_data_alti_baro_mode = -1;
static gint hf_ambit_write_data_use_hrbelt_and_pods = -1;
static gint hf_ambit_write_data_recording_interval = -1;
static gint hf_ambit_write_data_auto_lap = -1;
static gint hf_ambit_write_data_auto_pause = -1;
static gint hf_ambit_write_auto_scroll = -1;
static gint hf_ambit_write_data_use_interval_timer = -1;
static gint hf_ambit_write_data_interval_timer_time = -1;
static gint hf_ambit_write_data_interval_timer_max = -1;
static gint hf_ambit_write_data_interval_timer_min = -1;
static gint hf_ambit_write_data_use_hr_limits = -1;
static gint hf_ambit_write_data_backlight = -1;
static gint hf_ambit_write_data_display = -1;
static gint hf_ambit_write_data_display_type = -1;
static gint hf_ambit_write_quick_navigation  = -1;
static gint hf_ambit_write_data_gps_interval = -1;
static gint hf_ambit_log_activity_custom_mode_row = -1;
static gint hf_ambit_log_activity_custom_mode_display = -1;
static gint hf_ambit_log_activity_custom_mode_view = -1;

static gint hf_ambit_write_data_apps_nbr_of = -1;
static gint hf_ambit_write_data_apps_header_len = -1;
static gint hf_ambit_write_data_apps_end_pos = -1;
static gint hf_ambit_write_data_app_data = -1;
static gint hf_ambit_write_data_app_checksum = -1;
static gint hf_ambit_write_data_app_index = -1;

static ambit_reassembly_entry_t *reassembly_entries = NULL;
static guint32 reassembly_entries_alloc = 0;

static guint32 address_to_frame_lookup[4096];

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

static const value_string log_samples_type_vals[] = {
    { 0x00, "Periodic sample specifier" },
    { 0x02, "Periodic sample" },
    { 0x03, "Other sample" },
    { 0, NULL }
};

static const value_string log_samples_spec_type_vals[] = {
    { 0x03, "Distance" },
    { 0x04, "Speed" },
    { 0x05, "HR" },
    { 0x06, "Time" },
    { 0x0c, "Altitude" },
    { 0x0e, "Energy consumption" },
    { 0x0f, "Temperature" },
    { 0x18, "Pressure" },
    { 0x19, "Vertical speed" },
    { 0, NULL }
};

static const value_string log_samples_time_event_type_vals[] = {
    { 0x01, "Manual lap" },
    { 0x14, "High interval end" },
    { 0x15, "Low interval end" },
    { 0x16, "Interval start" },
    { 0x1e, "Pause" },
    { 0x1f, "Start" },
    { 0, NULL }
};

static const value_string log_samples_distance_source_type_vals[] = {
    { 0x00, "Bikepod" },
    { 0x01, "Footpod" },
    { 0x02, "GPS" },
    { 0x03, "Wrist" },
    { 0x04, "Indoorswimming" },
    { 0x05, "Outdoorswimming" },
    { 0, NULL }
};

static const value_string log_samples_altitude_source_type_vals[] = {
    { 0x04, "Pressure" },
    { 0, NULL }
};

static const value_string log_samples_cadence_source_type_vals[] = {
    { 0x40, "Wrist" },
    { 0, NULL }
};

static const value_string log_samples_swimming_style_vals[] = {
    { 0x00, "Other" },
    { 0x01, "Butterfly" },
    { 0x02, "Backstroke" },
    { 0x03, "Breaststroke" },
    { 0x04, "Freestyle" },
    { 0x05, "Drill" },
    { 0, NULL }
};

static const value_string custom_modes_alti_baro_mode_vals[] = {
    { 0x00, "Alti" },
    { 0x01, "Baro" },
    { 0x02, "Automatic" },
    { 0, NULL }
};

static const value_string custom_modes_backlight_vals[] = {
    { 0x00, "Normal" },
    { 0x01, "Off" },
    { 0x02, "Night" },
    { 0x03, "Toggle" },
    { 0xff, "Default" },
    { 0, NULL }
};

static const value_string custom_modes_display_vals[] = {
    { 0x00, "Light" },
    { 0x01, "Dark" },
    { 0xff, "Default" },
    { 0, NULL }
};

static const value_string custom_modes_quick_navigation_vals[] = {
    { 0x00, "Off" },
    { 0x01, "POI" },
    { 0x02, "Route" },
    { 0, NULL }
};

static const value_string custom_modes_gps_accuracy_vals[] = {
    { 0x00, "GPS Off" },
    { 0x01, "GPS Best (16h)" },
    { 0x05, "GPS Good (24h)" },
    { 0x3c, "GPS Ok (50h)" },
    { 0, NULL }
};

static const value_string custom_modes_interval_timer_unit_vals[] = {
    { 0x0000, "meter" },
    { 0x0100, "sec" },
    { 0, NULL }
};

static const value_string custom_modes_display_type_val[] = {
    { 0x0101, "Barograph display" },
    { 0x0104, "3 rows display" },
    { 0x0105, "2 rows display" },
    { 0x0106, "1 row display" },
    { 0, NULL }
};

static const value_string custom_modes_header_vals[] = {
    { 0x0100, "[Custom modes header]" },
    { 0x0101, "[Custom mode activity header]" },
    { 0x0102, "[Custom mode activity settings header]" },
    { 0x0104, "[Unknown header] ????" },
    { 0x0105, "[Display header]" },
    { 0x0106, "[Display page header]" },
    { 0x0107, "[Display ???? header]" },
    { 0x0108, "[Display row ???? header]" },
    { 0x0109, "[Row header]" },
    { 0x010a, "[View header (lower row)]" },
    { 0x010b, "[Unknown header] ????" },
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
    { 0x0b060500, "Get log count", dissect_ambit_log_count_get },
    { 0x0b060a00, "Log count reply", dissect_ambit_log_count_reply },
    { 0x0b070500, "Log header unwind request", dissect_ambit_log_header_unwind_get },
    { 0x0b070a00, "Log header unwind reply", dissect_ambit_log_header_unwind_reply },
    { 0x0b080500, "Log header peek request", dissect_ambit_log_header_peek_get },
    { 0x0b080a00, "Log header peek reply", dissect_ambit_log_header_peek_reply },
    { 0x0b0a0500, "Log header step request", dissect_ambit_log_header_step_get },
    { 0x0b0a0a00, "Log header step reply", dissect_ambit_log_header_step_reply },
    { 0x0b0b0500, "Get log header", dissect_ambit_log_header_get },
    { 0x0b0b0a00, "Log header reply", dissect_ambit_log_header_reply },
    { 0x0b150500, "GPS data peek request", dissect_ambit_gps_data_peek_get },
    { 0x0b150a00, "GPS data peek reply", dissect_ambit_gps_data_peek_reply },
    { 0x0b160500, "Data write", dissect_ambit_data_write },
    { 0x0b160a00, "Data write reply", dissect_ambit_data_write_reply },
    { 0x0b170500, "Get log data", dissect_ambit_log_data_get },
    { 0x0b170a00, "Log data reply", dissect_ambit_log_data_reply },
    { 0x0b190500, "Get lock status", dissect_ambit_lock_status_get },
    { 0x0b190a00, "Lock status reply", dissect_ambit_lock_status_reply },
    { 0x0b1a0500, "Lock set", dissect_ambit_lock_set },
    { 0x0b1a0a00, "Lock set reply", dissect_ambit_lock_set_reply },
    { 0x0b1e0500, "Ambit3 - Get Compact serial", dissect_ambit3_device_compact_serial_get },
    { 0x0b1e0a00, "Ambit3 - Compact serial reply", dissect_ambit3_device_compact_serial_reply },
    { 0x11000500, "Ambit3 - Get settings", dissect_ambit3_settings_get },
    { 0x11000a00, "Ambit3 - Settings reply", dissect_ambit3_settings_reply },
    { 0x11010500, "Ambit3 - Write settings", dissect_ambit3_settings_reply },
    { 0x11010a00, "Ambit3 - Settings write reply", dissect_ambit3_settings_get },
    { 0x12000500, "Ambit3 - Get log headers", dissect_ambit3_log_headers_get },
    { 0x12000a00, "Ambit3 - Log headers reply", dissect_ambit3_log_headers_reply },
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
#if VERSION_MAJOR >= 1 && VERSION_MINOR >= 11
    /* TODO port to new expert info API */
#else
    expert_add_info_format(pinfo, unknown_item, PI_UNDECODED, PI_WARN, "Not dissected yet");
#endif
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
    guint16 year = tvb_get_letohs(tvb, 0);
    guint8 month = tvb_get_guint8(tvb, 2);
    guint8 day = tvb_get_guint8(tvb, 3);
    guint8 hour = tvb_get_guint8(tvb, 4);
    guint8 minute = tvb_get_guint8(tvb, 5);
    guint16 seconds = tvb_get_letohs(tvb, 6) / 1000;
    proto_tree_add_string_format_value(tree, hf_ambit_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
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
    guint8 kv1, kv2;
    guint16 kv3;
    gint offset = 0;
    kv1 = tvb_get_guint8(tvb, offset);
    kv2 = tvb_get_guint8(tvb, offset+1);
    kv3 = tvb_get_letohs(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_komposti_version, tvb, offset, 4, "Komposti version", "%d.%d.%d", kv1, kv2, kv3);
    offset +=4;
}

static gint dissect_ambit_device_info_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint8 fw1,fw2;
    guint16 fw3;
    guint8 hw1,hw2;
    guint16 hw3;
    guint8 bv1,bv2;
    guint16 bv3;
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_model, tvb, offset, 16, ENC_LITTLE_ENDIAN);
    offset += 16;
    proto_tree_add_item(tree, hf_ambit_serial, tvb, offset, 16, ENC_LITTLE_ENDIAN);
    offset += 16;
    fw1 = tvb_get_guint8(tvb, offset);
    fw2 = tvb_get_guint8(tvb, offset+1);
    fw3 = tvb_get_letohs(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_fw_version, tvb, offset, 4, "FW version", "%d.%d.%d", fw1, fw2, fw3);
    offset += 4;
    hw1 = tvb_get_guint8(tvb, offset);
    hw2 = tvb_get_guint8(tvb, offset+1);
    hw3 = tvb_get_letohs(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_hw_version, tvb, offset, 4, "HW version", "%d.%d.%d", hw1, hw2, hw3);
    offset += 4;
    bv1 = tvb_get_guint8(tvb, offset);
    bv2 = tvb_get_guint8(tvb, offset+1);
    bv3 = tvb_get_letohs(tvb, offset+2);
    proto_tree_add_string_format_value(tree, hf_ambit_bsl_version, tvb, offset, 4, "BSL version", "%d.%d.%d", bv1, bv2, bv3);
    offset += 4;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
    offset += 4;
}

static gint dissect_ambit3_device_compact_serial_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    proto_tree_add_string(tree, hf_ambit_compact_serial, tvb, offset, 17, "Ambit3 get compact serial");
    offset +=17;
}

static gint dissect_ambit3_device_compact_serial_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint8 compact_serial[11];
    int i;
    gint offset = 0;
    offset += 9;
    compact_serial[10] = 0;
    for(i=0; i<sizeof(compact_serial)-1; i++)
        compact_serial[i] = tvb_get_guint8(tvb, offset+i);
    proto_tree_add_string_format_value(tree, hf_ambit_compact_serial, tvb, offset, 10, "Ambit3 compact serial", "%s", compact_serial);
}

static gint dissect_ambit_personal_settings_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_personal_settings_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_lock_sports_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_lock_time_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    guint8 declination_direction = tvb_get_guint8(tvb, offset);
    guint8 declination_deg = tvb_get_guint8(tvb, offset + 1);
    if (declination_direction == 0) {
        proto_tree_add_string_format_value(tree, hf_ambit_personal_compass_declination, tvb, offset, 2, "Compass declination", "None");
    }
    else {
        proto_tree_add_string_format_value(tree, hf_ambit_personal_compass_declination, tvb, offset, 2, "Compass declination", "%s %f", declination_direction == 1 ? "E" : "W", 0.5*declination_deg);
    }
    offset += 2;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_personal_unit_system, tvb, offset, 11, ENC_LITTLE_ENDIAN);
    offset += 11;
    proto_tree_add_item(tree, hf_ambit_personal_coordinate_system, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_language, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_map_orientation, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_personal_gps_sync_time, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_time_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    guint8 alarm_hour = tvb_get_guint8(tvb, offset);
    guint8 alarm_minute = tvb_get_guint8(tvb, offset + 1);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_alarm_time, tvb, offset, 2, "Alarm time", "%d:%d", alarm_hour, alarm_minute);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_personal_alarm_enable, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    guint8 dual_hour = tvb_get_guint8(tvb, offset);
    guint8 dual_minute = tvb_get_guint8(tvb, offset + 1);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_dual_time, tvb, offset, 2, "Dual time", "%d:%d", dual_hour, dual_minute);
    offset += 2;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 3);
    offset += 3;
    proto_tree_add_item(tree, hf_ambit_personal_date_format, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 3);
    offset += 3;
    proto_tree_add_item(tree, hf_ambit_personal_tones, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 3);
    offset += 3;
    proto_tree_add_item(tree, hf_ambit_personal_backlight_mode, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_backlight_brightness, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_display_contrast, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_invert_display, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    guint16 weight = tvb_get_letohs(tvb, offset);
    proto_tree_add_float(tree, hf_ambit_personal_weight, tvb, offset, 2, ((float)weight)/100);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_personal_birthyear, tvb, offset, 2, ENC_LITTLE_ENDIAN); 
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_personal_max_hb, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_rest_hb, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_fitness_level, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    guint8 sex = tvb_get_guint8(tvb, offset);
    proto_tree_add_string_format_value(tree, hf_ambit_personal_sex, tvb, offset, 1, "Is male", "%s", sex == 1 ? "true" : "false");
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_length, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_alti_baro_profile, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    proto_tree_add_item(tree, hf_ambit_personal_alti_baro_fused_alti, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
}

static gint dissect_ambit_log_header_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    guint16 msg_id = tvb_get_letohs(tvb, 2);
    guint32 datalen = tvb_get_letohl(tvb, 4);
    guint32 header_1_len;

    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_log_header_seq, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    header_1_len = tvb_get_letohl(tvb, offset);
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
        guint16 seconds = tvb_get_guint8(tvb, offset + 2);
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
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_temp_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_temp_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_sample_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_cadence_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(tree, hf_ambit_log_header_cadence_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
        offset += 2;
        proto_tree_add_item(tree, hf_ambit_log_header_swimming_lengths, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
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
        proto_tree_add_item(tree, hf_ambit_log_header_cadence_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_header_swimming_pool_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
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
        if (datalen > offset) {
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 24);
            offset += 24;
            proto_tree_add_item(tree, hf_ambit_log_header_activity_name, tvb, offset, 16, ENC_LITTLE_ENDIAN);
            offset += 16;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
            offset += 4;
            year = tvb_get_letohs(tvb, offset);
            month = tvb_get_guint8(tvb, offset + 2);
            day = tvb_get_guint8(tvb, offset + 3);
            proto_tree_add_string_format_value(tree, hf_ambit_log_header_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
            offset += 4;
            hour = tvb_get_guint8(tvb, offset);
            minute = tvb_get_guint8(tvb, offset + 1);
            seconds = tvb_get_guint8(tvb, offset + 2);
            proto_tree_add_string_format_value(tree, hf_ambit_log_header_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
            offset += 3;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
            offset += 1;
            proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
            offset += 2;
            proto_tree_add_item(tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
            offset += 2;
            proto_tree_add_item(tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
            offset += 4;
            proto_tree_add_item(tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(tree, hf_ambit_log_header_activity_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, tree, offset, (header_1_len - 211));
            offset += (header_1_len - 211);
        }
    }
}

static gint dissect_ambit_log_count_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_count_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_log_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
}

static gint dissect_ambit_log_header_unwind_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_unwind_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_log_header_more, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
}

static gint dissect_ambit_log_header_peek_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_peek_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_log_header_more, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
}

static gint dissect_ambit_log_header_step_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_log_header_step_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    gint offset = 0;
    proto_tree_add_item(tree, hf_ambit_log_header_more, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
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
    
    guint32 link_addr;
    proto_item *pi;

    proto_tree_add_item(tree, hf_ambit_log_data_address, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    proto_tree_add_item(tree, hf_ambit_log_data_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;

    if (address == 0x000f4240) {
        /* Known start address, then we know a bit about the header... */
        proto_tree_add_item(tree, hf_ambit_log_data_last_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        link_addr = tvb_get_letohl(tvb, offset);
        pi = proto_tree_add_uint(tree, hf_ambit_log_data_addr_frame_ref, tvb, 0, 0, address_to_frame_lookup[(link_addr - 0x000f4240) / 0x400]);
        PROTO_ITEM_SET_GENERATED(pi);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_first_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        link_addr = tvb_get_letohl(tvb, offset);
        pi = proto_tree_add_uint(tree, hf_ambit_log_data_addr_frame_ref, tvb, 0, 0, address_to_frame_lookup[(link_addr - 0x000f4240) / 0x400]);
        PROTO_ITEM_SET_GENERATED(pi);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_entry_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        proto_tree_add_item(tree, hf_ambit_log_data_next_free_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        link_addr = tvb_get_letohl(tvb, offset);
        pi = proto_tree_add_uint(tree, hf_ambit_log_data_addr_frame_ref, tvb, 0, 0, address_to_frame_lookup[(link_addr - 0x000f4240) / 0x400]);
        PROTO_ITEM_SET_GENERATED(pi);
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
                i = dissect_ambit_log_data_content(tvb, pinfo, tree, data, offset, length);
            }
        }
    }
}

static gint dissect_ambit_log_data_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length)
{
    guint16 header_1_len;
    guint32 link_addr;
    guint32 sample_count = 0;
    proto_item *pi;
    proto_item *sample_ti = NULL;
    proto_tree *samples_tree = NULL;
    guint32 period_sample_spec;

    proto_tree_add_item(tree, hf_ambit_log_data_sof, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_data_next_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    link_addr = tvb_get_letohl(tvb, offset);
    pi = proto_tree_add_uint(tree, hf_ambit_log_data_addr_frame_ref, tvb, 0, 0, address_to_frame_lookup[(link_addr - 0x000f4240) / 0x400]);
    PROTO_ITEM_SET_GENERATED(pi);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_data_prev_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    link_addr = tvb_get_letohl(tvb, offset);
    pi = proto_tree_add_uint(tree, hf_ambit_log_data_addr_frame_ref, tvb, 0, 0, address_to_frame_lookup[(link_addr - 0x000f4240) / 0x400]);
    PROTO_ITEM_SET_GENERATED(pi);
    offset += 4;
    if (offset + 2 >= length) return offset;
    header_1_len = tvb_get_letohs(tvb, offset);
    proto_tree_add_item(tree, hf_ambit_log_header_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + header_1_len >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_sample_desc, tvb, offset, header_1_len, ENC_LITTLE_ENDIAN);
    period_sample_spec = offset - 2; // Include length as well
    offset += header_1_len;
    if (offset + 2 >= length) return offset;
    header_1_len = tvb_get_letohs(tvb, offset);
    proto_tree_add_item(tree, hf_ambit_log_header_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    
    if (offset + 1 >= length) return offset;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    if (offset + 4 >= length) return offset;
    guint16 year = tvb_get_letohs(tvb, offset);
    guint8 month = tvb_get_guint8(tvb, offset + 2);
    guint8 day = tvb_get_guint8(tvb, offset + 3);
    proto_tree_add_string_format_value(tree, hf_ambit_log_header_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
    offset += 4;
    if (offset + 3 >= length) return offset;
    guint8 hour = tvb_get_guint8(tvb, offset);
    guint8 minute = tvb_get_guint8(tvb, offset + 1);
    guint16 seconds = tvb_get_guint8(tvb, offset + 2);
    proto_tree_add_string_format_value(tree, hf_ambit_log_header_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
    offset += 3;
    if (offset + 5 >= length) return offset;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 5);
    offset += 5;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_ascent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_descent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_avg_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_altitude_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_altitude_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_activity_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 16 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_activity_name, tvb, offset, 16, ENC_LITTLE_ENDIAN);
    offset += 16;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_hr_min, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
    offset += 1;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_temp_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_temp_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_sample_count, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    sample_count = tvb_get_letohl(tvb, offset);
    offset += 4;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_cadence_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_cadence_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 2 >= length) return offset;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_swimming_lengths, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_speed_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_alt_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_alt_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_hr_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_hr_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_temp_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_temp_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_cadence_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_swimming_pool_length, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (offset + 2 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_time_first_fix, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_battery_start, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 1 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_battery_stop, tvb, offset, 1, ENC_LITTLE_ENDIAN);
    offset += 1;
    if (offset + 4 >= length) return offset;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
    offset += 4;
    if (offset + 4 >= length) return offset;
    proto_tree_add_item(tree, hf_ambit_log_header_distance_before_calib, tvb, offset, 4, ENC_LITTLE_ENDIAN);
    offset += 4;
    if (header_1_len >= 913) { /* Long header */
        if (offset + 24 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 24);
        offset += 24;
        if (offset + 16 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_activity_name, tvb, offset, 16, ENC_LITTLE_ENDIAN);
        offset += 16;
        if (offset + 4 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
        offset += 4;
        if (offset + 4 >= length) return offset;
        year = tvb_get_letohs(tvb, offset);
        month = tvb_get_guint8(tvb, offset + 2);
        day = tvb_get_guint8(tvb, offset + 3);
        proto_tree_add_string_format_value(tree, hf_ambit_log_header_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
        offset += 4;
        if (offset + 3 >= length) return offset;
        hour = tvb_get_guint8(tvb, offset);
        minute = tvb_get_guint8(tvb, offset + 1);
        seconds = tvb_get_guint8(tvb, offset + 2);
        proto_tree_add_string_format_value(tree, hf_ambit_log_header_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
        offset += 3;
        if (offset + 1 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
        offset += 1;
        if (offset + 4 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        if (offset + 4 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
        offset += 4;
        if (offset + 2 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        if (offset + 2 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
        offset += 2;
        if (offset + 2 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        if (offset + 2 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        if (offset + 2 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
        offset += 2;
        if (offset + 1 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        if (offset + 1 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        if (offset + 4 >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 4);
        offset += 4;
        if (offset + 2 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        if (offset + 2 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_energy, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        if (offset + 1 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        if (offset + 1 >= length) return offset;
        proto_tree_add_item(tree, hf_ambit_log_header_activity_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        if (offset + (header_1_len - 211) >= length) return offset;
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, (header_1_len - 211));
        offset += (header_1_len - 211);
    }

    sample_ti = proto_tree_add_text(tree, tvb, 0, 0, "Samples");
    samples_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_samples);

    guint16 sample_len;
    guint32 sample_num = 1;

    while (offset + 2 < length) {
        sample_len = tvb_get_letohs(tvb, offset);
        if (offset + 2 + sample_len <= length) {
            dissect_ambit_log_data_sample(tvb, pinfo, samples_tree, data, offset, length, &sample_num, &period_sample_spec);
        }
        offset += sample_len + 2;
    }

    return offset;
}

static gint dissect_ambit_log_data_sample(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length, guint32 *sampleno, guint32 *periodic_sample_specifier)
{
    gint ret = 0, i;
    proto_tree *sample_tree = NULL;
    proto_tree *subtree = NULL, *subsubtree = NULL;
    proto_item *sample_ti = NULL;
    guint16 sample_len;
    guint8 sample_type;
    guint8 inner_type;
    guint16 count;
    guint16 type;
    guint16 spec_offset;
    guint16 spec_len;

    sample_len = tvb_get_letohs(tvb, offset);
    sample_type = tvb_get_guint8(tvb, offset + 2);

    switch(sample_type) {
      case 0:
        *periodic_sample_specifier = offset;
        sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Period sample specifier");
        sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
        proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        proto_tree_add_item(sample_tree, hf_ambit_log_sample_peri_spec_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        count = tvb_get_letohs(tvb, offset);
        offset += 2;
        sample_ti = proto_tree_add_text(sample_tree, tvb, offset, count*6, "Values");
        subtree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
        for (i=0; i<count; i++) {
            sample_ti = proto_tree_add_text(subtree, tvb, offset, 6, "Value %d", i+1);
            subsubtree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(subsubtree, hf_ambit_log_sample_peri_spec_id, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(subsubtree, hf_ambit_log_sample_peri_spec_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(subsubtree, hf_ambit_log_sample_peri_spec_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
        }
        break;
      case 2:
        sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Periodic sample)", (*sampleno)++);
        sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
        proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;
        proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        // Loop through specification
        count = tvb_get_letohs(tvb, (*periodic_sample_specifier) + 3);
        for (i=0; i<count; i++) {
            type = tvb_get_letohs(tvb, (*periodic_sample_specifier) + 5 + i*6);
            spec_offset = tvb_get_letohs(tvb, (*periodic_sample_specifier) + 7 + i*6);
            spec_len = tvb_get_letohs(tvb, (*periodic_sample_specifier) + 9 + i*6);
            switch(type) {
              case 0x03:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_distance, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x04:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_speed, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x05:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_hr, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x06:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_time, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x0c:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_altitude, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x0e:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_energy, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x0f:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_temp, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x18:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_pressure, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
              case 0x19:
                proto_tree_add_item(sample_tree, hf_ambit_log_sample_periodic_vert_speed, tvb, offset + spec_offset, spec_len, ENC_LITTLE_ENDIAN);
                break;
            }
        }
        break;
      case 3:
        inner_type = tvb_get_guint8(tvb, offset + 7);
        switch(inner_type) {
          case 0x04:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Pause)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            break;
          case 0x05:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Restart)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            break;
            break;
          case 0x06:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (IBI)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            for(i=0; i<sample_len-6; i+=2) {
                proto_tree_add_item(sample_tree, hf_ambit_log_ibi, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
            }
            break;
          case 0x07:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (TTFF)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 2);
            offset += 2;
            break;
          case 0x08:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Distance source)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_distance_source_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 7);
            }
            break;
          case 0x09:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Time event)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_time_event_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                guint16 year = tvb_get_letohs(tvb, offset);
                guint8 month = tvb_get_guint8(tvb, offset + 2);
                guint8 day = tvb_get_guint8(tvb, offset + 3);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_time_event_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
                offset += 4;
                guint8 hour = tvb_get_guint8(tvb, offset);
                guint8 minute = tvb_get_guint8(tvb, offset + 1);
                guint16 seconds = tvb_get_guint8(tvb, offset + 2);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_time_event_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
                offset += 3;
            }
            proto_tree_add_item(sample_tree, hf_ambit_log_time_event_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_time_event_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 22);
            break;
          case 0x0d:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Altitude source)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_altitude_source_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_altitude_source_altitude_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_altitude_source_pressure_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 11);
            }
            break;
          case 0x0f:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (gps-base)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 2);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_navtype, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            {
                guint16 year = tvb_get_letohs(tvb, offset);
                guint8 month = tvb_get_guint8(tvb, offset + 2);
                guint8 day = tvb_get_guint8(tvb, offset + 3);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_date, tvb, offset, 4, "Date", "%04d-%02d-%02d", year, month, day);
                offset += 4;
                guint8 hour = tvb_get_guint8(tvb, offset);
                guint8 minute = tvb_get_guint8(tvb, offset + 1);
                guint16 seconds = tvb_get_letohs(tvb, offset + 2);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_time, tvb, offset, 4, "Time", "%02d:%02d:%2.3f", hour, minute, ((float)seconds/1000.0));
                offset += 4;
            }
            {
                guint32 latlong = tvb_get_letohl(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_latitude, tvb, offset, 4, "Latitude", "%f", (float)latlong/10000000);
                offset += 4;
                latlong = tvb_get_letohl(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_longitude, tvb, offset, 4, "Longitude", "%f", (float)latlong/10000000);
                offset += 4;
            }
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_altitude, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_heading, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_ehpe, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_satelite_no, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_base_hdop, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            sample_ti = proto_tree_add_text(sample_tree, tvb, offset, sample_len-40, "Satellites");
            subtree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            for(i=0; i<sample_len-40; i+=6) {
                sample_ti = proto_tree_add_text(subtree, tvb, offset, 6, "Satellite");
                subsubtree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
                proto_tree_add_item(subsubtree, hf_ambit_log_gps_base_sv, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                proto_tree_add_item(subsubtree, hf_ambit_log_gps_base_state, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
                dissect_ambit_add_unknown(tvb, pinfo, subsubtree, offset, 1);
                offset += 1;
                proto_tree_add_item(subsubtree, hf_ambit_log_gps_base_snr, tvb, offset, 1, ENC_LITTLE_ENDIAN);
                offset += 1;
            }
            break;
          case 0x10:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (gps-small)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                gint16 latlong_offset = tvb_get_letohs(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_small_latitude, tvb, offset, 2, "Latitude offset", "%d (%f)", latlong_offset, (float)latlong_offset/1000000);
                offset += 2;
                latlong_offset = tvb_get_letohs(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_small_longitude, tvb, offset, 2, "Longitude offset", "%d (%f)", latlong_offset, (float)latlong_offset/1000000);
                offset += 2;
            }
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_small_time, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_small_ehpe, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_gps_small_satelite_no, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 14);
            }
            break;
          case 0x11:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (gps-tiny)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                gint8 latlong_offset = tvb_get_guint8(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_tiny_latitude, tvb, offset, 1, "Latitude offset", "%d (%f)", latlong_offset, (float)latlong_offset/1000000);
                offset += 1;
                latlong_offset = tvb_get_guint8(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_tiny_longitude, tvb, offset, 1, "Longitude offset", "%d (%f)", latlong_offset, (float)latlong_offset/1000000);
                offset += 1;
            }
            {
                guint8 time = tvb_get_guint8(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_tiny_time, tvb, offset, 1, "Time", "%d", time & 0x3f);
                // Unknown bits
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 1);
                offset += 1;
            }
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 9);
            }
            break;
          case 0x12:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Time)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                guint8 hour = tvb_get_guint8(tvb, offset);
                guint8 minute = tvb_get_guint8(tvb, offset + 1);
                guint16 seconds = tvb_get_guint8(tvb, offset + 2);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_time_event_time, tvb, offset, 3, "Time", "%02d:%02d:%02d", hour, minute, seconds);
                offset += 3;
            }
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 9);
            }
            break;
          case 0x14:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Swimming turn)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 1);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_delayed_store, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 1);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_lengths, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 4);
            offset += 4;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 2);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 8);
            offset += 8;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_lengths_wo_change, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_classification0, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_classification1, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_classification2, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_classification3, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_prev_style, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, 1);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_swimming_turn_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            break;
          case 0x15:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Swimming stroke)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_delayed_store, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            break;
          case 0x18:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Activity)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_activity_type, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_activity_custom_mode_id, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 12);
            }
            break;
          case 0x1a:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Cadence source)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_cadence_source_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 7);
            }
            break;
          case 0x1b:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (lat-long)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                guint32 latlong = tvb_get_letohl(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_latitude, tvb, offset, 4, "Latitude", "%f", (float)latlong/10000000);
                offset += 4;
                latlong = tvb_get_letohl(tvb, offset);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_gps_base_longitude, tvb, offset, 4, "Longitude", "%f", (float)latlong/10000000);
                offset += 4;
            }
            if (offset < sample_len - 2) {
                dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 14);
            }
            break;
          case 0x1c:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Firmware info)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            {
                guint8 fw1,fw2;
                guint16 fw3;
                fw1 = tvb_get_guint8(tvb, offset);
                fw2 = tvb_get_guint8(tvb, offset+1);
                fw3 = tvb_get_letohs(tvb, offset+2);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_fw_version, tvb, offset, 4, "FW version", "%d.%d.%d", fw1, fw2, fw3);
                offset += 4;
                guint16 year = tvb_get_letohs(tvb, offset);
                guint8 month = tvb_get_guint8(tvb, offset + 2);
                guint8 day = tvb_get_guint8(tvb, offset + 3);
                guint8 hour = tvb_get_guint8(tvb, offset + 4);
                guint8 minute = tvb_get_guint8(tvb, offset + 5);
                guint16 seconds = tvb_get_letohs(tvb, offset + 6);
                proto_tree_add_string_format_value(sample_tree, hf_ambit_log_fwinfo_build_date, tvb, offset, 8, "Builddate", "%04d-%02d-%02d %02d:%02d:%2.3f", year, month, day, hour, minute, ((float)seconds/1000.0));
                offset += 8;
            }
            break;
          default:
            sample_ti = proto_tree_add_text(tree, tvb, offset, sample_len + 2, "Sample #%u (Unknown)", (*sampleno)++);
            sample_tree = proto_item_add_subtree(sample_ti, ett_ambit_log_sample);
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(sample_tree, hf_ambit_log_sample_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_time_offset, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(sample_tree, hf_ambit_log_other_type, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            dissect_ambit_add_unknown(tvb, pinfo, sample_tree, offset, sample_len - 6);
            break;
        }
        break;
      default:
        break;
    }

    return 0;
}

static gint dissect_ambit_lock_status_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_lock_status_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_lock_set(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_lock_set_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_gps_data_peek_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit_gps_data_peek_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_tree_add_item(tree, hf_ambit_gps_data_head, tvb, 0, 9, ENC_LITTLE_ENDIAN);
}

static gint dissect_ambit_data_write(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    guint32 address = tvb_get_letohl(tvb, 0);
    guint32 length = tvb_get_letohl(tvb, 4);

    proto_tree_add_item(tree, hf_ambit_log_data_address, tvb, 0, 4, ENC_LITTLE_ENDIAN);
    proto_tree_add_item(tree, hf_ambit_log_data_length, tvb, 4, 4, ENC_LITTLE_ENDIAN);
/*
    if (address >= 0x2000 && address <= 0x3800) { // Custom sport modes address.
        dissect_ambit_data_write_sport_modes(tvb, pinfo, tree, address, length + 8);
    }
    else if (address >= 0x927c0 && address <= 0x9a7c0) {
        dissect_ambit_data_write_apps(tvb, pinfo, tree, address, length + 8);
    }
    else {
        proto_tree_add_text(tree, tvb, 8, length, "Payload");
    }
*/
    if ((address >= 0x2000 && address <= 0x3800) || (address >= 0x927c0 && address <= 0x9a7c0)) { // Custom sport modes address.
        int i;
        for (i=0; i<length+8; i++) {
            g_printf("%.2x", tvb_get_guint8(tvb,i));
        }
        g_printf("\n");

    }
}


static gint dissect_ambit_data_write_sport_modes(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 address, guint32 length)
{
    guint32 offset = 8;

    static guint32 leftToReadArray[6] = {0};
    int index = (address - 0x2000) / 0x400;
    uint leftToRead = leftToReadArray[index];

    if (leftToRead)
    {
        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, leftToRead);
        offset += leftToRead;
    }


    if (address == 0x00002000)
    {
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
        offset += 2;
    }

    leftToRead = dissect_ambit_data_write_content(tvb, pinfo, tree, &offset, length);
    leftToReadArray[index + 1] = leftToRead;
}

static gint dissect_ambit_data_write_apps(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, guint32 address, guint32 length)
{
    static u_int32_t apps_length[10] = {0};

    static guint32 leftToReadArray[6] = {0};
    static guint32 currentAppIndexArray[6] = {0};
    int arrayIndex = (address - 0x927c0) / 0x400;
    uint leftToRead = leftToReadArray[arrayIndex];
    uint currentAppIndex = currentAppIndexArray[arrayIndex];
    uint header_len = 0;

    guint32 offset = 8;

    if (address == 0x927c0) {
        proto_tree_add_item(tree, hf_ambit_write_data_apps_nbr_of, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        offset += 2;

        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 1);
        offset += 1;

        proto_tree_add_item(tree, hf_ambit_write_data_apps_header_len, tvb, offset, 2, ENC_LITTLE_ENDIAN);
        header_len = tvb_get_letohs(tvb, offset);
        offset += 2;

        dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
        offset += 2;

        int i;
        for (i=0; i*4<header_len-7; i++)
        {
            proto_tree_add_item(tree, hf_ambit_write_data_apps_end_pos, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            apps_length[i] = tvb_get_letohl(tvb, offset) - header_len;
            offset += 4;
        }
    }
    else {
        proto_tree_add_item(tree, hf_ambit_write_data_app_data, tvb, offset, leftToRead, ENC_LITTLE_ENDIAN);
        offset += leftToRead;

        proto_tree_add_item(tree, hf_ambit_write_data_app_checksum, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        currentAppIndex++;
    }

    int i = currentAppIndex;
    while (offset < length) {

        uint read_len = i==0 ? apps_length[i]-1 : apps_length[i]-apps_length[i-1]-1;

        if (read_len > (length-offset) ) {
            leftToReadArray[arrayIndex+1] = read_len-(length-offset);
            currentAppIndexArray[arrayIndex+1] = i;
            read_len = length-offset;
        }

        proto_tree_add_item(tree, hf_ambit_write_data_app_data, tvb, offset, read_len, ENC_LITTLE_ENDIAN);
        offset += read_len;

        if (offset < length) {
            proto_tree_add_item(tree, hf_ambit_write_data_app_checksum, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;

            if (offset == length) {
                leftToReadArray[arrayIndex+1] = apps_length[i] - (length-offset);
                currentAppIndexArray[arrayIndex+1] = i;
            }
        }

        i++;
    }
}

static gint dissect_ambit_data_write_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length)
{
    uint leftToRead = 0;

    while (*offset < length)
    {
        if (*offset + 2 >= length) return *offset;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
//        uint dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 >= length) return *offset;
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        uint dataLength = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        uint startOffset = *offset;
        dissect_ambit_data_write_read_data_0101(tvb, pinfo, tree, offset, length, dataLength);
        leftToRead = dataLength - (*offset - startOffset);
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0101(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint leftToRead = 0;
    uint startOffset = *offset;

    while (*offset < length && *offset - startOffset < packageLength)
    {
        if (*offset + 2 >= length) return *offset;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 >= length) return *offset;
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataLength2 = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        switch(dataHeader) {
          case 0x0102:
            leftToRead = dissect_ambit_data_write_read_data_0102(tvb, pinfo, tree, offset, length, dataLength2);
            break;
          case 0x0105:
            leftToRead = dissect_ambit_data_write_read_data_0105(tvb, pinfo, tree, offset, length, dataLength2);
            break;
          case 0x010c:
            leftToRead = dissect_ambit_data_write_read_data_010c(tvb, pinfo, tree, offset, length, dataLength2);
            break;
        }
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0105(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint leftToRead = 0;
    uint startOffset = *offset;
    while (*offset < length && *offset - startOffset < packageLength)
    {

        if (*offset + 2 > length) return packageLength;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 > length) return packageLength - (*offset - startOffset);
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataLength = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        switch(dataHeader) {
          case 0x0106:
            leftToRead = dissect_ambit_data_write_read_data_0106(tvb, pinfo, tree, offset, length, dataLength);
            break;
        }
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0106(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint leftToRead = 0;
    uint startOffset = *offset;
    while (*offset < length && *offset - startOffset < packageLength)
    {

        if (*offset + 2 > length) return packageLength;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 > length)
        {
            return packageLength - (*offset - startOffset);
        }
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataLength = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        switch(dataHeader) {
          case 0x0107:
            leftToRead = dissect_ambit_data_write_read_data_0107(tvb, pinfo, tree, offset, length, dataLength);
            break;
          case 0x0108:
            leftToRead = dissect_ambit_data_write_read_data_0108(tvb, pinfo, tree, offset, length, dataLength);
            break;
        }
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0107(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint leftToRead = 0;
    uint startOffset = *offset;
    while (*offset < length && *offset - startOffset < packageLength)
    {

        if (*offset + 2 > length) return packageLength;
        proto_tree_add_item(tree, hf_ambit_write_data_display_type, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        *offset += 2;

        dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, packageLength - 2);
        *offset += packageLength - 2;
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0108(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint leftToRead = 0;
    uint startOffset = *offset;
    while (*offset < length && *offset - startOffset < packageLength)
    {

        if (*offset + 2 > length) return packageLength;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 > length) return packageLength - (*offset - startOffset);
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataLength = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        switch(dataHeader) {
          case 0x0109:
            leftToRead = dissect_ambit_data_write_read_data_0109(tvb, pinfo, tree, offset, length, dataLength);
            break;
        case 0x010a:
          leftToRead = dissect_ambit_data_write_read_data_010a(tvb, pinfo, tree, offset, length, dataLength);
          break;
        }
    }

    return leftToRead;
}

static uint dissect_ambit_data_write_read_data_0109(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint startOffset = *offset;

    if (*offset + 2 > length) return packageLength;
    proto_tree_add_item(tree, hf_ambit_log_activity_custom_mode_row, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_log_activity_custom_mode_display, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    return 0;
}

static uint dissect_ambit_data_write_read_data_010a(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    if (*offset + 2 > length) return packageLength;
    proto_tree_add_item(tree, hf_ambit_log_activity_custom_mode_view, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    return 0;
}

static uint dissect_ambit_data_get_length_to_read(guint32 *offset, guint32 length, guint16 packageLength)
{
    if (*offset + packageLength > length)
    {
        return (length - *offset);
    }
    return packageLength;
}

static uint dissect_ambit_data_write_read_data_010c(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint startOffset = *offset;
    while (*offset < length && *offset - startOffset < packageLength)
    {
        if (*offset + 2 > length) return packageLength;
        proto_tree_add_item(tree, hf_ambit_write_data_header, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataHeader = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        if (*offset + 2 > length) return packageLength - (*offset - startOffset);
        proto_tree_add_item(tree, hf_ambit_write_data_length, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
        guint16 dataLength = tvb_get_letohs(tvb, *offset);
        *offset += 2;

        switch(dataHeader) {
          case 0x010d:
            if (*offset + dataLength > length) return packageLength - (*offset - startOffset);
            proto_tree_add_item(tree, hf_ambit_write_data_app_index, tvb, *offset, dataLength, ENC_LITTLE_ENDIAN);
            *offset += dataLength;
            break;
        }
    }

    return 0;
}

static uint dissect_ambit_data_write_read_data_0102(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, /*void *data _U_,*/ guint32 *offset, guint32 length, guint16 packageLength)
{
    uint startOffset = *offset;

    if (*offset + 16 > length) return packageLength;
    proto_tree_add_item(tree, hf_ambit_log_header_activity_name /*hf_ambit_write_data_custom_mode_name*/, tvb, *offset, 16, ENC_LITTLE_ENDIAN);
    *offset += 16;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_log_activity_custom_mode_id, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_activity_id, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 2);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_use_hrbelt_and_pods, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_alti_baro_mode, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_gps_interval, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_recording_interval, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_auto_lap, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_log_header_hr_max, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_log_header_hr_min, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_use_hr_limits, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 2);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_auto_pause, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_auto_scroll, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_use_interval_timer, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_interval_repetitions, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_interval_timer_time, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 6 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 6);
    *offset += 6;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_interval_timer_max, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 2);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_interval_timer_time, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 6 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 6);
    *offset += 6;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_interval_timer_min, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 14 > length) return packageLength - (*offset - startOffset);
    dissect_ambit_add_unknown(tvb, pinfo, tree, *offset, 14);
    *offset += 14;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_backlight, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_data_display, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    if (*offset + 2 > length) return packageLength - (*offset - startOffset);
    proto_tree_add_item(tree, hf_ambit_write_quick_navigation, tvb, *offset, 2, ENC_LITTLE_ENDIAN);
    *offset += 2;

    return 0;
}

static gint dissect_ambit_data_write_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit3_settings_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit3_settings_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
}

static gint dissect_ambit3_log_headers_get(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    dissect_ambit_add_unknown(tvb, pinfo, tree, 0, 18);
}

static gint dissect_ambit3_log_headers_reply(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    dissect_ambit_add_unknown(tvb, pinfo, tree, 0, 6);
}

static gint dissect_ambit3_log_headers_content(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_, guint32 offset, guint32 length)
{
    gint namelen = 0;
    guint16 log_count = 0;
    guint16 log_cntr = 0;
    gint header_len = 0;
    proto_item *logs_ti = NULL;
    proto_tree *logs_tree = NULL;
    proto_item *log_ti = NULL;
    proto_tree *log_tree = NULL;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 10);
    offset += 10;
    log_count = tvb_get_letohs(tvb, offset);
    proto_tree_add_item(tree, hf_ambit_log_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    dissect_ambit_add_unknown(tvb, pinfo, tree, offset, 2);
    offset += 2;
    proto_tree_add_item(tree, hf_ambit_unsynced_log_count, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset += 2;
    logs_ti = proto_tree_add_text(tree, tvb, 0, 0, "Logs");
    logs_tree = proto_item_add_subtree(logs_ti, ett_ambit3_log_headers);
    while (offset + 2 < length) {
        header_len = tvb_get_guint8(tvb, offset+1);
        log_cntr++;
        log_ti = proto_tree_add_text(logs_tree, tvb, offset, header_len + 2, "Header #%u", log_cntr);
        log_tree = proto_item_add_subtree(log_ti, ett_ambit3_log_header);
        dissect_ambit_add_unknown(tvb, pinfo, log_tree, offset, 1);
        offset += 1;
        proto_tree_add_item(log_tree, hf_ambit_log_header_length, tvb, offset, 1, ENC_LITTLE_ENDIAN);
        offset += 1;
        if (offset + header_len <= length) {
            proto_tree_add_item(log_tree, hf_ambit_time, tvb, offset, 20, ENC_LITTLE_ENDIAN);
            offset += 20;
            proto_tree_add_item(log_tree, hf_ambit_log_header_synced, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_data_address, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_data_next_addr, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            dissect_ambit_add_unknown(tvb, pinfo, log_tree, offset, 8);
            offset += 8;
            proto_tree_add_item(log_tree, hf_ambit_log_header_hr_min, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_hr_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_hr_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_hr_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_hr_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            dissect_ambit_add_unknown(tvb, pinfo, log_tree, offset, 2);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_temp_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_temp_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_altitude_min, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_altitude_max, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_alt_min_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_alt_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_cadence_avg, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_cadence_max, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_cadence_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_avg_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_max_speed, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_speed_max_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            dissect_ambit_add_unknown(tvb, pinfo, log_tree, offset, 4);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_duration, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_ascent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_descent, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_ascent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_descent_time, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_recovery, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            proto_tree_add_item(log_tree, hf_ambit_log_header_peak_effect, tvb, offset, 1, ENC_LITTLE_ENDIAN);
            offset += 1;
            namelen = 0;
            while (tvb_get_guint8(tvb, offset+namelen) != 0) {
                namelen++;
            }
            proto_tree_add_item(log_tree, hf_ambit_log_header_activity_name, tvb, offset, namelen+1, ENC_LITTLE_ENDIAN);
            offset += namelen + 1;
            proto_tree_add_item(log_tree, hf_ambit_log_header_distance, tvb, offset, 4, ENC_LITTLE_ENDIAN);
            offset += 4;
            proto_tree_add_item(log_tree, hf_ambit_log_header_energy, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;
            dissect_ambit_add_unknown(tvb, pinfo, log_tree, offset, 26);
            offset += 26;
        }
    }
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
    tvbuff_t *new_tvb = NULL, *next_tvb = NULL, *log_tvb = NULL, *log_header_tvb = NULL;
    static guint32 fragments_start_frame;
    static guint16 fragments_offset;
    static guint16 fragments_data_len;
    int data_len = 0;
    int data_offset = 0;
    const ambit_protocol_type_t *subdissector;

    static guint32 current_log_start_frame = 0xffffffff;
    static guint32 log_after_end_of_use = 0xffffffff;
    static guint32 log_last_known_address;

    static guint32 current_log_header_start_frame = 0xffffffff;
    static guint32 current_log_header_end_found = 0;

    if (usbid == D_AMBIT_USBID) {
        if (msg_part == 0x5d) {
            data_len = data_header_len - 12;
            data_offset = 20;
        }
        else {
            data_len = data_header_len;
            data_offset = 8;
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
                reassembly_entries[pinfo->fd->num].frame_index = 0;
                reassembly_entries[pinfo->fd->num].frame_total = msg_count;
                reassembly_entries[pinfo->fd->num].size = pkt_len;
                reassembly_entries[pinfo->fd->num].data = (unsigned char*)g_malloc(pkt_len);
                fragments_start_frame = pinfo->fd->num;
                fragments_offset = data_len;
                fragments_data_len = pkt_len;

                tvb_memcpy(tvb, reassembly_entries[pinfo->fd->num].data, data_offset, data_len);
            }
            else if (msg_part == 0x5e) {
                if (fragments_data_len >= fragments_offset + data_len) {
                    //fragments_start_frame = pinfo->fd->num-msg_count;
                    reassembly_entries[pinfo->fd->num].valid = 2;
                    reassembly_entries[pinfo->fd->num].command = reassembly_entries[fragments_start_frame].command;
                    reassembly_entries[pinfo->fd->num].frame_index = msg_count;
                    reassembly_entries[pinfo->fd->num].frame_total = reassembly_entries[fragments_start_frame].frame_total;
                    reassembly_entries[pinfo->fd->num].size = fragments_data_len;
                    reassembly_entries[pinfo->fd->num].data = reassembly_entries[fragments_start_frame].data;
                    tvb_memcpy(tvb, &(reassembly_entries[fragments_start_frame].data[fragments_offset]), data_offset, data_len);
                    fragments_offset += data_len;

                    command = reassembly_entries[fragments_start_frame].command;
                }
                else {
                }
            }
            else {
                reassembly_entries[pinfo->fd->num].valid = 1;
                reassembly_entries[pinfo->fd->num].command = command;
            }

            // Handle dissection of logs
            if (command == 0x0b170a00) {
                // First check for initial known header
                if (msg_part == 0x5d) {
                    guint32 address = tvb_get_letohl(tvb, data_offset);
                    guint32 length = tvb_get_letohl(tvb, data_offset + 4);

                    log_last_known_address = address;
                    if (address == 0x000f4240) {
                        log_after_end_of_use = tvb_get_letohl(tvb, data_offset + 20);
                    }
                    // Set address to lookup table
                    address_to_frame_lookup[(log_last_known_address - 0x000f4240) / 0x400] = pinfo->fd->num + 19;

                    // Adjust data pointers for extra address and length fields
                    data_offset += 8;
                    data_len -= 8;
                }

                // Loop through entry to find PMEM
                int i;
                guint8 a,b,c,d;
                for(i=0; i<data_len; i++) {
                    if((a = tvb_get_guint8(tvb, data_offset+i)) == 'P' && i+3<data_len) {
                        b = tvb_get_guint8(tvb, data_offset+i+1);
                        c = tvb_get_guint8(tvb, data_offset+i+2);
                        d = tvb_get_guint8(tvb, data_offset+i+3);

                        if (b == 'M' && c == 'E' && d == 'M') {
                            if (current_log_start_frame != 0xffffffff) {
                                reassembly_entries[current_log_start_frame].log.entry = g_realloc(reassembly_entries[current_log_start_frame].log.entry, reassembly_entries[current_log_start_frame].log.entry_size + i);
                                tvb_memcpy(tvb, &reassembly_entries[current_log_start_frame].log.entry[reassembly_entries[current_log_start_frame].log.entry_size], data_offset, i);
                                reassembly_entries[current_log_start_frame].log.entry_size += i;
                            }
                            reassembly_entries[pinfo->fd->num].log.entry = (unsigned char*)g_malloc(data_len-i);
                            tvb_memcpy(tvb, reassembly_entries[pinfo->fd->num].log.entry, data_offset+i, data_len-i);
                            reassembly_entries[pinfo->fd->num].log.entry_size = data_len-i;
                            current_log_start_frame = pinfo->fd->num;
                            break;
                        }
                    }
                }

                if (i == data_len && current_log_start_frame != 0xffffffff) {
                    // No new PMEM found
                    reassembly_entries[current_log_start_frame].log.entry = g_realloc(reassembly_entries[current_log_start_frame].log.entry, reassembly_entries[current_log_start_frame].log.entry_size + data_len);
                    tvb_memcpy(tvb, &reassembly_entries[current_log_start_frame].log.entry[reassembly_entries[current_log_start_frame].log.entry_size], data_offset, data_len);

                    reassembly_entries[current_log_start_frame].log.entry_size += data_len;
                }

                // Increment address
                log_last_known_address += data_len;

                reassembly_entries[pinfo->fd->num].log.start_frame = current_log_start_frame;

                if (log_last_known_address >= log_after_end_of_use) {
                    // If we are after end of file, reset log entry!
                    current_log_start_frame = 0xffffffff;
                }
            }

            // Handle dissection of Ambit3 log headers
            if (command == 0x12000a00) {
                if (msg_part == 0x5d) {
                    // Check if this is the first packet of the last header chunk
                    if (tvb_get_guint8(tvb, data_offset) == 0x00 &&
                        tvb_get_guint8(tvb, data_offset+1) == 0x00 &&
                        tvb_get_guint8(tvb, data_offset+2) == 0x00 &&
                        tvb_get_guint8(tvb, data_offset+3) == 0x00 &&
                        tvb_get_guint8(tvb, data_offset+4) == 0x01 &&
                        tvb_get_guint8(tvb, data_offset+5) == 0x00) {
                        current_log_header_end_found = 1;
                    }

                    // Adjust data pointers for extra address and length fields
                    data_offset += 6;
                    data_len -= 6;
                }

                // Look for start of entry (at SBEM0102)
                if (tvb_get_guint8(tvb, data_offset) == 'S' &&
                    tvb_get_guint8(tvb, data_offset+1) == 'B' &&
                    tvb_get_guint8(tvb, data_offset+2) == 'E' &&
                    tvb_get_guint8(tvb, data_offset+3) == 'M') {
                    reassembly_entries[pinfo->fd->num].log_header.entry = (unsigned char*)g_malloc(data_len);
                    tvb_memcpy(tvb, reassembly_entries[pinfo->fd->num].log_header.entry, data_offset, data_len);
                    reassembly_entries[pinfo->fd->num].log_header.entry_size = data_len;
                    current_log_header_start_frame = pinfo->fd->num;
                }
                else if (current_log_header_start_frame != 0xffffffff) {
                    // Append to current entry
                    reassembly_entries[current_log_header_start_frame].log_header.entry = g_realloc(reassembly_entries[current_log_header_start_frame].log_header.entry, reassembly_entries[current_log_header_start_frame].log_header.entry_size + data_len);
                    tvb_memcpy(tvb, &reassembly_entries[current_log_header_start_frame].log_header.entry[reassembly_entries[current_log_header_start_frame].log_header.entry_size], data_offset, data_len);

                    reassembly_entries[current_log_header_start_frame].log_header.entry_size += data_len;
                }

                reassembly_entries[pinfo->fd->num].log_header.start_frame = current_log_header_start_frame;

                // If we have reached last packet in last chunk, reset "pointers"
                if (reassembly_entries[pinfo->fd->num].frame_index + 1 == reassembly_entries[pinfo->fd->num].frame_total &&
                    current_log_header_end_found == 1) {
                    current_log_header_start_frame = 0xffffffff;
                    current_log_header_end_found = 0;
                }
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
            offset = 0;
            proto_tree_add_item(ambit_tree, hf_ambit_usbid, tvb, offset, 1, ENC_NA);
            offset += 1;
            proto_tree_add_item(ambit_tree, hf_ambit_usblength, tvb, offset, 1, ENC_NA);
            offset += 1;
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
                proto_tree_add_item(ambit_tree, hf_ambit_pktformat, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                proto_tree_add_item(ambit_tree, hf_ambit_pktseqno, tvb, offset, 2, ENC_LITTLE_ENDIAN);
                offset += 2;
                proto_tree_add_item(ambit_tree, hf_ambit_pktlen, tvb, offset, 4, ENC_LITTLE_ENDIAN);
                offset += 4;
            }

            if (reassembly_entries[pinfo->fd->num].valid == 1) {
                new_tvb = tvb_new_subset_remaining(tvb, offset);
                pkt_len = data_len;
                col_set_str(pinfo->cinfo, COL_INFO, "");
            }
            else if (reassembly_entries[pinfo->fd->num].frame_index + 1 == reassembly_entries[pinfo->fd->num].frame_total) {
                new_tvb = tvb_new_real_data(reassembly_entries[pinfo->fd->num].data, reassembly_entries[pinfo->fd->num].size, reassembly_entries[pinfo->fd->num].size);
                //tvb_set_child_real_data_tvbuff(tvb, new_tvb);
                add_new_data_source(pinfo, new_tvb, "Reassembled");
                pkt_len = reassembly_entries[pinfo->fd->num].size;

                col_add_fstr(pinfo->cinfo, COL_INFO, " (#%u of #%u) Reassembled", reassembly_entries[pinfo->fd->num].frame_index + 1, reassembly_entries[pinfo->fd->num].frame_total);

                if (reassembly_entries[pinfo->fd->num].log.start_frame != 0xffffffff &&
                    reassembly_entries[reassembly_entries[pinfo->fd->num].log.start_frame].log.entry != NULL) {
                    log_tvb = tvb_new_real_data(reassembly_entries[reassembly_entries[pinfo->fd->num].log.start_frame].log.entry, reassembly_entries[reassembly_entries[pinfo->fd->num].log.start_frame].log.entry_size, reassembly_entries[reassembly_entries[pinfo->fd->num].log.start_frame].log.entry_size);
                    add_new_data_source(pinfo, log_tvb, "Log");
                }

                if (reassembly_entries[pinfo->fd->num].log_header.start_frame != 0xffffffff &&
                    reassembly_entries[reassembly_entries[pinfo->fd->num].log_header.start_frame].log_header.entry != NULL) {
                    log_header_tvb = tvb_new_real_data(reassembly_entries[reassembly_entries[pinfo->fd->num].log_header.start_frame].log_header.entry, reassembly_entries[reassembly_entries[pinfo->fd->num].log_header.start_frame].log_header.entry_size, reassembly_entries[reassembly_entries[pinfo->fd->num].log_header.start_frame].log_header.entry_size);
                    add_new_data_source(pinfo, log_header_tvb, "Log header");
                }
            }
            else {
                col_add_fstr(pinfo->cinfo, COL_INFO, " (#%u of #%u)", reassembly_entries[pinfo->fd->num].frame_index + 1, reassembly_entries[pinfo->fd->num].frame_total);
            }

            subdissector = find_subdissector(reassembly_entries[pinfo->fd->num].command);
            if (subdissector != NULL) {
                col_prepend_fstr(pinfo->cinfo, COL_INFO, "%s", subdissector->name);
            }

            if (new_tvb != NULL) {
                if (subdissector != NULL) {
                    data_ti = proto_tree_add_text(ambit_tree, new_tvb, 0, pkt_len, "%s", subdissector->name);
                }
                else {
                    data_ti = proto_tree_add_text(ambit_tree, new_tvb, 0, pkt_len, "Payload");
                }
                data_tree = proto_item_add_subtree(data_ti, ett_ambit_data);

                if (subdissector != NULL) {
                    subdissector->dissector(new_tvb, pinfo, data_tree, data);
                }
            }

            if (log_tvb != NULL) {
                data_ti = proto_tree_add_text(ambit_tree, new_tvb, 0, pkt_len, "Full log entry");
                data_tree = proto_item_add_subtree(data_ti, ett_ambit_log_data);
                dissect_ambit_log_data_content(log_tvb, pinfo, data_tree, data, 0, reassembly_entries[reassembly_entries[pinfo->fd->num].log.start_frame].log.entry_size);
            }

            if (log_header_tvb != NULL) {
                data_ti = proto_tree_add_text(ambit_tree, new_tvb, 0, pkt_len, "Full log headers");
                data_tree = proto_item_add_subtree(data_ti, ett_ambit_log_data);
                dissect_ambit3_log_headers_content(log_header_tvb, pinfo, data_tree, data, 0, reassembly_entries[reassembly_entries[pinfo->fd->num].log_header.start_frame].log_header.entry_size);
            }

            offset += data_len;
            proto_tree_add_item(ambit_tree, hf_ambit_payloadchksum, tvb, offset, 2, ENC_LITTLE_ENDIAN);
            offset += 2;

            if (offset < 64) {
                proto_tree_add_text(ambit_tree, tvb, offset, 64 - offset, "Padding");
            }
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
        { &hf_ambit_pktformat,
          { "Packet format", "ambit.pktformat", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_pktseqno,
          { "Packet sequence no", "ambit.pktseqno", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_pktlen,
          { "Packet length", "ambit.pktlen", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_payloadchksum,
          { "Payload checksum", "ambit.payloadchksum", FT_UINT16, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_date,
          { "Date", "ambit.date", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_time,
          { "Time", "ambit.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_charge,
          { "Charge", "ambit.charge", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_komposti_version,
          { "Komposti version", "ambit.komposti", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_model,
          { "Model", "ambit.model", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_serial,
          { "Serial", "ambit.serial", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_compact_serial,
          { "Compact Serial", "ambit.compact_serial", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_fw_version,
          { "FW version", "ambit.fwversion", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_hw_version,
          { "HW version", "ambit.hwversion", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_bsl_version,
          { "BSL version", "ambit.bslversion", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
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
        { &hf_ambit_personal_unit_system,
          { "Units", "ambit.personal.units", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_ambit_personal_coordinate_system,
          { "Coordinate system", "ambit.personal.coordinate_system", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_language,
          { "Language", "ambit.personal.language", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_alarm_enable,
          { "Alarm enable", "ambit.personal.alarm_enable", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_alarm_time,
          { "Alarm time", "ambit.personal.alarm_time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_dual_time,
          { "Dual time", "ambit.personal.dual_time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_personal_gps_sync_time,
          { "Sync time with GPS", "ambit.personal.gps_sync_time", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
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
          { "Avg speed (10 m/h)", "ambit.log_header.avg_speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_max_speed,
          { "Max speed (10 m/h)", "ambit.log_header.max_speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_altitude_max,
          { "Altitude max", "ambit.log_header.alt_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_altitude_min,
          { "Altitude min", "ambit.log_header.alt_min", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_avg,
          { "Heartrate avg", "ambit.log_header.hr_avg", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_max,
          { "Heartrate max", "ambit.log_header.hr_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_peak_effect,
          { "Peak training effect (1/10)", "ambit.log_header.peak_effect", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_activity_type,
          { "Activity type", "ambit.log_header.activity_type", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_activity_name,
          { "Activity name", "ambit.log_header.activity_name", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_hr_min,
          { "Heartrate min", "ambit.log_header.hr_min", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_temp_max,
          { "Temperature max (1/10 Celsius)", "ambit.log_header.temp_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_temp_min,
          { "Temperature min (1/10 Celsius)", "ambit.log_header.temp_min", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_distance,
          { "Distance (m)", "ambit.log_header.distance", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_sample_count,
          { "Sample count", "ambit.log_header.sample_count", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_energy,
          { "Energy consumption (kcal)", "ambit.log_header.energy_consumption", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_cadence_max,
          { "Cadence max (rpm)", "ambit.log_header.cadence_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_cadence_avg,
          { "Cadence avg (rpm)", "ambit.log_header.cadence_avg", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_swimming_lengths,
          { "Swimming pool lengths", "ambit.log_header.swimming_lengths", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
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
        { &hf_ambit_log_header_cadence_max_time,
          { "Time max cadence (ms)", "ambit.log_header.cadence_maxtime", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_swimming_pool_length,
          { "Swimming pool length (m)", "ambit.log_header.swimming_pool_length", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_time_first_fix,
          { "Time of first fix", "ambit.log_header.time_first_fix", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_battery_start,
          { "Battery at start", "ambit.log_header.battery_start", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_battery_stop,
          { "Battery at end", "ambit.log_header.battery_stop", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_distance_before_calib,
          { "Distance before calibration", "ambit.log_header.distance_before_calib", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_header_synced,
          { "Syncronized", "ambit.log_header.synced", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_header_more,
          { "More values", "ambit.log_header.more", FT_UINT32, BASE_HEX, VALS(log_header_more_vals), 0, NULL, HFILL } },

        { &hf_ambit_log_count,
          { "Log count", "ambit.log.count", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_unsynced_log_count,
          { "Not synced log count", "ambit.log.unsynced.count", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_data_addr_frame_ref,
          { "In frame", "ambit.log_data.inframe", FT_FRAMENUM, BASE_NONE, NULL, 0, NULL, HFILL } },
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


        { &hf_ambit_log_sample_length,
          { "Sample length", "ambit.log_sample.length", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_type,
          { "Sample type", "ambit.log_sample.type", FT_UINT8, BASE_HEX, VALS(log_samples_type_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_peri_spec_count,
          { "Value count", "ambit.log_sample.spec.count", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_peri_spec_id,
          { "Type", "ambit.log_sample.spec.type", FT_UINT16, BASE_DEC, VALS(log_samples_spec_type_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_peri_spec_offset,
          { "Offset", "ambit.log_sample.spec.offset", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_peri_spec_length,
          { "Length", "ambit.log_sample.spec.length", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_distance,
          { "Distance", "ambit.log_sample.periodic.distance", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_speed,
          { "Speed", "ambit.log_sample.periodic.speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_hr,
          { "HR", "ambit.log_sample.periodic.hr", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_time,
          { "Time", "ambit.log_sample.periodic.time", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_altitude,
          { "Altitude", "ambit.log_sample.periodic.altitude", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_energy,
          { "Energy consumption", "ambit.log_sample.periodic.energy", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_temp,
          { "Temperature", "ambit.log_sample.periodic.temp", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_pressure,
          { "Pressure", "ambit.log_sample.periodic.pressure", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_sample_periodic_vert_speed,
          { "Vertical speed", "ambit.log_sample.periodic.vert_speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_other_time_offset,
          { "Time offset (from last periodic sample)", "ambit.log_sample.other.time", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_other_type,
          { "Other log type", "ambit.log_sample.other.type", FT_UINT8, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_activity_type,
          { "Activity type", "ambit.log_sample.activity.type", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_activity_custom_mode_id,
          { "Custom mode ID", "ambit.log_sample.activity.custom_mode_id", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_time_event_type,
          { "Time event type", "ambit.log_sample.time_event.type", FT_UINT8, BASE_HEX, VALS(log_samples_time_event_type_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_log_time_event_date,
          { "Date", "ambit.log_sample.time_event.date", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_time_event_time,
          { "Time", "ambit.log_sample.time_event.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_time_event_duration,
          { "Duration (1/10 s)", "ambit.log_sample.time_event.duration", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_time_event_distance,
          { "Distance (m)", "ambit.log_sample.time_event.distance", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_gps_base_navtype,
          { "NavType", "ambit.log_sample.gps_base.navtype", FT_UINT16, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_date,
          { "Date (UTC)", "ambit.log_sample.gps_base.date", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_time,
          { "Time (UTC)", "ambit.log_sample.gps_base.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_latitude,
          { "Latitude", "ambit.log_sample.gps_base.latitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_longitude,
          { "Longitude", "ambit.log_sample.gps_base.longitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_altitude,
          { "Altitude (1/100 m)", "ambit.log_sample.gps_base.altitude", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_speed,
          { "Speed", "ambit.log_sample.gps_base.speed", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_heading,
          { "Direction (1/100 degree)", "ambit.log_sample.gps_base.direction", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_ehpe,
          { "EHPE", "ambit.log_sample.gps_base.ehpe", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_satelite_no,
          { "No of satellites", "ambit.log_sample.gps_base.satellite_no", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_hdop,
          { "HDOP (1/5)", "ambit.log_sample.gps_base.hdop", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_sv,
          { "SV", "ambit.log_sample.gps_base.sv", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_state,
          { "State", "ambit.log_sample.gps_base.state", FT_UINT8, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_base_snr,
          { "SNR", "ambit.log_sample.gps_base.snr", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_gps_small_time,
          { "Time (only seconds, ms)", "ambit.log_sample.gps_small.time", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_small_latitude,
          { "Latitude (offset from gps-base)", "ambit.log_sample.gps_small.latitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_small_longitude,
          { "Longitude (offset from gps-base)", "ambit.log_sample.gps_small.longitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_small_ehpe,
          { "EHPE", "ambit.log_sample.gps_small.ehpe", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_small_satelite_no,
          { "No of satellites", "ambit.log_sample.gps_small.satellite_no", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_gps_tiny_time,
          { "Time (only seconds, s)", "ambit.log_sample.gps_tiny.time", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_tiny_latitude,
          { "Latitude (offset from gps-base/gps-small)", "ambit.log_sample.gps_tiny.latitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_gps_tiny_longitude,
          { "Longitude (offset from gps-base/gps-small)", "ambit.log_sample.gps_tiny.longitude", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_distance_source_type,
          { "Type", "ambit.log_sample.distance_source.type", FT_UINT8, BASE_HEX, VALS(log_samples_distance_source_type_vals), 0x0,NULL, HFILL } },

        { &hf_ambit_log_altitude_source_type,
          { "Type", "ambit.log_sample.altitude_source.type", FT_UINT8, BASE_HEX, VALS(log_samples_altitude_source_type_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_log_altitude_source_altitude_offset,
          { "Altitude offset", "ambit.log_sample.altitude_source.alt_offset", FT_INT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_altitude_source_pressure_offset,
          { "Pressure", "ambit.log_sample.altitude_source.pres_offset", FT_INT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_cadence_source_type,
          { "Type", "ambit.log_sample.cadence_source.type", FT_UINT8, BASE_HEX, VALS(log_samples_cadence_source_type_vals), 0x0,NULL, HFILL } },

        { &hf_ambit_log_fwinfo_build_date,
          { "Build date", "ambit.log_sample.fwinfo.builddate", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_swimming_turn_distance,
          { "Distance (cm)", "ambit.log_sample.swimming_turn.distance", FT_UINT32, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_lengths,
          { "Total pool lengths", "ambit.log_sample.swimming_turn.lengths", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_lengths_wo_change,
          { "Total pool lengths w/o style change(!?)", "ambit.log_sample.swimming_turn.lengths_wo_change", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_prev_style,
          { "Style", "ambit.log_sample.swimming_turn.style", FT_UINT8, BASE_HEX, VALS(log_samples_swimming_style_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_classification0,
          { "ClassificationVector[0]", "ambit.log_sample.swimming_turn.classification0", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_classification1,
          { "ClassificationVector[1]", "ambit.log_sample.swimming_turn.classification1", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_classification2,
          { "ClassificationVector[2]", "ambit.log_sample.swimming_turn.classification2", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_swimming_turn_classification3,
          { "ClassificationVector[3]", "ambit.log_sample.swimming_turn.classification3", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_delayed_store,
          { "Log write delay (1/10 s)", "ambit.log_sample.delayed_store", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_log_ibi,
          { "IBI entry", "ambit.log_sample.ibi", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_gps_data_head,
          { "Current GPS position data version", "ambit.gps.data.head", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },

        { &hf_ambit_write_data_length,
          { "Data Length", "ambit.write_data.custom_modes.length", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_header,
          { "Data Header", "ambit.write_data.custom_modes.header", FT_UINT16, BASE_HEX, VALS(custom_modes_header_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_activity_id,
          { "Activity ID", "ambit.write_data.custom_modes.activity_id", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_interval_repetitions,
          { "IntervalRepetitions", "ambit.write_data.custom_modes.interval_repetitions", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_show_navigation,
          { "Show Navigation Selection", "ambit.write_data.custom_modes.show_navigation", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_alti_baro_mode,
          { "AltiBaroMode", "ambit.write_data.custom_modes.alti_baro_mode", FT_UINT16, BASE_DEC, VALS(custom_modes_alti_baro_mode_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_backlight,
          { "Backlight mode", "ambit.write_data.custom_modes.alti_baro_mode", FT_UINT16, BASE_DEC, VALS(custom_modes_backlight_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_display,
          { "Display mode", "ambit.write_data.custom_modes.alti_baro_mode", FT_UINT16, BASE_DEC, VALS(custom_modes_display_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_quick_navigation,
          { "Quick navigation", "ambit.write_data.custom_modes.alti_baro_mode", FT_UINT16, BASE_DEC, VALS(custom_modes_quick_navigation_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_use_hrbelt_and_pods,
          { "HR belt and PODs", "ambit.write_data.custom_modes.hrbelt_and_pods", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_use_hr_limits,
          { "Use HR limits", "ambit.write_data.custom_modes.use_hr_limits", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_recording_interval,
          { "Recording interval", "ambit.write_data.custom_modes.recording_interval", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_auto_lap,
          { "Autolap (m)", "ambit.write_data.custom_modes.auto_lap", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_gps_interval,
          { "GPS interval", "ambit.write_data.custom_modes.gps_interval", FT_UINT16, BASE_DEC, VALS(custom_modes_gps_accuracy_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_display_type,
          { "Display type", "ambit.write_data.custom_modes.display_type", FT_UINT16, BASE_HEX, VALS(custom_modes_display_type_val), 0x0,NULL, HFILL } },
        { &hf_ambit_log_activity_custom_mode_row,
          { "            Display row", "ambit.write_data.custom_modes.display_row", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_activity_custom_mode_display,
          { "            Display type", "ambit.write_data.custom_modes.display_type", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_log_activity_custom_mode_view,
          { "            View type (lower row)", "ambit.write_data.custom_modes.view", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_auto_pause,
          { "Auto pause", "ambit.write_data.custom_modes.auto_pause", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_auto_scroll,
          { "Auto scroll (sec)", "ambit.write_data.custom_modes.auto_scroll", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_use_interval_timer,
          { "Use interval timer", "ambit.write_data.custom_modes.interval_timer", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_interval_timer_time,
          { "Interval timer unit", "ambit.write_data.custom_modes.interval_timer_time", FT_UINT16, BASE_DEC, VALS(custom_modes_interval_timer_unit_vals), 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_interval_timer_max,
          { "Interval timer max (sec or meters)", "ambit.write_data.custom_modes.interval_timer_max", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_interval_timer_min,
          { "Interval timer min (sec or meters)", "ambit.write_data.custom_modes.interval_timer_min", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },

        { &hf_ambit_write_data_apps_nbr_of,
          { "Number of apps", "ambit.write_data.apps.nbr", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_apps_header_len,
          { "Apps header length", "ambit.write_data.apps.header_len", FT_UINT16, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_apps_end_pos,
          { "Pos of app checksum", "ambit.write_data.apps.end_pos", FT_UINT32, BASE_HEX, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_app_data,
          { "App data", "ambit.write_data.apps.data", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_app_checksum,
          { "App checksum (xor of app data + app data length)", "ambit.write_data.apps.checksum", FT_UINT8, BASE_DEC, NULL, 0x0,NULL, HFILL } },
        { &hf_ambit_write_data_app_index,
          { "App index", "ambit.write_data.apps.index", FT_STRING, BASE_NONE, NULL, 0x0,NULL, HFILL } },
    };

    static gint *ett[] = {
        &ett_ambit,
        &ett_ambit_data,
        &ett_ambit_log_data,
        &ett_ambit_log_samples,
        &ett_ambit_log_sample,
        &ett_ambit3_log_headers,
        &ett_ambit3_log_header
    };

    proto_ambit = proto_register_protocol (
        "Suunto Ambit USB Protocol",
        "Ambit",
        "ambit"
        );

    proto_register_field_array(proto_ambit, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
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
