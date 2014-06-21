#define __REENTRANT
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <pthread.h>
#include <sched.h>
#include <chrono>
#include "threadhandler.h"

using namespace std;

class DoubleTree
{
	private:
	map<long, long> pageTree;
	multimap<long, long> tickTree;
	long getUnixTimeChrono() const;

	public:
	void insertPage(const long pageNumber);
	bool locatePage(const long pageNumber) const;
	void removePage(const long pageNumber);
	long oldestPage() const;
	long treeSize() const { return pageTree.size();}
};

long DoubleTree::getUnixTimeChrono() const
{
    auto timeSinceEpoch = chrono::system_clock::now().time_since_epoch();
    return chrono::duration_cast<chrono::milliseconds>(timeSinceEpoch).count();
}

void DoubleTree::insertPage(const long pageNumber)
{
	long insertTime = getUnixTimeChrono();
	pageTree.insert(pair<long, long>(pageNumber, insertTime));
	tickTree.insert(pair<long, long>(insertTime, pageNumber));
}

bool DoubleTree::locatePage(const long PageNumber) const
{
	if (pageTree.find(pageNumber) == pageTree.end())
	{
		return false;
	} else {
		return true;
	}
}

void DoubleTree::removePage(const long pageNumber)
{
	map<long, long>::iterator itPage;
	pair<multimap<long, long>::iterator,
		multimap<long, long>::iterator> itTick;

	itPage = pageTree.find(pageNumber);
	if (itPage = pageTree.end() )
	{
		cout << "ERROR: page does not exist in tree: " << pageNumber;
		cout << "\n";
		return;
	}
	long timeToGo = itPage->second;
	itTick = tickTree.equal_range(timeToGo);
	pageTree.erase(itPage);
	for (multimap<long, long>::iterator it = itTick.first;
		it != itTick.second; it++)
	{
		if (it->second == pageNumber) {
			itTick.erase(it);
			break;
		}
	}
}

long DoubleTree::oldestPage() const
{
	return tickTree.begin()->second;
}


extern "C" {

long findNextInstruction(unsigned long cI, long pN, void* tree);
void insertIntoTree(long, long, void*);


void* createPageTree(void)
{
	return static_cast<void*>(new DoubleTree());
}

void removePageTree(void* tree)
{
	map<chrono::milliseconds, long>* prTree;
	prTree = static_cast<DoubleTree *>(tree);
	delete prTree;
}

void insertIntoPageTree(long pageNumber, void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	long insert
	prTree->insert(pair<long, long>(
		pageNumber,
		getUnixTimeChrono());
}

long locatePageTreePR(long pageNumber, void* tree)
{
	set<long>* prTree;
	prTree = static_cast<map<long, long> *>(tree);
	set<long>::iterator it;
	it = prTree->find(pageNumber);
	if (it != prTree->end()) {
		return *it;
	} else {
		return 0;
	}
}

void removeFromPageTree(long pageNumber, void* tree)
{
	set<long>* prTree = static_cast<set<long> *>(tree);
	prTree->erase(pageNumber);
}

int countPageTree(void* tree)
{
	set<long> *prTree;
	prTree = static_cast<set<long> *>(tree);
	return prTree->size();
}

void
fillInstructionTree(void* global, void* iTree, void* oTree, long instruction)
{
	set<long>* prTree;
	prTree = static_cast<set<long> *>(global);
	set<long>::iterator it;
	for (it = prTree->begin(); it != prTree->end(); it++) {
		long pageNumber = *it;
		long nextInstruction = findNextInstruction(instruction,
			pageNumber, oTree);
		insertIntoTree(pageNumber, nextInstruction, iTree);
	}
}

}// end extern "C"		
