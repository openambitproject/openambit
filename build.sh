SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"$SOURCE_LOCATION\" && pwd )`"

CORES=$(cat /proc/cpuinfo | grep processor | wc -l)

cd $SOURCE_LOCATION

echo "------building libambit------"
mkdir -p libambit-build
cd libambit-build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ../src/libambit
make -j$CORES
if [ "$DO_INSTALL" == "1" ]; then
    echo "------installing libambit------"
    sudo make install
fi

cd $SOURCE_LOCATION
echo "------building openambit------"
mkdir -p openambit-build
cd openambit-build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ../src/openambit
make -j$CORES
if [ "$DO_INSTALL" == "1" ]; then
    echo "------installing openambit------"
    sudo make install
fi
