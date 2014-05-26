default: all

all: analysis 

debug: debuganalysis

#get rid of junk
clean:
	rm -f *.o

# normal build
analysis: runtimer.o insttree.o rbpages.o opttree.o threadhandler.o \
	analysis.o
	g++ -O2 -o analysis -Wall insttree.o rbpages.o opttree.o analysis.o \
		threadhandler.o runtimer.o -lexpat -lpthread -lncurses

analysis.o: analysis.cpp threadhandler.h analysis.h
	g++ -O2 -o analysis.o -c -Wall analysis.cpp

insttree.o: insttree.cpp insttree.h 
	g++ -O2 -o insttree.o -c -Wall insttree.cpp

threadhandler.o: threadhandler.c threadhandler.h analysis.h
	gcc -O2 -o threadhandler.o -c -Wall threadhandler.c

opttree.o: opttree.cpp
	g++ -O2 -o opttree.o -c -Wall opttree.cpp

rbpages.o: pages.cpp 
	g++ -O2 -o rbpages.o -c -Wall pages.cpp

runtimer.o: runtimer.c threadhandler.h 
	gcc -O2 -o runtimer.o -c -Wall runtimer.c

# debug build
debuganalysis: druntimer.o dinsttree.o drbpages.o dopttree.o dthreadhandler.o \
	danalysis.o
	g++ -g -o analysis -Wall dinsttree.o drbpages.o dopttree.o \
		danalysis.o \
		dthreadhandler.o druntimer.o -lexpat -lpthread -lncurses

danalysis.o: analysis.cpp threadhandler.h analysis.h
	g++ -g -o danalysis.o -c -Wall analysis.cpp

dinsttree.o: insttree.cpp insttree.h 
	g++ -g -o dinsttree.o -c -Wall insttree.cpp

dthreadhandler.o: threadhandler.c threadhandler.h analysis.h
	gcc -g -o dthreadhandler.o -c -Wall threadhandler.c

dopttree.o: opttree.cpp
	g++ -g -o dopttree.o -c -Wall opttree.cpp

drbpages.o: pages.cpp
	g++ -g -o drbpages.o -c -Wall pages.cpp

druntimer.o: runtimer.c threadhandler.h 
	gcc -g -o druntimer.o -c -Wall runtimer.c


