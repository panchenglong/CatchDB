#PREFIX=/usr/local/catchdb
PREFIX=~/tmp/

$(shell sh build.sh 1>&2)
include build_config.mk

all:
	chmod u+x "${LEVELDB_PATH}/build_detect_platform"
	cd "${LEVELDB_PATH}"; ${MAKE}
	cd src; ${MAKE}

install:
	mkdir -p ${PREFIX}
	mkdir -p ${PREFIX}/deps
	mkdir -p ${PREFIX}/var
	cp catchdb-server catchdb.conf ${PREFIX}
	chmod -R ugo+rw ${PREFIX}
	rm -f ${PREFIX}/Makefile

clean:
	cd src; ${MAKE} clean

clean_all: clean
	cd "${LEVELDB_PATH}"; ${MAKE} clean
	rm -f ${JEMALLOC_PATH}/Makefile
	rm -f ${SNAPPY_PATH}/Makefile
	
