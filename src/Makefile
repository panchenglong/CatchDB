include ../build_config.mk

OBJS = CatchDB.o EventManager.o Util.o KV.o HashMap.o ZSet.o Queue.o Client.o \
	Networking.o Protocol.o Logger.o  Buffer.o Config.o Iterator.o
EXES = ../catchdb-server


all: ${OBJS} catchdb-server.o
	${CXX} -o ../catchdb-server catchdb-server.o ${OBJS} ${CLIBS}

catchdb-server.o: Util.h Logger.h Config.h EventManager.h Networking.h Protocol.h Client.h catchdb-server.cc
	${CXX} ${CFLAGS} -c catchdb-server.cc

CatchDB.o: CatchDB.h Logger.h AggregateComparator.hh CatchDB.cc
	${CXX} ${CFLAGS} -c CatchDB.cc

Config.o: Config.h Config.cc
	${CXX} ${CFLAGS} -c Config.cc

EventManager.o: EventManager.h Util.h Logger.h EventManager.cc
	${CXX} ${CFLAGS} -c EventManager.cc

KV.o: KV.h CatchDB.h Status.h KV.cc
	${CXX} ${CFLAGS} -c KV.cc

HashMap.o: HashMap.h Logger.h Util.h HashMap.cc
	${CXX} ${CFLAGS} -c HashMap.cc

ZSet.o: ZSet.h Logger.h Util.h ZSet.cc
	${CXX} ${CFLAGS} -c ZSet.cc

Queue.o: Queue.h Logger.h Util.h Queue.cc
	${CXX} ${CFLAGS} -c Queue.cc

Client.o: Client.h Networking.h Util.h Client.cc
	${CXX} ${CFLAGS} -c Client.cc

Util.o: Util.h Util.cc
	${CXX} ${CFLAGS} -c Util.cc

Logger.o: Logger.h Logger.cc
	${CXX} ${CFLAGS} -c Logger.cc

Buffer.o: Buffer.h Buffer.cc
	${CXX} ${CFLAGS} -c Buffer.cc

Networking.o: Networking.h Util.h Networking.cc
	${CXX} ${CFLAGS} -c Networking.cc

Protocol.o: Protocol.h Protocol.cc
	${CXX} ${CFLAGS} -c Protocol.cc

Iterator.o: Iterator.h Iterator.cc
	${CXX} ${CFLAGS} -c Iterator.cc

clean:
	rm -f ${EXES} *.o *.exe

