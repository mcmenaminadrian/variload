#define __REENTRANT
#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <curses.h>
#include "threadhandler.h"
#include "pages.h"
#include "analysis.h"

static int threadline = 10;

//launch a thread
static void
spawnThread(int parentThread, int threadNo, struct ThreadGlobal* globals)
{
	move(++threadline, 0);
	printw("Spawning thread %i. at tick %li\n", threadNo,
		globals->totalTicks);
	refresh();

	//find the file that matches the thread number
	struct ThreadRecord* threadRecord = globals->head;

	while (threadRecord) {
		if (threadRecord->number == threadNo) {
			break;
		}
		threadRecord = threadRecord->next;
	}

	struct ThreadLocal* localThreadStuff = (struct ThreadLocal*)
		malloc(sizeof (struct ThreadLocal));
	if (!localThreadStuff) {
		fprintf(stderr, "Could not create local stuff for thread %i\n",
			threadNo);
		goto failTL;
	}

	localThreadStuff->threadNumber = threadNo;
	localThreadStuff->instructionCount = 0;
	localThreadStuff->tickCount = 0;
	localThreadStuff->prevTickCount = 0;
	localThreadStuff->faultCount = 0;
	localThreadStuff->prevInstructionCount = 0;
	localThreadStuff->prevFaultCount = 0;

	int errL = pthread_mutex_init(&localThreadStuff->threadLocalLock, NULL);
	if (errL) {
		fprintf(stderr,
			"Error %i when initialising lock on thread %i\n",
			errL, threadNo);
		goto failLock;
	}

	struct ThreadResources* threadResources = (struct ThreadResources*)
		malloc(sizeof (struct ThreadResources));
	if (!threadResources) {
		fprintf(stderr,
			"Could not allocate memory for ThreadResources for thread %i\n",
			threadNo);
		goto failTR;
	}

	threadResources->records = threadRecord;
	threadResources->globals = globals;
	threadResources->local = localThreadStuff;
	threadResources->records->local = localThreadStuff;

	struct ThreadArray* anotherThread = (struct ThreadArray*)
		malloc(sizeof (struct ThreadArray));
	if (!anotherThread) {
		fprintf(stderr,
			"Could not create pThread memory for thread %i\n",
			threadNo);
		goto failTA;
	}
	anotherThread->threadNumber = parentThread;
	anotherThread->nextThread = NULL;

	pthread_mutex_lock(&globals->threadGlobalLock);
	struct ThreadArray* tArray = globals->threads;	
	while (tArray) {
		if (tArray->nextThread == NULL) {
			tArray->nextThread = anotherThread;
			break;
		}
		tArray = tArray->nextThread;
	}
	pthread_mutex_unlock(&globals->threadGlobalLock);
	
	pthread_create(&anotherThread->aPThread, NULL, startThreadHandler,
		(void*)threadResources);
	return;

failTA:
	free(threadResources);
failTR:
failLock:
	free(localThreadStuff);
failTL:
	return;
}

static int faultPage(long pageNumber, struct ThreadResources *thResources)
{
	int countDown = tickWait/MEMWIDTH;
	while (countDown) {
		if (locatePageTreePR(pageNumber,
			thResources->globals->lowTree) || locatePageTreePR(
			pageNumber, thResources->globals->highTree)) {
			return 0;
		}
		updateTickCount(thResources);
		countDown--;
	}
	thResources->local->faultCount++;
	return 1;
}

static void
promoteToHighTree(long pageNumber, struct ThreadResources *thResources,
	long offset)
{
	struct ThreadGlobal *globals = thResources->globals;
	removeFromPageTree(pageNumber, globals->lowTree);
	if (countPageTree(globals->highTree) >= globals->maxHighSize) {
		long oldestPage = removeOldestPage(globals->highTree);
		insertIntoPageTree(oldestPage, globals->lowTree);
	}
	insertIntoPageTree(pageNumber, globals->highTree);
	pthread_mutex_unlock(&globals->threadGlobalLock);
	updateTickCount(thResources);
	updateTickCount(thResources);
}

static void
updateHighTree(long pageNumber, struct ThreadResources *thResources,
	long offset)
{
	struct ThreadGlobal *globals = thResources->globals;
	removeFromPageTree(pageNumber, globals->highTree);
	insertIntoPageTree(pageNumber, globals->highTree);
	pthread_mutex_unlock(&globals->threadGlobalLock);
	updateTickCount(thResources);
	updateTickCount(thResources);
}

static void 
notInGlobalTree(long pageNumber, struct ThreadResources *thResources,
	long offset)
{
	struct ThreadGlobal *globals = thResources->globals;
	decrementCoresInUse();
	pthread_mutex_unlock(&globals->threadGlobalLock);
	if (faultPage(pageNumber, thResources) > 0) {
		pthread_mutex_lock(&globals->threadGlobalLock);
			if (countPageTree(globals->lowTree)
				>= globals->maxLowSize)
		{
			long killPage = removeOldestPage(globals->lowTree);
			doneWithRecord(killPage, thResources);
		}
		insertIntoPageTree(pageNumber, globals->lowTree, offset);
		pthread_mutex_unlock(&globals->threadGlobalLock);
	}
	incrementCoresInUse(thResources);
}
	
static void XMLCALL
threadXMLProcessor(void* data, const XML_Char *name, const XML_Char **attr)
{ 
	int i, overrun;
	long address, pageNumber, size, resSize, offset;
	struct ThreadResources *thResources;
	struct ThreadGlobal *globals;
	struct ThreadLocal *local;
	thResources = (struct ThreadResources *)data;
	globals = thResources->globals;
	local = thResources->local;
	overrun = 0;
	if (strcmp(name, "instruction") == 0 || strcmp(name, "load") == 0 ||
		strcmp(name, "store") == 0 || strcmp(name, "modify") == 0) {
		for (i = 0; attr[i]; i += 2) {
			if (strcmp(name, "instruction") == 0) {
				local->anType = 'c';
			} else {
				if (strcmp(name, "load") == 0) {
					local->anType = 'l';
				} else {
					if (strcmp(name, "store") == 0) {
						local->anType = 's';
					} else {
						local->anType = 'm';
					}
				}
			}
			if (strcmp(attr[i], "address") == 0) {
				address = strtol(attr[i+1], NULL, 16);
				pageNumber = address >> BITSHIFT;
				offset = address ^ (pageNumber << BITSHIFT);
				local->anDestination = address;
				local->anPage = pageNumber;
				continue;
			}
			if (strcmp(attr[i], "size") == 0) {
				size = strtol(attr[i+1], NULL, 16);
				local->anSize = size;
				if ((address + size) >> BITSHIFT != pageNumber)
				{
					overrun = 1;
					resSize = (address + size) -
						((pageNumber + 1) << BITSHIFT);
					size = size - resSize;
				}
			}
		}
		//have address - is it already present?
		pthread_mutex_lock(&globals->threadGlobalLock);
		if (locatePageTreePR(pageNumber, globals->lowTree)) {
			promoteToHighTree(pageNumber, thResources, offset);
		} else {
			if (locatePageTreePR(pageNumber, globals->highTree)) {
				updateHighTree(pageNumber, thResources, offset);
			} else {
				notInGlobalTree(pageNumber, thResources, offset);
			}
		}
		insertRecord(thResources);
		if (overrun) {
			pthread_mutex_lock(&globals->threadGlobalLock);
			offset = 0;
			local->anSize = resSize;
			local->anDestination = (pageNumber + 1) << BITSHIFT;
			local->anPage = pageNumber + 1;
			if (locatePageTreePR(pageNumber + 1,
				globals->lowTree)) {
				promoteToHighTree(pageNumber + 1, thResources,
					offset);
			} else {
				if (locatePageTreePR(pageNumber + 1,
					globals->highTree, offset)) {
					updateHighTree(pageNumber + 1,
						thResources, offset);
				} else {
					notInGlobalTree(pageNumber + 1,
						thResources, offset);
				}
			}
			insertRecord(thResources);
		}

		if (strcmp(name, "modify") == 0) {
			//do it again
			pthread_mutex_lock(&globals->threadGlobalLock);
			if (locatePageTreePR(pageNumber, globals->lowTree)){
				promoteToHighTree(pageNumber, thResources);
			} else {
				if (locatePageTreePR(pageNumber,
					globals->highTree)) {
					updateHighTree(pageNumber,
						thResources, offset);
				} else {
					notInGlobalTree(pageNumber,
						thResources, offset);
				}
			}
			insertRecord(thResources);
			if (overrun) {
				pthread_mutex_lock(&globals->threadGlobalLock);
				offset = 0;
				local->anSize = resSize;
				local->anDestination =
					(pageNumber + 1) << BITSHIFT;
				local->anPage = pageNumber + 1;
				if (locatePageTreePR(pageNumber + 1,
					globals->lowTree, offset)) {
					promoteToHighTree(pageNumber + 1,
						thResources, offset);
				} else {
					if (locatePageTreePR(pageNumber + 1,
						globals->highTree)) {
							updateHighTree(
								pageNumber + 1,
								thResources,
								offset);
					} else {
						notInGlobalTree(
							pageNumber + 1,
							thResources,
							offset);
					}
				}
				insertRecord(thResources);
			}
		}
		local->instructionCount++;
	} else {
		if (strcmp(name, "spawn") == 0) {
			for (i = 0; attr[i]; i += 2) {
				if (strcmp(attr[i], "thread") == 0) {
					int threadNo = atoi(attr[i+1]);
					spawnThread(local->threadNumber,
						threadNo, globals);
				}
			}
		}
	}
}

void* startThreadHandler(void *resources)
{
	FILE* inThreadXML;
	size_t len = 0;
	int done;
	char data[BUFFSZ];

	struct ThreadResources *thResources;
	thResources = (struct ThreadResources*)resources;
	//Setup the Parser
	XML_Parser p_threadParser = XML_ParserCreate("UTF-8");
	if (!p_threadParser) {
		fprintf(stderr, "Could not create parser for thread\n");
		return (void*)(-1);
	}
	//Pass the parser thread specific data
	XML_SetUserData(p_threadParser, resources);
	//Start the Parser
	XML_SetStartElementHandler(p_threadParser, threadXMLProcessor);
	inThreadXML = fopen(thResources->records->path, "r");
	if (inThreadXML == NULL) {
		fprintf(stderr, "Could not open %s\n",
			thResources->records->path);
		XML_ParserFree(p_threadParser);
		goto cleanup;
	}
	incrementCoresInUse(thResources);
	incrementActive();
	thResources->local->dead = 0;
	do { 
		len = fread(data, 1, sizeof(data), inThreadXML);
		done = len < sizeof(data);
		
		if (XML_Parse(p_threadParser, data, len, 0) == 0) {
			enum XML_Error errcde =
				XML_GetErrorCode(p_threadParser);
			printf("PARSE ERROR: %s\n", XML_ErrorString(errcde));
			printf("Error at column number %lu\n",
				XML_GetCurrentColumnNumber(p_threadParser));
			printf("Error at line number %lu\n",
				XML_GetCurrentLineNumber(p_threadParser));
			goto cleanup;
		}
	} while(!done);
	decrementActive();
	decrementCoresInUse();
	thResources->local->prevTickCount = 0;
	thResources->local->dead = 1;
	updateTickCount(thResources);
	move(++threadline, 0);
	printw("Thread %i finished at tick %li\n",
		thResources->local->threadNumber,
		thResources->globals->totalTicks);
	refresh();
	
	struct ThreadArray* aThread = thResources->globals->threads;
	while (aThread) {
		if (aThread->threadNumber == thResources->local->threadNumber){
			pthread_join(aThread->aPThread, NULL);
		}
		aThread = aThread->nextThread;
	}

cleanup:
	
	if (thResources->local->threadNumber == 1) {
		getch();
		removePageTree(thResources->globals->lowTree);
		removePageTree(thResources->globals->highTree);
		free(thResources->globals);
		removeRecordsTree(thResources);
	}
	free(thResources);
	
	return NULL;
}
