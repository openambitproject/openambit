#!/bin/sh

set -eu

SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"${SOURCE_LOCATION}\" && pwd )`"

cd ${SOURCE_LOCATION}

application=openambit

if [ $# -gt 0 ];then
    echo running $1
    application=$1
    shift
fi

case "$application" in
    openambit)          builddir=${application}-build;;
    ambitconsole)       builddir=example-build;;
    openambit-cli)      builddir=${application}-build;;
    *)
	echo "$application: not supported" >&2
	exit 1
	;;
esac

echo "------running $application------"
LD_LIBRARY_PATH=./libambit-build ./${builddir}/${application} "$@"
exit $?
