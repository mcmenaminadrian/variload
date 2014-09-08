default: all

all: variload 

debug: debugvariload

#get rid of junk
clean:
	rm -f *.o

# normal build
variload: runtimer.o pages.o threadhandler.o \
	analysis.o
	g++ -std=c++0x -O2 -o variload -Wall pages.o analysis.o \
		threadhandler.o runtimer.o -lexpat -lpthread -lncurses

analysis.o: analysis.cpp threadhandler.h analysis.h
	g++ -std=c++0x -O2 -o analysis.o -c -Wall analysis.cpp

threadhandler.o: threadhandler.c threadhandler.h analysis.h
	gcc -O2 -o threadhandler.o -c -Wall threadhandler.c

pages.o: pages.cpp 
	g++ -std=c++0x -I /opt/local/include/ -O2 -o pages.o -c -Wall pages.cpp

runtimer.o: runtimer.c threadhandler.h 
	gcc -O2 -o runtimer.o -c -Wall runtimer.c

# debug build
debugvariload: druntimer.o dpages.o dthreadhandler.o \
	danalysis.o
	g++ -g -std=c++0x -O0 -o variload -Wall dpages.o \
		danalysis.o \
		dthreadhandler.o druntimer.o -lexpat -lpthread -lncurses

danalysis.o: analysis.cpp threadhandler.h analysis.h
	g++ -g -std=c++0x -O0 -o danalysis.o -c -Wall analysis.cpp

dthreadhandler.o: threadhandler.c threadhandler.h analysis.h
	gcc -g -O0 -o dthreadhandler.o -c -Wall threadhandler.c

dpages.o: pages.cpp
	g++ -g -std=c++0x -O0 -I /opt/local/include/ -o dpages.o -c -Wall pages.cpp

druntimer.o: runtimer.c threadhandler.h 
	gcc -g -O0 -o druntimer.o -c -Wall runtimer.c


