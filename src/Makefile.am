bin_PROGRAMS=ftserver
ftserver_SOURCES=daemon.cpp GLog.cpp IOServiceKeep.cpp MessageQueue.cpp NTPClient.cpp tcpasio.cpp \
                 AsciiProtocol.cpp FileWritter.cpp DataTransfer.cpp FileReceiver.cpp TransferAgent.cpp \
                 ftserver.cpp

ftserver_LDFLAGS=-L/usr/local/lib
COMMON_LIBS=-lpthread -lcurl -lm
BOOST_LIBS=-lboost_system-mt -lboost_thread-mt -lboost_filesystem-mt
ftserver_LDADD=${COMMON_LIBS} ${BOOST_LIBS}
