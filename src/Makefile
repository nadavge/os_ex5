CC=g++
FLAGS=-Wall -pthread -std=c++11

TAR=tar
TARFLAGS=cvf
TARNAME=ex5.tar
TARSRCS=clftp.cpp srftp.cpp utils.* README Makefile performance.jpg

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
