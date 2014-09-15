#ifndef __ANALYSIS_H_
#define __ANALYSIS_H_

void insertRecord(struct ThreadResources* thResources);
void doneWithRecord(long page, struct ThreadResources* thResources);
void createRecordsTree(struct ThreadResources* thResources, char *fileOut);
void removeRecordsTree(struct ThreadResources* thResources);

#endif
