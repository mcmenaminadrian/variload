//C header file for C++ functions
//Copyright Adrian McMenamin, 2014
//Licensed for reuse under the GPL

#ifndef __PAGES_H_
#define __PAGES_H_


void* createPageTree(void);
void removePageTree(void* tree);
void insertIntoPageTree(void *page, void *tree);
void* locatePageTreePR(long pageNumber, void* tree);
void removeFromPageTree(long pageNumber, void* tree);
int countPageTree(void* tree);
long removeOldestPage(void* tree);
#endif
