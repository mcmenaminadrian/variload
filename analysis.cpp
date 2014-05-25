#define __REENTRANT

#include <iostream>
#include <map>
#include "threadhandler.h"

using namespace std;

struct ActivePage {
	unsigned long tickIn;
	unsigned long tickOut;
	map<unsigned long, unsigned int> mReferences;
	map<unsigned long, unsigned int> cReferences;
};


extern "C" {

void insertRecord(struct ThreadResources* thResources)
{
	struct ThreadLocal* local = thResources->local;
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(globals->activePages);

	map<unsigned long, struct activePage*>::iterator it;
	pthread_mutex_lock(&globals->threadGlobalLock);
	it = activePages->find(local->anPage);
	if (it == activePages->end()) {
		struct ActivePage* nextPage = new ActivePage();
		nextPage->tickIn = globals->totalTicks;
		nextPage->tickOut = 0;
		activePages->insert(pair<unsigned long, struct ActivePage*>
			(local->anPage, nextPage));
		it = activePages->find(local->anPage);
	}
	if (local->anType == 'c') {
		it->cReferences.insert(pair<unsigned long, unsigned int>
			(local->anDestination, local->anCount));
	} else {
		it->mReferences.insert(pair<unsigned long, unsigned int>
			(local->anDestination, local->anCount));
	}
	pthread_mutex_unlock(&globals->threadGlobalLock);
}

//called with lock on
void doneWithRecord(long page, struct ThreadResources* thResources)
{
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(globals->activePages);
	it = activePages->find(page);
	if (it == activePages->end()) {
		//failed
	}
	//write out record

	//remove record
}

	

void createRecordsTree(struct ThreadResources* thResources)
{
	thResources->globals->activePages = (void*)
		(new map<unsigned long, struct ActivePage*>());
}

void removeRecordsTree(struct ThreadResources* thResources)
{
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*> activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(thResources->globals->activePages);
	for (it = activePages->begin(); it != activePages->end(); it++) {
		delete it->second;
	}
}
