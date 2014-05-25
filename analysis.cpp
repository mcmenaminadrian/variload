#define __REENTRANT

#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include "threadhandler.h"

using namespace std;

struct ActivePage {
	unsigned long tickIn;
	unsigned long tickOut;
	map<unsigned long, unsigned int> mReferences;
	map<unsigned long, unsigned int> cReferences;
};


static ofstream xmlAnalysisFile;

void writeLongToFile(ofstream& inFile, long& value)
{
	stringstream stringy;
	stringy << value;
	inFile << stringy.rdbuf();
}

void writeIntToFile(ofstream& inFile, int& value)
{
	stringstream stringy;
	stringy << value;
	inFile << stringy.redbuf();
}

ofstream& openXMLAnalysisFile()
{
	xmlAnalysisFile.open("pageanalysis.xml");

	xmlAnalysis << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	xmlAnalysis << "<!DOCTYPE pageanalysis [\n";
	xmlAnalysis << "<!ELEMENT pageanalysis (page*)>\n";
	xmlAnalysis <<
		"<!ATTLIST pageanalysis version CDATA #FIXED \"0.1\">\n";
	xmlAnalysis << "<!ATTLIST pageanalysis xmlns CDATA #FIXED";
	xmlAnalysis << " \"http://cartesianproduct.wordpress.com\">\n";
	xmlAnalysis << "<!ELEMENT page (code*, rw*)>\n";
	xmlAnalysis << "<!ATTLIST page in CDATA #REQUIRED>\n";
	xmlAnalysis << "<!ATTLIST page out CDATA #REQUIRED>\n";
	xmlAnalysis << "<!ELEMENT code EMPTY>\n";
	xmlAnalysis << "<!ATTLIST code address CDATA #REQUIRED>\n";
	xmlAnalysis << "<!ATTLIST code size CDATA #REQUIRED>\n";
	xmlAnalysis << "<!ELEMENT rw EMPTY>\n";
	xmlAnalysis << "<!ATTLIST rw address CDATA #REQUIRED>\n";
	xmlAnalysis << "<!ATTLIST rw size CDATA #REQUIRED>\n";
	xmlAnalysis << "]>\n";
	xmlAnalysis << "<pageanalysis ";
	xmlAnalysis << "xmlns=\"http://cartesianproduct.wordpress.com\">\n";

	return xmlAnalysis;
}

extern "C" {

void insertRecord(struct ThreadResources* thResources)
{
	struct ThreadLocal* local = thResources->local;
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(globals->activePages);

	map<unsigned long, struct activePage*>::iterator it;
	pthread_mutex_lock(&globals->threadGlobalLock);
	it = activePages->find(local->anPage);
	if (it == activePages->end()) {
		struct ActivePage* nextPage = new ActivePage();
		nextPage->tickIn = globals->totalTicks;
		nextPage->tickOut = 0;
		activePages->insert(pair<unsigned long, struct ActivePage*>
			(local->anPage, nextPage));
		it = activePages->find(local->anPage);
	}
	if (local->anType == 'c') {
		it->cReferences.insert(pair<unsigned long, unsigned int>
			(local->anDestination, local->anCount));
	} else {
		it->mReferences.insert(pair<unsigned long, unsigned int>
			(local->anDestination, local->anCount));
	}
	pthread_mutex_unlock(&globals->threadGlobalLock);
}

//called with lock on
void doneWithRecord(long page, struct ThreadResources* thResources)
{
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(globals->activePages);
	it = activePages->find(page);
	if (it == activePages->end()) {
		//failed
	}
	//write out record
	xmlAnalysisFile << "<page in=\"";
	writeLongToFile(xmlAnalysisFile, it->tickIn);
	xmlAnalysisFile << "\" out=\"";
	writeLongToFile(xmlAnalysisFile, globals->totalTicks);
	xmlAnalysisFile << "\" >\n";
	map<unsigned long, unsigned int>::iterator recIT;
	for (recIT = it->cReferences.begin(); recIt != it->cReferences.end();
		recIt++) {
		xmlAnalysisFile << "<code address=\"";
		writeLongToFile(xmlAnalysisFile, recIt->first);
		xmlAnalysisFile << "\" size=\"";
		writeIntToFile(xmlAnalysisFile, recIt->second);
		xmlAnalysisFile << "\" />\n";
	}
	for (recIT = it->mReferences.begin(); recIt != it->mReferences.end();
		recIt++) {
		xmlAnalysisFile << "<rw address=\"";
		writeLongToFile(xmlAnalysisFile, recIt->first);
		xmlAnalysisFile << "\" size=\"";
		writeIntToFile(xmlAnalysisFile, recIt->second);
		xmlAnalysisFile << "\" />\n";
	}
	xmlAnalysisFile << "</page>\n";
	//remove record
	activePages->erase(it);
}

void createRecordsTree(struct ThreadResources* thResources)
{
	thResources->globals->activePages = (void*)
		(new map<unsigned long, struct ActivePage*>());
	xmlAnalysisFile = openXMLAnalysisFile();
}

void removeRecordsTree(struct ThreadResources* thResources)
{
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*> activePages =
		static_cast<map<unsigned long, struct ActivePage*> >
		(thResources->globals->activePages);
	for (it = activePages->begin(); it != activePages->end(); it++) {
		delete it->second;
	}
	xmlAnalysisFile << "</pageanalysis>\n";
	xmlAnalysisFile.close();
}
