//C header file for C++ functions
//Copyright Adrian McMenamin, 2014
//Licensed for reuse under the GPL

#ifndef __PAGES_H_
#define __PAGES_H_
int locateSegment(long pageNumber, long segment, void *tree);
void markSegmentPresent(long pageNumber, long segment, void *tree);
void swapOldestPageToLow(struct ThreadResources *thResources);
void updateTree(long pageNumber, void *tree);
void insertNewIntoPageTree(long pageNumber, void *tree);
void pushPageHigh(long pageNumber, void *lowTree, void *highTree);
void* createPageTree(void);
void removePageTree(void* tree);
void* locatePageTreePR(long pageNumber, void* tree);
int countPageTree(void* tree);
long removeOldestPage(void* tree);
#endif
