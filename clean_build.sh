#!/bin/bash
TOP_DIR="$(pwd)"

[ ! -d "build" ] && mkdir build
INSTALL_DIR="$TOP_DIR/build"

echo $TOP_DIR $INSTALL_DIR

cmake $TOP_DIR -Btmp_cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
cmake --build tmp_cmake --clean-first --target install
$INSTALL_DIR/bin/demo
rm -rf tmp_cmake
rm -rf build
