CC=g++
FLAGS=-Wall -pthread -std=c++11 -g #TODO remove debugging

TAR=tar
TARFLAGS=cvf
TARNAME=ex5.tar
TARSRCS=#TODO add files

all: clftp srftp

clftp: clftp.o utils.o
	$(CC) $(FLAGS) $^ -o $@

srftp: srftp.o utils.o
	$(CC) $(FLAGS) $^ -o $@

#======== OBJECTS ==========

%.o: %.cpp
	$(CC) $(FLAGS) -c $<

#======== SOURCE ===========

clftp.cpp: utils.h

srftp.cpp: utils.h

utils.cpp: utils.h

#======== MISC =============

tar: $(TARSRCS)
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean tar remake

clean:
	rm -f *.o srftp clftp

remake: clean all
