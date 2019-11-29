bin_PROGRAMS=ftserver
ftserver_SOURCES=daemon.cpp GLog.cpp IOServiceKeep.cpp MessageQueue.cpp NTPClient.cpp tcpasio.cpp \
                 AsciiProtocol.cpp FileWritter.cpp FileReceiver.cpp TransferAgent.cpp \
                 DBCurl.cpp ftserver.cpp

if DEBUG
  AM_CFLAGS = -g3 -O0 -Wall -DNDEBUG
  AM_CXXFLAGS = -g3 -O0 -Wall -DNDEBUG
else
  AM_CFLAGS = -O3 -Wall
  AM_CXXFLAGS = -O3 -Wall
endif

ftserver_LDFLAGS=-L/usr/local/lib
COMMON_LIBS=-lpthread -lcurl -lm
BOOST_LIBS=-lboost_system-mt -lboost_date_time-mt -lboost_filesystem-mt -lboost_chrono-mt
ftserver_LDADD=${COMMON_LIBS} ${BOOST_LIBS}

if LINUX
ftserver_LDADD += -lboost_thread -lrt
endif
if OSX
ftserver_LDADD += -lboost_thread-mt
endif
