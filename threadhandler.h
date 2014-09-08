#ifndef __THREAD_HANDLER_H_
#define __THREAD_HANDLER_H_

#define BUFFSZ 512
#define BITSHIFT 11
#define CORES 16
#define COREMEM 28672
#define HIGH 0.67
#define MEMWIDTH 16
#define PAGESIZE_ (1 << BITSHIFT) 
#define MAXTHREADS 18
#define BITLENGTH (COREMEM >> 4)
#define COUNTDOWN 100
#define TICKFIND 4

struct ThreadLocal;


struct PageToKill
{
	long pageNumbers[MAXTHREADS];
	long instructionCounts[MAXTHREADS];
};

struct ThreadArray
{
	int threadNumber;
	pthread_t aPThread;
	struct ThreadArray* nextThread;
};

struct ThreadRecord
{
	int number;
	char path[BUFFSZ];
	struct ThreadLocal *local;
	struct ThreadRecord *next;
};

struct ThreadLocal
{
	int threadNumber;
	long instructionCount;
	long prevInstructionCount;
	long faultCount;
	long prevFaultCount;
	long tickCount;
	long prevTickCount;
	int dead;
	pthread_mutex_t threadLocalLock;
	//page analysis data
	unsigned long anPage;
	unsigned long anDestination;
	unsigned long anSize;
	char anType;
};

struct ThreadGlobal
{
	long totalTicks;
	struct ThreadRecord* head;
	void* highTree;
	void* lowTree;
	int maxHighSize;
	int maxLowSize;
	struct ThreadArray *threads;
	void* activePages;
	const int totalCores;
	const int usableMemoryPerCore;
	const int waitingTicks;
	const int loadingTicks;
	pthread_mutex_t threadGlobalLock;
};

struct ThreadResources
{
	struct ThreadRecord *records;
	struct ThreadGlobal *globals;
	struct ThreadLocal *local;
	void *activePages;
};

struct PageChain {
	long page;
	struct PageChain *next;
};


void* startThreadHandler(void *resources);
void incrementActive(void);
void decrementActive(void);
void updateTickCount(struct ThreadResources *tRes);

void incrementCoresInUse(struct ThreadResources *tRes);
void decrementCoresInUse(void);

#endif

