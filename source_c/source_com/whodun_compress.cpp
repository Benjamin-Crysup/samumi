#include "whodun_compress.h"

#include <string.h>
#include <algorithm>

using namespace whodun;

CompressionMethod::~CompressionMethod(){}

CompressionFactory::~CompressionFactory(){}

RawCompressionMethod::~RawCompressionMethod(){}
void RawCompressionMethod::decompressData(){
	theData = compData;
}
void RawCompressionMethod::compressData(){
	compData = theData;
}
CompressionMethod* RawCompressionFactory::instantiate(){
	return new RawCompressionMethod();
}


GZipCompressionMethod::~GZipCompressionMethod(){}
void GZipCompressionMethod::decompressData(){
	uintptr_t curBuffLen = theData.capacity();
	if(compData.size() > curBuffLen){ curBuffLen = compData.size(); }
	if(1024 > curBuffLen){ curBuffLen = 1024; }
	theData.resize(curBuffLen);
	unsigned long bufEndSStore = curBuffLen;
	int compRes = 0;
	while((compRes = uncompress(((unsigned char*)(&(theData[0]))), &bufEndSStore, ((const unsigned char*)(&(compData[0]))), compData.size())) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw std::runtime_error("Error decompressing gzip data.");
		}
		curBuffLen = curBuffLen << 1;
		theData.resize(curBuffLen);
		bufEndSStore = curBuffLen;
	}
	theData.resize(bufEndSStore);
}
void GZipCompressionMethod::compressData(){
	uintptr_t curBuffLen = compData.capacity();
	if(theData.size() > curBuffLen){ curBuffLen = theData.size(); }
	if(1024 > curBuffLen){ curBuffLen = 1024; }
	compData.resize(curBuffLen);
	unsigned long bufEndSStore = curBuffLen;
	int compRes = 0;
	while((compRes = compress(((unsigned char*)(&(compData[0]))), &bufEndSStore, ((const unsigned char*)(&(theData[0]))), theData.size())) != Z_OK){
		if((compRes == Z_MEM_ERROR) || (compRes == Z_DATA_ERROR)){
			throw std::runtime_error("Error compressing gzip data.");
		}
		curBuffLen = curBuffLen << 1;
		compData.resize(curBuffLen);
		bufEndSStore = curBuffLen;
	}
	compData.resize(bufEndSStore);
}
CompressionMethod* GZipCompressionFactory::instantiate(){
	return new GZipCompressionMethod();
}

void RunLengthCompressionMethod::decompressData(){
	theData.clear();
	uintptr_t i = 0;
	while(i < compData.size()){
		char curControl = compData[i];
		int curLen = curControl & 0x007F;
		if(curControl & 0x0080){
			if(i+1 >= compData.size()){ throw std::runtime_error("Malformed run length encoded data."); }
			theData.insert(theData.end(), curLen, compData[i+1]);
			i += 2;
		}
		else{
			if(i+curLen >= compData.size()){ throw std::runtime_error("Malformed run length encoded data."); }
			std::vector<char>::iterator begIt = compData.begin() + i + 1;
			theData.insert(theData.end(), begIt, begIt + curLen);
			i += (1+curLen);
		}
	}
}
void RunLengthCompressionMethod::compressData(){
	compData.clear();
	std::vector<char>::iterator begIt = theData.begin();
	uintptr_t i = 0;
	while(i < theData.size()){
		//go forward until the characters are equal
		uintptr_t j = i+1;
		while(j < theData.size()){
			if(theData[j]==theData[j-1]){ break; }
			j++;
		}
		if(j < theData.size()){ j--; }
		//dump out all between i and j
		while((j - i) > 127){
			compData.push_back(127);
			compData.insert(compData.end(), begIt + i, begIt + i + 127);
			i+=127;
		}
		if(j-i){
			compData.push_back(j-i);
			compData.insert(compData.end(), begIt + i, begIt + j);
		}
		i = j;
		if(i >= theData.size()){ break; }
		//figure out how many are equal
		char curC = theData[i];
		j = i + 1;
		while(j < theData.size()){
			if(theData[j]!=curC){ break; }
			j++;
		}
		//dump out
		while((j - i) > 127){
			compData.push_back(255);
			compData.push_back(curC);
			i+=127;
		}
		if(j-i){
			compData.push_back(128 | (j-i));
			compData.push_back(curC);
		}
		i = j;
	}
	compData = theData;
}
CompressionMethod* RunLengthCompressionFactory::instantiate(){
	return new RunLengthCompressionMethod();
}

GZipOutStream::GZipOutStream(int append, const char* fileName){
	myName = fileName;
	if(append){
		baseFile = gzopen(fileName, "ab");
	}
	else{
		baseFile = gzopen(fileName, "wb");
	}
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
GZipOutStream::~GZipOutStream(){}
void GZipOutStream::writeByte(int toW){
	if(gzputc(baseFile, toW) < 0){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void GZipOutStream::writeBytes(const char* toW, uintptr_t numW){
	if(gzwrite(baseFile, toW, numW)!=((int)numW)){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void GZipOutStream::close(){
	isClosed = 1;
	if(gzclose(baseFile)){ throw std::runtime_error("Problem closing " + myName); }
}

GZipInStream::GZipInStream(const char* fileName){
	myName = fileName;
	baseFile = gzopen(fileName, "rb");
	if(baseFile == 0){
		throw std::runtime_error("Could not open file " + myName);
	}
}
GZipInStream::~GZipInStream(){}
int GZipInStream::readByte(){
	int toR = gzgetc(baseFile);
	if(toR < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		if(gzerrcode && (gzerrcode != Z_STREAM_END)){
			std::string errRep = "Problem reading file ";
				errRep.append(myName);
				errRep.append(" : ");
				errRep.append(errMess);
			throw std::runtime_error(errRep);
		}
	}
	return toR;
}
uintptr_t GZipInStream::readBytes(char* toR, uintptr_t numR){
	int toRet = gzread(baseFile, toR, numR);
	if(toRet < 0){
		int gzerrcode;
		const char* errMess = gzerror(baseFile, &gzerrcode);
		std::string errRep = "Problem reading file ";
			errRep.append(myName);
			errRep.append(" : ");
			errRep.append(errMess);
		throw std::runtime_error(errRep);
	}
	return toRet;
}
void GZipInStream::close(){
	isClosed = 1;
	if(gzclose(baseFile)){ throw std::runtime_error("Problem closing " + myName); }
}

BlockCompOutStream::BlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth){
	chunkSize = blockSize;
	if(append && fileExists(annotFN)){
		intmax_t annotLen = fileGetSize(annotFN);
		if(annotLen < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
		if(annotLen % BLOCKCOMP_ANNOT_ENTLEN){throw std::runtime_error("Malformed annotation file.");}
		FileInStream checkIt(annotFN);
		checkIt.seek(annotLen - BLOCKCOMP_ANNOT_ENTLEN);
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		checkIt.forceReadBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
		checkIt.close();
		preCompBS = unpackBE64(annotBuff) + unpackBE64(annotBuff + 16);
		postCompBS = unpackBE64(annotBuff+8) + unpackBE64(annotBuff + 24);
	}
	else{
		preCompBS = 0;
		postCompBS = 0;
	}
	myComp = compMeth->instantiate();
	mainF = new FileOutStream(append, mainFN);
	annotF = new FileOutStream(append, annotFN);
}
BlockCompOutStream::~BlockCompOutStream(){
	delete(myComp);
	delete(mainF);
	delete(annotF);
}
void BlockCompOutStream::writeByte(int toW){
	myComp->theData.push_back(toW);
	if(myComp->theData.size() >= chunkSize){
		flush();
	}
}
void BlockCompOutStream::writeBytes(const char* toW, uintptr_t numW){
	const char* curFoc = toW;
	uintptr_t addOutstand = numW;
	while((myComp->theData.size() + addOutstand) > chunkSize){
		uintptr_t numNeedA = chunkSize - myComp->theData.size();
		myComp->theData.insert(myComp->theData.end(), curFoc, curFoc + numNeedA);
		flush();
		curFoc += numNeedA;
		addOutstand -= numNeedA;
	}
	myComp->theData.insert(myComp->theData.end(), curFoc, curFoc + addOutstand);
}
void BlockCompOutStream::close(){
	isClosed = 1;
	flush();
	mainF->close();
	annotF->close();
}
void BlockCompOutStream::flush(){
	if(myComp->theData.size() == 0){ return; }
	myComp->compressData();
	mainF->writeBytes(&(myComp->compData[0]), myComp->compData.size());
	char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
	packBE64(preCompBS, annotBuff);
	packBE64(postCompBS, annotBuff+8);
	packBE64(myComp->theData.size(), annotBuff+16);
	packBE64(myComp->compData.size(), annotBuff+24);
	annotF->writeBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
	preCompBS += myComp->theData.size();
	postCompBS += myComp->compData.size();
	myComp->theData.clear();
	myComp->compData.clear();
}
uintmax_t BlockCompOutStream::tell(){
	return preCompBS + myComp->theData.size();
}

BlockCompInStream::BlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth){
	myComp = compMeth->instantiate();
	intmax_t numBlocksT = fileGetSize(annotFN);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
	numBlocks = numBlocksT / BLOCKCOMP_ANNOT_ENTLEN;
	numLastLine = 0;
	lastLineBI0 = 0;
	mainF = new FileInStream(mainFN);
	annotF = new FileInStream(annotFN);
	nextReadI = 0;
	lastLineBuff = (char*)malloc(BLOCKCOMP_ANNOT_ENTLEN*BLOCKCOMPIN_LASTLINESEEK);
	lastLineAddrs = (uintptr_t*)malloc(sizeof(uintptr_t)*BLOCKCOMP_ANNOT_ENTLEN*BLOCKCOMPIN_LASTLINESEEK);
	totalReads = 0;
}
BlockCompInStream::~BlockCompInStream(){
	free(lastLineBuff);
	free(lastLineAddrs);
	delete(myComp);
	delete(mainF);
	delete(annotF);
}
int BlockCompInStream::readByte(){
	char tmpLoad;
	uintptr_t numRead = readBytes(&tmpLoad, 1);
	if(numRead){
		return 0x00FF & tmpLoad;
	}
	return -1;
}
uintptr_t BlockCompInStream::readBytes(char* toR, uintptr_t numR){
	uintptr_t totRead = 0;
	char* nextR = toR;
	uintptr_t leftR = numR;
	
	tailRecurTgt:
	uintptr_t numBuff = myComp->theData.size() - nextReadI;
	//get the remainder and return
	if(numBuff >= leftR){
		memcpy(nextR, &(myComp->theData[nextReadI]), leftR);
		totRead += leftR;
		nextReadI += leftR;
		totalReads += totRead;
		return totRead;
	}
	//get as much as you can
	if(numBuff){
		memcpy(nextR, &(myComp->theData[nextReadI]), numBuff);
		nextReadI += numBuff;
		totRead += numBuff;
		nextR += numBuff;
		leftR -= numBuff;
	}
	//load in the next buffer
	char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
	uintmax_t numRead = annotF->readBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
	if(numRead == 0){ totalReads += totRead; return totRead; }
	if(numRead != BLOCKCOMP_ANNOT_ENTLEN){ throw std::runtime_error("Annotation file truncated."); }
	uintptr_t numPost = unpackBE64(annotBuff+24);
	if(numPost){
		myComp->compData.resize(numPost);
		mainF->forceReadBytes(&(myComp->compData[0]), numPost);
		myComp->decompressData();
		nextReadI = 0;
	}
	//and retry
	goto tailRecurTgt;
}
void BlockCompInStream::close(){
	isClosed = 1;
	mainF->close();
	annotF->close();
}
void BlockCompInStream::seek(uintmax_t toAddr){
	totalReads = toAddr;
	retryWithCachedArena:
	if(numLastLine){
		uintmax_t* winBlk = std::upper_bound(lastLineAddrs, lastLineAddrs + numLastLine, toAddr) - 1;
		intmax_t winBI = winBlk - lastLineAddrs;
		if(winBI < 0){ goto arenaNotCached; }
		char* focEnt = lastLineBuff + BLOCKCOMP_ANNOT_ENTLEN*winBI;
		uintmax_t focLI = unpackBE64(focEnt);
		uintmax_t focHI = focLI + unpackBE64(focEnt + 16);
		if(toAddr >= focHI){
			//check for a seek to the end
			if((toAddr == focHI) && ((lastLineBI0+winBI+1) == numBlocks)){
				annotF->seek(BLOCKCOMP_ANNOT_ENTLEN*numBlocks);
				myComp->theData.clear();
				nextReadI = 0;
				return;
			}
			goto arenaNotCached;
		}
		uintmax_t blockSAddr = unpackBE64(focEnt + 8);
		uintmax_t blockCLen = unpackBE64(focEnt + 24);
		mainF->seek(blockSAddr);
		myComp->compData.resize(blockCLen);
		mainF->forceReadBytes(&(myComp->compData[0]), blockCLen);
		myComp->decompressData();
		nextReadI = toAddr - focLI;
		annotF->seek(BLOCKCOMP_ANNOT_ENTLEN*(lastLineBI0 + winBI + 1));
		return;
	}
	arenaNotCached:
	uintmax_t fromBlock = 0;
	uintmax_t toBlock = numBlocks;
	while((toBlock - fromBlock) > BLOCKCOMPIN_LASTLINESEEK){
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		uintmax_t midBlock = (fromBlock + toBlock)/2;
		annotF->seek(BLOCKCOMP_ANNOT_ENTLEN*midBlock);
		annotF->forceReadBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
		uintptr_t precomLI = unpackBE64(annotBuff);
		//uintptr_t precomHI = precomLI + unpackBE64(annotBuff+16);
		if(toAddr < precomLI){
			toBlock = midBlock;
		}
		else{
			fromBlock = midBlock;
		}
	}
	uintmax_t numRBlock = toBlock - fromBlock;
	numLastLine = numRBlock;
	lastLineBI0 = fromBlock;
	annotF->seek(BLOCKCOMP_ANNOT_ENTLEN*fromBlock);
	annotF->forceReadBytes(lastLineBuff, numRBlock*BLOCKCOMP_ANNOT_ENTLEN);
	for(uintptr_t i = 0; i<numRBlock; i++){
		lastLineAddrs[i] = unpackBE64(lastLineBuff + i*BLOCKCOMP_ANNOT_ENTLEN);
	}
	goto retryWithCachedArena;
}
uintmax_t BlockCompInStream::tell(){
	return totalReads;
}
uintmax_t BlockCompInStream::size(){
	uintmax_t retLoc = annotF->tell();
	annotF->seek(BLOCKCOMP_ANNOT_ENTLEN * (numBlocks - 1));
	char tmpBuff[BLOCKCOMP_ANNOT_ENTLEN];
	annotF->forceReadBytes(tmpBuff, BLOCKCOMP_ANNOT_ENTLEN);
	annotF->seek(retLoc);
	return unpackBE64(tmpBuff) + unpackBE64(tmpBuff+16);
}

MultithreadBlockCompOutStreamUniform::MultithreadBlockCompOutStreamUniform(){
	myComp = 0;
}

MultithreadBlockCompOutStreamUniform::~MultithreadBlockCompOutStreamUniform(){
	if(myComp){ delete(myComp); }
}

void MultithreadBlockCompOutStreamUniform::doIt(){
	myComp->theData.insert(myComp->theData.end(), compData, compData + numData);
	myComp->compressData();
}

MultithreadBlockCompOutStream::MultithreadBlockCompOutStream(int append, uintptr_t blockSize, const char* mainFN, const char* annotFN, CompressionFactory* compMeth, int numThreads, ThreadPool* useThreads) : stringDumpHandle(numThreads, useThreads){
	mainF = 0;
	annotF = 0;
	try{
		chunkSize = blockSize;
		if(append && fileExists(annotFN)){
			intmax_t annotLen = fileGetSize(annotFN);
			if(annotLen < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
			if(annotLen % BLOCKCOMP_ANNOT_ENTLEN){throw std::runtime_error("Malformed annotation file.");}
			FileInStream checkIt(annotFN);
			checkIt.seek(annotLen - BLOCKCOMP_ANNOT_ENTLEN);
			char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
			checkIt.forceReadBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
			checkIt.close();
			preCompBS = unpackBE64(annotBuff) + unpackBE64(annotBuff + 16);
			postCompBS = unpackBE64(annotBuff+8) + unpackBE64(annotBuff + 24);
		}
		else{
			preCompBS = 0;
			postCompBS = 0;
		}
		mainF = new FileOutStream(append, mainFN);
		annotF = new FileOutStream(append, annotFN);
		totalWrite = preCompBS;
		compThreads = useThreads;
		maxMarshal = chunkSize * numThreads;
		numMarshal = 0;
		chunkMarshal = (char*)malloc(maxMarshal);
		threadUnis.resize(numThreads);
		for(uintptr_t i = 0; i<threadUnis.size(); i++){
			threadUnis[i].myComp = compMeth->instantiate();
			threadPass.push_back(&(threadUnis[i]));
		}
	}
	catch(std::exception& errE){
		if(mainF){ try{mainF->close();}catch(std::exception& errB){} delete(mainF); }
		if(annotF){ try{annotF->close();}catch(std::exception& errB){} delete(annotF); }
		isClosed = 1;
		throw;
	}
}
MultithreadBlockCompOutStream::~MultithreadBlockCompOutStream(){
	delete(mainF);
	delete(annotF);
	free(chunkMarshal);
}
void MultithreadBlockCompOutStream::writeByte(int toW){
	char tmpW = toW;
	writeBytes(&tmpW, 1);
}
void MultithreadBlockCompOutStream::writeBytes(const char* toW, uintptr_t numW){
	const char* nextW = toW;
	uintptr_t leftW = numW;
	
	tailRecurTgt:
	if(leftW < (maxMarshal - numMarshal)){
		stringDumpHandle.memcpy(chunkMarshal + numMarshal, nextW, leftW);
		numMarshal += leftW;
		return;
	}
	uintptr_t numEat = maxMarshal - numMarshal;
	const char* useCompSrc = nextW;
	if(numMarshal){
		useCompSrc = chunkMarshal;
		if(numEat){
			stringDumpHandle.memcpy(chunkMarshal + numMarshal, nextW, numEat);
			numMarshal += numEat;
		}
	}
	compressAndOut(maxMarshal, useCompSrc);
	leftW -= numEat;
	nextW += numEat;
	numMarshal = 0;
	if(leftW){ goto tailRecurTgt; }
}
void MultithreadBlockCompOutStream::flush(){
	if(numMarshal){
		compressAndOut(numMarshal, chunkMarshal);
		numMarshal = 0;
	}
}
void MultithreadBlockCompOutStream::close(){
	isClosed = 1;
	flush();
	mainF->close();
	annotF->close();
}
uintmax_t MultithreadBlockCompOutStream::tell(){
	return totalWrite;
}
void MultithreadBlockCompOutStream::compressAndOut(uintptr_t numDump, const char* dumpFrom){
	//do the compression
	uintptr_t numPT = numDump / threadUnis.size();
	uintptr_t numET = numDump % threadUnis.size();
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<threadUnis.size(); i++){
		MultithreadBlockCompOutStreamUniform* curU = &(threadUnis[i]);
		uintptr_t curNum = numPT + (i < numET);
		curU->numData = curNum;
		curU->compData = dumpFrom + curOff;
		curOff += curNum;
	}
	uintptr_t fromI = compThreads->addTasks(threadUnis.size(), &(threadPass[0]));
	compThreads->joinTasks(fromI, fromI + threadUnis.size());
	//dump them out
	for(uintptr_t ui = 0; ui < threadUnis.size(); ui++){
		MultithreadBlockCompOutStreamUniform* curUni = &(threadUnis[ui]);
		//write out the compressed data
		uintptr_t origLen = curUni->myComp->theData.size();
		uintptr_t compLen = curUni->myComp->compData.size();
		if(compLen){
			mainF->writeBytes(&(curUni->myComp->compData[0]), compLen);
		}
		//write out an annotation
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		packBE64(preCompBS, annotBuff);
		packBE64(postCompBS, annotBuff+8);
		packBE64(origLen, annotBuff+16);
		packBE64(compLen, annotBuff+24);
		annotF->writeBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
		//and prepare for the next round
		preCompBS += origLen;
		postCompBS += compLen;
		curUni->myComp->theData.clear();
	}
}

MultithreadBlockCompInStreamUniform::MultithreadBlockCompInStreamUniform(){
	myComp = 0;
}
MultithreadBlockCompInStreamUniform::~MultithreadBlockCompInStreamUniform(){
	if(myComp){ delete(myComp); }
}
void MultithreadBlockCompInStreamUniform::doIt(){
	myComp->decompressData();
}

MultithreadBlockCompInStream::MultithreadBlockCompInStream(const char* mainFN, const char* annotFN, CompressionFactory* compMeth, int numThreads, ThreadPool* useThreads) : stringDumpHandle(1, useThreads){
	intmax_t numBlocksT = fileGetSize(annotFN);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(annotFN); throw std::runtime_error(errMess);}
	numBlocks = numBlocksT / BLOCKCOMP_ANNOT_ENTLEN;
	mainF = new FileInStream(mainFN);
	annotF = new FileInStream(annotFN);
	compThreads = useThreads;
	numReadBlocks = 0;
	nextOutUni = numThreads;
	nextOutByte = 0;
	threadUnis.resize(numThreads);
	stringDumpUnis.resize(numThreads);
	stringDumpHot.resize(numThreads);
	for(int i = 0; i<numThreads; i++){
		threadUnis[i].myComp = compMeth->instantiate();
	}
}
MultithreadBlockCompInStream::~MultithreadBlockCompInStream(){
	delete(mainF);
	delete(annotF);
}
int MultithreadBlockCompInStream::readByte(){
	char toR;
	uintptr_t numR = readBytes(&toR, 1);
	if(numR){
		return 0x00FF & toR;
	}
	return -1;
}
uintptr_t MultithreadBlockCompInStream::readBytes(char* toR, uintptr_t numR){
	char* nextR = toR;
	uintptr_t leftR = numR;
	
	tailRecurTgt:
	//start copying
	uintptr_t startUni = nextOutUni;
	uintptr_t highUni = nextOutUni;
	while(nextOutUni < threadUnis.size()){
		MultithreadBlockCompInStreamUniform* curUni = &(threadUnis[nextOutUni]);
		uintptr_t maxCopy = curUni->myComp->theData.size() - nextOutByte;
		if(maxCopy == 0){
			stringDumpHot[nextOutUni] = 0;
			nextOutUni++;
			nextOutByte = 0;
			continue;
		}
		stringDumpHot[nextOutUni] = 1;
		int tooSmall = leftR < maxCopy;
		if(tooSmall){
			maxCopy = leftR;
		}
		stringDumpHandle.initMemcpy(nextR, &(curUni->myComp->theData[nextOutByte]), maxCopy, &(stringDumpUnis[nextOutUni]));
		nextR += maxCopy;
		leftR -= maxCopy;
		highUni = nextOutUni + 1;
		if(tooSmall){
			nextOutByte += maxCopy;
			break;
		}
		else{
			nextOutUni++;
			nextOutByte = 0;
		}
	}
	//wait for all copies to finish
	for(uintptr_t i = startUni; i<highUni; i++){
		if(stringDumpHot[i]){
			stringDumpHandle.wait(&(stringDumpUnis[i]));
		}
	}
	//if exhasted, reload
	if(nextOutUni == threadUnis.size()){
		if(numReadBlocks == numBlocks){
			return numR - leftR;
		}
		fillBuffer();
	}
	//if stuff left, retry
	if(leftR){ goto tailRecurTgt; }
	return numR;
}
void MultithreadBlockCompInStream::close(){
	isClosed = 1;
	mainF->close();
	annotF->close();
}
void MultithreadBlockCompInStream::fillBuffer(){
	nextOutUni = 0;
	nextOutByte = 0;
	//read in compressed data
	for(uintptr_t i = 0; i<threadUnis.size(); i++){
		MultithreadBlockCompInStreamUniform* curUni = &(threadUnis[i]);
		
		emptyBlockTryAgain:
		if(numReadBlocks == numBlocks){
			for(uintptr_t j = i; j<threadUnis.size(); j++){
				stringDumpHot[j] = 0;
				threadUnis[j].myComp->theData.clear();
			}
			break;
		}
		char annotBuff[BLOCKCOMP_ANNOT_ENTLEN];
		uintmax_t numRead = annotF->readBytes(annotBuff, BLOCKCOMP_ANNOT_ENTLEN);
		if(numRead != BLOCKCOMP_ANNOT_ENTLEN){ throw std::runtime_error("Annotation file truncated."); }
		numReadBlocks++;
		uintptr_t numPost = unpackBE64(annotBuff+24);
		if(numPost == 0){ goto emptyBlockTryAgain; }
		
		stringDumpHot[i] = 1;
		curUni->myComp->compData.resize(numPost);
		mainF->forceReadBytes(&(curUni->myComp->compData[0]), numPost);
		curUni->compressID = compThreads->addTask(curUni);
	}
	//wait for the decompression
	for(uintptr_t i = 0; i<threadUnis.size(); i++){
		if(stringDumpHot[i]){
			compThreads->joinTask(threadUnis[i].compressID);
		}
	}
}

