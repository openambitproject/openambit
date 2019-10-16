#!/bin/bash

set -e

# ensure that openambit2gpx.py works

python2.7 tools/openambit2gpx.py test-data/testlog.log "$TMP"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "$TMP"/testlog.gpx

python3 tools/openambit2gpx.py test-data/testlog.log "$TMP"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "$TMP"/testlog.gpx
