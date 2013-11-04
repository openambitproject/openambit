#include <stdlib.h>
#include <stdio.h>
#include <libambit.h>

int main(int argc, char *argv[])
{
    ambit_object_t *ambit_object;
    ambit_device_info_t info;
    ambit_device_status_t status;
    ambit_personal_settings_t settings;
    time_t current_time;
    struct tm *local_time;

    if ((ambit_object = libambit_detect()) != NULL) {
        printf("Found clock!!!\n");

        if (libambit_device_info_get(ambit_object, &info) == 0) {
            printf("Model: %s, serial: %s, FW version: %d.%d.%d\n", info.model, info.serial, info.fw_version[0], info.fw_version[1], info.fw_version[2]);
        }
        else {
            printf("Failed to read info\n");
        }

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

        libambit_log_read_get_next(ambit_object);

        libambit_close(ambit_object);
    }
    else {
        printf("No clock found, exiting\n");
    }

    return 0;
}
