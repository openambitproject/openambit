#include <stdlib.h>
#include <stdio.h>
#include <libambit.h>

static int log_skip_cb(void *ambit_object, ambit_log_header_t *log_header);
static void log_data_cb(void *object, ambit_log_entry_t *log_entry);

int main(int argc, char *argv[])
{
    ambit_object_t *ambit_object;
    ambit_device_info_t info;
    ambit_device_status_t status;
    ambit_personal_settings_t settings;
    time_t current_time;
    struct tm *local_time;

    if ((ambit_object = libambit_detect()) != NULL) {
        libambit_device_info_get(ambit_object, &info);

        if (libambit_device_supported(ambit_object)) {
            printf("Device: %s, serial: %s, FW version: %d.%d.%d\n", info.name, info.serial, info.fw_version[0], info.fw_version[1], info.fw_version[2] | (info.fw_version[3] << 8));

            if (libambit_device_status_get(ambit_object, &status) == 0) {
                printf("Current charge: %d%%\n", status.charge);
            }
            else {
                printf("Failed to read status\n");
            }

            if (libambit_personal_settings_get(ambit_object, &settings) == 0) {
            }
            else {
                printf("Failed to read status\n");
            }

            current_time = time(NULL);
            local_time = localtime(&current_time);
            //if (libambit_date_time_set(ambit_object, local_time) == 0) {
            //}
            //else {
            //    printf("Failed to set date and time\n");
            //}

            libambit_log_read(ambit_object, log_skip_cb, log_data_cb, NULL, ambit_object);
        }
        else {
            printf("Device: %s (fw_version: %d.%d.%d) is not supported yet!\n", info.name, info.fw_version[0], info.fw_version[1], info.fw_version[2] | (info.fw_version[3] << 8));
        }

        libambit_close(ambit_object);
    }
    else {
        printf("No clock found, exiting\n");
    }

    return 0;
}

static int log_skip_cb(void *ambit_object, ambit_log_header_t *log_header)
{
    static int log_count = 0;

    printf("Got log header \"%s\" %d-%02d-%02d %02d:%02d:%02d\n", log_header->activity_name, log_header->date_time.year, log_header->date_time.month, log_header->date_time.day, log_header->date_time.hour, log_header->date_time.minute, log_header->date_time.msec/1000);

    if (log_count++ > 1) {
        return 0;
    }

    return 1;
}

static void log_data_cb(void *object, ambit_log_entry_t *log_entry)
{
    printf("Got log entry \"%s\" %d-%02d-%02d %02d:%02d:%02d\n", log_entry->header.activity_name, log_entry->header.date_time.year, log_entry->header.date_time.month, log_entry->header.date_time.day, log_entry->header.date_time.hour, log_entry->header.date_time.minute, log_entry->header.date_time.msec/1000);

    int i;
    for (i=0; i<log_entry->header.samples_count; i++) {
        printf("Sample #%d, type: %d, time: %04u-%02u-%02u %02u:%02u:%2.3f\n", i, log_entry->samples[i].type, log_entry->samples[i].utc_time.year, log_entry->samples[i].utc_time.month, log_entry->samples[i].utc_time.day, log_entry->samples[i].utc_time.hour, log_entry->samples[i].utc_time.minute, (1.0*log_entry->samples[i].utc_time.msec)/1000);
    }
}
