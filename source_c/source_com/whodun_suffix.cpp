#include "whodun_suffix.h"

#include <math.h>
#include <signal.h>

using namespace whodun;

SuffixArray::~SuffixArray(){}

/**
 * Compare entries by rank/next-rank
 * @param unif Ignored
 * @param itemA The first entry.
 * @param itemB The second entry.
 */
bool whodun_suffixRankCompare(void* unif, void* itemA,void* itemB){
	//compare ranks first
	uintmax_t rankA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_RANK);
	uintmax_t rankB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_RANK);
	if(rankA < rankB){
		return true;
	}
	else if(rankA > rankB){
		return false;
	}
	//next ranks next
	uintmax_t nextA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
	uintmax_t nextB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
	if(nextA < nextB){
		return true;
	}
	else if(nextA > nextB){
		return false;
	}
	//otherwise, impose a total ordering (string index)
	uintmax_t sindA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	uintmax_t sindB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	if(sindA < sindB){
		return true;
	}
	else if(sindA > sindB){
		return false;
	}
	//as a fallback, character index: should never
	uintmax_t charA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	uintmax_t charB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	return charA < charB;
}

/**
 * Compare entries by string/character
 * @param unif Ignored
 * @param itemA The first entry.
 * @param itemB The second entry.
 */
bool whodun_suffixStringIndCompare(void* unif, void* itemA,void* itemB){
	uintmax_t rankA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	uintmax_t rankB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	if(rankA < rankB){
		return true;
	}
	else if(rankA > rankB){
		return false;
	}
	uintmax_t nextA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	uintmax_t nextB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	return nextA < nextB;
}

#define RAW_COMP_BUFF_SIZE 0x0100
/**Compare between external suffix array entries.*/
class SuffixArrayInternalCompare : public RawComparator{
public:
	/**Set up*/
	SuffixArrayInternalCompare(BulkStringSource* useCompare);
	/**Tear down.*/
	~SuffixArrayInternalCompare();
	uintptr_t itemSize();
	bool compare(const void* itemA, const void* itemB);
	/**
	 * Do a comparison.
	 * @param itemA THe first suffix array entry.
	 * @param itemB THe second suffix array entry.
	 * @param knownCommon The number of bases known to be in common.
	 * @return Whether A preceedes B (- 0 +) and the number of common bases.
	 */
	std::pair<int,uintmax_t> compare(const void* itemA, const void* itemB, uintmax_t knownCommon);
	/**The place to get text data from.*/
	BulkStringSource* useComp;
};
SuffixArrayInternalCompare::SuffixArrayInternalCompare(BulkStringSource* useCompare){
	useComp = useCompare;
}
SuffixArrayInternalCompare::~SuffixArrayInternalCompare(){}
uintptr_t SuffixArrayInternalCompare::itemSize(){
	return WHODUN_OOM_SUFFIX_ENTRY_SIZE;
}
std::pair<int,uintmax_t> SuffixArrayInternalCompare::compare(const void* itemA, const void* itemB, uintmax_t knownCommon){
	uintmax_t strIndA = unpackBE64(WHODUN_SUFFIX_ENTRY_OFF_STRING + (const char*)itemA);
	uintmax_t strIndB = unpackBE64(WHODUN_SUFFIX_ENTRY_OFF_STRING + (const char*)itemB);
	uintmax_t chrIndA = unpackBE64(WHODUN_SUFFIX_ENTRY_OFF_CHAR + (const char*)itemA);
	uintmax_t chrIndB = unpackBE64(WHODUN_SUFFIX_ENTRY_OFF_CHAR + (const char*)itemB);
	uintmax_t strALen = useComp->stringLength(strIndA);
	uintmax_t strBLen = useComp->stringLength(strIndB);
//	if(chrIndA > strALen){
//		raise(SIGSEGV);
//	}
//	if(chrIndB > strBLen){
//		raise(SIGSEGV);
//	}
	chrIndA += knownCommon;
	chrIndB += knownCommon;
	strALen = strALen - chrIndA;
	strBLen = strBLen - chrIndB;
	char buffA[RAW_COMP_BUFF_SIZE];
	char buffB[RAW_COMP_BUFF_SIZE];
	uintmax_t comLen = std::min(strALen,strBLen);
	for(uintmax_t i = 0; i<comLen; i+=RAW_COMP_BUFF_SIZE){
		uintmax_t curLen = comLen - i; curLen = std::min(curLen, (uintmax_t)RAW_COMP_BUFF_SIZE);
		useComp->getString(strIndA, chrIndA + i, chrIndA + i + curLen, buffA);
		useComp->getString(strIndB, chrIndB + i, chrIndB + i + curLen, buffB);
		for(uintmax_t j = 0; j<curLen; j++){
			int charA = 0x00FF & buffA[j];
			int charB = 0x00FF & buffB[j];
			if(charA != charB){
				uintmax_t comRes = knownCommon + i + j;
				return std::pair<int,uintmax_t>(charA - charB, comRes);
			}
		}
	}
	int lenCR = (strALen < strBLen) ? -1 : ((strALen > strBLen) ? 1 : 0);
	return std::pair<int,uintmax_t>(lenCR, knownCommon + comLen);
}
bool SuffixArrayInternalCompare::compare(const void* itemA, const void* itemB){
	return compare(itemA,itemB,0).first < 0;
}

void SuffixArrayUniform::doIt(){
	switch(phase){
		case 1:{
			SizePtrString* curStr = p1FillWith + p1StartS;
			uintptr_t curC = p1StartC;
			uintptr_t curStrI = p1StartS;
			for(uintptr_t i = 0; i<p1NumFill; i++){
				while(curC >= curStr->len){
					curStr++;
					curC = 0;
					curStrI++;
				}
				char* curLoc = p1SetLoc + WHODUN_SUFFIX_ENTRY_SIZE*i;
				packBE64(curStrI, curLoc + WHODUN_SUFFIX_ENTRY_OFF_STRING);
				packBE64(curC, curLoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
				//skip LCP
				packBE64(1 + (0x00FF & curStr->txt[curC]), curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
				curC++;
				packBE64((curC < curStr->len) ? (1 + (0x00FF & curStr->txt[curC])) : 0, curLoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
			}
		} break;
		case 2:{
			p2MaxRank = 0;
			if(p1NumFill == 0){ return; }
			//do the first
			char* curLoc = p1SetLoc;
			uintmax_t prevRank = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
			uintmax_t prevComp = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
			uintmax_t nextRank = 0;
			packBE64(nextRank, curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
			//do the rest
			for(uintptr_t i = 1; i<p1NumFill; i++){
				curLoc += WHODUN_SUFFIX_ENTRY_SIZE;
				uintmax_t curRank = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
				uintmax_t curComp = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
				if((curRank != prevRank) || (curComp != prevComp)){
					nextRank++;
					prevRank = curRank; prevComp = curComp;
				}
				packBE64(nextRank, curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
			}
			p2MaxRank = nextRank;
		} break;
		case 3:{
			char* curLoc = p1SetLoc;
			for(uintptr_t i = 0; i<p1NumFill; i++){
				char* curRankL = curLoc + WHODUN_SUFFIX_ENTRY_OFF_RANK;
				packBE64(p3Offset + unpackBE64(curRankL), curRankL);
				curLoc += WHODUN_SUFFIX_ENTRY_SIZE;
			}
		} break;
		case 4:{
			char* curLoc = p1SetLoc;
			uintmax_t curSAI = (curLoc - p4BaseLoc) / WHODUN_SUFFIX_ENTRY_SIZE;
			for(uintptr_t i = 0; i<p1NumFill; i++){
				uintmax_t curStrI = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_STRING);
				uintmax_t curCharI = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
				uintmax_t curSOffset = p4StrOffset[curStrI];
				p4InvFill[curSOffset + curCharI] = curSAI;
				curSAI++;
				curLoc += WHODUN_SUFFIX_ENTRY_SIZE;
			}
		} break;
		case 5:{
			char* curLoc = p1SetLoc;
			for(uintptr_t i = 0; i<p1NumFill; i++){
				uintmax_t curStrI = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_STRING);
				uintmax_t curCharI = unpackBE64(curLoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
				uintmax_t testCharI = curCharI + p5SkipLen;
				uintmax_t testRank = 0;
				if(testCharI < p5OrigStrs[curStrI].len){
					uintmax_t curSOffset = p4StrOffset[curStrI];
					char* testFoc = p4BaseLoc + p4InvFill[curSOffset + testCharI]*WHODUN_SUFFIX_ENTRY_SIZE;
					testRank = unpackBE64(testFoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
				}
				packBE64(testRank, curLoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
				curLoc += WHODUN_SUFFIX_ENTRY_SIZE;
			}
		} break;
		case 6:{
			//Kasai's algorithm
			uintmax_t prevStrI = (intmax_t)-1;
			uintmax_t prevLCP = 0;
			uintmax_t curSCI = (p1SetLoc - p4BaseLoc) / WHODUN_SUFFIX_ENTRY_SIZE;
			for(uintptr_t i = 0; i<p1NumFill; i++){
				uintmax_t curSAI = p4InvFill[curSCI];
				char* curFoc = p4BaseLoc + curSAI*WHODUN_SUFFIX_ENTRY_SIZE;
				uintmax_t curStrI = unpackBE64(curFoc + WHODUN_SUFFIX_ENTRY_OFF_STRING);
				uintmax_t curCharI = unpackBE64(curFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
				SizePtrString curStr = p5OrigStrs[curStrI];
				if(curStrI != prevStrI){ prevLCP = 0; prevStrI = curStrI; }
				if(curSAI == (p6NumEntry-1)){ prevLCP = 0; packBE64(0, curFoc + WHODUN_SUFFIX_ENTRY_OFF_LCP); curSCI++; continue; }
				char* nextFoc = curFoc + WHODUN_SUFFIX_ENTRY_SIZE;
				uintmax_t nextStrI = unpackBE64(nextFoc + WHODUN_SUFFIX_ENTRY_OFF_STRING);
				uintmax_t nextCharI = unpackBE64(nextFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
				SizePtrString nextStr = p5OrigStrs[nextStrI];
				
				//idiot check
//				uintmax_t maxCheckP = std::min(prevLCP, (uintmax_t)128);
//				for(uintmax_t j = 0; j<maxCheckP; j++){
//					if(curStr.txt[curCharI+j] != nextStr.txt[nextCharI+j]){
//						raise(SIGSEGV);
//					}
//				}
				
				uintmax_t maxLCP = std::min(curStr.len - curCharI, nextStr.len - nextCharI);
				while(prevLCP < maxLCP){
					if(curStr.txt[curCharI + prevLCP] != nextStr.txt[nextCharI + prevLCP]){ break; }
					prevLCP++;
				}
				packBE64(prevLCP, curFoc + WHODUN_SUFFIX_ENTRY_OFF_LCP);
				if(prevLCP){prevLCP--;}
				curSCI++;
			}
		} break;
		case 7:{
			memcpy(p7Target, p7Source, p7NumCopy);
		} break;
		case 8:{
			for(uintptr_t i = 0; i<p8NumOffset; i++){
				char* curTgt = p8Entries + i*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
				uintmax_t curStrInd = unpackBE64(curTgt);
				packBE64(curStrInd + p8Offset, curTgt);
			}
		} break;
		case 9:{
			//figure where the quantiles are
			SuffixArrayInternalCompare useComp(p9Compare);
			SortOptions useCOpts(&useComp);
			std::pair<uintptr_t,uintptr_t> fromInds = chunkyPairedQuantile(p9FullNumA, p9EntsA, p9FullNumB, p9EntsB, p9QuantileLow, &useCOpts);
			std::pair<uintptr_t,uintptr_t> toInds = chunkyPairedQuantile(p9FullNumA, p9EntsA, p9FullNumB, p9EntsB, p9QuantileHigh, &useCOpts);
			char* nextFromA = p9EntsA + fromInds.first*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			char* nextFromB = p9EntsB + fromInds.second*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			char* prevSet = 0;
			char* nextDump = p9EntsDump;
			char* lastEatA = 0;
			char* lastEatB = 0;
			int lastFromA = 0;
			uintmax_t lastLCPA = 0;
			uintmax_t lastLCPB = 0;
			uintmax_t lastLCPX = 0;
			//quickly burn through any leading elements: otherwise will be slow
			if((fromInds.first != toInds.first) && (fromInds.second != toInds.second)){
				char* realStartA = chunkyLowerBound(toInds.first - fromInds.first, nextFromA, nextFromB, &useCOpts);
				char* realStartB = chunkyLowerBound(toInds.second - fromInds.second, nextFromB, nextFromA, &useCOpts);
				if(realStartA != nextFromA){
					lastFromA = 1;
					lastEatA = realStartA - WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					uintptr_t numBytePref = realStartA - nextFromA;
					memcpy(nextDump, nextFromA, numBytePref);
					fromInds.first += (numBytePref / WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					nextFromA += numBytePref;
					nextDump += numBytePref;
					prevSet = nextDump - WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					lastLCPA = unpackBE64(lastEatA + WHODUN_SUFFIX_ENTRY_OFF_LCP);
				}
				else if(realStartB != nextFromB){
					lastFromA = 0;
					lastEatB = realStartB - WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					uintptr_t numBytePref = realStartB - nextFromB;
					memcpy(nextDump, nextFromB, numBytePref);
					fromInds.second += (numBytePref / WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					nextFromB += numBytePref;
					nextDump += numBytePref;
					prevSet = nextDump - WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					lastLCPB = unpackBE64(lastEatB + WHODUN_SUFFIX_ENTRY_OFF_LCP);
				}
			}
			//run through the rest
			int moreMerge = (fromInds.first != toInds.first) && (fromInds.second != toInds.second);
			while(moreMerge){
				int getFromA;
				if((lastLCPA < lastLCPB) && (lastLCPA < lastLCPX)){
					getFromA = 0;
				}
				else if((lastLCPB < lastLCPA) && (lastLCPB < lastLCPX)){
					getFromA = 1;
				}
				else{
					uintmax_t minCom = std::min(lastLCPA, lastLCPB);
						minCom = std::min(minCom, lastLCPX);
					getFromA = useComp.compare(nextFromA,nextFromB,minCom).first < 0;
				}
				if(getFromA){
					memcpy(nextDump, nextFromA, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					if(lastFromA && prevSet){ packBE64(lastLCPA, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
					if(lastLCPA == lastLCPX){
						if(lastEatB){ lastLCPX = useComp.compare(nextFromA, lastEatB, lastLCPX).second; }
					}
					else{
						lastLCPX = std::min(lastLCPA, lastLCPX);
					}
					if(!lastFromA && prevSet){ packBE64(lastLCPX, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
					lastLCPA = unpackBE64(nextFromA + WHODUN_SUFFIX_ENTRY_OFF_LCP);
					lastEatA = nextFromA;
					nextFromA += WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					fromInds.first++;
					moreMerge = (fromInds.first != toInds.first);
				}
				else{
					memcpy(nextDump, nextFromB, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					if(!lastFromA && prevSet){ packBE64(lastLCPB, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
					if(lastLCPB == lastLCPX){
						if(lastEatA){ lastLCPX = useComp.compare(nextFromB, lastEatA, lastLCPX).second; }
					}
					else{
						lastLCPX = std::min(lastLCPB, lastLCPX);
					}
					if(lastFromA && prevSet){ packBE64(lastLCPX, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
					lastLCPB = unpackBE64(nextFromB + WHODUN_SUFFIX_ENTRY_OFF_LCP);
					lastEatB = nextFromB;
					nextFromB += WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					fromInds.second++;
					moreMerge = (fromInds.second != toInds.second);
				}
				lastFromA = getFromA;
				prevSet = nextDump;
				nextDump += WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			}
			//run down
			if(fromInds.first != toInds.first){
				if(lastFromA && prevSet){ packBE64(lastLCPA, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
				if(lastLCPA == lastLCPX){
					if(lastEatB){ lastLCPX = useComp.compare(nextFromA, lastEatB, lastLCPX).second; }
				}
				else{
					lastLCPX = std::min(lastLCPA, lastLCPX);
				}
				if(!lastFromA && prevSet){ packBE64(lastLCPX, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
				memcpy(nextDump, nextFromA, (toInds.first - fromInds.first)*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			}
			if(fromInds.second != toInds.second){
				if(!lastFromA && prevSet){ packBE64(lastLCPB, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
				if(lastLCPB == lastLCPX){
					if(lastEatA){ lastLCPX = useComp.compare(nextFromB, lastEatA, lastLCPX).second; }
				}
				else{
					lastLCPX = std::min(lastLCPB, lastLCPX);
				}
				if(lastFromA && prevSet){ packBE64(lastLCPX, prevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP); }
				memcpy(nextDump, nextFromB, (toInds.second - fromInds.second)*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			}
		} break;
		case 10:{
			if(p10PrevSet == 0){ return; }
			SuffixArrayInternalCompare useComp(p9Compare);
			uintmax_t lastLCP = useComp.compare(p9EntsDump, p10PrevSet, 0).second;
			packBE64(lastLCP, p10PrevSet + WHODUN_SUFFIX_ENTRY_OFF_LCP);
		} break;
		case 11:{
			char* curGet = p11PackSrc;
			char* curSet = p11PackTgt;
			for(uintptr_t i = 0; i<p11NumPack; i++){
				memcpy(curSet, curGet, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
				packBE64(unpackBE64(curSet) + p11Offset, curSet);
				curGet += WHODUN_SUFFIX_ENTRY_SIZE;
				curSet += WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			}
		} break;
		default:
			throw std::runtime_error("Da fuq?");
	};
}
/*
				if(curCharI != curSCI){
					raise(SIGSEGV);
				}
				if(prevLCP < 10000){
					for(uintptr_t j = 0; j<prevLCP; j++){
						if(curStr.txt[curCharI+j] != nextStr.txt[nextCharI+j]){
							raise(SIGSEGV);
						}
					}
				}
*/

InMemorySuffixArray::InMemorySuffixArray() {
	numAlloc = 0;
	allocBuff = 0;
}
InMemorySuffixArray::~InMemorySuffixArray(){
	if(allocBuff){ free(allocBuff); }
}
uintmax_t InMemorySuffixArray::numEntry(){
	return numEntries;
}
void InMemorySuffixArray::getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc){
	char* curEnt = allocBuff + WHODUN_SUFFIX_ENTRY_SIZE*entInd;
	storeLoc->stringInd = unpackBE64(curEnt + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	storeLoc->charInd = unpackBE64(curEnt + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	storeLoc->lcpNext = unpackBE64(curEnt + WHODUN_SUFFIX_ENTRY_OFF_LCP);
}



InMemorySuffixArrayBuilder::InMemorySuffixArrayBuilder() : soptsRank(WHODUN_SUFFIX_ENTRY_SIZE, 0, internalSuffixRankCompare), soptsInd(WHODUN_SUFFIX_ENTRY_SIZE, 0, whodun_suffixStringIndCompare), sortMethod(&soptsRank) {
	allocInverse = 0;
	inverseBuff = 0;
	numThread = 1;
	usePool = 0;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){ passUnis.push_back(&(threadUnis[i])); }
	rankChunkBreak.resize(numThread);
}
InMemorySuffixArrayBuilder::InMemorySuffixArrayBuilder(uintptr_t numThr, ThreadPool* mainPool) : soptsRank(WHODUN_SUFFIX_ENTRY_SIZE, 0, internalSuffixRankCompare), soptsInd(WHODUN_SUFFIX_ENTRY_SIZE, 0, whodun_suffixStringIndCompare), sortMethod(&soptsRank, numThr, mainPool) {
	allocInverse = 0;
	inverseBuff = 0;
	numThread = numThr;
	usePool = mainPool;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){ passUnis.push_back(&(threadUnis[i])); }
	rankChunkBreak.resize(numThread);
}
InMemorySuffixArrayBuilder::~InMemorySuffixArrayBuilder(){
	if(inverseBuff){ free(inverseBuff); }
}
void InMemorySuffixArrayBuilder::build(SizePtrString buildOn, InMemorySuffixArray* toStore){
	allStrings.clear();
	allStrings.push_back(buildOn);
	toStore->numEntries = 0;
	build(1, &(allStrings[0]), toStore);
}
void InMemorySuffixArrayBuilder::build(uintptr_t numOn, SizePtrString** buildOn, InMemorySuffixArray* toStore){
	allStrings.clear();
	for(uintptr_t i = 0; i<numOn; i++){
		allStrings.push_back(*(buildOn[i]));
	}
	toStore->numEntries = 0;
	if(numOn){
		build(numOn, &(allStrings[0]), toStore);
	}
}
void debug_outputInternalSuffixArray(char* theArray, uintptr_t numEntries){
	char* curPrint = theArray;
	for(uintptr_t i = 0; i<numEntries; i++){
		std::cout << unpackBE64(curPrint + WHODUN_SUFFIX_ENTRY_OFF_STRING) << " ";
		std::cout << unpackBE64(curPrint + WHODUN_SUFFIX_ENTRY_OFF_CHAR) << " ";
		std::cout << unpackBE64(curPrint + WHODUN_SUFFIX_ENTRY_OFF_LCP) << " ";
		std::cout << unpackBE64(curPrint + WHODUN_SUFFIX_ENTRY_OFF_RANK) << " ";
		std::cout << unpackBE64(curPrint + WHODUN_SUFFIX_ENTRY_OFF_NEXT) << std::endl;
		curPrint += WHODUN_SUFFIX_ENTRY_SIZE;
	}
}
void InMemorySuffixArrayBuilder::build(uintptr_t numOn, SizePtrString* buildOn, InMemorySuffixArray* toStore){
	//if not in the vector, save
	if((allStrings.size() != numOn) || (allStrings.size() && (buildOn != &(allStrings[0])))){
		allStrings.clear();
		allStrings.insert(allStrings.end(), buildOn, buildOn + numOn);
	}
	toStore->numEntries = 0;
	if(numOn == 0){ return; }
	//make the initial array (and sort it)
		allStringC0.clear();
		uintmax_t numEntries = 0;
		uintptr_t maxLen = 0;
		for(uintptr_t i = 0; i<numOn; i++){
			allStringC0.push_back(numEntries);
			numEntries += buildOn[i].len;
			maxLen = std::max(maxLen, buildOn[i].len);
		}
		toStore->numEntries = numEntries;
		if(numEntries == 0){ return; }
		if(numEntries > toStore->numAlloc){
			if(toStore->allocBuff){ free(toStore->allocBuff); }
			toStore->numAlloc = toStore->numEntries;
			toStore->allocBuff = (char*)malloc(WHODUN_SUFFIX_ENTRY_SIZE*toStore->numAlloc);
		}
		if(numEntries > allocInverse){
			if(inverseBuff){ free(inverseBuff); }
			allocInverse = numEntries;
			inverseBuff = (uintmax_t*)malloc(allocInverse * sizeof(uintmax_t));
		}
		uintptr_t numPT = numEntries / numThread;
		uintptr_t numET = numEntries % numThread;
		uintptr_t curChar = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			SuffixArrayUniform* curU = &(threadUnis[i]);
			uintptr_t curNum = numPT + (i < numET);
			curU->phase = 1;
			curU->p1SetLoc = toStore->allocBuff + curChar*WHODUN_SUFFIX_ENTRY_SIZE;
			curU->p1NumFill = curNum;
			curU->p1FillWith = &(allStrings[0]);
			uintptr_t winStrI = std::lower_bound(allStringC0.begin(), allStringC0.end(), curChar) - allStringC0.begin();
				winStrI = std::min(winStrI, (uintptr_t)(allStringC0.size()-1));
				if(allStringC0[winStrI] > curChar){ winStrI--; }
			curU->p1StartS = winStrI;
			curU->p1StartC = (curChar - allStringC0[winStrI]);
			curChar += curNum;
			//prepare for future work
			curU->p4StrOffset = &(allStringC0[0]);
			curU->p4InvFill = inverseBuff;
			curU->p4BaseLoc = toStore->allocBuff;
			curU->p5OrigStrs = &(allStrings[0]);
			curU->p6NumEntry = numEntries;
		}
		#define RUN_TASKS \
			if(usePool){\
				uintptr_t taskID = usePool->addTasks(numThread, &(passUnis[0]));\
				usePool->joinTasks(taskID, taskID + numThread);\
			}\
			else{\
				passUnis[0]->doIt();\
			}
		RUN_TASKS
		sortMethod.useOpts = soptsRank;
		sortMethod.sort(numEntries, toStore->allocBuff);
//		debugCheckSortedToK(2, numOn, buildOn, toStore);
	//rerank and resort to get the array
		for(uintptr_t k = 4; k < 2*maxLen; k = k<<1){
//			std::cout << k << " / " << maxLen << std::endl;
			//first pass: figure breaks between threads and rank within
			curChar = numPT + (numET != 0);
			rankChunkBreak[0] = true;
			threadUnis[0].phase = 2;
			for(uintptr_t i = 1; i<numThread; i++){
				threadUnis[i].phase = 2;
				uintptr_t curNum = numPT + (i < numET);
				if(curNum == 0){ rankChunkBreak[i] = false; continue; }
				char* curFoc = toStore->allocBuff + curChar * WHODUN_SUFFIX_ENTRY_SIZE;
				char* prevFoc = curFoc - WHODUN_SUFFIX_ENTRY_SIZE;
				rankChunkBreak[i] = (unpackBE64(curFoc + WHODUN_SUFFIX_ENTRY_OFF_RANK) != unpackBE64(prevFoc + WHODUN_SUFFIX_ENTRY_OFF_RANK)) || (unpackBE64(curFoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT) != unpackBE64(prevFoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT));
				curChar += curNum;
			}
			RUN_TASKS
			//second pass: offset each thread
			uintmax_t curOffset = 0;
			for(uintptr_t i = 0; i<numThread; i++){
				if(rankChunkBreak[i]){ curOffset++; }
				SuffixArrayUniform* curU = &(threadUnis[i]);
				curU->phase = 3;
				curU->p3Offset = curOffset;
				curOffset += curU->p2MaxRank;
			}
			RUN_TASKS
			//get the inverse suffix array
			for(uintptr_t i = 0; i<numThread; i++){
				threadUnis[i].phase = 4;
			}
			RUN_TASKS
			//test inverse build
//			uintptr_t invCurIndex = 0;
//			for(uintptr_t i = 0; i<numOn; i++){
//				SizePtrString curStr = buildOn[i];
//				for(uintptr_t j = 0; j<curStr.len; j++){
//					uintmax_t curSAInd = inverseBuff[invCurIndex];
//					char* curSAFoc = toStore->allocBuff + curSAInd*WHODUN_SUFFIX_ENTRY_SIZE;
//					if(unpackBE64(curSAFoc + WHODUN_SUFFIX_ENTRY_OFF_STRING) != i){
//						raise(SIGSEGV);
//					}
//					if(unpackBE64(curSAFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR) != j){
//						raise(SIGSEGV);
//					}
//					invCurIndex++;
//				}
//			}
			//get next rank
			uintptr_t skipLen = k >> 1;
			for(uintptr_t i = 0; i<numThread; i++){
				SuffixArrayUniform* curU = &(threadUnis[i]);
				curU->phase = 5;
				curU->p5SkipLen = skipLen;
			}
			RUN_TASKS
			//check next rank
//			invCurIndex = 0;
//			for(uintptr_t i = 0; i<numOn; i++){
//				SizePtrString curStr = buildOn[i];
//				for(uintptr_t j = 0; j<curStr.len; j++){
//					uintmax_t curSAInd = inverseBuff[invCurIndex];
//					char* curSAFoc = toStore->allocBuff + curSAInd*WHODUN_SUFFIX_ENTRY_SIZE;
//					uintmax_t checkInd = unpackBE64(curSAFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR) + skipLen;
//					uintmax_t wantNR = 0;
//					if(checkInd < curStr.len){
//						uintmax_t nextSAInd = inverseBuff[invCurIndex + skipLen];
//						char* nextSAFoc = toStore->allocBuff + nextSAInd*WHODUN_SUFFIX_ENTRY_SIZE;
//						wantNR = unpackBE64(nextSAFoc + WHODUN_SUFFIX_ENTRY_OFF_RANK);
//					}
//					if(unpackBE64(curSAFoc + WHODUN_SUFFIX_ENTRY_OFF_NEXT) != wantNR){
//						raise(SIGSEGV);
//					}
//					invCurIndex++;
//				}
//			}
			//sort by rank/next-rank
			sortMethod.useOpts = soptsRank;
			sortMethod.sort(numEntries, toStore->allocBuff);
			//see if its sorted up to k
//			debugCheckSortedToK(k, numOn, buildOn, toStore);
		}
	//fill in the inverse suffix array
		for(uintptr_t i = 0; i<numThread; i++){
			threadUnis[i].phase = 4;
		}
		RUN_TASKS
		for(uintptr_t i = 0; i<numThread; i++){
			threadUnis[i].phase = 6;
		}
		RUN_TASKS
	//debug_outputInternalSuffixArray(toStore->allocBuff, numEntries);
}
void InMemorySuffixArrayBuilder::debugCheckSortedToK(uintptr_t kCheck, uintptr_t numOn, SizePtrString* buildOn, InMemorySuffixArray* toStore){
	char* prevFoc = toStore->allocBuff;
	for(uintptr_t i = 1; i<toStore->numEntries; i++){
		SizePtrString prevStr = buildOn[unpackBE64(prevFoc)];
		uintptr_t prevChrI = unpackBE64(prevFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
		char* curFoc = prevFoc + WHODUN_SUFFIX_ENTRY_SIZE;
		SizePtrString curStr = buildOn[unpackBE64(curFoc)];
		uintptr_t curChrI = unpackBE64(curFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
		uintptr_t maxComp = std::min(prevStr.len - prevChrI, curStr.len - curChrI);
			maxComp = std::min(maxComp, kCheck);
		int dirCompR = memcmp(prevStr.txt + prevChrI, curStr.txt + curChrI, maxComp);
		if(dirCompR > 0){
			raise(SIGSEGV);
		}
		if((dirCompR == 0) && (maxComp < kCheck) && ((prevStr.len-prevChrI) > (curStr.len-curChrI))){
			raise(SIGSEGV);
		}
		prevFoc = curFoc;
	}
}

/*
		for(uintptr_t i = 0; i<numEntries; i++){
			uintmax_t curSAInd = inverseBuff[i];
			char* curSAFoc = toStore->allocBuff + curSAInd*WHODUN_SUFFIX_ENTRY_SIZE;;
			if(unpackBE64(curSAFoc + WHODUN_SUFFIX_ENTRY_OFF_CHAR) != i){
				raise(SIGSEGV);
			}
		}
*/

#define WHODUN_OOM_BLOCKSIZE 0x000400

#define LOAD_CHECK_RESCALE(countVar, fileName) \
	if(countVar % WHODUN_OOM_SUFFIX_ENTRY_SIZE){ std::string errMess("Malformed suffix array file: "); errMess.append(fileName); throw std::runtime_error(errMess); }\
	countVar = countVar / WHODUN_OOM_SUFFIX_ENTRY_SIZE;

ExternalSuffixArray::ExternalSuffixArray(const char* baseFile, const char* blockFile){
	GZipCompressionFactory compMeth;
	nextEntry = 0;
	myStr = new BlockCompInStream(baseFile, blockFile, &compMeth);
	numEntries = myStr->size();
	LOAD_CHECK_RESCALE(numEntries, baseFile)
}
ExternalSuffixArray::~ExternalSuffixArray(){
	try{
		myStr->close();
	}
	catch(std::exception& errE){
		std::cerr << "Closing a file open for read threw an exception?" << std::endl;
		std::cerr << errE.what() << std::endl;
	}
	delete(myStr);
}
uintmax_t ExternalSuffixArray::numEntry(){
	return numEntries;
}
void ExternalSuffixArray::getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc){
	if(entInd != nextEntry){
		myStr->seek(entInd * WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	}
	char workBuff[WHODUN_OOM_SUFFIX_ENTRY_SIZE];
	myStr->forceReadBytes(workBuff, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	storeLoc->stringInd = unpackBE64(workBuff + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	storeLoc->charInd = unpackBE64(workBuff + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	storeLoc->lcpNext = unpackBE64(workBuff + WHODUN_SUFFIX_ENTRY_OFF_LCP);
	nextEntry = entInd + 1;
}

ExternalSuffixArrayMerger::ExternalSuffixArrayMerger(uintptr_t maxBytes){
	numThread = 1;
	usePool = 0;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<threadUnis.size(); i++){ passUnis.push_back(&(threadUnis[i])); }
	maxLoadBank = maxBytes / (4*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	maxLoadBank = std::max(maxLoadBank, (uintptr_t)2);
	loadBankA = (char*)malloc((4*maxLoadBank + 1)*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	loadBankB = loadBankA + maxLoadBank*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
	mergeBank = loadBankB + maxLoadBank*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
}
ExternalSuffixArrayMerger::ExternalSuffixArrayMerger(uintptr_t maxBytes, uintptr_t numThr, ThreadPool* mainPool){
	numThread = numThr;
	usePool = mainPool;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<threadUnis.size(); i++){ passUnis.push_back(&(threadUnis[i])); }
	maxLoadBank = maxBytes / (4*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	maxLoadBank = std::max(maxLoadBank, (uintptr_t)2);
	loadBankA = (char*)malloc((4*maxLoadBank + 1)*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	loadBankB = loadBankA + maxLoadBank*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
	mergeBank = loadBankB + maxLoadBank*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
}
ExternalSuffixArrayMerger::~ExternalSuffixArrayMerger(){
	free(loadBankA);
}

void ExternalSuffixArrayMerger::mergeArrays(ExternalSuffixMergeOptions* opts, BulkStringSource* useCompare){
	InStream* suffA = 0;
	InStream* suffB = 0;
	OutStream* suffO = 0;
	try{
		GZipCompressionFactory compMeth;
		suffA = usePool ? (InStream*)new MultithreadBlockCompInStream(opts->baseFA, opts->blockFA, &compMeth, numThread, usePool) : (InStream*)new BlockCompInStream(opts->baseFA, opts->blockFA, &compMeth);
		suffB = usePool ? (InStream*)new MultithreadBlockCompInStream(opts->baseFB, opts->blockFB, &compMeth, numThread, usePool) : (InStream*)new BlockCompInStream(opts->baseFB, opts->blockFB, &compMeth);
		suffO = usePool ? (OutStream*)new MultithreadBlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, opts->baseOut, opts->blockOut, &compMeth, numThread, usePool) : (OutStream*)new BlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, opts->baseOut, opts->blockOut, &compMeth);
		
		SuffixArrayInternalCompare doComps(useCompare);
		SortOptions saStrFOpts(&doComps);
		int stillMoreA = 1;
		uintptr_t curOffA = maxLoadBank;
		uintptr_t numLoadA = 0;
		int stillMoreB = 1;
		uintptr_t curOffB = maxLoadBank;
		uintptr_t numLoadB = 0;
		uintptr_t numLeftover = 0;
		
		loadMore:
		//repack tail and load from A (remember to offset)
			if(stillMoreA && (numLoadA < curOffA)){
				if(numLoadA){
					uintptr_t numPT = numLoadA / numThread;
					uintptr_t numET = numLoadA % numThread;
					uintptr_t curChar = 0;
					for(uintptr_t i = 0; i<numThread; i++){
						SuffixArrayUniform* curU = &(threadUnis[i]);
						uintptr_t curNum = numPT + (i < numET);
						curU->phase = 7;
						curU->p7NumCopy = curNum*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curU->p7Target = loadBankA + curChar*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curU->p7Source = loadBankA + (curOffA+curChar)*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curChar += curNum;
					}
					RUN_TASKS
				}
				curOffA = 0;
				uintptr_t extLoadA = suffA->readBytes(loadBankA + numLoadA*WHODUN_OOM_SUFFIX_ENTRY_SIZE, WHODUN_OOM_SUFFIX_ENTRY_SIZE*(maxLoadBank - numLoadA));
				LOAD_CHECK_RESCALE(extLoadA, opts->baseFA)
				uintptr_t numPT = extLoadA / numThread;
				uintptr_t numET = extLoadA % numThread;
				uintptr_t curChar = numLoadA;
				for(uintptr_t i = 0; i<numThread; i++){
					SuffixArrayUniform* curU = &(threadUnis[i]);
					uintptr_t curNum = numPT + (i < numET);
					curU->phase = 8;
					curU->p8NumOffset = curNum;
					curU->p8Entries = loadBankA + curChar*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					curU->p8Offset = opts->offsetA;
					curChar += curNum;
				}
				RUN_TASKS
				numLoadA += extLoadA;
			}
		//repack tail and load from B (remember to offset)
			if(stillMoreB && (numLoadB < curOffB)){
				if(numLoadB){
					uintptr_t numPT = numLoadB / numThread;
					uintptr_t numET = numLoadB % numThread;
					uintptr_t curChar = 0;
					for(uintptr_t i = 0; i<numThread; i++){
						SuffixArrayUniform* curU = &(threadUnis[i]);
						uintptr_t curNum = numPT + (i < numET);
						curU->phase = 7;
						curU->p7NumCopy = curNum*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curU->p7Target = loadBankB + curChar*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curU->p7Source = loadBankB + (curOffB+curChar)*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
						curChar += curNum;
					}
					RUN_TASKS
				}
				curOffB = 0;
				uintptr_t extLoadB = suffB->readBytes(loadBankB + numLoadB*WHODUN_OOM_SUFFIX_ENTRY_SIZE, WHODUN_OOM_SUFFIX_ENTRY_SIZE*(maxLoadBank - numLoadB));
				LOAD_CHECK_RESCALE(extLoadB, opts->baseFB)
				uintptr_t numPT = extLoadB / numThread;
				uintptr_t numET = extLoadB % numThread;
				uintptr_t curChar = numLoadB;
				for(uintptr_t i = 0; i<numThread; i++){
					SuffixArrayUniform* curU = &(threadUnis[i]);
					uintptr_t curNum = numPT + (i < numET);
					curU->phase = 8;
					curU->p8NumOffset = curNum;
					curU->p8Entries = loadBankB + curChar*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
					curU->p8Offset = opts->offsetB;
					curChar += curNum;
				}
				RUN_TASKS
				numLoadB += extLoadB;
			}
		//if both empty, quit
			if(!numLoadA && !numLoadB){
				if(numLeftover){
					packBE64(0, mergeBank + WHODUN_SUFFIX_ENTRY_OFF_LCP);
					suffO->writeBytes(mergeBank, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
				}
				suffO->close(); delete(suffO); suffO = 0;
				suffA->close(); delete(suffA); suffA = 0;
				suffB->close(); delete(suffB); suffB = 0;
				return;
			}
		//if either empty, dump the other and continue
			char* firstElemA = loadBankA + curOffA*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			char* firstElemB = loadBankB + curOffB*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			if(numLoadA == 0){
				if(numLeftover){
					packBE64(doComps.compare(mergeBank, firstElemB, 0).second, mergeBank + WHODUN_SUFFIX_ENTRY_OFF_LCP);
					suffO->writeBytes(mergeBank, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					numLeftover = 0;
				}
				stillMoreA = 0;
				suffO->writeBytes(firstElemB, numLoadB*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
				curOffB = 0;
				numLoadB = 0;
				goto loadMore;
			}
			if(numLoadB == 0){
				if(numLeftover){
					packBE64(doComps.compare(mergeBank, firstElemA, 0).second, mergeBank + WHODUN_SUFFIX_ENTRY_OFF_LCP);
					suffO->writeBytes(mergeBank, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
					numLeftover = 0;
				}
				stillMoreB = 0;
				suffO->writeBytes(firstElemA, numLoadA*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
				curOffA = 0;
				numLoadA = 0;
				goto loadMore;
			}
		//limit at the ends
			uintptr_t maxDoA = (chunkyUpperBound(numLoadA, firstElemA, firstElemB + (numLoadB-1)*WHODUN_OOM_SUFFIX_ENTRY_SIZE, &saStrFOpts) - firstElemA)/WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			uintptr_t maxDoB = (chunkyUpperBound(numLoadB, firstElemB, firstElemA + (numLoadA-1)*WHODUN_OOM_SUFFIX_ENTRY_SIZE, &saStrFOpts) - firstElemB)/WHODUN_OOM_SUFFIX_ENTRY_SIZE;
			uintptr_t maxDoC = maxDoA + maxDoB;
		//merge
			uintptr_t numPT = maxDoC / numThread;
			uintptr_t numET = maxDoC % numThread;
			uintptr_t curChar = 0;
			for(uintptr_t i = 0; i<numThread; i++){
				SuffixArrayUniform* curU = &(threadUnis[i]);
				uintptr_t curNum = numPT + (i < numET);
				curU->phase = 9;
				curU->p9FullNumA = maxDoA;
				curU->p9EntsA = firstElemA;
				curU->p9FullNumB = maxDoB;
				curU->p9EntsB = firstElemB;
				curU->p9EntsDump = mergeBank + (numLeftover+curChar)*WHODUN_OOM_SUFFIX_ENTRY_SIZE;
				curU->p9Compare = useCompare;
				curU->p9QuantileLow = curChar;
				curChar += curNum;
				curU->p9QuantileHigh = curChar;
			}
			RUN_TASKS
		//link LCP between the merged pieces
			if(numLeftover){
				packBE64(doComps.compare(mergeBank, threadUnis[0].p9EntsDump, 0).second, mergeBank + WHODUN_SUFFIX_ENTRY_OFF_LCP);
			}
			for(uintptr_t i = 1; i<numThread; i++){
				SuffixArrayUniform* curU = &(threadUnis[i]);
				if(curU->p9QuantileLow == curU->p9QuantileHigh){ continue; }
				char* prevEnt = curU->p9EntsDump - WHODUN_OOM_SUFFIX_ENTRY_SIZE;
				packBE64(doComps.compare(prevEnt, curU->p9EntsDump, 0).second, prevEnt + WHODUN_SUFFIX_ENTRY_OFF_LCP);
			}
		//output all but the last one
			uintptr_t numDump = numLeftover + (maxDoC - 1);
			suffO->writeBytes(mergeBank, numDump*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			curOffA += maxDoA;
			curOffB += maxDoB;
			numLoadA -= maxDoA;
			numLoadB -= maxDoB;
			numLeftover = 1;
			memcpy(mergeBank, mergeBank + numDump*WHODUN_OOM_SUFFIX_ENTRY_SIZE, WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			goto loadMore;
	}catch(std::exception& errE){
		if(suffA){ suffA->close(); delete(suffA); }
		if(suffB){ suffB->close(); delete(suffB); }
		if(suffO){ suffO->close(); delete(suffO); }
		throw;
	}
}

#define MARSHALL_PER_THREAD 4096
ExternalSuffixArrayBuilder::ExternalSuffixArrayBuilder(const char* workingFolder, BulkStringSource* useCompare){
	workFold = workingFolder;
	numThread = 1;
	usePool = 0;
	useComp = useCompare;
	totNumString = 0;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){ passUnis.push_back(&(threadUnis[i])); }
	numTemps = 0;
	madeTemp = 0;
	if(!directoryExists(workingFolder)){
		if(directoryCreate(workingFolder)){ throw std::runtime_error("Problem creating " + workFold); }
		madeTemp = 1;
	}
	marshalBuff = (char*)malloc(MARSHALL_PER_THREAD*numThread*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	maxByteLoad = 0x0100000*WHODUN_OOM_SUFFIX_ENTRY_SIZE*numThread;
}
ExternalSuffixArrayBuilder::ExternalSuffixArrayBuilder(const char* workingFolder, BulkStringSource* useCompare, uintptr_t numThr, ThreadPool* mainPool){
	workFold = workingFolder;
	numThread = numThr;
	usePool = mainPool;
	useComp = useCompare;
	totNumString = 0;
	threadUnis.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){ passUnis.push_back(&(threadUnis[i])); }
	numTemps = 0;
	madeTemp = 0;
	if(!directoryExists(workingFolder)){
		if(directoryCreate(workingFolder)){ throw std::runtime_error("Problem creating " + workFold); }
		madeTemp = 1;
	}
	marshalBuff = (char*)malloc(MARSHALL_PER_THREAD*numThread*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
	maxByteLoad = 0x0100000*WHODUN_OOM_SUFFIX_ENTRY_SIZE*numThread;
}
ExternalSuffixArrayBuilder::~ExternalSuffixArrayBuilder(){
	for(uintptr_t i = 0; i<allTempBase.size(); i++){
		fileKill(allTempBase[i].c_str());
		fileKill(allTempBlock[i].c_str());
	}
	if(madeTemp){ directoryKill(workFold.c_str()); }
	free(marshalBuff);
}
void ExternalSuffixArrayBuilder::addSuffixArray(uintmax_t numString, InMemorySuffixArray* toAdd){
	GZipCompressionFactory compMeth;
	std::string baseN; newTempName(&baseN);
	std::string blockN = baseN; blockN.append(".blk");
	allTempBase.push_back(baseN);
	allTempBlock.push_back(blockN);
	//allTempOffset.push_back(totNumString);
	allTempOffset.push_back(0);
	//make the output
	OutStream* toDump;
	if(usePool){
		toDump = new MultithreadBlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, baseN.c_str(), blockN.c_str(), &compMeth, numThread, usePool);
	}
	else{
		toDump = new BlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, baseN.c_str(), blockN.c_str(), &compMeth);
	}
	try{
		char* curDump = toAdd->allocBuff;
		uintptr_t numLeft = toAdd->numEntries;
		while(numLeft){
			char* curTgt = marshalBuff;
			uintptr_t curEat = std::min(numLeft, MARSHALL_PER_THREAD*numThread);
			uintptr_t numPT = curEat / numThread;
			uintptr_t numET = curEat % numThread;
			for(uintptr_t i = 0; i<numThread; i++){
				SuffixArrayUniform* curU = &(threadUnis[i]);
				curU->phase = 11;
				uintptr_t curNum = numPT + (i<numET);
				curU->p11NumPack = curNum;
				curU->p11PackSrc = curDump;
				curU->p11PackTgt = curTgt;
				curU->p11Offset = totNumString;
				curDump += (curNum*WHODUN_SUFFIX_ENTRY_SIZE);
				curTgt += (curNum*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			}
			RUN_TASKS
			toDump->writeBytes(marshalBuff, curEat*WHODUN_OOM_SUFFIX_ENTRY_SIZE);
			numLeft -= curEat;
		}
		toDump->close();
	}catch(std::exception& errE){
		try{toDump->close();}catch(std::exception& errB){}
		delete(toDump);
		throw;
	}
	delete(toDump);
	totNumString += numString;
}
void ExternalSuffixArrayBuilder::collectSuffixArrays(const char* baseFile, const char* blockFile){
	ExternalSuffixArrayMerger* doMerge = usePool ? new ExternalSuffixArrayMerger(maxByteLoad, numThread, usePool) : new ExternalSuffixArrayMerger(maxByteLoad);
	try{
		ExternalSuffixMergeOptions passOpts;
		std::vector<std::string> nextTempBase;
		std::vector<std::string> nextTempBlock;
		std::vector<uintmax_t> nextTempOffset;
		while(allTempBase.size() > 2){
			uintptr_t i = 0;
			while(i<allTempBase.size()){
				passOpts.baseFA = allTempBase[i].c_str();
				passOpts.blockFA = allTempBlock[i].c_str();
				passOpts.offsetA = allTempOffset[i];
				i++;
				if(i == allTempBase.size()){
					nextTempBase.push_back(allTempBase[i-1]);
					nextTempBlock.push_back(allTempBlock[i-1]);
					nextTempOffset.push_back(passOpts.offsetA);
					continue;
				}
				passOpts.baseFB = allTempBase[i].c_str();
				passOpts.blockFB = allTempBlock[i].c_str();
				passOpts.offsetB = allTempOffset[i];
				std::string baseN; newTempName(&baseN);
				std::string blockN = baseN; blockN.append(".blk");
				passOpts.baseOut = baseN.c_str();
				passOpts.blockOut = blockN.c_str();
				doMerge->mergeArrays(&passOpts, useComp);
				nextTempOffset.push_back(0);
				nextTempBase.push_back(baseN);
				nextTempBlock.push_back(blockN);
				fileKill(passOpts.baseFA);
				fileKill(passOpts.blockFA);
				fileKill(passOpts.baseFB);
				fileKill(passOpts.blockFB);
				i++;
			}
			allTempBase.clear();
			allTempBlock.clear();
			allTempOffset.clear();
			std::swap(allTempBase, nextTempBase);
			std::swap(allTempBlock, nextTempBlock);
			std::swap(allTempOffset, nextTempOffset);
		}
		if(allTempBase.size() == 0){
			GZipCompressionFactory compMeth;
			BlockCompOutStream tmpDump(0, WHODUN_OOM_BLOCKSIZE, baseFile, blockFile, &compMeth);
			tmpDump.close();
		}
		else if(allTempBase.size() == 1){
			GZipCompressionFactory compMeth;
			InStream* fileA = 0;
			OutStream* endDump = 0;
			uintptr_t numLoad = 0x0100000 * numThread;
			char* workLoad = (char*)malloc(numLoad);
			try{
				if(usePool){
					fileA = new MultithreadBlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth, numThread, usePool);
					endDump = new MultithreadBlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, baseFile, blockFile, &compMeth, numThread, usePool);
				}
				else{
					fileA = new BlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth);
					endDump = new BlockCompOutStream(0, WHODUN_OOM_BLOCKSIZE, baseFile, blockFile, &compMeth);
				}
				uintptr_t numRead = fileA->readBytes(workLoad, numLoad);
				while(numRead){
					endDump->writeBytes(workLoad,numRead);
					numRead = fileA->readBytes(workLoad, numLoad);
				}
				fileA->close(); endDump->close();
				delete(fileA); delete(endDump);
			}catch(std::exception& errE){
				if(fileA){ fileA->close(); delete(fileA); }
				if(endDump){ endDump->close(); delete(endDump); }
				free(workLoad);
				throw;
			}
			free(workLoad);
		}
		else{
			passOpts.baseFA = allTempBase[0].c_str();
			passOpts.blockFA = allTempBlock[0].c_str();
			passOpts.offsetA = allTempOffset[0];
			passOpts.baseFB = allTempBase[1].c_str();
			passOpts.blockFB = allTempBlock[1].c_str();
			passOpts.offsetB = allTempOffset[1];
			passOpts.baseOut = baseFile;
			passOpts.blockOut = blockFile;
			doMerge->mergeArrays(&passOpts, useComp);
		}
	}catch(std::exception& errE){
		delete(doMerge);
		throw;
	}
}
void ExternalSuffixArrayBuilder::newTempName(std::string* toFill){
	char asciiBuff[8*sizeof(uintmax_t)+8];
	toFill->append(workFold);
	if(!strendswith(toFill->c_str(), filePathSeparator)){
		toFill->append(filePathSeparator);
	}
	toFill->append("suffix_temp");
	sprintf(asciiBuff, "%ju", (uintmax_t)numTemps);
	toFill->append(asciiBuff);
	numTemps++;
}

#define SUFFIX_BATCH_SIZE 64

SuffixArraySearch::SuffixArraySearch(){
	inArr = 0;
	withStrs = 0;
	charInd = 0;
	fromInd = 0;
	toInd = 0;
}
SuffixArraySearch::SuffixArraySearch(SuffixArray* inArray, BulkStringSource* withStrings){
	inArr = inArray;
	withStrs = withStrings;
	charInd = 0;
	fromInd = 0;
	toInd = inArray->numEntry();
}
void SuffixArraySearch::limitMatch(SizePtrString toStr){
	SizePtrString workBreak = toStr;
	while(workBreak.len > SUFFIX_BATCH_SIZE){
		SizePtrString subBreak = {SUFFIX_BATCH_SIZE, toStr.txt};
		limitMatch(subBreak);
		workBreak.len -= SUFFIX_BATCH_SIZE;
		workBreak.txt += SUFFIX_BATCH_SIZE;
	}
	char laBuff[SUFFIX_BATCH_SIZE];
	//find the lower bound
	uintmax_t lowBndL = fromInd;
	uintmax_t lowBndH = toInd;
	while(lowBndH != lowBndL){
		uintmax_t count = lowBndH - lowBndL;
		uintmax_t step = count / 2;
		uintmax_t midBnd = lowBndL + step;
		SuffixArrayEntry midEnt;
		inArr->getEntry(midBnd, &midEnt);
		uintmax_t strLength = withStrs->stringLength(midEnt.stringInd);
		uintmax_t availLen = strLength - (midEnt.charInd + charInd);
		int isLow = availLen < workBreak.len;
		if(!isLow){
			withStrs->getString(midEnt.stringInd, midEnt.charInd + charInd, midEnt.charInd + charInd + workBreak.len, laBuff);
			isLow = memcmp(laBuff, workBreak.txt, workBreak.len) < 0;
		}
		if(isLow){
			lowBndL = midBnd + 1;
		}
		else{
			lowBndH = midBnd;
		}
	}
	fromInd = lowBndL;
	//find the upper bound
	lowBndL = fromInd;
	lowBndH = toInd;
	while(lowBndH != lowBndL){
		uintmax_t count = lowBndH - lowBndL;
		uintmax_t step = count / 2;
		uintmax_t midBnd = lowBndL + step;
		SuffixArrayEntry midEnt;
		inArr->getEntry(midBnd, &midEnt);
		uintmax_t strLength = withStrs->stringLength(midEnt.stringInd);
		uintmax_t availLen = strLength - (midEnt.charInd + charInd);
		int isLow = availLen < workBreak.len;
		if(!isLow){
			withStrs->getString(midEnt.stringInd, midEnt.charInd + charInd, midEnt.charInd + charInd + workBreak.len, laBuff);
			isLow = memcmp(laBuff, workBreak.txt, workBreak.len) <= 0;
		}
		if(isLow){
			lowBndL = midBnd + 1;
		}
		else{
			lowBndH = midBnd;
		}
	}
	toInd = lowBndL;
	//update the known overlap
	charInd += workBreak.len;
}

bool whodun::operator<(const SuffixArraySearch& searchA, const SuffixArraySearch& searchB){
	if(searchA.inArr != searchB.inArr){
		return searchA.inArr < searchB.inArr;
	}
	if(searchA.withStrs != searchB.withStrs){
		return searchA.withStrs < searchB.withStrs;
	}
	if(searchA.charInd != searchB.charInd){
		return searchA.charInd < searchB.charInd;
	}
	if(searchA.fromInd != searchB.fromInd){
		return searchA.fromInd < searchB.fromInd;
	}
	return searchA.toInd < searchB.toInd;
}

SuffixArrayCache::SuffixArrayCache(uintptr_t maxCacheSize, SuffixArray* baseArray){
	maxCache = maxCacheSize;
	baseArr = baseArray;
}
SuffixArrayCache::~SuffixArrayCache(){}
uintmax_t SuffixArrayCache::numEntry(){
	return baseArr->numEntry();
}
void SuffixArrayCache::getEntry(uintmax_t entInd, SuffixArrayEntry* storeLoc){
	int haveRet = 0;
	cacheLock.lockRead();
		std::map<uintmax_t,SuffixArrayEntry>::iterator findIt = cacheRes.find(entInd);
		if(findIt != cacheRes.end()){
			*storeLoc = findIt->second;
			haveRet = 1;
		}
	cacheLock.unlockRead();
	if(haveRet){ return; }
	cacheLock.lockWrite();
		if(cacheRes.size()*2*sizeof(SuffixArrayEntry) > maxCache){
			cacheRes.clear();
		}
		baseArr->getEntry(entInd, storeLoc);
		cacheRes[entInd] = *storeLoc;
	cacheLock.unlockWrite();
}

SuffixArraySearchCache::SuffixArraySearchCache(uintptr_t maxCacheSize){
	maxCache = maxCacheSize;
}
SuffixArraySearchCache::~SuffixArraySearchCache(){}
SuffixArraySearch SuffixArraySearchCache::updateMatch(SuffixArraySearch prevSearch, char updateWith){
	std::pair<SuffixArraySearch,char> lookFor(prevSearch, updateWith);
	int haveRet = 0;
	SuffixArraySearch toRet;
	cacheLock.lockRead();
		std::map< std::pair<SuffixArraySearch,char>, SuffixArraySearch >::iterator findIt = arraySearchCache.find(lookFor);
		if(findIt != arraySearchCache.end()){
			toRet = findIt->second;
			haveRet = 1;
		}
	cacheLock.unlockRead();
	if(haveRet){
		return toRet;
	}
	SizePtrString curDel = {1, &updateWith};
	toRet = prevSearch;
	cacheLock.lockWrite();
		if(3*sizeof(SuffixArraySearch)*arraySearchCache.size() > maxCache){
			arraySearchCache.clear();
		}
		toRet.limitMatch(curDel);
		arraySearchCache[lookFor] = toRet;
	cacheLock.unlockWrite();
	return toRet;
}

