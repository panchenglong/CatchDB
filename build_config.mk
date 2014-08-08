CC=gcc
CXX=g++
MAKE=make
LEVELDB_PATH=/home/panchenglong/catchdb/deps/leveldb-1.15.0
JEMALLOC_PATH=/home/panchenglong/catchdb/deps/jemalloc-3.6.0
SNAPPY_PATH=/home/panchenglong/catchdb/deps/snappy-1.1.1
CFLAGS=
CFLAGS += -std=c++0x
CFLAGS += -DNDEBUG -D__STDC_FORMAT_MACROS -Wall -g -Wno-sign-compare
CFLAGS += 
CFLAGS += -I "/home/panchenglong/catchdb/deps/leveldb-1.15.0/include"
CLIBS=
CLIBS += -pthread
CLIBS += "/home/panchenglong/catchdb/deps/leveldb-1.15.0/libleveldb.a"
CLIBS += "/home/panchenglong/catchdb/deps/snappy-1.1.1/.libs/libsnappy.a"
CLIBS += "/home/panchenglong/catchdb/deps/jemalloc-3.6.0/lib/libjemalloc.a"
CFLAGS += -I "/home/panchenglong/catchdb/deps/jemalloc-3.6.0/include"
