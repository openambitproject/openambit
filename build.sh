#!/bin/bash

SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"$SOURCE_LOCATION\" && pwd )`"

CORES=$(cat /proc/cpuinfo | grep processor | wc -l)

for target in libambit movescount openambit
do
    cd $SOURCE_LOCATION
    echo "------building $target------"
    mkdir -p $target-build
    cd $target-build
    cmake "$@" ../src/$target
    make -j$CORES
    if [ "$DO_INSTALL" == "1" ]; then
	echo "------installing $target------"
	sudo make install
    fi
done

if [ "$BUILD_EXTRAS" == "1" ]; then
    cd $SOURCE_LOCATION
    echo "------building example------"
    mkdir -p example-build
    cd example-build
    cmake "$@" ../src/example
    make -j$CORES

    cd $SOURCE_LOCATION
    echo "------building wireshark dissector------"
    mkdir -p dissector-build
    cd dissector-build
    cmake "$@" ../wireshark_dissector
    make -j$CORES
fi
