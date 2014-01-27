SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"$SOURCE_LOCATION\" && pwd )`"

CORES=$(cat /proc/cpuinfo | grep processor | wc -l)

cd $SOURCE_LOCATION

echo "------building libambit------"
mkdir -p libambit-build
cd libambit-build
cmake -DCMAKE_BUILD_TYPE=Debug ../src/libambit
make -j$CORES

cd $SOURCE_LOCATION
echo "------building openambit------"
mkdir -p openambit-build
cd openambit-build
cmake -DCMAKE_BUILD_TYPE=Debug ../src/openambit
make -j$CORES
