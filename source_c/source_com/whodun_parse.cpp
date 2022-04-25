#include "whodun_parse.h"

#include <string.h>

using namespace whodun;

StringSplitter::StringSplitter(){}
StringSplitter::~StringSplitter(){}

#define CHAR_SPLIT_PUSH(startV, endV) \
	if(numFoundSpl == allocSplit){\
		uintptr_t nextSpl = 1 + (allocSplit << 1);\
		char** newAlloc = (char**)malloc(nextSpl*sizeof(char*));\
		memcpy(newAlloc, allocStarts, numFoundSpl*sizeof(char*));\
		if(allocStarts){free(allocStarts);}\
		allocStarts = newAlloc;\
		newAlloc = (char**)malloc(nextSpl*sizeof(char*));\
		memcpy(newAlloc, allocEnds, numFoundSpl*sizeof(char*));\
		if(allocEnds){free(allocEnds);}\
		allocEnds = newAlloc;\
		allocSplit = nextSpl;\
	}\
	allocStarts[numFoundSpl] = startV;\
	allocEnds[numFoundSpl] = endV;\
	numFoundSpl++;

CharacterSplitter::CharacterSplitter(){
	splitOn = '\n';
	allocSplit = 0;
	allocStarts = 0;
	allocEnds = 0;
}
CharacterSplitter::~CharacterSplitter(){
	if(allocStarts){ free(allocStarts); }
	if(allocEnds){ free(allocEnds); }
}
char* CharacterSplitter::split(uintptr_t inpLen, char* inpStr){
	uintptr_t numFoundSpl = 0;
	char* curStart = inpStr;
	char* curEnd;
	for(uintptr_t i = 0; i<inpLen; i++){
		if(inpStr[i] == splitOn){
			curEnd = inpStr + i;
			CHAR_SPLIT_PUSH(curStart, curEnd)
			curStart = curEnd + 1;
		}
	}
	res.numSplits = numFoundSpl;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
	return curStart;
}
void CharacterSplitter::splitAll(uintptr_t inpLen, char* inpStr){
	char* parseEnd = split(inpLen, inpStr);
	char* strEnd = inpStr + inpLen;
	if(parseEnd == strEnd){ return; }
	uintptr_t numFoundSpl = res.numSplits;
	CHAR_SPLIT_PUSH(parseEnd, strEnd)
	res.numSplits = numFoundSpl;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
}

MultithreadCharacterSplitterFindUniform::MultithreadCharacterSplitterFindUniform(){}
MultithreadCharacterSplitterFindUniform::~MultithreadCharacterSplitterFindUniform(){}
void MultithreadCharacterSplitterFindUniform::doIt(){
	if(packLocStart == 0){
		foundChar.clear();
		char* curLook = lookFrom;
		while(curLook != lookTo){
			if(*curLook == splitOn){
				foundChar.push_back(curLook);
			}
			curLook++;
		}
	}
	else if(foundChar.size()){
		packLocStart[0] = prevStart;
		packLocEnd[0] = foundChar[0];
		for(uintptr_t i = 1; i<foundChar.size(); i++){
			packLocStart[i] = foundChar[i-1]+1;
			packLocEnd[i] = foundChar[i];
		}
	}
}
MultithreadCharacterSplitter::MultithreadCharacterSplitter(int numThread, ThreadPool* mainPool){
	usePool = mainPool;
	findUnis.resize(numThread);
	for(int i = 0; i<numThread; i++){
		findUnis[i].mainCont = this;
	}
}
MultithreadCharacterSplitter::~MultithreadCharacterSplitter(){}
char* MultithreadCharacterSplitter::split(uintptr_t inpLen, char* inpStr){
	//do the first search
	uintptr_t numPerT = inpLen / findUnis.size();
	uintptr_t numExtT = inpLen % findUnis.size();
	char* curFoc = inpStr;
	usePool->bigStart();
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		uintptr_t numCurT = numPerT + (i < numExtT);
		MultithreadCharacterSplitterFindUniform* curUni = &(findUnis[i]);
		curUni->splitOn = splitOn;
		curUni->lookFrom = curFoc;
		curFoc += numCurT;
		curUni->lookTo = curFoc;
		curUni->packLocStart = 0;
		curUni->threadID = usePool->bigAdd(curUni);
	}
	//how many splits are there
	uintptr_t numToken = 0;
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		usePool->bigJoin(findUnis[i].threadID);
		numToken += findUnis[i].foundChar.size();
	}
	usePool->bigEnd();
	//make some space
	if(numToken > allocSplit){
		if(allocStarts){ free(allocStarts); }
		if(allocEnds){ free(allocEnds); }
		allocStarts = (char**)malloc(numToken*sizeof(char*));
		allocEnds = (char**)malloc(numToken*sizeof(char*));
		allocSplit = numToken;
	}
	//pack in the tokens
	char** curFocStart = allocStarts;
	char** curFocEnd = allocEnds;
	char* prevStart = inpStr;
	usePool->bigStart();
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		MultithreadCharacterSplitterFindUniform* curUni = &(findUnis[i]);
		curUni->prevStart = prevStart;
		curUni->packLocStart = curFocStart;
		curUni->packLocEnd = curFocEnd;
		curUni->threadID = usePool->bigAdd(curUni);
		uintptr_t numChar = curUni->foundChar.size();
		curFocStart += numChar;
		curFocEnd += numChar;
		if(numChar){
			prevStart = curUni->foundChar[numChar - 1] + 1;
		}
	}
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		usePool->bigJoin(findUnis[i].threadID);
	}
	usePool->bigEnd();
	res.numSplits = numToken;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
	return prevStart;
}

CharacterSetSplitter::CharacterSetSplitter(){
	memset(splitOn, 0, 256);
	allocSplit = 0;
	allocStarts = 0;
	allocEnds = 0;
}
CharacterSetSplitter::~CharacterSetSplitter(){
	if(allocStarts){ free(allocStarts); }
	if(allocEnds){ free(allocEnds); }
}
char* CharacterSetSplitter::split(uintptr_t inpLen, char* inpStr){
	uintptr_t numFoundSpl = 0;
	char* curStart = inpStr;
	char* curEnd;
	for(uintptr_t i = 0; i<inpLen; i++){
		if(splitOn[0x00FF & inpStr[i]]){
			curEnd = inpStr + i;
			CHAR_SPLIT_PUSH(curStart, curEnd)
			curStart = curEnd + 1;
		}
	}
	res.numSplits = numFoundSpl;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
	return curStart;
}
void CharacterSetSplitter::splitAll(uintptr_t inpLen, char* inpStr){
	char* parseEnd = split(inpLen, inpStr);
	char* strEnd = inpStr + inpLen;
	if(parseEnd == strEnd){ return; }
	uintptr_t numFoundSpl = res.numSplits;
	CHAR_SPLIT_PUSH(parseEnd, strEnd)
	res.numSplits = numFoundSpl;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
}

MultithreadCharacterSetSplitterFindUniform::MultithreadCharacterSetSplitterFindUniform(){}
MultithreadCharacterSetSplitterFindUniform::~MultithreadCharacterSetSplitterFindUniform(){}
void MultithreadCharacterSetSplitterFindUniform::doIt(){
	if(packLocStart == 0){
		foundChar.clear();
		char* curLook = lookFrom;
		while(curLook != lookTo){
			if(splitOn[0x00FF & *curLook]){
				foundChar.push_back(curLook);
			}
			curLook++;
		}
	}
	else if(foundChar.size()){
		packLocStart[0] = prevStart;
		packLocEnd[0] = foundChar[0];
		for(uintptr_t i = 1; i<foundChar.size(); i++){
			packLocStart[i] = foundChar[i-1]+1;
			packLocEnd[i] = foundChar[i];
		}
	}
}
MultithreadCharacterSetSplitter::MultithreadCharacterSetSplitter(int numThread, ThreadPool* mainPool){
	usePool = mainPool;
	findUnis.resize(numThread);
	for(int i = 0; i<numThread; i++){
		findUnis[i].mainCont = this;
	}
}
MultithreadCharacterSetSplitter::~MultithreadCharacterSetSplitter(){}
char* MultithreadCharacterSetSplitter::split(uintptr_t inpLen, char* inpStr){
	//do the first search
	uintptr_t numPerT = inpLen / findUnis.size();
	uintptr_t numExtT = inpLen % findUnis.size();
	char* curFoc = inpStr;
	usePool->bigStart();
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		uintptr_t numCurT = numPerT + (i < numExtT);
		MultithreadCharacterSetSplitterFindUniform* curUni = &(findUnis[i]);
		curUni->splitOn = splitOn;
		curUni->lookFrom = curFoc;
		curFoc += numCurT;
		curUni->lookTo = curFoc;
		curUni->packLocStart = 0;
		curUni->threadID = usePool->bigAdd(curUni);
	}
	//how many splits are there
	uintptr_t numToken = 0;
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		usePool->bigJoin(findUnis[i].threadID);
		numToken += findUnis[i].foundChar.size();
	}
	usePool->bigEnd();
	//make some space
	if(numToken > allocSplit){
		if(allocStarts){ free(allocStarts); }
		if(allocEnds){ free(allocEnds); }
		allocStarts = (char**)malloc(numToken*sizeof(char*));
		allocEnds = (char**)malloc(numToken*sizeof(char*));
		allocSplit = numToken;
	}
	//pack in the tokens
	char** curFocStart = allocStarts;
	char** curFocEnd = allocEnds;
	char* prevStart = inpStr;
	usePool->bigStart();
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		MultithreadCharacterSetSplitterFindUniform* curUni = &(findUnis[i]);
		curUni->prevStart = prevStart;
		curUni->packLocStart = curFocStart;
		curUni->packLocEnd = curFocEnd;
		curUni->threadID = usePool->bigAdd(curUni);
		uintptr_t numChar = curUni->foundChar.size();
		curFocStart += numChar;
		curFocEnd += numChar;
		if(numChar){
			prevStart = curUni->foundChar[numChar - 1];
		}
	}
	for(uintptr_t i = 0; i<findUnis.size(); i++){
		usePool->bigJoin(findUnis[i].threadID);
	}
	usePool->bigEnd();
	res.numSplits = numToken;
	res.splitStart = allocStarts;
	res.splitEnd = allocEnds;
	return (prevStart == inpStr) ? inpStr : (prevStart + 1);
}

#define MEMCPY_STAGE_SIZE 0x00010000

SplitterInStream::SplitterInStream(InStream* fromStream, StringSplitter* splitMeth, uintptr_t bufferSize){
	fromStr = fromStream;
	splMeth = splitMeth;
	buffAlloc = bufferSize;
	buff = (char*)malloc(buffAlloc);
	useCopy = 0;
	prevBStart = buff;
	prevBEnd = buff;
	moveBuff = 0;
	hitEnd = 0;
}
SplitterInStream::SplitterInStream(InStream* fromStream, StringSplitter* splitMeth, uintptr_t bufferSize, int numThreads, ThreadPool* mainPool){
	fromStr = fromStream;
	splMeth = splitMeth;
	buffAlloc = bufferSize;
	buff = (char*)malloc(buffAlloc);
	useCopy = new MultithreadedStringH(numThreads, mainPool);
	prevBStart = buff;
	prevBEnd = buff;
	threadStageSize = MEMCPY_STAGE_SIZE * numThreads;
	moveBuff = (char*)malloc(threadStageSize);
	hitEnd = 0;
}
SplitterInStream::~SplitterInStream(){
	free(buff);
	if(moveBuff){ free(moveBuff); }
	if(useCopy){ delete(useCopy); }
}
StringSplitData SplitterInStream::read(){
	if(hitEnd){
		StringSplitData toRet;
		toRet.numSplits = 0;
		return toRet;
	}
	//pack everything to the start of buff
	uintptr_t numBuffOcc = prevBEnd - prevBStart;
	if(prevBStart != buff){
		uintptr_t numBuffPre = prevBStart - buff;
		if(useCopy){
			for(uintptr_t i = 0; i<numBuffOcc; i+=threadStageSize){
				uintptr_t numMove = numBuffOcc - i;
					numMove = std::min(numMove, threadStageSize);
				//copy to stage, then to target
				useCopy->memcpy(moveBuff, buff + i + numBuffPre, numMove);
				useCopy->memcpy(buff + i, moveBuff, numMove);
			}
		}
		else{
			memmove(buff, buff + numBuffPre, numBuffOcc);
		}
		prevBStart = buff;
		prevBEnd = buff + numBuffOcc;
	}
	
	//read until something
	readMore(0);
	while((splMeth->res.numSplits == 0) && !hitEnd){
		readMore(0);
	}
	return splMeth->res;
}
int SplitterInStream::readMore(StringSplitData* lastRet){
	if(hitEnd){
		return 0;
	}
	uintptr_t numBuffOcc = prevBEnd - buff;
	uintptr_t maxRead = buffAlloc - numBuffOcc;
	if(maxRead == 0){
		//need a bigger buffer
		buffAlloc = 1 + (buffAlloc << 1);
		char* altBuff = (char*)malloc(buffAlloc);
		if(useCopy){
			useCopy->memcpy(altBuff, buff, numBuffOcc);
		}
		else{
			memcpy(altBuff, buff, numBuffOcc);
		}
		free(buff);
		buff = altBuff;
		maxRead = buffAlloc - numBuffOcc;
		prevBEnd = buff + numBuffOcc;
	}
	//read
	uintptr_t numRead = fromStr->readBytes(prevBEnd, maxRead);
	numBuffOcc += numRead;
	prevBEnd = buff + numBuffOcc;
	if(numRead != maxRead){
		splMeth->splitAll(numBuffOcc, buff);
		prevBStart = prevBEnd;
		hitEnd = 1;
		if(lastRet){ *lastRet = splMeth->res; }
		return 1;
	}
	//split
	prevBStart = splMeth->split(numBuffOcc, buff);
	if(lastRet){ *lastRet = splMeth->res; }
	return 1;
}

short whodunParseHexNibbleArr[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int whodun::parseHexNibble(char cval){
	return whodunParseHexNibbleArr[0x00FF&cval];
}

int whodun::parseHexCode(char cvalA, char cvalB){
	int ivalA = whodunParseHexNibbleArr[0x00FF&cvalA];
	int ivalB = whodunParseHexNibbleArr[0x00FF&cvalB];
	int toRet = (ivalA << 4) | ivalB;
	if(toRet < 0){ return -1; }
	return toRet;
}


