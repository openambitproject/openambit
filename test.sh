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
    echo
    echo Building ${b} with hid: ${hid}

	  # fully clean previous compilation
    rm -rf *-build

    # build with current build-options
    BUILD_EXTRAS=1 HIDAPI_DRIVER=${hid} ./build.sh -DCMAKE_BUILD_TYPE=${b}

    # run unit-tests
    unittest-build/unittest
  done
done

# ensure that openambit2gpx.py works

echo
echo Testing openambit2gpx

rm -f "${TMP:-/tmp}"/testlog.gpx

python2.7 tools/openambit2gpx.py test-data/testlog.log -out "${TMP:-/tmp}"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "${TMP:-/tmp}"/testlog.gpx

rm -f "${TMP:-/tmp}"/testlog.gpx

python3 tools/openambit2gpx.py test-data/testlog.log -out "${TMP:-/tmp}"/testlog.gpx

diff --ignore-all-space test-data/testlog.gpx "${TMP:-/tmp}"/testlog.gpx

echo
echo Done, all tests passed
