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
	long getUnixTimeChrono() const;

	public:
	void insertPage(const long pageNumber);
	bool locatePage(const long pageNumber) const;
	long removePage(const long pageNumber);
	long oldestPage() const;
	long treeSize() const { return pageTree.size();}
};

long DoubleTree::getUnixTimeChrono() const
{
    auto timeSinceEpoch = chrono::system_clock::now().time_since_epoch();
    return chrono::duration_cast<chrono::microseconds>(timeSinceEpoch).count();
}

void DoubleTree::insertPage(const long pageNumber)
{
	long insertTime = getUnixTimeChrono();
	pageTree.insert(pair<long, long>(pageNumber, insertTime));
}

bool DoubleTree::locatePage(const long pageNumber) const
{
	if (pageTree.find(pageNumber) == pageTree.end())
	{
		return false;
	} else {
		return true;
	}
}

long DoubleTree::removePage(const long pageNumber)
{
	itPage = pageTree.find(pageNumber);
	if (itPage == pageTree.end())
	{
		cout << "ERROR: page does not exist in tree: " << pageNumber;
		cout << "\n";
		return -1;
	}
	pageTree.erase(pageNumber);
	return pageNumber;
}

long DoubleTree::oldestPage() const
{
	map<long, long>::iterator it;
	long pageToKill = pageTree.begin()->first;
	long timeToKill = pageTree.begin()->second;
	for (it = pageTree.begin(); it != pageTree.end(); it++) {
		if (it->second < timeToKill) {
			timeToKill = it->second;
			pageToKill = it->first;
		}
	}	
	return pageToKill;
}

extern "C" {

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
	if (prTree->locatePage(pageNumber)) {
		return 1;
	} else {
		return 0;
	}
}

void removeFromPageTree(long pageNumber, void* tree)
{
	DoubleTree *prTree = static_cast<DoubleTree *>(tree);
	prTree->removePage(pageNumber);
}

int countPageTree(void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	return prTree->treeSize();
}

long removeOldestPage(void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	long oldPage = prTree->oldestPage();
	return prTree->removePage(oldPage);
}

}// end extern "C"		
