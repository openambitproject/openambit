/*
 * (C) Copyright 2013 Emil Ljungdahl
 *
 * This file is part of Openambit.
 *
 * Openambit is free software: you can redistribute it and/or modify
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
#include "logentry.h"

LogEntry::LogEntry() :
    deviceInfo(NULL),
    personalSettings(NULL),
    logEntry(NULL)
{
}

LogEntry::LogEntry(const LogEntry &other)
{
    u_int32_t i;

    device = other.device;
    time = other.time;
    movescountId = other.movescountId;

    if (other.deviceInfo != NULL) {
        deviceInfo = (ambit_device_info_t*)malloc(sizeof(ambit_device_info_t));
        memcpy(deviceInfo, other.deviceInfo, sizeof(ambit_device_info_t));
    }
    else {
        deviceInfo = NULL;
    }

    if (other.personalSettings != NULL) {
        personalSettings = (ambit_personal_settings_t*)malloc(sizeof(ambit_personal_settings_t));
        memcpy(personalSettings, other.personalSettings, sizeof(ambit_personal_settings_t));
    }
    else {
        personalSettings = NULL;
    }

    if (other.logEntry != NULL) {
        logEntry = (ambit_log_entry_t*)malloc(sizeof(ambit_log_entry_t));
        memcpy(logEntry, other.logEntry, sizeof(ambit_log_entry_t));
        if (other.logEntry->samples != NULL) {
            logEntry->samples = (ambit_log_sample_t*)malloc(sizeof(ambit_log_sample_t)*other.logEntry->samples_count);
            memcpy(logEntry->samples, other.logEntry->samples, sizeof(ambit_log_sample_t)*other.logEntry->samples_count);
            for (i=0; i<other.logEntry->samples_count; i++) {
                if (other.logEntry->samples[i].type == ambit_log_sample_type_periodic) {
                    if (other.logEntry->samples[i].u.periodic.values != NULL) {
                        logEntry->samples[i].u.periodic.values = (ambit_log_sample_periodic_value_t*)malloc(sizeof(ambit_log_sample_periodic_value_t)*other.logEntry->samples[i].u.periodic.value_count);
                        memcpy(logEntry->samples[i].u.periodic.values, other.logEntry->samples[i].u.periodic.values, sizeof(ambit_log_sample_periodic_value_t)*other.logEntry->samples[i].u.periodic.value_count);
                    }
                }
                if (other.logEntry->samples[i].type == ambit_log_sample_type_gps_base) {
                    if (other.logEntry->samples[i].u.gps_base.satellites != NULL) {
                        logEntry->samples[i].u.gps_base.satellites = (ambit_log_gps_satellite_t*)malloc(sizeof(ambit_log_gps_satellite_t)*logEntry->samples[i].u.gps_base.satellites_count);
                        memcpy(logEntry->samples[i].u.gps_base.satellites, other.logEntry->samples[i].u.gps_base.satellites, sizeof(ambit_log_gps_satellite_t)*logEntry->samples[i].u.gps_base.satellites_count);
                    }
                }
                if (other.logEntry->samples[i].type == ambit_log_sample_type_unknown) {
                    if (other.logEntry->samples[i].u.unknown.datalen > 0 && other.logEntry->samples[i].u.unknown.data != NULL) {
                        logEntry->samples[i].u.unknown.data = (uint8_t*)malloc(other.logEntry->samples[i].u.unknown.datalen);
                        memcpy(logEntry->samples[i].u.unknown.data, other.logEntry->samples[i].u.unknown.data, other.logEntry->samples[i].u.unknown.datalen);
                    }
                }
            }
        }
    }
}

LogEntry& LogEntry::operator=(const LogEntry &rhs)
{
    LogEntry tmp(rhs);

    std::swap(device, tmp.device);
    std::swap(time, tmp.time);
    std::swap(movescountId, tmp.movescountId);
    std::swap(deviceInfo, tmp.deviceInfo);
    std::swap(personalSettings, tmp.personalSettings);
    std::swap(logEntry, tmp.logEntry);

    return *this;
}

LogEntry::~LogEntry()
{
    u_int32_t i;

    if (deviceInfo != NULL) {
        free(deviceInfo);
        deviceInfo = NULL;
    }

    if (personalSettings != NULL) {
        free(personalSettings);
        personalSettings = NULL;
    }

    if (logEntry != NULL) {
        if (logEntry->samples != NULL) {
            for (i=0; i<logEntry->samples_count; i++) {
                if (logEntry->samples[i].type == ambit_log_sample_type_periodic) {
                    if (logEntry->samples[i].u.periodic.values != NULL) {
                        free(logEntry->samples[i].u.periodic.values);
                    }
                }
                if (logEntry->samples[i].type == ambit_log_sample_type_gps_base) {
                    if (logEntry->samples[i].u.gps_base.satellites != NULL) {
                        free(logEntry->samples[i].u.gps_base.satellites);
                    }
                }
                if (logEntry->samples[i].type == ambit_log_sample_type_unknown) {
                    if (logEntry->samples[i].u.unknown.data != NULL) {
                        free(logEntry->samples[i].u.unknown.data);
                    }
                }
            }
            free(logEntry->samples);
        }
        free(logEntry);
    }

    logEntry = NULL;
}

bool LogEntry::isUploaded(){
    if (this->movescountId == NULL){
        return false;
    }
    return true;
}

QString LogEntry::toHtml(){
    QString log_html;

    log_html += "<h1>" + QString::fromLatin1(this->logEntry->header.activity_name) + "</h1>";
    if (this->isUploaded()){
        log_html += "<a href='http://www.movescount.com/moves/move" + this->movescountId + "'>see on movescount.com</a>";
    }
    else {
        log_html += "Not uploaded yet";
    }
    log_html += "<h2>Details</h2>";
    log_html += "<h4>" + this->time.toString() + "</h4>";
    log_html += "<h4>Duration: " + QTime(0, 0, 0,0 ).addMSecs(this->logEntry->header.duration).toString("HH:mm:ss") + "</h4>";
    log_html += "<h4>Distance: " + QString::number(this->logEntry->header.distance) + " m</h4>";
    log_html += "<h2>Training values</h2>";
    log_html += "<h4>Avg HR: " + QString::number(this->logEntry->header.heartrate_avg) + " bpm</h4>";
    log_html += "<h4>Max HR: " + QString::number(this->logEntry->header.heartrate_max) + " bpm</h4>";
    log_html += "<h4>Min HR: " + QString::number(this->logEntry->header.heartrate_min) + " bpm</h4>";
    log_html += "<h4>PTE: " + QString::number(this->logEntry->header.peak_training_effect/10.0) + "</h4>";
    log_html += "<h2>Device</h2>";
    log_html += "<h4>Name: " + QString(this->deviceInfo->name) + "</h4>";
    log_html += "<h4>Variant: " + QString(this->deviceInfo->model) + "</h4>";
    log_html += "<h4>Serial: " + QString(this->deviceInfo->serial) + "</h4>";
    //log_string += "Device info: " + QString::number(this->deviceInfo->fw_version) + "\n";
    //log_string += "Device info: " + QString::number(this->deviceInfo->hw_version) + "\n";

    //log_string += "Personal Settings: " + this->personalSettings + "\n";


    return log_html;
}


