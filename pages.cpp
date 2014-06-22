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
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	delete prTree;
}

void insertIntoPageTree(long pageNumber, void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	prTree->insertPage(pageNumber);
}

long locatePageTreePR(long pageNumber, void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	if (prTree->locatePage(pageNumber) {
		return 1;
	} else {
		return 0;
	}
}

void removeFromPageTree(long pageNumber, void* tree)
{
	set<long>* prTree = static_cast<set<long> *>(tree);
	prTree->removePage(pageNumber);
}

int countPageTree(void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	return prTree->treeSize();
}

void removeOldestPage(void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	prTree->removePage(prTree->oldestPage());
}

}// end extern "C"		
