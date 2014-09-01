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
#include <boost/dynamic_bitset.hpp>
#include "threadhandler.h"

using namespace std;

class PartialPage
{
	private:
	boost::dynamic_bitset bitmap;
	long time;

	public:
	PartialPage(const long bitLength, const long t):time(t);
	const bool getBitmap(const long sequence) const;
	const bool setBitmap(const long sequence);
}

PartialPage::PartialPage(const long bitlength, const long t):time(t)
{
	bitmap(bitlength);
}

const bool PartialPage::getBitmap(const long sequence) const
{
	if (sequence > bitmap.size()) {
		return false;
	}
	return (bitmap[sequence] == 1);
}

const bool PartialPage::setBitmap(const long sequence)
{
	if (sequence > bitmap.size()) {
		return false;
	}
	bitmap[sequence] = 1;
	return true;
}

class DoubleTree
{
	private:
	map<long, long> pageTree;
	long getUnixTimeChrono() const;

	public:
	void insertPage(const long pageNumber);
	bool locatePage(const long pageNumber) const;
	long removePage(const long pageNumber);
	long oldestPage();
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
	map<long, long>::iterator itPage = pageTree.find(pageNumber);
	if (itPage == pageTree.end())
	{
		cout << "ERROR: page does not exist in tree: " << pageNumber;
		cout << "\n";
		return -1;
	}
	pageTree.erase(pageNumber);
	return pageNumber;
}

long DoubleTree::oldestPage()
{
	long pageToKill = pageTree.begin()->first;
	long timeToKill = pageTree.begin()->second;
	for (map<long, long>::iterator itOld = pageTree.begin();
		itOld != pageTree.end(); itOld++) {
		if (itOld->second < timeToKill) {
			timeToKill = itOld->second;
			pageToKill = itOld->first;
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
