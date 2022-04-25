#include "whodun_sort.h"

using namespace whodun;

RawComparator::~RawComparator(){}

/**Go between between function pointer based and classes.*/
bool whodun_comparatorSortFunc(void* useUni, void* itemA, void* itemB){
	return ((RawComparator*)(useUni))->compare(itemA, itemB);
}
SortOptions::SortOptions(){
	compMeth = 0;
	useUni = 0;
	itemSize = 0;
}
SortOptions::SortOptions(uintptr_t itemS, void* useU, bool(*compM)(void*,void*,void*)){
	compMeth = compM;
	useUni = useU;
	itemSize = itemS;
}
SortOptions::SortOptions(RawComparator* useComp){
	compMeth = whodun_comparatorSortFunc;
	useUni = useComp;
	itemSize = useComp->itemSize();
}

/**
 * Performs a small mergesort in memory in a single thread..
 * @param numEnts The entries to merge.
 * @param inMem The data to sort.
 * @param opts Sort options.
 * @param tmpStore Temporary storage (same size as inMem).
 * @return Whether the end result is in tmpStore.
 */
int whodun_inMemoryMergesortSmall(uintptr_t numEnts, char* inMem, SortOptions* opts, char* tmpStore){
	uintptr_t itemSize = opts->itemSize;
	char* curStore = inMem;
	char* nxtStore = tmpStore;
	uintptr_t size = 1;
	while(size < numEnts){
		uintptr_t curBase = 0;
		while(curBase < numEnts){
			char* curFromA = curStore + (curBase*itemSize);
			char* curTo = nxtStore + (curBase*itemSize);
			uintptr_t nxtBase = curBase + size;
			if(nxtBase > numEnts){
				memcpy(curTo, curFromA, (numEnts - curBase)*itemSize);
				break;
			}
			char* curFromB = curStore + (nxtBase*itemSize);
			uintptr_t finBase = std::min(nxtBase + size, numEnts);
			uintptr_t numLeftA = nxtBase - curBase;
			uintptr_t numLeftB = finBase - nxtBase;
			while(numLeftA && numLeftB){
				if(opts->compMeth(opts->useUni, curFromA, curFromB)){
					memcpy(curTo, curFromA, itemSize);
					curTo += itemSize;
					curFromA += itemSize;
					numLeftA--;
				}
				else{
					memcpy(curTo, curFromB, itemSize);
					curTo += itemSize;
					curFromB += itemSize;
					numLeftB--;
				}
			}
			if(numLeftA){ memcpy(curTo, curFromA, numLeftA*itemSize); }
			if(numLeftB){ memcpy(curTo, curFromB, numLeftB*itemSize); }
			curBase = finBase;
		}
		size = (size << 1);
		char* swapSave = curStore;
		curStore = nxtStore;
		nxtStore = swapSave;
	}
	return curStore != inMem;
}

/**Actually do the stuff for mergesorts.*/
class MergesortUniform : public ThreadTask{
public:
	void doIt();
	//TODO
	/**The task to do.*/
	int taskID;
	/**The sorting options.*/
	SortOptions* useOpts;
	
	//phase 0 options (simple sort)
	/**WHether phase 0 should save in main or extra.*/
	int p0NeedMain;
	/**The number of entries to sort*/
	uintptr_t p0NumEntries;
	/**The entries to sort.*/
	char* p0Entries;
	/**Extra space*/
	char* p0Extra;
	
	//phase 1 options (in memory merge)
	/**Whether to look for a quantile.*/
	int p1LookQuantile;
	/**The starting number.*/
	uintptr_t p1StartQuantile;
	/**The ending number.*/
	uintptr_t p1EndQuantile;
	/**The number of things in A*/
	uintptr_t p1NumA;
	/**The number of things in B*/
	uintptr_t p1NumB;
	/**The entries of A.*/
	char* p1EntsA;
	/**The entries of B.*/
	char* p1EntsB;
	/**The place to store.*/
	char* p1EntsRes;
	
	//phase 2 options (memcpy)
	/**The plae to copy from*/
	char* p2Source;
	/**The plae to copy to*/
	char* p2Target;
	/**The amount to copy*/
	uintptr_t p2Number;
};
void MergesortUniform::doIt(){
	bool (*compMeth)(void*,void*,void*) = useOpts->compMeth;
	void* compUni = useOpts->useUni;
	uintptr_t itemSize = useOpts->itemSize;
	switch(taskID){
		case 0:{
			int endOnExt = whodun_inMemoryMergesortSmall(p0NumEntries, p0Entries, useOpts, p0Extra);
			if(p0NeedMain && endOnExt){
				memcpy(p0Entries, p0Extra, itemSize * p0NumEntries);
			}
			else if(!p0NeedMain && !endOnExt){
				memcpy(p0Extra, p0Entries, itemSize * p0NumEntries);
			}
		}break;
		case 1:{
			if(p1LookQuantile){
				std::pair<uintptr_t,uintptr_t> fromInds = chunkyPairedQuantile(p1NumA, p1EntsA, p1NumB, p1EntsB, p1StartQuantile, useOpts);
				std::pair<uintptr_t,uintptr_t> toInds = chunkyPairedQuantile(p1NumA, p1EntsA, p1NumB, p1EntsB, p1EndQuantile, useOpts);
				p1NumA = toInds.first - fromInds.first;
				p1NumB = toInds.second - fromInds.second;
				p1EntsA = p1EntsA + itemSize * fromInds.first;
				p1EntsB = p1EntsB + itemSize * fromInds.second;
			}
			p1StillMoreInBoth:
			uintptr_t maxGo = std::min(p1NumA, p1NumB);
			for(uintptr_t i = 0; i<maxGo; i++){
				if(compMeth(compUni, p1EntsA, p1EntsB)){
					memcpy(p1EntsRes, p1EntsA, itemSize);
					p1EntsA += itemSize;
					p1NumA--;
				}
				else{
					memcpy(p1EntsRes, p1EntsB, itemSize);
					p1EntsB += itemSize;
					p1NumB--;
				}
				p1EntsRes += itemSize;
			}
			if(p1NumA && p1NumB){ goto p1StillMoreInBoth; }
			//drain
			if(p1NumA){
				memcpy(p1EntsRes, p1EntsA, p1NumA * itemSize);
			}
			if(p1NumB){
				memcpy(p1EntsRes, p1EntsB, p1NumB * itemSize);
			}
		}break;
		case 2:{
			memcpy(p2Target, p2Source, p2Number);
		}break;
		default:
			throw std::runtime_error("Da fuq?");
	};
	//TODO
}

InMemoryMergesort::InMemoryMergesort(SortOptions* theOpts){
	useOpts = *theOpts;
	allocSize = 0;
	allocTmp = 0;
	usePool = 0;
	numThread = 1;
	threadUniStore = 0;
}
InMemoryMergesort::InMemoryMergesort(SortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool){
	useOpts = *theOpts;
	allocSize = 0;
	allocTmp = 0;
	usePool = mainPool;
	numThread = numThr;
	std::vector<MergesortUniform>* thrUnis = new std::vector<MergesortUniform>();
	thrUnis->resize(numThr);
	threadUniStore = thrUnis;
	for(uintptr_t i = 0; i<numThr; i++){
		MergesortUniform* curU = &((*thrUnis)[i]);
		curU->useOpts = &useOpts;
		passUnis.push_back(curU);
	}
}
InMemoryMergesort::~InMemoryMergesort(){
	if(allocTmp){ free(allocTmp); }
	std::vector<MergesortUniform>* thrUnis = (std::vector<MergesortUniform>*)threadUniStore;
	if(thrUnis){ delete(thrUnis); }
}
void InMemoryMergesort::sort(uintptr_t numEntries, char* entryStore){
	//test storage allocation
		if(numEntries > allocSize){
			if(allocTmp){ free(allocTmp); }
			allocSize = numEntries;
			allocTmp = (char*)malloc(allocSize * useOpts.itemSize);
		}
	//simple case for a single thread
		if(usePool == 0){
			if(whodun_inMemoryMergesortSmall(numEntries, entryStore, &useOpts, allocTmp)){
				memcpy(entryStore, allocTmp, useOpts.itemSize * numEntries);
			}
			return;
		}
	//figure out whether the initial sort should put things in entryStore or allocTmp
	int p0NeedMain = 1;
	{
		uintptr_t testNumT = numThread;
		int notPow2 = 0;
		while(testNumT > 1){
			notPow2 = notPow2 || (testNumT & 0x01);
			p0NeedMain = !p0NeedMain;
			testNumT = testNumT >> 1;
		}
		if(notPow2){ p0NeedMain = !p0NeedMain; }
	}
	//sort in pieces
	uintptr_t itemSize = useOpts.itemSize;
	std::vector<MergesortUniform>* thrUnis = (std::vector<MergesortUniform>*)threadUniStore;
	{
		uintptr_t numPT = numEntries / numThread;
		uintptr_t numET = numEntries % numThread;
		uintptr_t curOff = 0;
		for(uintptr_t i = 0; i<numThread; i++){
			uintptr_t curNum = numPT + (i < numET);
			MergesortUniform* curU = &((*thrUnis)[i]);
			curU->taskID = 0;
			curU->p0NeedMain = p0NeedMain;
			curU->p0NumEntries = curNum;
			curU->p0Entries = entryStore + curOff;
			curU->p0Extra = allocTmp + curOff;
			curOff += (curNum * itemSize);
			SizePtrString curRes = {curNum * itemSize, p0NeedMain ? curU->p0Entries : curU->p0Extra};
			threadPieces.push_back(curRes);
		}
		uintptr_t taskID = usePool->addTasks(numThread, &(passUnis[0]));
		usePool->joinTasks(taskID, taskID + numThread);
	}
	#define BULK_MEMCPY(fromStr,toLoc) \
		uintptr_t numPT = fromStr.len / numThread;\
		uintptr_t numET = fromStr.len % numThread;\
		uintptr_t curOff = 0;\
		for(uintptr_t ijk = 0; ijk < numThread; ijk++){\
			uintptr_t curNum = numPT + (i < numET);\
			MergesortUniform* curU = &((*thrUnis)[i]);\
			curU->taskID = 2;\
			curU->p2Source = fromStr.txt + curOff;\
			curU->p2Target = toLoc + curOff;\
			curU->p2Number = curNum;\
		}\
		uintptr_t taskID = usePool->addTasks(numThread, &(passUnis[0]));\
		usePool->joinTasks(taskID, taskID + numThread);
	//merge quantiles
		int piecesTmp = !p0NeedMain;
		while(threadPieces.size() > 1){
			uintptr_t numPiece = threadPieces.size();
			char* curDump = piecesTmp ? entryStore : allocTmp;
			uintptr_t i = 0;
			while(i < numPiece){
				SizePtrString curP0 = threadPieces[0]; threadPieces.pop_front(); i++;
				if(i >= numPiece){
					SizePtrString curRes = {curP0.len, curDump};
					BULK_MEMCPY(curRes, curDump);
					threadPieces.push_back(curRes);
					break;
				}
				SizePtrString curP1 = threadPieces[0]; threadPieces.pop_front(); i++;
				uintptr_t numLeftA = curP0.len / itemSize;
				uintptr_t numLeftB = curP1.len / itemSize;
				uintptr_t totalNE = numLeftA + numLeftB;
				uintptr_t elemPT = totalNE / numThread;
				uintptr_t elemET = totalNE % numThread;
				uintptr_t curOff = 0;
				SizePtrString curRes = {curP0.len + curP1.len, curDump};
				threadPieces.push_back(curRes);
				for(uintptr_t j = 0; j<numThread; j++){
					MergesortUniform* curU = &((*thrUnis)[j]);
					uintptr_t curNum = elemPT + (j < elemET);
					curU->taskID = 1;
					curU->p1LookQuantile = 1;
					curU->p1StartQuantile = curOff;
					curOff += curNum;
					curU->p1EndQuantile = curOff;
					curU->p1NumA = numLeftA;
					curU->p1NumB = numLeftB;
					curU->p1EntsA = curP0.txt;
					curU->p1EntsB = curP1.txt;
					curU->p1EntsRes = curDump;
					curDump += (itemSize*curNum);
				}
				uintptr_t taskID = usePool->addTasks(numThread, &(passUnis[0]));
				usePool->joinTasks(taskID, taskID + numThread);
			}
			piecesTmp = !piecesTmp;
		}
		threadPieces.pop_front();
}

char* whodun::chunkyLowerBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	char* first = inMem;
	uintptr_t count = numEnts;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(opts->compMeth(opts->useUni, it, lookFor)){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return first;
}

char* whodun::chunkyUpperBound(uintptr_t numEnts, char* inMem, char* lookFor, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	char* first = inMem;
	uintptr_t count = numEnts;
	while(count){
		uintptr_t step = count / 2;
		char* it = first + (step * itemSize);
		if(!(opts->compMeth(opts->useUni, lookFor, it))){
			first = it + itemSize;
			count = count - (step + 1);
		}
		else{
			count = step;
		}
	}
	return first;
}

std::pair<uintptr_t,uintptr_t> whodun::chunkyPairedQuantile(uintptr_t numEntsA, char* inMemA, uintptr_t numEntsB, char* inMemB, uintptr_t tgtQuantile, SortOptions* opts){
	uintptr_t itemSize = opts->itemSize;
	if(numEntsA){
		//look for upper bound
		uintptr_t fromALow = 0;
		uintptr_t fromAHig = numEntsA;
		while(fromAHig - fromALow){
			uintptr_t fromAMid = fromALow + ((fromAHig - fromALow) >> 1);
			char* midVal = inMemA + itemSize * fromAMid;
			uintptr_t fromBMid = (chunkyLowerBound(numEntsB, inMemB, midVal, opts) - inMemB) / itemSize;
			if((fromAMid + fromBMid) < tgtQuantile){
				fromALow = fromAMid + 1;
			}
			else{
				fromAHig = fromAMid;
			}
		}
		return std::pair<uintptr_t,uintptr_t>(fromALow, tgtQuantile - fromALow);
	}
	else{
		return std::pair<uintptr_t,uintptr_t>(0,tgtQuantile);
	}
}

#define TEMPORARY_BLOCK_SIZE 0x010000

ExternalMergeSort::ExternalMergeSort(const char* workDirName, SortOptions* theOpts){
	maxLoad = 0x040000;
	numTemps = 0;
	tempName = workDirName;
	madeTemp = 0;
	useOpts = *theOpts;
	usePool = 0;
	numThread = 1;
	threadUniStore = 0;
	if(!directoryExists(workDirName)){
		if(directoryCreate(workDirName)){ throw std::runtime_error("Problem creating " + tempName); }
		madeTemp = 1;
	}
}
ExternalMergeSort::ExternalMergeSort(const char* workDirName, SortOptions* theOpts, uintptr_t numThr, ThreadPool* mainPool){
	maxLoad = numThr * 0x040000;
	numTemps = 0;
	tempName = workDirName;
	madeTemp = 0;
	useOpts = *theOpts;
	usePool = mainPool;
	numThread = numThr;
	threadUniStore = 0;
	if(!directoryExists(workDirName)){
		if(directoryCreate(workDirName)){ throw std::runtime_error("Problem creating " + tempName); }
		madeTemp = 1;
	}
	std::vector<MergesortUniform>* thrUnis = new std::vector<MergesortUniform>();
	thrUnis->resize(numThr);
	threadUniStore = thrUnis;
	for(uintptr_t i = 0; i<numThr; i++){
		MergesortUniform* curU = &((*thrUnis)[i]);
		curU->useOpts = &useOpts;
		passUnis.push_back(curU);
	}
}
ExternalMergeSort::~ExternalMergeSort(){
	for(uintptr_t i = 0; i<allTempBase.size(); i++){
		fileKill(allTempBase[i].c_str());
		fileKill(allTempBlock[i].c_str());
	}
	if(madeTemp){ directoryKill(tempName.c_str()); }
	std::vector<MergesortUniform>* thrUnis = (std::vector<MergesortUniform>*)threadUniStore;
	if(thrUnis){ delete(thrUnis); }
}
void ExternalMergeSort::addData(uintptr_t numEntries, char* entryStore){
	GZipCompressionFactory compMeth;
	std::string baseN; newTempName(&baseN);
	std::string blockN = baseN; blockN.append(".blk");
	allTempBase.push_back(baseN);
	allTempBlock.push_back(blockN);
	//make the output
	if(usePool){
		MultithreadBlockCompOutStream dumpF(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth, numThread, usePool);
		try{dumpF.writeBytes(entryStore, numEntries * useOpts.itemSize);}catch(std::exception& errE){dumpF.close(); throw;}
		dumpF.close();
	}
	else{
		BlockCompOutStream dumpF(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth);
		try{dumpF.writeBytes(entryStore, numEntries * useOpts.itemSize);}catch(std::exception& errE){dumpF.close(); throw;}
		dumpF.close();
	}
}
void ExternalMergeSort::mergeData(OutStream* toDump){
	uintptr_t maxLoadEnts = maxLoad / useOpts.itemSize;
		maxLoadEnts = std::max(maxLoadEnts, (uintptr_t)4);
	uintptr_t sinLoadEnts = maxLoadEnts / 4;
	char* loadTempStore = (char*)malloc(maxLoadEnts * useOpts.itemSize);
	GZipCompressionFactory compMeth;
	while(allTempBase.size() > 2){
		std::string baseN; newTempName(&baseN);
		std::string blockN = baseN; blockN.append(".blk");
		OutStream* endDump = 0;
		try{
			if(usePool){
				endDump = new MultithreadBlockCompOutStream(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth, numThread, usePool);
			}
			else{
				endDump = new BlockCompOutStream(0, TEMPORARY_BLOCK_SIZE, baseN.c_str(), blockN.c_str(), &compMeth);
			}
			mergeSingle(loadTempStore, sinLoadEnts, endDump);
			endDump->close();
			delete(endDump);
		}
		catch(std::exception& errE){
			if(endDump){ try{endDump->close();}catch(std::exception& errB){} delete(endDump); }
			fileKill(baseN.c_str()); fileKill(blockN.c_str());
			free(loadTempStore);
			throw;
		}
		allTempBase.push_back(baseN);
		allTempBlock.push_back(blockN);
	}
	try{
		mergeSingle(loadTempStore, sinLoadEnts, toDump);
	}
	catch(std::exception& errE){
		free(loadTempStore); throw;
	}
	free(loadTempStore);
}
void ExternalMergeSort::newTempName(std::string* toFill){
	char asciiBuff[8*sizeof(uintmax_t)+8];
	toFill->append(tempName);
	if(!strendswith(toFill->c_str(), filePathSeparator)){
		toFill->append(filePathSeparator);
	}
	toFill->append("sort_temp");
	sprintf(asciiBuff, "%ju", (uintmax_t)numTemps);
	toFill->append(asciiBuff);
	numTemps++;
}
void ExternalMergeSort::mergeSingle(char* tempStore, uintptr_t loadEnts, OutStream* toDump){
	GZipCompressionFactory compMeth;
	uintptr_t itemSize = useOpts.itemSize;
	std::vector<MergesortUniform>* thrUnis = (std::vector<MergesortUniform>*)threadUniStore;
	MergesortUniform mergeFB;
	MultithreadedStringH* copyHelp = 0;
	InStream* fileA = 0;
	InStream* fileB = 0;
	try{
		if(allTempBase.size() == 0){ return; }
		if(usePool){
			fileA = new MultithreadBlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth, numThread, usePool);
		}
		else{
			fileA = new BlockCompInStream(allTempBase[0].c_str(), allTempBlock[0].c_str(), &compMeth);
		}
		if(allTempBase.size() == 1){
			//just copy the single file
			uintptr_t toReadA = loadEnts*4*itemSize;
			uintptr_t numRead = fileA->readBytes(tempStore, toReadA);
			while(numRead){
				toDump->writeBytes(tempStore, numRead);
				numRead = fileA->readBytes(tempStore, toReadA);
			}
			fileA->close();
			delete(fileA); fileA = 0;
			fileKill(allTempBase[0].c_str());
			fileKill(allTempBlock[0].c_str());
			allTempBase.pop_front();
			allTempBlock.pop_front();
		}
		else{
			//merge the two files
			if(usePool){
				fileB = new MultithreadBlockCompInStream(allTempBase[1].c_str(), allTempBlock[1].c_str(), &compMeth, numThread, usePool);
				copyHelp = new MultithreadedStringH(numThread, usePool);
			}
			else{
				fileB = new BlockCompInStream(allTempBase[1].c_str(), allTempBlock[1].c_str(), &compMeth);
			}
			char* buffA = tempStore;
			char* buffB = buffA + loadEnts*itemSize;
			char* buffR = buffB + loadEnts*itemSize;
			uintptr_t numLoadA = 0;
			uintptr_t numLoadB = 0;
			uintptr_t offsetA = loadEnts;
			uintptr_t offsetB = loadEnts;
			uintptr_t numCLoad;
			while(1){
				if(offsetA > numLoadA){
					//pack and reload
					if(copyHelp){ copyHelp->memcpy(buffA, buffA + offsetA*itemSize, numLoadA*itemSize); }else{ memcpy(buffA, buffA + offsetA*itemSize, numLoadA*itemSize); }
					offsetA = 0;
					numCLoad = fileA->readBytes(buffA + numLoadA*itemSize, (loadEnts - numLoadA)*itemSize);
					if(numCLoad % itemSize){ throw std::runtime_error("Truncated file."); }
					numLoadA += (numCLoad / itemSize);
				}
				if(offsetB > numLoadB){
					//pack and reload
					if(copyHelp){ copyHelp->memcpy(buffB, buffB + offsetB*itemSize, numLoadB*itemSize); }else{ memcpy(buffB, buffB + offsetB*itemSize, numLoadB*itemSize); }
					offsetB = 0;
					numCLoad = fileB->readBytes(buffB + numLoadB*itemSize, (loadEnts - numLoadB)*itemSize);
					if(numCLoad % itemSize){ throw std::runtime_error("Truncated file."); }
					numLoadB += (numCLoad / itemSize);
				}
				//if either empty, go to the flush
				if(numLoadA == 0){ break; }
				if(numLoadB == 0){ break; }
				//note where to stop merging (stop in A after the tail end of B, and vice versa)
				char* stopALoc = chunkyUpperBound(numLoadA, buffA + itemSize*offsetA, buffB + itemSize*(offsetB+numLoadB-1), &useOpts);
				char* stopBLoc = chunkyUpperBound(numLoadB, buffB + itemSize*offsetB, buffA + itemSize*(offsetA+numLoadA-1), &useOpts);
				uintptr_t numMergeA = ((stopALoc - buffA)/itemSize) - offsetA;
				uintptr_t numMergeB = ((stopBLoc - buffB)/itemSize) - offsetB;
				//merge as much as possible and write
				if(usePool){
					uintptr_t elemPT = (numMergeA + numMergeB) / numThread;
					uintptr_t elemET = (numMergeA + numMergeB) % numThread;
					uintptr_t curOff = 0;
					for(uintptr_t j = 0; j<numThread; j++){
						MergesortUniform* curU = &((*thrUnis)[j]);
						uintptr_t curNum = elemPT + (j < elemET);
						curU->taskID = 1;
						curU->p1EntsRes = buffR + itemSize*curOff;
						curU->p1LookQuantile = 1;
						curU->p1StartQuantile = curOff;
						curOff += curNum;
						curU->p1EndQuantile = curOff;
						curU->p1NumA = numMergeA;
						curU->p1NumB = numMergeB;
						curU->p1EntsA = buffA + itemSize*offsetA;
						curU->p1EntsB = buffB + itemSize*offsetB;
					}
					uintptr_t taskID = usePool->addTasks(numThread, &(passUnis[0]));
					usePool->joinTasks(taskID, taskID + numThread);
				}
				else{
					mergeFB.taskID = 1;
					mergeFB.useOpts = &useOpts;
					mergeFB.p1LookQuantile = 0;
					mergeFB.p1NumA = numMergeA;
					mergeFB.p1NumB = numMergeB;
					mergeFB.p1EntsA = buffA + offsetA*itemSize;
					mergeFB.p1EntsB = buffB + offsetB*itemSize;
					mergeFB.p1EntsRes = buffR;
					mergeFB.doIt();
				}
				toDump->writeBytes(buffR, (numMergeA+numMergeB)*itemSize);
				offsetA += numMergeA;
				numLoadA -= numMergeA;
				offsetB += numMergeB;
				numLoadB -= numMergeB;
			}
			//flush
			toDump->writeBytes(buffA + offsetA*itemSize, numLoadA*itemSize);
			toDump->writeBytes(buffB + offsetB*itemSize, numLoadB*itemSize);
			uintptr_t bigLoad = loadEnts*4*itemSize;
			numCLoad = fileA->readBytes(tempStore, bigLoad);
			while(numCLoad){
				toDump->writeBytes(tempStore, numCLoad);
				numCLoad = fileA->readBytes(tempStore, bigLoad);
			}
			numCLoad = fileB->readBytes(tempStore, bigLoad);
			while(numCLoad){
				toDump->writeBytes(tempStore, numCLoad);
				numCLoad = fileB->readBytes(tempStore, bigLoad);
			}
			//and finish
			fileA->close();
			delete(fileA); fileA = 0;
			fileKill(allTempBase[0].c_str());
			fileKill(allTempBlock[0].c_str());
			allTempBase.pop_front();
			allTempBlock.pop_front();
			fileB->close();
			delete(fileB); fileB = 0;
			fileKill(allTempBase[0].c_str());
			fileKill(allTempBlock[0].c_str());
			allTempBase.pop_front();
			allTempBlock.pop_front();
			if(copyHelp){ delete(copyHelp); }
		}
	}
	catch(std::exception& errE){
		if(fileA){ try{fileA->close();}catch(std::exception& errB){} delete(fileA); }
		if(fileB){ try{fileB->close();}catch(std::exception& errB){} delete(fileB); }
		if(copyHelp){ delete(copyHelp); }
		throw;
	}
}

