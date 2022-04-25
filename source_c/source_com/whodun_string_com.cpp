#include "whodun_string.h"

#include <string.h>
#include <algorithm>

using namespace whodun;

bool whodun::operator < (const SizePtrString& strA, const SizePtrString& strB){
	uintptr_t compTo = std::min(strA.len, strB.len);
	int compV = memcmp(strA.txt, strB.txt, compTo);
	if(compV){ return compV < 0; }
	return (strA.len < strB.len);
}
bool whodun::operator > (const SizePtrString& strA, const SizePtrString& strB){
	return strB < strA;
}
bool whodun::operator <= (const SizePtrString& strA, const SizePtrString& strB){
	uintptr_t compTo = std::min(strA.len, strB.len);
	int compV = memcmp(strA.txt, strB.txt, compTo);
	if(compV){ return compV < 0; }
	return (strA.len <= strB.len);
}
bool whodun::operator >= (const SizePtrString& strA, const SizePtrString& strB){
	return strB <= strA;
}
bool whodun::operator == (const SizePtrString& strA, const SizePtrString& strB){
	if(strA.len != strB.len){ return false; }
	return memcmp(strA.txt, strB.txt, strA.len) == 0;
}
bool whodun::operator != (const SizePtrString& strA, const SizePtrString& strB){
	return !(strA == strB);
}

SizePtrString whodun::trim(SizePtrString toTrim){
	SizePtrString toRet = toTrim;
	#define IS_COMMON_WS(forC) (forC == ' ' || forC=='\t' || forC=='\n' || forC == '\r')
	while(toRet.len){
		char curC = *(toRet.txt);
		if(! IS_COMMON_WS(curC) ){ break; }
		toRet.txt++;
		toRet.len--;
	}
	while(toRet.len){
		char curC = toRet.txt[toRet.len-1];
		if(! IS_COMMON_WS(curC) ){ break; }
		toRet.len--;
	}
	return toRet;
}

std::ostream& whodun::operator<<(std::ostream& os, SizePtrString const & toOut){
	os.write(toOut.txt, toOut.len);
	return os;
}

int whodun::strendswith(const char* str1, const char* str2){
	size_t str1L = strlen(str1);
	size_t str2L = strlen(str2);
	if(str1L < str2L){ return 0; }
	return strcmp(str1 + (str1L - str2L), str2) == 0;
}

int whodun::strstartswith(const char* str1, const char* str2){
	const char* curStr1 = str1;
	const char* curStr2 = str2;
	char cur2 = *curStr2;
	while(cur2){
		if(*curStr1 != cur2){ return 0; }
		curStr1++;
		curStr2++;
		cur2 = *curStr2;
	}
	return 1;
}

const char* whodun::fuzzyFind(const char* lookForS, const char* lookForE, const char* lookInS, const char* lookInE, uintptr_t maxFuzz){
	uintptr_t lookForLen = lookForE - lookForS;
	uintptr_t lookInLen = lookInE - lookInS;
	if(lookForLen == 0){ return lookInS; }
	if(lookForLen > lookInLen){ return 0; }
	const char* lastInS = lookInE - (lookForE - lookForS);
	for(const char* curInS = lookInS; curInS <= lastInS; curInS++){
		uintptr_t totFuzz = 0;
		for(uintptr_t i = 0; i<lookForLen; i++){
			if(curInS[i] != lookForS[i]){
				totFuzz++;
				if(totFuzz > maxFuzz){ break; }
			}
		}
		if(totFuzz <= maxFuzz){
			return curInS;
		}
	}
	return 0;
}
char* whodun::fuzzyFind(const char* lookForS, const char* lookForE, char* lookInS, char* lookInE, uintptr_t maxFuzz){
	const char* lookInSTmp = lookInS;
	const char* lookInETmp = lookInE;
	return (char*)fuzzyFind(lookForS, lookForE, lookInSTmp, lookInETmp, maxFuzz);
}

#define EDIT_LOOK_PUSH_STACK(inInd, eatFor, eatIn, edLeft)\
	lookStack.push_back(inInd);\
	lookStack.push_back(eatFor);\
	lookStack.push_back(eatIn);\
	lookStack.push_back(edLeft);
	
EditFinder::EditFinder(){}
EditFinder::~EditFinder(){}
void EditFinder::startSearch(char* lookForS, char* lookForE, char* lookInS, char* lookInE, uintptr_t maxEdit){
	lookFor.txt = lookForS;
	lookFor.len = lookForE - lookForS;
	lookIn.txt = lookInS;
	lookIn.len = lookInE - lookInS;
	//start searches from each spot
	lookStack.clear();
	uintptr_t i = lookIn.len;
	while(i){
		i--;
		EDIT_LOOK_PUSH_STACK(i,0,0,maxEdit)
	}
	lookEdit = maxEdit;
}
int EditFinder::findNext(){
	tailRecurTgt:
	uintptr_t lss = lookStack.size();
	if(lss == 0){ return 0; }
	uintptr_t lookInInd = lookStack[lss - 4];
	uintptr_t numEatFor = lookStack[lss - 3];
	uintptr_t numEatIn = lookStack[lss - 2];
	uintptr_t numEdit = lookStack[lss - 1];
	lookStack.erase(lookStack.begin() + lss - 4, lookStack.end());
	//get search status
	int moreInLookFor = numEatFor < lookFor.len;
	int moreInLookIn = ((numEatIn + lookInInd) < lookIn.len);
	//allow for deletion
	if(numEdit && moreInLookIn){
		EDIT_LOOK_PUSH_STACK(lookInInd,numEatFor,numEatIn+1,numEdit-1)
	}
	//allow for insertion
	if(numEdit && moreInLookFor){
		EDIT_LOOK_PUSH_STACK(lookInInd,numEatFor+1,numEatIn,numEdit-1)
	}
	//try to match/mismatch
	if(moreInLookFor && moreInLookIn){
		int sameChar = (lookFor.txt[numEatFor] == lookIn.txt[lookInInd + numEatIn]);
		if(numEdit || sameChar){
			EDIT_LOOK_PUSH_STACK(lookInInd,numEatFor+1,numEatIn+1,numEdit-!sameChar)
		}
	}
	//report if ready
	if(numEatFor == lookFor.len){
		foundAt = lookInInd;
		foundTo = lookInInd + numEatIn;
		foundEdit = lookEdit - numEdit;
		return 1;
	}
	//tail recurse otherwise
	goto tailRecurTgt;
}
int EditFinder::findBest(int biasEnd){
	if(!findNext()){
		return 0;
	}
	
	uintptr_t bestAt; uintptr_t bestTo; uintptr_t bestEdit;
	#define EDIT_FIND_UPDATE_BEST \
		bestAt = foundAt;\
		bestTo = foundTo;\
		bestEdit = foundEdit;
	EDIT_FIND_UPDATE_BEST
	
	while(findNext()){
		if(foundEdit < bestEdit){
			EDIT_FIND_UPDATE_BEST
		}
		else if(foundEdit == bestEdit){
			if(biasEnd){
				if(foundTo > bestTo){
					EDIT_FIND_UPDATE_BEST
				}
			}
			else{
				if(foundAt < bestAt){
					EDIT_FIND_UPDATE_BEST
				}
			}
		}
	}
	
	foundAt = bestAt;
	foundTo = bestTo;
	foundEdit = bestEdit;
	return 1;
}

int whodun::memendswith(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB1 < numB2){ return 0; }
	return memcmp(str1 + (numB1 - numB2), str2, numB2) == 0;
}

int whodun::memstartswith(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB1 < numB2){ return 0; }
	return memcmp(str1, str2, numB2) == 0;
}

MultithreadedStringH::MultithreadedStringH(uintptr_t numThread, ThreadPool* mainPool){
	numThr = numThread;
	usePool = mainPool;
	saveUniMCpy.resize(numThr);
	saveUniMSet.resize(numThr);
	saveUniMSwap.resize(numThr);
}

MultithreadedStringH::~MultithreadedStringH(){}

void MultithreadedStringH::initMemcpy(void* cpyTo, const void* cpyFrom, size_t copyNum, MultithreadedStringMemcpyTask* saveID){
	size_t numPerT = copyNum / numThr;
	size_t numExtT = copyNum % numThr;
	size_t curOff = 0;
	usePool->bigStart();
	for(unsigned i = 0; i<numThr; i++){
		void* curTo = ((char*)cpyTo) + curOff;
		const void* curFrom = ((const char*)cpyFrom) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		saveID[i].cpyTo = curTo;
		saveID[i].cpyFrom = curFrom;
		saveID[i].copyNum = curNum;
		saveID[i].myID = usePool->bigAdd(saveID + i);
		curOff += curNum;
	}
	usePool->bigEnd();
}

void MultithreadedStringH::initMemset(void* setP, int value, size_t numBts, MultithreadedStringMemsetTask* saveID){
	size_t numPerT = numBts / numThr;
	size_t numExtT = numBts % numThr;
	char* curOff = (char*)setP;
	usePool->bigStart();
	for(unsigned i = 0; i<numThr; i++){
		size_t curNum = numPerT + (i<numExtT);
		saveID[i].setP = curOff;
		saveID[i].value = value;
		saveID[i].numBts = curNum;
		saveID[i].myID = usePool->bigAdd(saveID + i);
		curOff += curNum;
	}
	usePool->bigEnd();
}

void MultithreadedStringH::initMemswap(char* arrA, char* arrB, size_t numBts, MultithreadedStringMemswapTask* saveID){
	size_t numPerT = numBts / numThr;
	size_t numExtT = numBts % numThr;
	size_t curOff = 0;
	usePool->bigStart();
	for(unsigned i = 0; i<numThr; i++){
		char* curTo = ((char*)arrA) + curOff;
		char* curFrom = ((char*)arrB) + curOff;
		size_t curNum = numPerT + (i<numExtT);
		saveID[i].arrA = curTo;
		saveID[i].arrB = curFrom;
		saveID[i].numBts = curNum;
		saveID[i].myID = usePool->bigAdd(saveID + i);
		curOff += curNum;
	}
	usePool->bigEnd();
}

void MultithreadedStringH::wait(MultithreadedStringMemcpyTask* allID){
	usePool->bigStart();
	for(uintptr_t i = 0; i<numThr; i++){ usePool->bigJoin(allID[i].myID); }
	usePool->bigEnd();
}

void MultithreadedStringH::wait(MultithreadedStringMemsetTask* allID){
	usePool->bigStart();
	for(uintptr_t i = 0; i<numThr; i++){ usePool->bigJoin(allID[i].myID); }
	usePool->bigEnd();
}

void MultithreadedStringH::wait(MultithreadedStringMemswapTask* allID){
	usePool->bigStart();
	for(uintptr_t i = 0; i<numThr; i++){ usePool->bigJoin(allID[i].myID); }
	usePool->bigEnd();
}

void MultithreadedStringH::memcpy(void* cpyTo, const void* cpyFrom, size_t copyNum){
	initMemcpy(cpyTo, cpyFrom, copyNum, &(saveUniMCpy[0]));
	wait(&(saveUniMCpy[0]));
}

void MultithreadedStringH::memset(void* setP, int value, size_t numBts){
	initMemset(setP, value, numBts, &(saveUniMSet[0]));
	wait(&(saveUniMSet[0]));
}

void MultithreadedStringH::memswap(char* arrA, char* arrB, size_t numBts){
	initMemswap(arrA, arrB, numBts, &(saveUniMSwap[0]));
	wait(&(saveUniMSwap[0]));
}

void MultithreadedStringMemcpyTask::doIt(){
	memcpy(cpyTo, cpyFrom, copyNum);
}
void MultithreadedStringMemsetTask::doIt(){
	memset(setP, value, numBts);
}
void MultithreadedStringMemswapTask::doIt(){
	memswap(arrA, arrB, numBts);
}

PistonInStream::PistonInStream(uintptr_t buffSize, uintptr_t numThread, ThreadPool* mainPool) : prodCond(&useMut), consCond(&useMut), useCopy(numThread, mainPool) {
	haveEnd = 0;
	allocBuff = buffSize;
	saveBuff = (char*)malloc(allocBuff);
	nextByte = 0;
	numByte = 0;
}
PistonInStream::~PistonInStream(){
	free(saveBuff);
}
int PistonInStream::readByte(){
	char toRet;
	uintptr_t numR = readBytes(&toRet, 1);
	if(numR){
		return 0x00FF & toRet;
	}
	return -1;
}
void PistonInStream::close(){
	useMut.lock();
	isClosed = 1;
	useMut.unlock();
}
uintptr_t PistonInStream::readBytes(char* toR, uintptr_t numR){
	useMut.lock();
	char* nextR = toR;
	uintptr_t leftR = numR;
	uintptr_t totalRead = 0;
	while(leftR){
		if(numByte){
			uintptr_t numCopy = std::min(numByte, leftR);
			useCopy.memcpy(nextR, saveBuff + nextByte, numCopy);
			numByte -= numCopy;
			leftR -= numCopy;
			nextR += numCopy;
			totalRead += numCopy;
		}
		if(numByte == 0){ prodCond.signal(); }
		if(haveEnd){ break; }
		if(leftR){ consCond.wait(); }
	}
	useMut.unlock();
	return totalRead;
}
void PistonInStream::waitForEmpty(){
	useMut.lock();
	while(numByte){
		prodCond.wait();
	}
}
void PistonInStream::noteFull(){
	consCond.signal();
	useMut.unlock();
}
void PistonInStream::noteEnd(){
	haveEnd = 1;
	consCond.signal();
	useMut.unlock();
}

PistonOutStream::PistonOutStream(uintptr_t buffSize, uintptr_t numThread, ThreadPool* mainPool)  : prodCond(&useMut), consCond(&useMut), useCopy(numThread, mainPool) {
	allocBuff = buffSize;
	saveBuff = (char*)malloc(allocBuff);
	nextByte = 0;
}
PistonOutStream::~PistonOutStream(){
	free(saveBuff);
}
void PistonOutStream::writeByte(int toW){
	char toWC = toW;
	writeBytes(&toWC, 1);
}
void PistonOutStream::close(){
	useMut.lock();
	isClosed = 1;
	consCond.signal();
	useMut.unlock();
}
void PistonOutStream::writeBytes(const char* toW, uintptr_t numW){
	useMut.lock();
	const char* nextW = toW;
	uintptr_t leftW = numW;
	while(leftW){
		if(nextByte != allocBuff){
			uintptr_t openSpace = allocBuff - nextByte;
			uintptr_t numCopy = std::min(openSpace, leftW);
			useCopy.memcpy(saveBuff + nextByte, nextW, numCopy);
			nextW += numCopy;
			leftW -= numCopy;
			nextByte += numCopy;
		}
		if(nextByte == allocBuff){
			consCond.signal();
			prodCond.wait();
		}
	}
	useMut.unlock();
}
void PistonOutStream::waitForFull(){
	useMut.lock();
	while(nextByte != allocBuff){
		if(isClosed){ break; }
		consCond.wait();
	}
}
void PistonOutStream::noteEmpty(){
	prodCond.signal();
	useMut.unlock();
}

BulkStringSource::~BulkStringSource(){}

SizePtrBulkStringSource::SizePtrBulkStringSource(uintptr_t numStrings, SizePtrString* theStrings){
	saveStr.insert(saveStr.end(), theStrings, theStrings + numStrings);
}
SizePtrBulkStringSource::~SizePtrBulkStringSource(){}
uintmax_t SizePtrBulkStringSource::numStrings(){
	return saveStr.size();
}
uintmax_t SizePtrBulkStringSource::stringLength(uintmax_t strInd){
	return saveStr[strInd].len;
}
void SizePtrBulkStringSource::getString(uintmax_t strInd, uintmax_t fromC, uintmax_t toC, char* saveC){
	memcpy(saveC, saveStr[strInd].txt + fromC, toC - fromC);
}

bool whodun::checkFileExtensions(const char* fileName, std::set<std::string>* extSet){
	std::set<std::string>::iterator curIt = extSet->begin();
	while(curIt != extSet->end()){
		if(strendswith(fileName, curIt->c_str())){
			return true;
		}
		curIt++;
	}
	return false;
}

