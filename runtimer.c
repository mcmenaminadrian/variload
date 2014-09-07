#define __REENTRANT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <curses.h>
#include <unistd.h>
#include "threadhandler.h"
#include "analysis.h"
#include "pages.h"

int BARRIER = 1;
int SUPER = 100000;
#define THREADLINE 5

struct ThreadRecord *startTR = NULL;
static char outputprefix[BUFFSZ];
static pthread_mutex_t updateLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrierThreshold = PTHREAD_COND_INITIALIZER;
static pthread_cond_t writeProgress = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t coresLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t enoughCores = PTHREAD_COND_INITIALIZER;
static int threadsActive = 0;
static int threadsLocked = 0;
static int coresInUse = 0;
static int writeCountDown;
static pthread_t dataThread;

int tickFind = 4;
int tickLoad = 100;
int cores = 16;
int pageShift = 11;
int shiftMemoryPerCore = 15;
int shiftPageTablePerCore = 12;


//self-contained thread code that writes out performance data
void* writeDataThread(void* tRes)
{
	//struct rlimit limit;
	struct ThreadResources* threadResources =
		(struct ThreadResources*) tRes;
	//lock down the globals until we have written files
	pthread_mutex_lock(&threadResources->globals->threadGlobalLock);
	FILE* fpInstructions;
	FILE* fpFaults;
	int i;
	char filenameInstructions[BUFFSZ];
	char filenameFaults[BUFFSZ];
	time_t now = time(NULL);
	sprintf(filenameInstructions, "Instructions%s.txt",
		asctime(localtime(&now)));
	sprintf(filenameFaults, "Faults%s.txt",
		asctime(localtime(&now)));
	fpInstructions = fopen(filenameInstructions, "w");
	fpFaults = fopen(filenameFaults, "w");
	pthread_mutex_unlock(&threadResources->globals->threadGlobalLock);
	fprintf(fpInstructions, "Count");
	fprintf(fpFaults, "Count");
	for (i = 1; i < 19; i++) {
		fprintf(fpInstructions, ", Thread%i" ,i);
		fprintf(fpFaults, ", Thread%i", i);
	}
	fprintf(fpInstructions, "\n");
	fprintf(fpFaults, "\n");
	struct ThreadRecord* records = threadResources->records;
	initscr();
	move(0,0);
	printw("Pageanalysis LRU/2");
	move(1,0);
	printw("Copyright, (c) Adrian McMenamin, 2014");
	move(2,0);
	printw("For licence details see http://github.com/mcmenaminadrian");
	//check and update system limits
	//getrlimit(RLIMIT_AS, &limit);
	//move(3,0);
	//printw("Current limit: %llu, max limit: %llu\n", limit.rlim_cur, limit.rlim_max);
	refresh();
	do {
		pthread_mutex_lock(
			&threadResources->globals->threadGlobalLock);
		fprintf(fpInstructions, "%li",
			threadResources->globals->totalTicks);
		fprintf(fpFaults, "%li",
			threadResources->globals->totalTicks);
		while (records) {
			fprintf(fpInstructions, ", ");
			fprintf(fpFaults, ", ");
			if (records->local) {
				long instDiff =
					records->local->instructionCount -
					records->local->prevInstructionCount;
				records->local->prevInstructionCount =
					records->local->instructionCount;
				long faultDiff = records->local->faultCount -
					records->local->prevFaultCount;
				records->local->prevFaultCount =
					records->local->faultCount;
				fprintf(fpInstructions, "%li", instDiff);
				fprintf(fpFaults, "%li", faultDiff);
			}
			records = records->next;
		}
		pthread_cond_wait(&writeProgress,
			&threadResources->globals->threadGlobalLock);
		pthread_mutex_unlock(
			&threadResources->globals->threadGlobalLock);
		fprintf(fpInstructions, "\n");
		fprintf(fpFaults, "\n");
		records = threadResources->records;
		fflush(fpInstructions);
		fflush(fpFaults);
		move(THREADLINE + 1,0);
		printw("FAULTS\n");
		move(THREADLINE + 3,0);
		printw("INSTRUCTIONS\n");
		move(THREADLINE + 2,0);
		printw("%li ", threadResources->globals->totalTicks);
		while (records) {
			printw(", ");
			if (records->local) {
				printw("%li", records->local->faultCount);
			}
			records = records->next;
		}
		records = threadResources->records;
		move(THREADLINE + 4, 0);
		printw("%li ", threadResources->globals->totalTicks);
		while (records) {
			printw(", ");
			if (records->local) {
				printw("%li",
					records->local->instructionCount);
			}
			records = records->next;
		}
		records = threadResources->records;
		refresh();	 
	} while (1);
}
	
void incrementActive(void)
{
	pthread_mutex_lock(&updateLock);
	threadsActive++;
	pthread_mutex_unlock(&updateLock);
}

void decrementActive(void)
{
	pthread_mutex_lock(&updateLock);
	threadsActive--;
	pthread_mutex_unlock(&updateLock);
}

void incrementCoresInUse(struct ThreadResources* tRes)
{
	struct timespec to;
	struct timeval tp;
	pthread_mutex_lock(&coresLock);
	//ten microsecond tick over
	while (coresInUse == CORES) {
		gettimeofday(&tp, NULL);
		to.tv_sec = tp.tv_sec;
		to.tv_nsec = (tp.tv_usec + 10) * 1000;
		updateTickCount(tRes);
		pthread_cond_timedwait(&enoughCores, &coresLock, &to);
	}
	coresInUse++;
	pthread_mutex_unlock(&coresLock);
}

void decrementCoresInUse(void)
{
	pthread_mutex_lock(&coresLock);
	coresInUse--;
	pthread_cond_signal(&enoughCores);
	pthread_mutex_unlock(&coresLock);
}

struct ThreadRecord* createThreadRecord(int tNum, char* fileName)
{
	struct ThreadRecord* newRecord = malloc(sizeof (struct ThreadRecord));
	if (!newRecord) {
		fprintf(stderr, "Could not create ThreadRecord.\n");
		return NULL;
	}
	newRecord->number = tNum;
	strcpy(newRecord->path, fileName);
	newRecord->next = NULL;
	newRecord->local = NULL;
	return newRecord;
}

void mapThread(struct ThreadRecord **root, int tNum, char *fileName)
{
	if (*root) {
		struct ThreadRecord* thRecord = *root;
		while (thRecord->next) {
			thRecord = thRecord->next;
		}
		thRecord->next = createThreadRecord(tNum, fileName);
	} else {
		*root = createThreadRecord(tNum, fileName);
	}
}

void cleanThreadList(struct ThreadRecord *root)
{
	if (root == NULL)
		return;
	struct ThreadRecord *nextOne = root->next;
	free(root);
	cleanThreadList(nextOne);
}

void usage()
{
	printf("USAGE: variload controlfile\n");
}

static void XMLCALL
	starthandler(void *data, const XML_Char *name, const XML_Char **attr)
{
	int i;
	int threadID = 0;
	char threadPath[BUFFSZ]; 
	if (strcmp(name, "file") == 0) {
		for (i = 0; attr[i]; i += 2) {
			if (strcmp(attr[i], "thread") == 0) {
				threadID = atoi(attr[i + 1]);
				break;
			}
		}
		for (i = 0; attr[i]; i += 2) {
			if (strcmp(attr[i], "path") == 0) {
				strcpy(threadPath, attr[i + 1]);
				break;
			}
		}
		mapThread(&startTR, threadID, threadPath);
	} 
}

void updateTickCount(struct ThreadResources* tRes)
{
	struct ThreadLocal* local = tRes->local;
	local->tickCount++;
	if (local->tickCount - local->prevTickCount >= BARRIER) {
		pthread_mutex_lock(&updateLock);
		threadsLocked++;
		if (threadsLocked >= threadsActive) {
			tRes->globals->totalTicks += BARRIER;
			pthread_cond_broadcast(&barrierThreshold);
			threadsLocked = 0;
			if (--writeCountDown <= 0) {
				writeCountDown = SUPER;
				pthread_cond_signal(&writeProgress);
			}
		} else {
			pthread_cond_wait(&barrierThreshold, &updateLock);
		}
		pthread_mutex_unlock(&updateLock);
		local->prevTickCount = local->tickCount;
	}
}			

int startFirstThread(char* outputprefix)
{
	int errL, errG;
	struct ThreadLocal *firstThreadLocal;
	struct ThreadResources *firstThreadResources;
	
	//start the first thread
	struct ThreadGlobal* globalThreadList =
		(struct ThreadGlobal*)malloc(sizeof (struct ThreadGlobal));
	if (!globalThreadList) {
		fprintf(stderr,
			"Could not allocate memory for threadGlobal.\n");
		goto failed;
	}
	globalThreadList->totalTicks = 0;
	globalThreadList->highTree = createPageTree();
	globalThreadList->lowTree = createPageTree();
	if (!globalThreadList->highTree || !globalThreadList->lowTree) {
		fprintf(stderr,
			"Could not create page trees");
		goto failGlobalTree;
	}

	globalThreadList->maxHighSize = ((COREMEM * CORES)/(PAGESIZE)) * HIGH;
	globalThreadList->maxLowSize = (COREMEM * CORES)/(PAGESIZE) - 
		globalThreadList->maxHighSize;

	firstThreadLocal =
		(struct ThreadLocal*)malloc(sizeof(struct ThreadLocal));
	if (!firstThreadLocal) {
		fprintf(stderr,
			"Could not allocate memory for threadLocal.\n");
		goto failFirstThreadLocal;
	}
	startTR->local = firstThreadLocal;
	firstThreadLocal->instructionCount = 0;
	firstThreadLocal->prevTickCount = 0;
	firstThreadLocal->tickCount = 0;
	firstThreadLocal->faultCount = 0;
	firstThreadLocal->prevInstructionCount = 0;
	firstThreadLocal->prevFaultCount = 0;

	firstThreadLocal->threadNumber = startTR->number;

	//prepare to start the thread
	firstThreadResources =
		(struct ThreadResources*)
		malloc(sizeof (struct ThreadResources));
	if (!firstThreadResources) {
		fprintf(stderr,
			"Could not allocate memory for threadResources.\n");
		goto failResources;
	}
	firstThreadResources->records = startTR;
	firstThreadResources->globals = globalThreadList;
	firstThreadResources->local = firstThreadLocal;
	globalThreadList->head = startTR;
	errL = pthread_mutex_init(&firstThreadLocal->threadLocalLock, NULL);
	errG = pthread_mutex_init(&globalThreadList->threadGlobalLock, NULL);
	if (errL || errG) {
		fprintf(stderr,
			"Mutex initialisation fails with %i and %i\n",
			errL, errG);
		goto failMutex;
	}
	struct ThreadArray* threads = (struct ThreadArray*)
		malloc(sizeof (struct ThreadArray));
	if (!threads) {
		fprintf(stderr, "Could not initialise ThreadArray.\n");
		goto failThreads;
	}
	threads->threadNumber = 0;
	threads->nextThread = NULL;
	globalThreadList->threads = threads;
	createRecordsTree(firstThreadResources);
	pthread_create(&dataThread, NULL, writeDataThread, (void*)firstThreadResources);
	pthread_create(&threads->aPThread, NULL, startThreadHandler,
		(void *)firstThreadResources);
	pthread_join(threads->aPThread, NULL);
	return 0;

failThreads:
failMutex:
	free(firstThreadResources);
failResources:
	free(firstThreadLocal);
failFirstThreadLocal:
	removePageTree(globalThreadList->highTree);
	removePageTree(globalThreadList->lowTree);
failGlobalTree:
	free(globalThreadList);	
failed:
	free(startTR);
	return -1;
}



int main(int argc, char* argv[])
{
	FILE* inXML;
	char *fileIn = NULL;
	char *fileOut = NULL;
	char data[BUFFSZ]; 
	size_t len = 0;
	int done;
	int i;
	writeCountDown = SUPER;	

	if (argc < 2) {
		usage();
		exit(-1);
	}

	while ((i = getopt(argc, argv, "f:l:c:p:t:s:i:o:")) != -1)
		switch(i) {
		case 'f':
			tickFind = atoi(optarg);
			break;
		case 'l':
			tickLoad = atoi(optarg);
			break;
		case 'c':
			cores = atoi(optarg);
			break;
		case 's':
			pageShift = atoi(optarg);
			break;
		case 'p':
			shiftMemoryPerCore = atoi(optarg);
			break;
		case 't':
			shiftPageTablePerCore = atoi(optarg);
			break;
		case 'i':
			fileIn = optarg;
			break;
		case 'o':
			fileOut = optarg;
			break;
	}

	XML_Parser p_ctrl = XML_ParserCreate("UTF-8");
	if (!p_ctrl) {
		fprintf(stderr, "Could not create parser\n");
		exit(-1);
	}

	XML_SetStartElementHandler(p_ctrl, starthandler);
	inXML = fopen(fileIn, "r");
	if (inXML == NULL) {
		fprintf(stderr, "Could not open %s\n", fileIn);
		XML_ParserFree(p_ctrl);
		exit(-1);
	}

	do {
		len = fread(data, 1, sizeof(data), inXML);
		done = len < sizeof(data);

		if (XML_Parse(p_ctrl, data, len, 0) == 0) {
			enum XML_Error errcde = XML_GetErrorCode(p_ctrl);
			printf("ERROR: %s\n", XML_ErrorString(errcde));
			printf("Error at column number %lu\n",
				XML_GetCurrentColumnNumber(p_ctrl));
			printf("Error at line number %lu\n",
				XML_GetCurrentLineNumber(p_ctrl));
			exit(-1);
		}
	} while(!done);

	XML_ParserFree(p_ctrl);
	fclose(inXML);
	if (startFirstThread(outputprefix) < 0) {
		printf("ERROR: thread parsing failed.\n");
		exit(-1);
	}
	cleanThreadList(startTR);
	return 0;
}
