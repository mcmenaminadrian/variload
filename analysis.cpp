#define __REENTRANT

#include <iostream>
#include <map>
#include "threadhandler.h"

using namespace std;

struct activePage {
	unsigned long tickIn;
	unsigned long tickOut;
	map<unsigned long, unsigned int> mReferences;
	map<unsigned long, unsigned int> cReferences;
};


extern "C" {

insertRecord(struct ThreadResources* thResources)
{
	struct ThreadLocal* local = thResources->local;
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct activePage>* activePages =
		static_cast<map<long, struct activePage*> >
		(globals->activePages);

	map<unsigned long, struct activePage*>::iterator it;
	it = activePages->find(local->anPage);
	if (it == activePages->end()) {
		struct activePage* nextPage = new activePage();
		nextPage->tickIn = globals->totalTicks;
		nextPage->tickOut = 0;
		activePages->insert(pair<unsigned long, struct activePage*>
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
}
	
	
