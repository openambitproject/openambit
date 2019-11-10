#!/bin/bash

set -eu

# clean before building
rm -rf *-build

# first build with default option
BUILD_EXTRAS=1 ./build.sh

# then build in various ways
for b in Debug Release;do
  # not testable here: mac windows
	for hid in libudev libusb pcapsimulate;do
    rm -rf *-build
    BUILD_EXTRAS=1 HIDAPI_DRIVER=${hid} ./build.sh -DCMAKE_BUILD_TYPE=${b}
  done
done

# ensure that openambit2gpx.py works

python2.7 tools/openambit2gpx.py test-data/testlog.log "${TMP:-/tmp}"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "${TMP:-/tmp}"/testlog.gpx

python3 tools/openambit2gpx.py test-data/testlog.log "${TMP:-/tmp}"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "${TMP:-/tmp}"/testlog.gpx
