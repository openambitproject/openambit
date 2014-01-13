SOURCE_LOCATION="`dirname \"$0\"`"
SOURCE_LOCATION="`( cd \"$SOURCE_LOCATION\" && pwd )`"

cd $SOURCE_LOCATION
echo "------running openambit------"
LD_LIBRARY_PATH=./libambit-build ./openambit-build/openambit


