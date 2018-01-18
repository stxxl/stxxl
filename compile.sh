#!/bin/sh
############################################################################
#  compile.sh
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2017 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

set -ex

git submodule init
git submodule update --recursive

CMAKE_OPTS="-DCMAKE_BUILD_TYPE=RelWithAssert -DSTXXL_BUILD_TESTS=ON -DSTXXL_BUILD_EXAMPLES=ON"

# try to find a modern C++ compiler
set +x
COMPILERS_LIST=
for MAJOR in `seq 9 -1 4`; do
    COMPILERS_LIST="$COMPILERS_LIST g++-$MAJOR"
    for MINOR in `seq 9 -1 0`; do
        COMPILERS_LIST="$COMPILERS_LIST g++-$MAJOR.$MINOR"
        for PATCH in `seq 9 -1 0`; do
            COMPILERS_LIST="$COMPILERS_LIST g++-$MAJOR.$MINOR.$PATCH"
        done
    done
done
#echo $COMPILERS_LIST

for CMD in $COMPILERS_LIST; do
    if command -v "$CMD" > /dev/null; then
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_CXX_COMPILER=$CMD"
        break
    fi
done
set -x

# detect number of cores
if [ -e /proc/cpuinfo ]; then
    CORES=$(grep -c ^processor /proc/cpuinfo)
elif [ "$(uname)" == "Darwin" ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi
MAKEOPTS=-j$CORES

if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake .. $CMAKE_OPTS $@
make $MAKEOPTS
ctest -V

################################################################################
