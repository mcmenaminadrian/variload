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
	boost::dynamic_bitset<> bitmap;
	long time;
	const long pageNumber;

	public:
	PartialPage(const long pNumber, const long bitLength,
		long t): pageNumber(pNumber) {
		time = t;
		bitmap = boost::dynamic_bitset<>(bitLength);
	}
	PartialPage& operator=(PartialPage& pp);
	const bool getBitmap(const unsigned long sequence) const;
	const bool setBitmap(const unsigned long sequence);
	const long getPageNumber() const;
	void setTime(const long timeIn){time = timeIn;}
	const long getTime() const {return time;}
};

PartialPage& PartialPage::operator=(PartialPage& pp)
{
	PartialPage& retPP(pp);
	return retPP;
}

const bool PartialPage::getBitmap(const unsigned long sequence) const
{
	if (sequence > bitmap.size()) {
		return false;
	}
	return (bitmap[sequence] == 1);
}

const bool PartialPage::setBitmap(const unsigned long sequence)
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

	public:
	long getUnixTimeChrono() const;
	void insertNewPage(const long pageNumber);
	void insertOldPage(const long pageNumber);
	const bool offsetPresent(PartialPage& pPage, const long offset) const;
	pair<bool, PartialPage&> locatePage(const long pageNumber);
	pair<bool, long> removePage(const long pageNumber);
	PartialPage& oldestPage();
	const long treeSize() const { return pageTree.size();}
};

long DoubleTree::getUnixTimeChrono() const
{
    auto timeSinceEpoch = chrono::system_clock::now().time_since_epoch();
    return chrono::duration_cast<chrono::microseconds>(timeSinceEpoch).count();
}

void DoubleTree::insertNewPage(const long pageNumber)
{
	pair<long, PartialPage> goIn(pageNumber,PartialPage(pageNumber,
		PAGESIZE_ >> 4, getUnixTimeChrono())); 
	pageTree.insert(goIn);
}

void DoubleTree::insertOldPage(const long pageNumber)
{
	insertNewPage(pageNumber);
}

pair<bool, PartialPage&> DoubleTree::locatePage(const long pageNumber)
{
	map<long, PartialPage>::iterator it;
	it = pageTree.find(pageNumber);
	if (pageTree.find(pageNumber) == pageTree.end())
	{
		PartialPage pp(0, 0, 0);
		return pair<bool, PartialPage&>(false, pp);
	} else {
		return pair<bool, PartialPage&>(true, it->second);
	}
}

pair<bool, long> DoubleTree::removePage(const long pageNumber)
{
	map<long, PartialPage>::iterator itPage = pageTree.find(pageNumber);
	if (itPage == pageTree.end())
	{
		cout << "ERROR: page does not exist in tree: " << pageNumber;
		cout << "\n";
		return pair<bool, long>(false, -1);
	}
	pageTree.erase(itPage);
	return pair<bool, long>(true, pageNumber);
}

PartialPage& DoubleTree::oldestPage()
{
	PartialPage& pageToKill = pageTree.begin()->second;
	long timeToKill = pageTree.begin()->second.getTime();
	map<long, PartialPage>::iterator itOld;
	for (itOld = pageTree.begin(); itOld != pageTree.end(); itOld++) {
		if (itOld->second.getTime() < timeToKill) {
			timeToKill = itOld->second.getTime();
			pageToKill = itOld->second;
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

void insertNewIntoPageTree(long pageNumber, void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	//insert page with empty bitmap
	prTree->insertNewPage(pageNumber);
}

void insertOldIntoPageTree(long pageNumber, void* oldTree, void* newTree)
{
	DoubleTree *destTree, *srcTree;
	destTree = static_cast<DoubleTree *>(newTree);
	srcTree = static_cast<DoubleTree *>(oldTree);
	pair<bool, long> result = srcTree->removePage(pageNumber);
	if (result.first == true) {
		destTree->insertOldPage(result.second);
		return;
	} else {
		fprintf(stderr, "Could not swap page to new tree \n");
	}
}

void swapOldestPageToLow(struct ThreadResources *thResources)
{
	DoubleTree *hTree, *lTree;
	hTree = static_cast<DoubleTree *>(thResources->globals->highTree);
	lTree = static_cast<DoubleTree *>(thResources->globals->lowTree);
	PartialPage oldPage = hTree->oldestPage();
	lTree->insertOldPage(oldPage.getPageNumber());
	hTree->removePage(oldPage.getPageNumber());
}

long locatePageTreePR(long pageNumber, void* tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	if (prTree->locatePage(pageNumber).first) {
		return 1;
	} else {
		return 0;
	}
}

int locateSegment(long pageNumber, long segment, void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	pair<bool, PartialPage&> finding = prTree->locatePage(pageNumber);
	if (finding.second.getBitmap(segment)) {
		return 1;
	} else {
		return 0;
	}
}

void markSegmentPresent(long pageNumber, long segment, void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	pair<bool, PartialPage&> finding = prTree->locatePage(pageNumber);
	finding.second.setBitmap(segment);
	finding.second.setTime(prTree->getUnixTimeChrono());
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

void updateTree(long pageNumber, void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	pair<bool, PartialPage&> finding = prTree->locatePage(pageNumber);
	if (finding.first) {
		finding.second.setTime(prTree->getUnixTimeChrono());
		return;
	} else {
		fprintf(stderr, "Tried update time on non existent page\n");
		return;
	}
}

long removeOldestPage(void *tree)
{
	DoubleTree *prTree;
	prTree = static_cast<DoubleTree *>(tree);
	long oldPage = (prTree->oldestPage()).getPageNumber();
	return prTree->removePage(oldPage).second;
}

}// end extern "C"		
