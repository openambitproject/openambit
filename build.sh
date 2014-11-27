#!/bin/bash

SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"$SOURCE_LOCATION\" && pwd )`"

CORES=$(cat /proc/cpuinfo | grep processor | wc -l)

cd $SOURCE_LOCATION

echo "------building libambit------"
mkdir -p libambit-build
cd libambit-build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr "$@" ../src/libambit
make -j$CORES
if [ "$DO_INSTALL" == "1" ]; then
    echo "------installing libambit------"
    sudo make install
fi

cd $SOURCE_LOCATION
echo "------building openambit------"
mkdir -p openambit-build
cd openambit-build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr "$@" ../src/openambit
make -j$CORES
if [ "$DO_INSTALL" == "1" ]; then
    echo "------installing openambit------"
    sudo make install
fi

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
