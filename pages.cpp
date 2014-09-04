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
	const long pageNumber;

	public:
	PartialPage(const long pNumber, const long bitLength, long t):
		time(t), pageNumber(pNumber);
	const bool getBitmap(const long sequence) const;
	const bool setBitmap(const long sequence);
	const long getPageNumber() const;
	void setTime(long timeIn){time = timeIn;}
	long getTime() const {return time;}
}

PartialPage::PartialPage(const long pNumber, const long bitlength, long t):
	time(t), pageNumber(pNumber)
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

const long PartialPage::getPageNumber() const
{
	return pageNumber;
}

class DoubleTree
{
	private:
	map<long, PartialPage> pageTree;
	long getUnixTimeChrono() const;

	public:
	void insertNewPage(const long pageNumber, const long offset);
	void insertOldPage(PartialPage& oldPage);
	const bool offsetPresent(PartialPage& pPage, const long offset) const;
	pair<bool, PartialPage&> locatePage(const long pageNumber) const;
	pair<bool, PartialPage&> removePage(const long pageNumber);
	PartialPage& oldestPage() const;
	const long treeSize() const { return pageTree.size();}
};

long DoubleTree::getUnixTimeChrono() const
{
    auto timeSinceEpoch = chrono::system_clock::now().time_since_epoch();
    return chrono::duration_cast<chrono::microseconds>(timeSinceEpoch).count();
}

void DoubleTree::insertNewPage(const long pageNumber, const long offset)
{
	long insertTime = getUnixTimeChrono();
	PartialPage inPage(pageNumber, BITLENGTH, insertTime);
	inPage.setBitmap(offset >> 4);
	pageTree.insert(pair<long, PartialPage>(pageNumber, inPage));
}

void DoubleTree::insertOldPage(PartialPage& oldPage)
{
	long insertTime = getUnixTimeChrono();
	oldPage.setTime(insertTime);
	pageTree.insert(oldPage.getPageNumber(), oldPage);
}

pair<bool, PartialPage&> DoubleTree::locatePage(const long pageNumber) const
{
	map<long, PartialPage>::iterator it;
	it = pageTree.find(pageNumber);
	if (pageTree.find(pageNumber) == pageTree.end())
	{
		return pair<false, PartialPage(0, 0, 0)>;
	} else {
		return pair<true, it->second>;
	}
}

pair<bool, PartialPage&> DoubleTree::removePage(const long pageNumber)
{
	map<long, long>::iterator itPage = pageTree.find(pageNumber);
	if (itPage == pageTree.end())
	{
		cout << "ERROR: page does not exist in tree: " << pageNumber;
		cout << "\n";
		return pair<false, PartialPage(0, 0, 0)>;
	}
	pageTree.erase(pageNumber);
	return pair<true, it->second>;
}

long DoubleTree::oldestPage()
{
	long pageToKill = pageTree.begin()->first;
	long timeToKill = pageTree.begin()->second.getTime();
	for (map<long, PartialPage>::iterator itOld = pageTree.begin();
		itOld != pageTree.end(); itOld++) {
		if (itOld->second.getTime() < timeToKill) {
			timeToKill = itOld->second.getTime();
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

void insertNewIntoPageTree(long pageNumber, long offset, void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	prTree->insertNewPage(pageNumber, offset);
}

void insert

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
