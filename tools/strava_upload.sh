#!/bin/sh

#
# Fetch logs from watch, convert them info .gpx and upload it to Strava
#

ROOT=.

if [ -d tools ] ; then
    ROOT=.
elif [ -d stravauploader ] ; then
    # We are in tools
    ROOT=../
fi

# Get current number of logs
ls -l ~/.openambit/*.log >/dev/null 2>&1
logs_avail=$?
if [ $logs_avail -eq 0 ] ; then
    nb_logs=`ls -l ~/.openambit/*.log|wc -l`
else
    nb_logs=0
fi

# Fetch new logs from watch
if [ -f ${ROOT}/openambit-cli-build/openambit-cli ] ; then
    OPENAMBIT_CLI=${ROOT}/openambit-cli-build/openambit-cli
else
    # Try installed openambit-cli
    OPENAMBIT_CLI=openambit-cli
fi

$OPENAMBIT_CLI --no-sync-sport-mode --no-sync-navigation $@ || exit 1

# Get new number of logs
nb_new_logs=`ls -l ~/.openambit/*.log|wc -l`
nb_new_logs=$(($nb_new_logs-$nb_logs))

if [ $nb_new_logs -eq 0 ] ; then
    echo "No new logs, exit"
    exit 0
fi

if [ ! -e ${ROOT}/gpx ] ; then
    mkdir ${ROOT}gpx
fi

# Convert new logs to gpx format
logs=`ls ~/.openambit/*.log|tail -n $nb_new_logs`

logs_out=""
for log in $logs ; do
    log_out="${ROOT}/gpx/$(basename $log .log).gpx"
    echo ${ROOT}/tools/openambit2gpx.py "$log" "$log_out"
    ${ROOT}/tools/openambit2gpx.py "$log" "$log_out"
    logs_out="$logs_out $log_out"
done

# Strava upload
echo ${ROOT}/tools/stravauploader/strava_uploader.py -l $logs_out
${ROOT}/tools/stravauploader/strava_uploader.py -l $logs_out

