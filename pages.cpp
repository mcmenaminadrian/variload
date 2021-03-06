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
	long pageNumber;

	public:
	PartialPage(const long pNumber, const long bitLength,
		long t): pageNumber(pNumber) {
		time = t;
		bitmap = boost::dynamic_bitset<>(bitLength);
	}
	PartialPage& operator=(PartialPage& pp);
	const bool getBitmap(const unsigned long sequence) const;
	const bool setBitmap(const unsigned long sequence);
	const boost::dynamic_bitset<> readBitmap() const;
	void copyBitmap(const boost::dynamic_bitset<> bitmapIn);
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

const boost::dynamic_bitset<> PartialPage::readBitmap() const
{
	return bitmap;
}

void PartialPage::copyBitmap(const boost::dynamic_bitset<> bitmapIn)
{
	bitmap = bitmapIn;
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
	void insertOldPage(const long pageNumber, const long timeIn,
		boost::dynamic_bitset<> bitIn);
	const bool offsetPresent(PartialPage& pPage, const long offset) const;
	pair<bool, PartialPage&> locatePage(const long pageNumber);
	pair<bool, long> removePage(const long pageNumber);
	PartialPage* oldestPage();
	const long treeSize() const { return pageTree.size();}
	void insert(pair<long, PartialPage> inPair)
	{
		pair<map<long, PartialPage>::iterator, bool> res =
			pageTree.insert(inPair);
		if (res.second == false) {
			cerr << "********* FALSE **********" << endl;
		}
	}
};

long DoubleTree::getUnixTimeChrono() const
{
	auto timeSinceEpoch = chrono::system_clock::now().time_since_epoch();
	return chrono::duration_cast<chrono::microseconds>
		(timeSinceEpoch).count();
}

void DoubleTree::insertNewPage(const long pageNumber)
{
	pair<long, PartialPage> goIn(pageNumber, PartialPage(pageNumber,
		PAGESIZE_ >> 4, getUnixTimeChrono())); 
	pair<map<long, PartialPage>::iterator, bool> inPage = 
		pageTree.insert(goIn);
	if (inPage.second == false) {
		cerr << "******* FAILURE ********" << endl;
	}
}

void DoubleTree::insertOldPage(const long pageNumber, const long timeIn,
	boost::dynamic_bitset<> bitIn)
{
	PartialPage pageIn(pageNumber, PAGESIZE_ >> 4, timeIn);
	pageIn.copyBitmap(bitIn);
	pair<long, PartialPage> goIn(pageNumber, pageIn); 
	pageTree.insert(goIn);
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
		cerr << "ERROR: page does not exist in tree: " << pageNumber;
		cerr << "\n";
		return pair<bool, long>(false, -1);
	}
	pageTree.erase(itPage);
	return pair<bool, long>(true, pageNumber);
}

PartialPage* DoubleTree::oldestPage()
{
	PartialPage* pageToKill = &(pageTree.begin()->second);
	long timeToKill = pageTree.begin()->second.getTime();
	map<long, PartialPage>::iterator itOld;
	//cerr << "Page " << pageToKill->getPageNumber() << " with time " << timeToKill << endl;
	for (itOld = pageTree.begin(); itOld != pageTree.end(); itOld++) {
	//	cerr << itOld->first << " : " << itOld->second.getTime() << ",";
		if (itOld->second.getTime() < timeToKill) {
			timeToKill = itOld->second.getTime();
			pageToKill = &(itOld->second);
	//		cerr << endl << "Replaced with " << pageToKill->getPageNumber() << " with time " << timeToKill << endl;
		}
	}
	//cerr << "Tree has " << pageTree.size() << " elements and we picked page " << pageToKill->getPageNumber() << endl;
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

void pushPageHigh(long pageNumber, void *lowTree, void *highTree)
{
	DoubleTree *lTree, *hTree;
	lTree = static_cast<DoubleTree *>(lowTree);
	hTree = static_cast<DoubleTree *>(highTree);
	pair<bool, PartialPage&> finding = lTree->locatePage(pageNumber);
	if (!finding.first) {
		cerr << "Could not locate page " << pageNumber << "\n";
		return;
	}
//	cerr << "Time was " << finding.second.getTime();
	finding.second.setTime(hTree->getUnixTimeChrono());
//	cerr << " and set to " << finding.second.getTime();
	pair<long, PartialPage> pageIn(pageNumber, finding.second);
	hTree->insert(pageIn);
	//cerr << " and in the tree it has time " << hTree->locatePage(pageNumber).second.getTime() << " *** " << endl;
	//cerr << "Pushed up page " << pageNumber << endl;
	lTree->removePage(pageNumber);
}

void swapOldestPageToLow(struct ThreadResources *thResources)
{
	DoubleTree *hTree, *lTree;
	hTree = static_cast<DoubleTree *>(thResources->globals->highTree);
	lTree = static_cast<DoubleTree *>(thResources->globals->lowTree);
	PartialPage *oldPage = hTree->oldestPage();
	lTree->insert(pair<long, PartialPage>
		(oldPage->getPageNumber(), *oldPage));
	hTree->removePage(oldPage->getPageNumber());
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
	if (finding.first && finding.second.getBitmap(segment)) {
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
	if (!finding.first) {
		cerr << "Error - markSegmentPresent " << pageNumber << "\n";
		return;
	} 
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
	long oldPage = (prTree->oldestPage())->getPageNumber();
	return prTree->removePage(oldPage).second;
}

}// end extern "C"		
