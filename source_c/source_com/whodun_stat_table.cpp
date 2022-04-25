#include "whodun_stat_table.h"

#include <string.h>

using namespace whodun;

TextTableRow::TextTableRow(){
	numEntries = 0;
	entrySizes = 0;
	curEntries = 0;
	colAlloc = 0;
	entryAlloc = 0;
	entrySizes = 0;
	entryStore = 0;
}
TextTableRow::~TextTableRow(){
	if(entrySizes){ free(entrySizes); }
	if(curEntries){ free(curEntries); }
	if(entryStore){ free(entryStore); }
}
void TextTableRow::ensureCapacity(uintptr_t numCol, uintptr_t numChar){
	if(numCol > colAlloc){
		if(entrySizes){ free(entrySizes); }
		if(curEntries){ free(curEntries); }
		colAlloc = numCol;
		entrySizes = (uintptr_t*)malloc(colAlloc * sizeof(uintptr_t));
		curEntries = (char**)malloc(colAlloc * sizeof(char*));
	}
	if(numChar > entryAlloc){
		if(entryStore){ free(entryStore); }
		entryAlloc = numChar;
		entryStore = (char*)malloc(entryAlloc);
	}
}
void TextTableRow::trim(uintptr_t numChar, const char* theChar){
	for(uintptr_t i = 0; i<numEntries; i++){
		uintptr_t curLen = entrySizes[i];
		char* curEnt = curEntries[i];
		while(curLen){
			if(memchr(theChar, curEnt[0], numChar) == 0){ break; }
			curLen--;
			curEnt++;
		}
		while(curLen){
			if(memchr(theChar, curEnt[curLen-1], numChar) == 0){ break; }
			curLen--;
		}
		entrySizes[i] = curLen;
		curEntries[i] = curEnt;
	}
}
void TextTableRow::trim(){
	trim(4, " \r\t\n");
}
void TextTableRow::clone(TextTableRow* toCopy){
	uintptr_t totS = 0;
	for(uintptr_t i = 0; i<toCopy->numEntries; i++){
		totS += toCopy->entrySizes[i];
	}
	ensureCapacity(toCopy->numEntries, totS);
	numEntries = toCopy->numEntries;
	memcpy(entrySizes, toCopy->entrySizes, numEntries*sizeof(uintptr_t));
	char* curDump = entryStore;
	for(uintptr_t i = 0; i<numEntries; i++){
		curEntries[i] = curDump;
		memcpy(curDump, toCopy->curEntries[i], entrySizes[i]);
		curDump += entrySizes[i];
	}
}

TextTableReader::TextTableReader(){}
TextTableReader::~TextTableReader(){
	for(uintptr_t i = 0; i<cache.size(); i++){
		delete(cache[i]);
	}
}
TextTableRow* TextTableReader::allocate(){
	TextTableRow* toRet;
	if(cache.size()){
		toRet = cache[cache.size()-1];
		cache.pop_back();
		toRet->numEntries = 0;
	}
	else{
		toRet = new TextTableRow();
	}
	return toRet;
}
void TextTableReader::release(TextTableRow* toRet){
	cache.push_back(toRet);
}

TextTableWriter::TextTableWriter(){}
TextTableWriter::~TextTableWriter(){}
void TextTableWriter::write(uintptr_t numWrite, TextTableRow** allOut){
	for(uintptr_t i = 0; i<numWrite; i++){
		write(allOut[i]);
	}
}

void DelimitedTableUniform::doIt(){
	switch(actDo){
		case 0:
			for(uintptr_t bi = 0; bi < numManage; bi++){
				char* asStrStart = bulkStrStart[bi];
				char* asStrEnd = bulkStrEnd[bi];
				TextTableRow* asTTR = *(bulkTTR + bi);
				//unpack row from text
				doSplit.splitOn = colDelim;
				doSplit.splitAll(asStrEnd - asStrStart, asStrStart);
				asTTR->ensureCapacity(doSplit.res.numSplits, asStrEnd - asStrStart);
				asTTR->numEntries = doSplit.res.numSplits;
				char* curColS = asTTR->entryStore;
				if(replaceForw){
					for(uintptr_t i = 0; i<doSplit.res.numSplits; i++){
						asTTR->curEntries[i] = curColS;
						char* curCS = doSplit.res.splitStart[i];
						char* curCE = doSplit.res.splitEnd[i];
						char* curColE = curColS;
						while(curCS != curCE){
							if(*curCS == escapePref){
								curCS++;
								if(curCS == curCE){
									*curColE = escapePref;
									curColE++;
								}
								else{
									short repC = replaceForw[0x00FF & *curCS];
									if(repC > 255){
										*curColE = escapePref;
										curColE++;
									}
									*curColE = repC;
									curCS++;
									curColE++;
								}
							}
							else{
								*curColE = *curCS;
								curColE++;
								curCS++;
							}
						}
						asTTR->entrySizes[i] = (curColE - curColS);
						curColS = curColE;
					}
				}
				else{
					memcpy(curColS, asStrStart, asStrEnd - asStrStart);
					for(uintptr_t i = 0; i<doSplit.res.numSplits; i++){
						char* curCS = doSplit.res.splitStart[i];
						char* curCE = doSplit.res.splitEnd[i];
						asTTR->entrySizes[i] = curCE - curCS;
						asTTR->curEntries[i] = curColS + (curCS - asStrStart);
					}
				}
			}
		break;
		case 1:
		tmpPack.clear();
		for(uintptr_t bi = 0; bi < numManage; bi++){
			TextTableRow* asTTR = *(bulkoTTR + bi);
			//pack row into vector
			if(replaceForw){
				for(uintptr_t i = 0; i<asTTR->numEntries; i++){
					if(i){ tmpPack.push_back(colDelim); }
					uintptr_t curLen = asTTR->entrySizes[i];
					char* curEnt = asTTR->curEntries[i];
					for(uintptr_t j = 0; j<curLen; j++){
						short repC = replaceForw[0x00FF & curEnt[j]];
						if(repC < 256){ tmpPack.push_back(escapePref); }
						tmpPack.push_back(repC);
					}
				}
				tmpPack.push_back(rowDelim);
			}
			else{
				for(uintptr_t i = 0; i<asTTR->numEntries; i++){
					if(i){ tmpPack.push_back(colDelim); }
					uintptr_t curLen = asTTR->entrySizes[i];
					char* curEnt = asTTR->curEntries[i];
					tmpPack.insert(tmpPack.end(), curEnt, curEnt + curLen);
				}
				tmpPack.push_back(rowDelim);
			}
		}
		break;
		case 2:
		if(numManage && tmpPack.size()){
			memcpy(bulkStrStart[0], &(tmpPack[0]), tmpPack.size());
		}
		break;
		default:
			throw std::runtime_error("Da fuq...");
	};
}

#define TSV_PARSE_BUFFER_SIZE 0x010000

DelimitedTableReader::DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom){
	theStr = mainFrom;
	newlineSpl = new CharacterSplitter();
		newlineSpl->splitOn = rowDelim;
	newlineStr = new SplitterInStream(mainFrom, newlineSpl, TSV_PARSE_BUFFER_SIZE);
	useEscapes = 0;
	escapePref = 0;
	numThr = 1;
	usePool = 0;
	saveRD = rowDelim;
	saveCD = colDelim;
	needInitTask = 1;
	for(uintptr_t i = 0; i<256; i++){ replacePack[i] = 256 + i; }
}
DelimitedTableReader::DelimitedTableReader(char rowDelim, char colDelim, InStream* mainFrom, int numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	newlineSpl = new MultithreadCharacterSplitter(numThread, mainPool);
		newlineSpl->splitOn = rowDelim;
	newlineStr = new SplitterInStream(mainFrom, newlineSpl, numThread * TSV_PARSE_BUFFER_SIZE, numThread, mainPool);
	useEscapes = 0;
	escapePref = 0;
	numThr = numThread;
	usePool = mainPool;
	saveRD = rowDelim;
	saveCD = colDelim;
	needInitTask = 1;
	for(uintptr_t i = 0; i<256; i++){ replacePack[i] = 256 + i; }
}
DelimitedTableReader::~DelimitedTableReader(){
	for(uintptr_t i = 0; i<waitEnts.size(); i++){ delete(waitEnts[i]); }
	delete(newlineStr);
	delete(newlineSpl);
}
TextTableRow* DelimitedTableReader::read(){
	if(waitEnts.size()){
		TextTableRow* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	
	//split and allocate
	StringSplitData lastR = newlineStr->read();
	if(lastR.numSplits == 0){
		return 0;
	}
	for(uintptr_t i = 0; i<lastR.numSplits; i++){
		waitEnts.push_back(allocate());
	}
	
	//initialize
	if(needInitTask){
		needInitTask = 0;
		liveTask.resize(numThr);
		passTask.resize(numThr);
		for(uintptr_t i = 0; i<numThr; i++){
			DelimitedTableUniform* curU = &(liveTask[i]);
			passTask[i] = curU;
			curU->actDo = 0;
			curU->escapePref = escapePref;
			curU->replaceForw = useEscapes ? replacePack : 0;
			curU->rowDelim = saveRD;
			curU->colDelim = saveCD;
		}
	}
	
	//set up the uniforms
	uintptr_t numPT = lastR.numSplits / numThr;
	uintptr_t numET = lastR.numSplits % numThr;
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numThr; i++){
		uintptr_t curNum = numPT + (i<numET);
		DelimitedTableUniform* curU = &(liveTask[i]);
		curU->numManage = curNum;
		curU->bulkStrStart = lastR.splitStart + curOff;
		curU->bulkStrEnd = lastR.splitEnd + curOff;
		curU->bulkTTR = (waitEnts.begin() + curOff);
		curOff += curNum;
	}
	
	//run them
	if(usePool){
		uintptr_t taskID = usePool->addTasks(numThr, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(taskID, taskID + numThr);
	}
	else{
		liveTask[0].doIt();
	}
	return read();
}
void DelimitedTableReader::close(){}

DelimitedTableWriter::DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom){
	theStr = mainFrom;
	useEscapes = 0;
	escapePref = 0;
	numThr = 1;
	usePool = 0;
	saveRD = rowDelim;
	saveCD = colDelim;
	needInitTask = 1;
	for(uintptr_t i = 0; i<256; i++){ replacePack[i] = 256 + i; }
	packAlloc = 0;
	packBuffer = 0;
}
DelimitedTableWriter::DelimitedTableWriter(char rowDelim, char colDelim, OutStream* mainFrom, int numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	useEscapes = 0;
	escapePref = 0;
	numThr = numThread;
	usePool = mainPool;
	saveRD = rowDelim;
	saveCD = colDelim;
	needInitTask = 1;
	for(uintptr_t i = 0; i<256; i++){ replacePack[i] = 256 + i; }
	packAlloc = 0;
	packBuffer = 0;
}
DelimitedTableWriter::~DelimitedTableWriter(){
	if(packBuffer){ free(packBuffer); }
}
void DelimitedTableWriter::write(TextTableRow* nextOut){
	write(1, &nextOut);
}
void DelimitedTableWriter::write(uintptr_t numWrite, TextTableRow** allOut){
	//initialize
	if(needInitTask){
		needInitTask = 0;
		liveTask.resize(numThr);
		passTask.resize(numThr);
		packCut.resize(numThr);
		for(uintptr_t i = 0; i<numThr; i++){
			DelimitedTableUniform* curU = &(liveTask[i]);
			passTask[i] = curU;
			curU->escapePref = escapePref;
			curU->replaceForw = useEscapes ? replacePack : 0;
			curU->rowDelim = saveRD;
			curU->colDelim = saveCD;
		}
	}
	//set up the first phase of the pack
	uintptr_t numPT = numWrite / numThr;
	uintptr_t numET = numWrite % numThr;
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numThr; i++){
		uintptr_t curNum = numPT + (i < numET);
		DelimitedTableUniform* curU = &(liveTask[i]);
		curU->actDo = 1;
		curU->numManage = curNum;
		curU->bulkoTTR = allOut + curOff;
		curOff += curNum;
	}
	if(usePool){
		uintptr_t taskID = usePool->addTasks(numThr, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(taskID, taskID + numThr);
	}
	else{ liveTask[0].doIt(); }
	//figure out how big a buffer to make
	uintptr_t endSize = 0;
	for(uintptr_t i = 0; i<numThr; i++){ endSize += liveTask[i].tmpPack.size(); }
	if(endSize > packAlloc){
		if(packBuffer){ free(packBuffer); }
		packAlloc = endSize;
		packBuffer = (char*)malloc(packAlloc);
	}
	curOff = 0;
	for(uintptr_t i = 0; i<numThr; i++){
		packCut[i] = packBuffer + curOff;
		curOff += liveTask[i].tmpPack.size();
	}
	//set up the last phase
	for(uintptr_t i = 0; i<numThr; i++){
		DelimitedTableUniform* curU = &(liveTask[i]);
		curU->actDo = 2;
		curU->bulkStrStart = &(packCut[i]);
	}
	if(usePool){
		uintptr_t taskID = usePool->addTasks(numThr, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(taskID, taskID + numThr);
	}
	else{ liveTask[0].doIt(); }
	//and write
	theStr->writeBytes(packBuffer, endSize);
}
void DelimitedTableWriter::close(){}

TSVTableReader::TSVTableReader(InStream* mainFrom) : DelimitedTableReader('\n','\t',mainFrom){
	useEscapes = 1;
	escapePref = '\\';
	replacePack['n'] = '\n';
	replacePack['t'] = '\t';
	replacePack['r'] = '\r';
	replacePack['\\'] = '\\';
}
TSVTableReader::TSVTableReader(InStream* mainFrom, int numThread, ThreadPool* mainPool) : DelimitedTableReader('\n','\t', mainFrom,numThread,mainPool){
	useEscapes = 1;
	escapePref = '\\';
	replacePack['n'] = '\n';
	replacePack['t'] = '\t';
	replacePack['r'] = '\r';
	replacePack['\\'] = '\\';
}
TSVTableReader::~TSVTableReader(){}

TSVTableWriter::TSVTableWriter(OutStream* mainFrom) : DelimitedTableWriter('\n','\t',mainFrom){
	useEscapes = 1;
	escapePref = '\\';
	replacePack['\n'] = 'n';
	replacePack['\t'] = 't';
	replacePack['\r'] = 'r';
	replacePack['\\'] = '\\';
}
TSVTableWriter::TSVTableWriter(OutStream* mainFrom, int numThread, ThreadPool* mainPool) : DelimitedTableWriter('\n','\t', mainFrom,numThread,mainPool){
	useEscapes = 1;
	escapePref = '\\';
	replacePack['\n'] = 'n';
	replacePack['\t'] = 't';
	replacePack['\r'] = 'r';
	replacePack['\\'] = '\\';
}
TSVTableWriter::~TSVTableWriter(){}

#define BLOCKCOMPTAB_ANNOT_ENTLEN 16
//header annotation is
//int 64 location
//int 32 number of columns
//int 32 number of characters

void BlockCompTextTableUniform::doIt(){
	switch(taskStage){
		case 0:{
			char* myBuf = packFrom + packInd;
			//pack sizes
			for(uintptr_t i = 0; i<toPack->numEntries; i++){
				packBE64(toPack->entrySizes[i], myBuf);
				myBuf += 8;
			}
			//pack strings
			for(uintptr_t i = 0; i<toPack->numEntries; i++){
				memcpy(myBuf, toPack->curEntries[i], toPack->entrySizes[i]);
				myBuf += toPack->entrySizes[i];
			}
		break;}
		case 1:{
			char* myBuf = packFrom + packInd;
			//note the total size, make space
			uintptr_t totSize = 0;
			for(uintptr_t i = 0; i<toPack->numEntries; i++){
				totSize += unpackBE64(myBuf + (i<<3));
			}
			toPack->ensureCapacity(toPack->numEntries, totSize);
			//unpack sizes
			for(uintptr_t i = 0; i<toPack->numEntries; i++){
				toPack->entrySizes[i] = unpackBE64(myBuf);
				myBuf += 8;
			}
			//unpack strings
			uintptr_t curOff = 0;
			for(uintptr_t i = 0; i<toPack->numEntries; i++){
				toPack->curEntries[i] = toPack->entryStore + curOff;
				memcpy(toPack->curEntries[i], myBuf, toPack->entrySizes[i]);
				myBuf += toPack->entrySizes[i];
				curOff += toPack->entrySizes[i];
			}
		break;}
		default:
			throw std::runtime_error("Da fuq?");
	}
}

BlockCompTextTableReader::BlockCompTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool){
	//get the length of the annotation file
	intmax_t numBlocksT = fileGetSize(fileName);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	if(numBlocksT % BLOCKCOMPTAB_ANNOT_ENTLEN){std::string errMess("Malformed annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	numEntry = numBlocksT / BLOCKCOMPTAB_ANNOT_ENTLEN;
	//open it up
	GZipCompressionFactory useComp;
	std::string dataName(fileName);
		dataName.append(".dat");
	std::string blockName(fileName);
		blockName.append(".dat.blk");
	focusInd = 0;
	numThr = numThread;
	usePool = mainPool;
	liveTask.resize(5*numThr);
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		liveTask[i].taskStage = 1;
		passTask.push_back(&(liveTask[i]));
	}
	loadAlloc = 0;
	loadBuffer = 0;
	annotBuffer = (char*)malloc(liveTask.size() * BLOCKCOMPTAB_ANNOT_ENTLEN);
	indStr = new FileInStream(fileName);
	tsvStr = new MultithreadBlockCompInStream(dataName.c_str(), blockName.c_str(), &useComp, numThread, mainPool);
}
BlockCompTextTableReader::~BlockCompTextTableReader(){
	for(uintptr_t i = 0; i<waitEnts.size(); i++){ delete(waitEnts[i]); }
	waitEnts.clear();
	if(loadBuffer){ free(loadBuffer); }
	if(annotBuffer){ free(annotBuffer); }
	delete(tsvStr);
	delete(indStr);
}
TextTableRow* BlockCompTextTableReader::read(){
	if(waitEnts.size()){
		TextTableRow* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	
	if(focusInd >= numEntry){ return 0; }
	uintptr_t numLoad = std::min((numEntry - focusInd), liveTask.size());
	//read in the annotation
	indStr->forceReadBytes(annotBuffer, numLoad * BLOCKCOMPTAB_ANNOT_ENTLEN);
	char* curAnnot = annotBuffer;
	//set up the thread tasks
	uintptr_t totNumLoad = 0;
	for(uintptr_t i = 0; i<numLoad; i++){
		TextTableRow* curEnt = allocate();
		waitEnts.push_back(curEnt);
		BlockCompTextTableUniform* curU = &(liveTask[i]);
		curU->packInd = totNumLoad;
		curU->toPack = curEnt;
		curEnt->numEntries = unpackBE32(curAnnot + 8);
		totNumLoad += 8*curEnt->numEntries + unpackBE32(curAnnot + 12);
		curAnnot += BLOCKCOMPTAB_ANNOT_ENTLEN;
	}
	//reallocate if necessary
	if(totNumLoad > loadAlloc){
		loadAlloc = totNumLoad;
		if(loadBuffer){ free(loadBuffer); }
		loadBuffer = (char*)malloc(loadAlloc);
		for(uintptr_t i = 0; i<liveTask.size(); i++){
			liveTask[i].packFrom = loadBuffer;
		}
	}
	//load
	tsvStr->forceReadBytes(loadBuffer, totNumLoad);
	//run the pack
	if(usePool){
		uintptr_t fromI = usePool->addTasks(numLoad, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(fromI, fromI + numLoad);
	}
	else{
		liveTask[0].doIt();
	}
	//simple final call
	return read();
}
void BlockCompTextTableReader::close(){
	tsvStr->close();
	indStr->close();
}

RandacBlockCompTextTableReader::RandacBlockCompTextTableReader(const char* fileName){
	//get the length of the annotation file
	intmax_t numBlocksT = fileGetSize(fileName);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	if(numBlocksT % BLOCKCOMPTAB_ANNOT_ENTLEN){std::string errMess("Malformed annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	numEntry = numBlocksT / BLOCKCOMPTAB_ANNOT_ENTLEN;
	//open it up
	GZipCompressionFactory useComp;
	std::string dataName(fileName);
		dataName.append(".dat");
	std::string blockName(fileName);
		blockName.append(".dat.blk");
	needSeek = 0;
	focusInd = 0;
	indStr = new FileInStream(fileName);
	tsvStr = new BlockCompInStream(dataName.c_str(), blockName.c_str(), &useComp);
}
RandacBlockCompTextTableReader::~RandacBlockCompTextTableReader(){
	delete(tsvStr);
	delete(indStr);
}
TextTableRow* RandacBlockCompTextTableReader::read(){
	if(focusInd >= numEntry){
		return 0;
	}
	//read the header
	if(needSeek){
		indStr->seek(focusInd * BLOCKCOMPTAB_ANNOT_ENTLEN);
	}
	char indDatBuf[BLOCKCOMPTAB_ANNOT_ENTLEN];
	indStr->forceReadBytes(indDatBuf, BLOCKCOMPTAB_ANNOT_ENTLEN);
	//prepare to read data
	if(needSeek){
		tsvStr->seek(unpackBE64(indDatBuf));
	}
	TextTableRow* toRet = allocate();
	uintptr_t numCol = unpackBE32(indDatBuf + 8);
	uintptr_t numChar = unpackBE32(indDatBuf + 12);
	toRet->ensureCapacity(numCol, std::max(8*numCol, numChar));
	toRet->numEntries = numCol;
	//read the sizes
	tsvStr->forceReadBytes(toRet->entryStore, 8*numCol);
	for(uintptr_t i = 0; i<numCol; i++){
		toRet->entrySizes[i] = unpackBE64(toRet->entryStore + 8*i);
	}
	//read the data
	tsvStr->forceReadBytes(toRet->entryStore, numChar);
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numCol; i++){
		toRet->curEntries[i] = toRet->entryStore + curOff;
		curOff += toRet->entrySizes[i];
	}
	focusInd++;
	return toRet;
}
void RandacBlockCompTextTableReader::close(){
	tsvStr->close();
	indStr->close();
}
uintmax_t RandacBlockCompTextTableReader::getNumEntries(){
	return numEntry;
}
TextTableRow* RandacBlockCompTextTableReader::getEntry(uintmax_t endInd){
	needSeek = 1;
	//read the header
	indStr->seek(endInd * BLOCKCOMPTAB_ANNOT_ENTLEN);
	char indDatBuf[BLOCKCOMPTAB_ANNOT_ENTLEN];
	indStr->forceReadBytes(indDatBuf, BLOCKCOMPTAB_ANNOT_ENTLEN);
	//prepare to read data
	tsvStr->seek(unpackBE64(indDatBuf));
	TextTableRow* toRet = allocate();
	uintptr_t numCol = unpackBE32(indDatBuf + 8);
	uintptr_t numChar = unpackBE32(indDatBuf + 12);
	toRet->ensureCapacity(numCol, std::max(8*numCol, numChar));
	toRet->numEntries = numCol;
	//read the sizes
	tsvStr->forceReadBytes(toRet->entryStore, 8*numCol);
	for(uintptr_t i = 0; i<numCol; i++){
		toRet->entrySizes[i] = unpackBE64(toRet->entryStore + 8*i);
	}
	//read the data
	tsvStr->forceReadBytes(toRet->entryStore, numChar);
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numCol; i++){
		toRet->curEntries[i] = toRet->entryStore + curOff;
		curOff += toRet->entrySizes[i];
	}
	return toRet;
}

BlockCompTextTableWriter::BlockCompTextTableWriter(const char* fileName){
	GZipCompressionFactory useComp;
	std::string dataName(fileName);
		dataName.append(".dat");
	std::string blockName(fileName);
		blockName.append(".dat.blk");
	qualityAlloc = 0;
	qualityBuffer = 0;
	totalOut = 0;
	numThr = 1;
	usePool = 0;
	indStr = new FileOutStream(0, fileName);
	tsvStr = new BlockCompOutStream(0, 1024, dataName.c_str(), blockName.c_str(), &useComp);
	liveTask.resize(numThr);
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		liveTask[i].taskStage = 0;
		passTask.push_back(&(liveTask[i]));
	}
}
BlockCompTextTableWriter::BlockCompTextTableWriter(const char* fileName, int numThread, ThreadPool* mainPool){
	GZipCompressionFactory useComp;
	std::string dataName(fileName);
		dataName.append(".dat");
	std::string blockName(fileName);
		blockName.append(".dat.blk");
	qualityAlloc = 0;
	qualityBuffer = 0;
	totalOut = 0;
	numThr = numThread;
	usePool = mainPool;
	indStr = new FileOutStream(0, fileName);
	tsvStr = new MultithreadBlockCompOutStream(0, 1024, dataName.c_str(), blockName.c_str(), &useComp, numThread, mainPool);
	liveTask.resize(5*numThr);
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		liveTask[i].taskStage = 0;
		passTask.push_back(&(liveTask[i]));
	}
}
BlockCompTextTableWriter::~BlockCompTextTableWriter(){
	if(qualityBuffer){ free(qualityBuffer); }
	delete(tsvStr);
	delete(indStr);
}
void BlockCompTextTableWriter::write(TextTableRow* nextOut){
	write(1, &nextOut);
}
void BlockCompTextTableWriter::write(uintptr_t numWrite, TextTableRow** allOut){
	if(numWrite > liveTask.size()){
		uintptr_t leftW = numWrite;
		TextTableRow** nextW = allOut;
		while(leftW){
			uintptr_t curNumW = std::min(leftW, liveTask.size());
			write(curNumW, nextW);
			leftW -= curNumW;
			nextW += curNumW;
		}
		return;
	}
	
	char indPackBuff[16];
	//figure out how much space is needed (and write out the header stuff)
	uintptr_t totNeedSpace = 0;
	for(uintptr_t i = 0; i<numWrite; i++){
		TextTableRow* curOut = allOut[i];
		packBE64(totalOut, indPackBuff);
		packBE32(curOut->numEntries, indPackBuff + 8);
		uintptr_t txtNeedSpace = 0;
		for(uintptr_t j = 0; j<curOut->numEntries; j++){
			txtNeedSpace += curOut->entrySizes[j];
		}
		packBE32(txtNeedSpace, indPackBuff + 12);
		indStr->writeBytes(indPackBuff, 16);
		liveTask[i].toPack = curOut;
		liveTask[i].packInd = totNeedSpace;
		txtNeedSpace += 8*curOut->numEntries;
		totalOut += txtNeedSpace;
		totNeedSpace += txtNeedSpace;
	}
	//allocate if necessary
	if(totNeedSpace > qualityAlloc){
		if(qualityBuffer){ free(qualityBuffer); }
		qualityAlloc = totNeedSpace;
		qualityBuffer = (char*)malloc(qualityAlloc);
		for(uintptr_t i = 0; i<liveTask.size(); i++){
			liveTask[i].packFrom = qualityBuffer;
		}
	}
	//run the pack
	if(usePool){
		uintptr_t fromI = usePool->addTasks(numWrite, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(fromI, fromI + numWrite);
	}
	else{
		liveTask[0].doIt();
	}
	//dump out
	tsvStr->writeBytes(qualityBuffer, totNeedSpace);
}
void BlockCompTextTableWriter::close(){
	tsvStr->close();
	indStr->close();
}

ExtensionTextTableReader::ExtensionTextTableReader(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new TSVTableReader(baseStrs[0]);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".tsv")){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new TSVTableReader(baseStrs[0]);
			return;
		}
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new RandacBlockCompTextTableReader(fileName);
			return;
		}
		//compressed tsv
		if(strendswith(fileName,".tsv.gz") || strendswith(fileName,".tsv.gzip")){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new TSVTableReader(baseStrs[0]);
			return;
		}
		//tsv is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new TSVTableReader(baseStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionTextTableReader::ExtensionTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new TSVTableReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".tsv")){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new TSVTableReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new BlockCompTextTableReader(fileName, numThread, mainPool);
			return;
		}
		//compressed tsv
		if(strendswith(fileName,".tsv.gz") || strendswith(fileName,".tsv.gzip")){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new TSVTableReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//tsv is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new TSVTableReader(baseStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionTextTableReader::~ExtensionTextTableReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
TextTableRow* ExtensionTextTableReader::read(){
	return wrapStr->read();
}
void ExtensionTextTableReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
TextTableRow* ExtensionTextTableReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionTextTableReader::release(TextTableRow* toRet){
	wrapStr->release(toRet);
}

ExtensionRandacTextTableReader::ExtensionRandacTextTableReader(const char* fileName){
	try{
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new RandacBlockCompTextTableReader(fileName);
			return;
		}
		//there is no default
		throw std::runtime_error("Unknown file type.");
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionRandacTextTableReader::ExtensionRandacTextTableReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new RandacBlockCompTextTableReader(fileName);
			return;
		}
		//there is no default
		throw std::runtime_error("Unknown file type.");
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionRandacTextTableReader::~ExtensionRandacTextTableReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
TextTableRow* ExtensionRandacTextTableReader::read(){
	return wrapStr->read();
}
void ExtensionRandacTextTableReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
TextTableRow* ExtensionRandacTextTableReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionRandacTextTableReader::release(TextTableRow* toRet){
	wrapStr->release(toRet);
}
uintmax_t ExtensionRandacTextTableReader::getNumEntries(){
	return wrapStr->getNumEntries();
}
TextTableRow* ExtensionRandacTextTableReader::getEntry(uintmax_t endInd){
	return wrapStr->getEntry(endInd);
}

ExtensionTextTableWriter::ExtensionTextTableWriter(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new TSVTableWriter(baseStrs[0]);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".tsv")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new TSVTableWriter(baseStrs[0]);
			return;
		}
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new BlockCompTextTableWriter(fileName);
			return;
		}
		//compressed tsv
		if(strendswith(fileName,".tsv.gz") || strendswith(fileName,".tsv.gzip")){
			throw std::runtime_error("Cannot block compress a tsv.");
		}
		//tsv is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new TSVTableWriter(baseStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionTextTableWriter::ExtensionTextTableWriter(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new TSVTableWriter(baseStrs[0], numThread, mainPool);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".tsv")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new TSVTableWriter(baseStrs[0], numThread, mainPool);
			return;
		}
		//block compressed
		if(strendswith(fileName,".bctab")){
			wrapStr = new BlockCompTextTableWriter(fileName, numThread, mainPool);
			return;
		}
		//compressed tsv
		if(strendswith(fileName,".tsv.gz") || strendswith(fileName,".tsv.gzip")){
			throw std::runtime_error("Cannot block compress a tsv.");
		}
		//tsv is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new TSVTableWriter(baseStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionTextTableWriter::~ExtensionTextTableWriter(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
void ExtensionTextTableWriter::write(TextTableRow* nextOut){
	wrapStr->write(nextOut);
}
void ExtensionTextTableWriter::write(uintptr_t numWrite, TextTableRow** allOut){
	wrapStr->write(numWrite, allOut);
}
void ExtensionTextTableWriter::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}

void whodun::getTextTableExtensions(int modeWrite, std::set<std::string>* toFill){
	toFill->insert(".tsv");
	toFill->insert(".bctab");
	if(!modeWrite){
		toFill->insert(".tsv.gz");
		toFill->insert(".tsv.gzip");
	}
}

void whodun::getRandacTextTableExtensions(std::set<std::string>* toFill){
	toFill->insert(".bctab");
}

TextTableCache::TextTableCache(uintptr_t maxCacheSize, RandacTextTableReader* baseArray){
	maxCache = maxCacheSize;
	baseArr = baseArray;
	saveNumEnt = baseArr->getNumEntries();
	curCacheSize = 0;
}
TextTableCache::~TextTableCache(){
	std::map<uintmax_t,TextTableRow*>::iterator cacIt;
	for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
		baseArr->release(cacIt->second);
	}
}
TextTableRow* TextTableCache::read(){
	TextTableRow* toRet;
	cacheLock.lockWrite();
	try{
		toRet = baseArr->read();
	}catch(std::runtime_error& errE){
		cacheLock.unlockWrite(); throw;
	}
	cacheLock.unlockWrite();
	return toRet;
}
void TextTableCache::close(){}
TextTableRow* TextTableCache::allocate(){
	allocLock.lock();
	TextTableRow* toRet = baseArr->allocate();
	allocLock.unlock();
	return toRet;
}
void TextTableCache::release(TextTableRow* toRet){
	allocLock.lock();
	baseArr->release(toRet);
	allocLock.unlock();
}
uintmax_t TextTableCache::getNumEntries(){
	return saveNumEnt;
}
TextTableRow* TextTableCache::getEntry(uintmax_t entInd){
	int haveRet = 0;
	TextTableRow* toRet = allocate();
	
	cacheLock.lockRead();
		std::map<uintmax_t,TextTableRow*>::iterator cacIt = saveSeqs.find(entInd);
		if(cacIt != saveSeqs.end()){
			toRet->clone(cacIt->second);
			haveRet = 1;
		}
	cacheLock.unlockRead();
	if(haveRet){
		return toRet;
	}
	//need to actually load
	cacheLock.lockWrite();
		if(curCacheSize > maxCache){
			//clear
			for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
				baseArr->release(cacIt->second);
			}
			saveSeqs.clear();
			curCacheSize = 0;
		}
		whodun::TextTableRow* baseLoad = baseArr->getEntry(entInd);
		toRet->clone(baseLoad);
		saveSeqs[entInd] = baseLoad;
		for(uintptr_t i = 0; i<baseLoad->numEntries; i++){
			curCacheSize += baseLoad->entrySizes[i];
		}
	cacheLock.unlockWrite();
	return toRet;
}

DataTableDescription::DataTableDescription(){}
DataTableDescription::~DataTableDescription(){}
void DataTableDescription::initialize(){
	rowBytes = 0;
	colOffset.clear();
	for(uintptr_t i = 0; i<colTypes.size(); i++){
		colOffset.push_back(rowBytes);
		switch(colTypes[i]){
			case WHODUN_DATA_CAT:
				rowBytes += 9;
				break;
			case WHODUN_DATA_INT:
				rowBytes += 9;
				break;
			case WHODUN_DATA_REAL:
				rowBytes += 9;
				break;
			case WHODUN_DATA_STR:
				rowBytes += (1 + strLengths[i]);
				break;
			default:
				throw std::runtime_error("Unknown column type.");
		};
	}
}

DataTableRow::DataTableRow(){
	packData = 0;
}
DataTableRow::DataTableRow(DataTableDescription* useD){
	packData = (char*)malloc(useD->rowBytes);
}
DataTableRow::DataTableRow(DataTableDescription* useD, DataTableRow* toCopy){
	packData = (char*)malloc(useD->rowBytes);
	memcpy(packData, toCopy->packData, useD->rowBytes);
}
DataTableRow::~DataTableRow(){
	if(packData){ free(packData); }
}
DataTableEntry DataTableRow::getEntry(uintptr_t entInd, DataTableDescription* useD){
	uintptr_t entOff = useD->colOffset[entInd];
	DataTableEntry toRet;
	toRet.isNA = 0x01 & packData[entOff];
	switch(useD->colTypes[entInd]){
		case WHODUN_DATA_CAT:
			toRet.valC = unpackBE64(packData + entOff + 1);
			break;
		case WHODUN_DATA_INT:
			toRet.valI = unpackBE64(packData + entOff + 1);
			break;
		case WHODUN_DATA_REAL:
			toRet.valR = unpackDbl(unpackBE64(packData + entOff + 1));
			break;
		case WHODUN_DATA_STR:
			toRet.valS = packData + entOff + 1;
			break;
		default:
			throw std::runtime_error("Unknown column type.");
	};
	return toRet;
}
void DataTableRow::setEntry(uintptr_t entInd, DataTableDescription* useD, DataTableEntry toSet){
	uintptr_t entOff = useD->colOffset[entInd];
	packData[entOff] = (toSet.isNA ? 1 : 0);
	switch(useD->colTypes[entInd]){
		case WHODUN_DATA_CAT:
			packBE64(toSet.valC, packData + entOff + 1);
			break;
		case WHODUN_DATA_INT:
			packBE64(toSet.valI, packData + entOff + 1);
			break;
		case WHODUN_DATA_REAL:
			packBE64(packDbl(toSet.valR), packData + entOff + 1);
			break;
		case WHODUN_DATA_STR:
			memcpy(packData + entOff + 1, toSet.valS, useD->strLengths[entInd]);
			break;
		default:
			throw std::runtime_error("Unknown column type.");
	};
}

DataTableReader::DataTableReader(){}
DataTableReader::~DataTableReader(){
	for(uintptr_t i = 0; i<cache.size(); i++){
		delete(cache[i]);
	}
}
void DataTableReader::getDescription(DataTableDescription* toFill){
	*toFill = tabDesc;
}
DataTableRow* DataTableReader::allocate(){
	DataTableRow* toRet;
	if(cache.size()){
		toRet = cache[cache.size()-1];
		cache.pop_back();
	}
	else{
		toRet = new DataTableRow(&tabDesc);
	}
	return toRet;
}
void DataTableReader::release(DataTableRow* toRet){
	cache.push_back(toRet);
}

RandacDataTableReader::RandacDataTableReader(){}

DataTableWriter::DataTableWriter(){}
DataTableWriter::~DataTableWriter(){}
void DataTableWriter::write(uintptr_t numWrite, DataTableRow** allOut){
	for(uintptr_t i = 0; i<numWrite; i++){
		write(allOut[i]);
	}
}

void BinaryDataTableUniform::doIt(){
	switch(actDo){
		case 1:{
			std::deque<DataTableRow*>::iterator curTTR = bulkTTR;
			char* curPack = packLoc;
			for(uintptr_t i = 0; i<numManage; i++){
				DataTableRow* curRow = *curTTR;
				memcpy(curRow->packData, curPack, rowBytes);
				curTTR++;
				curPack += rowBytes;
			}
		break;}
		case 2:{
			DataTableRow** curTTR = bulkoTTR;
			char* curPack = packLoc;
			for(uintptr_t i = 0; i<numManage; i++){
				DataTableRow* curRow = *curTTR;
				memcpy(curPack, curRow->packData, rowBytes);
				curTTR++;
				curPack += rowBytes;
			}
		break;}
		default:
			throw std::runtime_error("Da fuq?");
	};
}

/**
 * Load a description
 * @param theStr The place to load from.
 * @param tabDesc The place to load to.
 */
void whodun_binaryDataTableLoadDesc(InStream* theStr, DataTableDescription* tabDesc){
	char stageB[8];
	std::vector<char> altBuff;
	tabDesc->colTypes.clear();
	tabDesc->colNames.clear();
	tabDesc->factorColMap.clear();
	tabDesc->strLengths.clear();
	//number of columns
	theStr->forceReadBytes(stageB, 8);
	uintptr_t numCol = unpackBE64(stageB);
	tabDesc->colTypes.resize(numCol);
	tabDesc->colNames.resize(numCol);
	tabDesc->factorColMap.resize(numCol);
	tabDesc->strLengths.resize(numCol);
	//columns types
	for(uintptr_t i = 0; i<numCol; i++){
		tabDesc->colTypes[i] = theStr->readByte();
		if(tabDesc->colTypes[i] < 0){ throw std::runtime_error("Truncated file."); }
	}
	//names
	for(uintptr_t i = 0; i<numCol; i++){
		theStr->forceReadBytes(stageB, 8);
		uintptr_t curLen = unpackBE64(stageB);
		altBuff.resize(curLen);
		if(curLen){ theStr->forceReadBytes(&(altBuff[0]), curLen); }
		tabDesc->colNames[i].insert(tabDesc->colNames[i].end(), altBuff.begin(), altBuff.end());
	}
	//categorical names
	for(uintptr_t i = 0; i<numCol; i++){
		if(tabDesc->colTypes[i] != WHODUN_DATA_CAT){ continue; }
		std::map<std::string,int>* curFacs = &(tabDesc->factorColMap[i]);
		theStr->forceReadBytes(stageB, 8);
		uintptr_t numFac = unpackBE64(stageB);
		for(uintptr_t j = 0; j<numFac; j++){
			theStr->forceReadBytes(stageB, 8);
			uintptr_t curLen = unpackBE64(stageB);
			altBuff.resize(curLen);
			if(curLen){ theStr->forceReadBytes(&(altBuff[0]), curLen); }
			theStr->forceReadBytes(stageB, 8);
			uintptr_t facVal = unpackBE64(stageB);
			(*curFacs)[std::string(altBuff.begin(), altBuff.end())] = facVal;
		}
	}
	//string lengths
	for(uintptr_t i = 0; i<numCol; i++){
		if(tabDesc->colTypes[i] != WHODUN_DATA_STR){ continue; }
		theStr->forceReadBytes(stageB, 8);
		tabDesc->strLengths[i] = unpackBE64(stageB);
	}
	tabDesc->initialize();
}

#define BINARY_DATA_BUFFER_SIZE 0x010000

BinaryDataTableReader::BinaryDataTableReader(InStream* mainFrom){
	whodun_binaryDataTableLoadDesc(mainFrom, &tabDesc);
	numThr = 1;
	usePool = 0;
	theStr = mainFrom;
	liveTask.resize(numThr);
	passTask.resize(numThr);
	for(uintptr_t i = 0; i<numThr; i++){
		passTask[i] = &(liveTask[i]);
		liveTask[i].rowBytes = tabDesc.rowBytes;
		liveTask[i].actDo = 1;
	}
	bufferSize = tabDesc.rowBytes*BINARY_DATA_BUFFER_SIZE;
	stageBuff = (char*)malloc(bufferSize);
}
BinaryDataTableReader::BinaryDataTableReader(InStream* mainFrom, int numThread, ThreadPool* mainPool){
	whodun_binaryDataTableLoadDesc(mainFrom, &tabDesc);
	numThr = numThread;
	usePool = mainPool;
	theStr = mainFrom;
	liveTask.resize(numThr);
	passTask.resize(numThr);
	for(uintptr_t i = 0; i<numThr; i++){
		passTask[i] = &(liveTask[i]);
		liveTask[i].rowBytes = tabDesc.rowBytes;
		liveTask[i].actDo = 1;
	}
	bufferSize = tabDesc.rowBytes * numThread * BINARY_DATA_BUFFER_SIZE;
	stageBuff = (char*)malloc(bufferSize);
}
BinaryDataTableReader::~BinaryDataTableReader(){
	free(stageBuff);
}
DataTableRow* BinaryDataTableReader::read(){
	if(waitEnts.size()){
		DataTableRow* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	
	//load in some more
	uintptr_t numRead = theStr->readBytes(stageBuff, bufferSize);
	if(numRead == 0){ return 0; }
	if(numRead % tabDesc.rowBytes){ throw std::runtime_error("Malformed binary table."); }
	uintptr_t numREnt = numRead / tabDesc.rowBytes;
	//set up the entities
	for(uintptr_t i = 0; i<numREnt; i++){
		waitEnts.push_back(allocate());
	}
	//set up the tasks
	uintptr_t numPT = numREnt / numThr;
	uintptr_t numET = numREnt % numThr;
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numThr; i++){
		uintptr_t curNum = numPT + (i < numET);
		BinaryDataTableUniform* curU = &(liveTask[i]);
		curU->numManage = curNum;
		curU->bulkTTR = waitEnts.begin() + curOff;
		curU->packLoc = stageBuff + (curOff * tabDesc.rowBytes);
		curOff += curNum;
	}
	//run the tasks
	if(usePool){
		uintptr_t taskID = usePool->addTasks(numThr, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(taskID, taskID + numThr);
	}
	else{
		liveTask[0].doIt();
	}
	return read();
}
void BinaryDataTableReader::close(){}

RandacBinaryDataTableReader::RandacBinaryDataTableReader(RandaccInStream* mainFrom){
	theStr = mainFrom;
	whodun_binaryDataTableLoadDesc(mainFrom, &tabDesc);
	//note the location, get the number of entries
	baseLoc = mainFrom->tell();
	uintmax_t numBytes = mainFrom->size() - baseLoc;
	if(numBytes % tabDesc.rowBytes){ throw std::runtime_error("Data table file truncated."); }
	numEnts = numBytes / tabDesc.rowBytes;
	nextSpit = 0;
}
RandacBinaryDataTableReader::~RandacBinaryDataTableReader(){}
DataTableRow* RandacBinaryDataTableReader::read(){
	if(nextSpit < numEnts){
		DataTableRow* toRet = getEntry(nextSpit);
		nextSpit++;
		return toRet;
	}
	return 0;
}
void RandacBinaryDataTableReader::close(){}
uintmax_t RandacBinaryDataTableReader::getNumEntries(){
	return numEnts;
}
DataTableRow* RandacBinaryDataTableReader::getEntry(uintmax_t entInd){
	DataTableRow* toRet = allocate();
	theStr->seek(baseLoc + entInd*tabDesc.rowBytes);
	theStr->forceReadBytes(toRet->packData, tabDesc.rowBytes);
	return toRet;
}

/**
 * Write a description
 * @param theStr The place to load from.
 * @param tabDesc The place to load to.
 */
void whodun_binaryDataTableDumpDesc(OutStream* theStr, DataTableDescription* tabDesc){
	char stageB[8];
	//number of columns
	uintptr_t numCol = tabDesc->colTypes.size();
	packBE64(numCol, stageB);
	theStr->writeBytes(stageB, 8);
	//column types
	for(uintptr_t i = 0; i<numCol; i++){
		theStr->writeByte(tabDesc->colTypes[i]);
	}
	//names
	for(uintptr_t i = 0; i<numCol; i++){
		packBE64(tabDesc->colNames[i].size(), stageB);
		theStr->writeBytes(stageB, 8);
		theStr->writeBytes(tabDesc->colNames[i].c_str(), tabDesc->colNames[i].size());
	}
	//categorical names
	for(uintptr_t i = 0; i<numCol; i++){
		if(tabDesc->colTypes[i] != WHODUN_DATA_CAT){ continue; }
		std::map<std::string,int>* curFacs = &(tabDesc->factorColMap[i]);
		packBE64(curFacs->size(), stageB);
		theStr->writeBytes(stageB, 8);
		std::map<std::string,int>::iterator cfIt = curFacs->begin();
		while(cfIt != curFacs->end()){
			packBE64(cfIt->first.size(), stageB);
			theStr->writeBytes(stageB, 8);
			theStr->writeBytes(cfIt->first.c_str(), cfIt->first.size());
			packBE64(cfIt->second, stageB);
			theStr->writeBytes(stageB, 8);
			cfIt++;
		}
	}
	//string lengths
	for(uintptr_t i = 0; i<numCol; i++){
		if(tabDesc->colTypes[i] != WHODUN_DATA_STR){ continue; }
		packBE64(tabDesc->strLengths[i], stageB);
		theStr->writeBytes(stageB, 8);
	}
}

BinaryDataTableWriter::BinaryDataTableWriter(DataTableDescription* forDat, OutStream* mainFrom){
	tabDesc = *forDat;
	tabDesc.initialize();
	whodun_binaryDataTableDumpDesc(mainFrom, &tabDesc);
	numThr = 1;
	usePool = 0;
	theStr = mainFrom;
	liveTask.resize(numThr);
	passTask.resize(numThr);
	for(uintptr_t i = 0; i<numThr; i++){
		passTask[i] = &(liveTask[i]);
		liveTask[i].rowBytes = tabDesc.rowBytes;
		liveTask[i].actDo = 2;
	}
	bufferSize = 0;
	stageBuff = 0;
}
BinaryDataTableWriter::BinaryDataTableWriter(DataTableDescription* forDat, OutStream* mainFrom, int numThread, ThreadPool* mainPool){
	tabDesc = *forDat;
	tabDesc.initialize();
	whodun_binaryDataTableDumpDesc(mainFrom, &tabDesc);
	numThr = numThread;
	usePool = mainPool;
	theStr = mainFrom;
	liveTask.resize(numThr);
	passTask.resize(numThr);
	for(uintptr_t i = 0; i<numThr; i++){
		passTask[i] = &(liveTask[i]);
		liveTask[i].rowBytes = tabDesc.rowBytes;
		liveTask[i].actDo = 2;
	}
	bufferSize = 0;
	stageBuff = 0;
}
BinaryDataTableWriter::~BinaryDataTableWriter(){
	if(stageBuff){ free(stageBuff); }
}
void BinaryDataTableWriter::write(DataTableRow* nextOut){
	write(1, &nextOut);
}
void BinaryDataTableWriter::write(uintptr_t numWrite, DataTableRow** allOut){
	//prepare the buffer
	uintptr_t needBuff = numWrite * tabDesc.rowBytes;
	if(needBuff > bufferSize){
		if(stageBuff){ free(stageBuff); }
		bufferSize = needBuff;
		stageBuff = (char*)malloc(bufferSize);
	}
	//set up the tasks
	uintptr_t numPT = numWrite / numThr;
	uintptr_t numET = numWrite % numThr;
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numThr; i++){
		uintptr_t curNum = numPT + (i < numET);
		BinaryDataTableUniform* curU = &(liveTask[i]);
		curU->numManage = curNum;
		curU->bulkoTTR = allOut + curOff;
		curU->packLoc = stageBuff + (curOff * tabDesc.rowBytes);
		curOff += curNum;
	}
	//run the tasks
	if(usePool){
		uintptr_t taskID = usePool->addTasks(numThr, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(taskID, taskID + numThr);
	}
	else{
		liveTask[0].doIt();
	}
	//and dump the data
	theStr->writeBytes(stageBuff, needBuff);
}
void BinaryDataTableWriter::close(){}

void whodun::getBinaryTableExtensions(int modeWrite, std::set<std::string>* toFill){
	toFill->insert(".bdat");
	if(!modeWrite){
		toFill->insert(".bdat.gz");
		toFill->insert(".bdat.gzip");
	}
}

void whodun::getRandacBinaryTableExtensions(std::set<std::string>* toFill){
	toFill->insert(".bdat");
}

ExtensionDataTableReader::ExtensionDataTableReader(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new BinaryDataTableReader(baseStrs[0]);
			goto endGetter;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new BinaryDataTableReader(baseStrs[0]);
			goto endGetter;
		}
		//compressed bdat
		if(strendswith(fileName,".bdat.gz") || strendswith(fileName,".bdat.gzip")){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new BinaryDataTableReader(baseStrs[0]);
			goto endGetter;
		}
		//bdat is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new BinaryDataTableReader(baseStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
	endGetter:
	wrapStr->getDescription(&tabDesc);
}
ExtensionDataTableReader::ExtensionDataTableReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new BinaryDataTableReader(baseStrs[0], numThread, mainPool);
			goto endGetter;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new BinaryDataTableReader(baseStrs[0], numThread, mainPool);
			goto endGetter;
		}
		//compressed bdat
		if(strendswith(fileName,".bdat.gz") || strendswith(fileName,".bdat.gzip")){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new BinaryDataTableReader(baseStrs[0], numThread, mainPool);
			goto endGetter;
		}
		//bdat is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new BinaryDataTableReader(baseStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
	endGetter:
	wrapStr->getDescription(&tabDesc);
}
ExtensionDataTableReader::~ExtensionDataTableReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
DataTableRow* ExtensionDataTableReader::read(){
	return wrapStr->read();
}
void ExtensionDataTableReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
DataTableRow* ExtensionDataTableReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionDataTableReader::release(DataTableRow* toRet){
	wrapStr->release(toRet);
}

ExtensionRandacDataTableReader::ExtensionRandacDataTableReader(const char* fileName){
	try{
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			RandaccInStream* baseStr = new FileInStream(fileName);
			baseStrs.push_back(baseStr);
			wrapStr = new RandacBinaryDataTableReader(baseStr);
			goto endGetter;
		}
		//bdat is the default
		RandaccInStream* baseStr = new FileInStream(fileName);
		baseStrs.push_back(baseStr);
		wrapStr = new RandacBinaryDataTableReader(baseStr);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
	endGetter:
	wrapStr->getDescription(&tabDesc);
}
ExtensionRandacDataTableReader::ExtensionRandacDataTableReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			RandaccInStream* baseStr = new FileInStream(fileName);
			baseStrs.push_back(baseStr);
			wrapStr = new RandacBinaryDataTableReader(baseStr);
			goto endGetter;
		}
		//bdat is the default
		RandaccInStream* baseStr = new FileInStream(fileName);
		baseStrs.push_back(baseStr);
		wrapStr = new RandacBinaryDataTableReader(baseStr);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
	endGetter:
	wrapStr->getDescription(&tabDesc);
}
ExtensionRandacDataTableReader::~ExtensionRandacDataTableReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
DataTableRow* ExtensionRandacDataTableReader::read(){
	return wrapStr->read();
}
void ExtensionRandacDataTableReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
DataTableRow* ExtensionRandacDataTableReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionRandacDataTableReader::release(DataTableRow* toRet){
	wrapStr->release(toRet);
}
uintmax_t ExtensionRandacDataTableReader::getNumEntries(){
	return wrapStr->getNumEntries();
}
DataTableRow* ExtensionRandacDataTableReader::getEntry(uintmax_t entInd){
	return wrapStr->getEntry(entInd);
}

ExtensionDataTableWriter::ExtensionDataTableWriter(DataTableDescription* forDat, const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0]);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0]);
			return;
		}
		//TODO compressed bdat
		//bdat is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionDataTableWriter::ExtensionDataTableWriter(DataTableDescription* forDat, const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0], numThread, mainPool);
			return;
		}
		//anything explicitly tsv
		if(strendswith(fileName,".bdat")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0], numThread, mainPool);
			return;
		}
		//TODO compressed bdat
		//bdat is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new BinaryDataTableWriter(forDat, baseStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionDataTableWriter::~ExtensionDataTableWriter(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
void ExtensionDataTableWriter::write(DataTableRow* nextOut){
	wrapStr->write(nextOut);
}
void ExtensionDataTableWriter::write(uintptr_t numWrite, DataTableRow** allOut){
	wrapStr->write(numWrite, allOut);
}
void ExtensionDataTableWriter::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}

DataTableCache::DataTableCache(uintptr_t maxCacheSize, RandacDataTableReader* baseArray){
	maxCache = maxCacheSize;
	baseArr = baseArray;
	saveNumEnt = baseArr->getNumEntries();
	baseArr->getDescription(&tabDesc);
}
DataTableCache::~DataTableCache(){
	std::map<uintmax_t,DataTableRow*>::iterator cacIt;
	for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
		baseArr->release(cacIt->second);
	}
}
DataTableRow* DataTableCache::read(){
	DataTableRow* toRet;
	cacheLock.lockWrite();
	try{
		toRet = baseArr->read();
	}catch(std::runtime_error& errE){
		cacheLock.unlockWrite(); throw;
	}
	cacheLock.unlockWrite();
	return toRet;
}
void DataTableCache::close(){}
DataTableRow* DataTableCache::allocate(){
	allocLock.lock();
	DataTableRow* toRet = baseArr->allocate();
	allocLock.unlock();
	return toRet;
}
void DataTableCache::release(DataTableRow* toRet){
	allocLock.lock();
	baseArr->release(toRet);
	allocLock.unlock();
}
uintmax_t DataTableCache::getNumEntries(){
	return saveNumEnt;
}
DataTableRow* DataTableCache::getEntry(uintmax_t entInd){
	int haveRet = 0;
	DataTableRow* toRet = allocate();
	
	cacheLock.lockRead();
		std::map<uintmax_t,DataTableRow*>::iterator cacIt = saveSeqs.find(entInd);
		if(cacIt != saveSeqs.end()){
			memcpy(toRet->packData, cacIt->second->packData, tabDesc.rowBytes);
			haveRet = 1;
		}
	cacheLock.unlockRead();
	if(haveRet){
		return toRet;
	}
	//need to actually load
	cacheLock.lockWrite();
		if(saveSeqs.size()*(tabDesc.rowBytes+sizeof(DataTableRow)+sizeof(uintmax_t)) > maxCache){
			//clear
			for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
				baseArr->release(cacIt->second);
			}
			saveSeqs.clear();
		}
		whodun::DataTableRow* baseLoad = baseArr->getEntry(entInd);
		memcpy(toRet->packData, baseLoad->packData, tabDesc.rowBytes);
		saveSeqs[entInd] = baseLoad;
	cacheLock.unlockWrite();
	return toRet;
}



