#!/usr/bin/env bash
BASE_DIR=`pwd`
JEMALLOC_PATH="$BASE_DIR/deps/jemalloc-3.6.0"
LEVELDB_PATH="$BASE_DIR/deps/leveldb-1.15.0"
SNAPPY_PATH="$BASE_DIR/deps/snappy-1.1.1"

if test -z "$TARGET_OS"; then
	TARGET_OS=`uname -s`
fi
if test -z "$MAKE"; then
	MAKE=make
fi
if test -z "$CC"; then
	CC=gcc
fi
if test -z "$CXX"; then
	CXX=g++
fi

#if [[ "$TARGET_OS" != "Linux" ]]; then
#    echo "Only support Linux Currently"
#    exit 1
#fi
PLATFORM_CLIBS="-pthread"

DIR=`pwd`
cd $SNAPPY_PATH
if [ ! -f Makefile ]; then
	echo ""
	echo "##### building snappy... #####"
	./configure $SNAPPY_HOST
	find . | xargs touch
	make
	echo "##### building snappy finished #####"
	echo ""
fi
cd "$DIR"


DIR=`pwd`
cd $JEMALLOC_PATH
if [ ! -f Makefile ]; then
    echo ""
    echo "##### building jemalloc... #####"
    ./configure
    make
    echo "##### building jemalloc finished #####"
    echo ""
fi
cd "$DIR"

rm -f src/version.h
echo "#ifndef CATCHDB_DEPS_H" >> src/version.h
echo "#ifndef CATCHDB_VERSION" >> src/version.h
echo "#define CATCHDB_VERSION \"`cat version`\"" >> src/version.h
echo "#endif" >> src/version.h
echo "#endif" >> src/version.h

echo "#include <stdlib.h>" >> src/version.h
echo "#include <jemalloc/jemalloc.h>" >> src/version.h

rm -f build_config.mk
echo CC=$CC >> build_config.mk
echo CXX=$CXX >> build_config.mk
echo "MAKE=$MAKE" >> build_config.mk
echo "LEVELDB_PATH=$LEVELDB_PATH" >> build_config.mk
echo "JEMALLOC_PATH=$JEMALLOC_PATH" >> build_config.mk
echo "SNAPPY_PATH=$SNAPPY_PATH" >> build_config.mk

echo "CFLAGS=" >> build_config.mk

GCC_VERSION=`g++ --version | head -n1 | grep -oE '[^ ]+$'`
#if [[ ! "$GCC_VERSION" < "4.8" ]]; then
#    echo "CFLAGS += -std=c++11" >> build_config.mk
#else
echo "CFLAGS += -std=c++0x" >> build_config.mk
#fi

echo "CFLAGS += -DNDEBUG -D__STDC_FORMAT_MACROS -Wall -O2 -Wno-sign-compare" >> build_config.mk
#echo "CFLAGS += -DNDEBUG -D__STDC_FORMAT_MACROS -Wall -g -Wno-sign-compare" >> build_config.mk
echo "CFLAGS += ${PLATFORM_CFLAGS}" >> build_config.mk
echo "CFLAGS += -I \"$LEVELDB_PATH/include\"" >> build_config.mk

echo "CLIBS=" >> build_config.mk
echo "CLIBS += ${PLATFORM_CLIBS}" >> build_config.mk
echo "CLIBS += \"$LEVELDB_PATH/libleveldb.a\"" >> build_config.mk
echo "CLIBS += \"$SNAPPY_PATH/.libs/libsnappy.a\"" >> build_config.mk


echo "CLIBS += \"$JEMALLOC_PATH/lib/libjemalloc.a\"" >> build_config.mk
echo "CFLAGS += -I \"$JEMALLOC_PATH/include\"" >> build_config.mk

if test -z "$TMPDIR"; then
    TMPDIR=/tmp
fi
