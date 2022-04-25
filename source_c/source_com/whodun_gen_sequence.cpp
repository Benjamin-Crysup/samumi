#include "whodun_gen_sequence.h"

#include <string.h>
#include <algorithm>

using namespace whodun;

#define INTERNAL_PHRED_TO_LOG(toConv) ((0x00FF & (toConv)) - 33)/-10.0
#define INTERNAL_LOG_TO_PHRED(toConv) (char)(std::min(126.0, std::max(33.0, ((toConv) * -10.0) + 33)))

double whodun::phredToLog10Prob(char toConv){
	return INTERNAL_PHRED_TO_LOG(toConv);
}

char whodun::phredLog10ToScore(double toConv){
	return INTERNAL_LOG_TO_PHRED(toConv);
}

char whodunRevCompMap[] = {
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0A', '\x0B', '\x0C', '\x0D', '\x0E', '\x0F',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1A', '\x1B', '\x1C', '\x1D', '\x1E', '\x1F',
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2A', '\x2B', '\x2C', '\x2D', '\x2E', '\x2F',
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3A', '\x3B', '\x3C', '\x3D', '\x3E', '\x3F',
	'\x40', '\x54', '\x42', '\x47', '\x44', '\x45', '\x46', '\x43', '\x48', '\x49', '\x4A', '\x4B', '\x4C', '\x4D', '\x4E', '\x4F',
	'\x50', '\x51', '\x52', '\x53', '\x41', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5A', '\x5B', '\x5C', '\x5D', '\x5E', '\x5F',
	'\x60', '\x74', '\x62', '\x67', '\x64', '\x65', '\x66', '\x63', '\x68', '\x69', '\x6A', '\x6B', '\x6C', '\x6D', '\x6E', '\x6F',
	'\x70', '\x71', '\x72', '\x73', '\x61', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7A', '\x7B', '\x7C', '\x7D', '\x7E', '\x7F',
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8A', '\x8B', '\x8C', '\x8D', '\x8E', '\x8F',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9A', '\x9B', '\x9C', '\x9D', '\x9E', '\x9F',
	'\xA0', '\xA1', '\xA2', '\xA3', '\xA4', '\xA5', '\xA6', '\xA7', '\xA8', '\xA9', '\xAA', '\xAB', '\xAC', '\xAD', '\xAE', '\xAF',
	'\xB0', '\xB1', '\xB2', '\xB3', '\xB4', '\xB5', '\xB6', '\xB7', '\xB8', '\xB9', '\xBA', '\xBB', '\xBC', '\xBD', '\xBE', '\xBF',
	'\xC0', '\xC1', '\xC2', '\xC3', '\xC4', '\xC5', '\xC6', '\xC7', '\xC8', '\xC9', '\xCA', '\xCB', '\xCC', '\xCD', '\xCE', '\xCF',
	'\xD0', '\xD1', '\xD2', '\xD3', '\xD4', '\xD5', '\xD6', '\xD7', '\xD8', '\xD9', '\xDA', '\xDB', '\xDC', '\xDD', '\xDE', '\xDF',
	'\xE0', '\xE1', '\xE2', '\xE3', '\xE4', '\xE5', '\xE6', '\xE7', '\xE8', '\xE9', '\xEA', '\xEB', '\xEC', '\xED', '\xEE', '\xEF',
	'\xF0', '\xF1', '\xF2', '\xF3', '\xF4', '\xF5', '\xF6', '\xF7', '\xF8', '\xF9', '\xFA', '\xFB', '\xFC', '\xFD', '\xFE', '\xFF',
};

GeneticSequenceUtilities::GeneticSequenceUtilities(){}
GeneticSequenceUtilities::~GeneticSequenceUtilities(){}
void GeneticSequenceUtilities::phredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore){
	for(uintptr_t i = 0; i<numConv; i++){
		toStore[i] = INTERNAL_PHRED_TO_LOG(toConv[i]);
	}
}
void GeneticSequenceUtilities::phredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore){
	for(uintptr_t i = 0; i<numConv; i++){
		toStore[i] = INTERNAL_LOG_TO_PHRED(toConv[i]);
	}
}
void GeneticSequenceUtilities::reverseComplement(uintptr_t numConv, char* toRev, double* toRevQ){
	for(uintptr_t i = 0; i<numConv; i++){
		toRev[i] = whodunRevCompMap[0x00FF & toRev[i]];
	}
	std::reverse(toRev, toRev + numConv);
	if(toRevQ){
		std::reverse(toRevQ, toRevQ + numConv);
	}
}

void MTGeneticSequenceUtilitiesUniform::doIt(){
	switch(forPhase){
		case 0:
			for(uintptr_t i = 0; i<numConv; i++){
				anyDouble[i] = INTERNAL_PHRED_TO_LOG(anyBytes[i]);
			}
			break;
		case 1:
			for(uintptr_t i = 0; i<numConv; i++){
				anyBytes[i] = INTERNAL_LOG_TO_PHRED(anyDouble[i]);
			}
			break;
		case 2:
			for(uintptr_t i = 0; i<numConv; i++){
				uintptr_t aI = numConv - (i+1);
				char tmpA = whodunRevCompMap[0x00FF&anyBytes[i]];
				char tmpB = whodunRevCompMap[0x00FF&anyBytesB[aI]];
				anyBytes[i] = tmpB;
				anyBytesB[aI] = tmpA;
			}
			if(anyDouble){
				for(uintptr_t i = 0; i<numConv; i++){
					uintptr_t aI = numConv - (i+1);
					double tmpD = anyDouble[i];
					anyDouble[i] = anyDoubleB[aI];
					anyDoubleB[aI] = tmpD;
				}
			}
			break;
		default:
			return;
	};
}
MTGeneticSequenceUtilities::MTGeneticSequenceUtilities(int numThread, ThreadPool* mainPool){
	usePool = mainPool;
	helpUnis.resize(numThread);
}
MTGeneticSequenceUtilities::~MTGeneticSequenceUtilities(){}
void MTGeneticSequenceUtilities::phredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore){
	startPhredsToLog10Prob(numConv, toConv, toStore, &(helpUnis[0]));
	wait(&(helpUnis[0]));
}
void MTGeneticSequenceUtilities::phredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore){
	startPhredsLog10ToScore(numConv, toConv, toStore, &(helpUnis[0]));
	wait(&(helpUnis[0]));
}
void MTGeneticSequenceUtilities::reverseComplement(uintptr_t numConv, char* toRev, double* toRevQ){
	startReverseComplement(numConv, toRev, toRevQ, &(helpUnis[0]));
	wait(&(helpUnis[0]));
}
void MTGeneticSequenceUtilities::startPhredsToLog10Prob(uintptr_t numConv, char* toConv, double* toStore, MTGeneticSequenceUtilitiesUniform* useUni){
	uintptr_t numPerT = numConv / helpUnis.size();
	uintptr_t numExtT = numConv % helpUnis.size();
	uintptr_t curOff = 0;
	usePool->bigStart();
	for(unsigned i = 0; i<helpUnis.size(); i++){
		uintptr_t curNum = numPerT + (i < numExtT);
		MTGeneticSequenceUtilitiesUniform* curUni = useUni + i;
		curUni->forPhase = 0;
		curUni->numConv = curNum;
		curUni->anyDouble = toStore + curOff;
		curUni->anyBytes = toConv + curOff;
		curUni->threadID = usePool->bigAdd(curUni);
		curOff += curNum;
	}
	usePool->bigEnd();
}
void MTGeneticSequenceUtilities::startPhredsLog10ToScore(uintptr_t numConv, double* toConv, char* toStore, MTGeneticSequenceUtilitiesUniform* useUni){
	uintptr_t numPerT = numConv / helpUnis.size();
	uintptr_t numExtT = numConv % helpUnis.size();
	uintptr_t curOff = 0;
	usePool->bigStart();
	for(unsigned i = 0; i<helpUnis.size(); i++){
		uintptr_t curNum = numPerT + (i < numExtT);
		MTGeneticSequenceUtilitiesUniform* curUni = useUni + i;
		curUni->forPhase = 1;
		curUni->numConv = curNum;
		curUni->anyDouble = toConv + curOff;
		curUni->anyBytes = toStore + curOff;
		curUni->threadID = usePool->bigAdd(curUni);
		curOff += curNum;
	}
	usePool->bigEnd();
}
void MTGeneticSequenceUtilities::startReverseComplement(uintptr_t numConv, char* toRev, double* toRevQ, MTGeneticSequenceUtilitiesUniform* useUni){
	uintptr_t halfConv = numConv >> 1;
	uintptr_t numPerT = halfConv / helpUnis.size();
	uintptr_t numExtT = halfConv % helpUnis.size();
	uintptr_t curOff = 0;
	uintptr_t curOffB = numConv;
	usePool->bigStart();
	for(unsigned i = 0; i<helpUnis.size(); i++){
		uintptr_t curNum = numPerT + (i < numExtT);
		curOffB -= curNum;
		MTGeneticSequenceUtilitiesUniform* curUni = useUni + i;
		curUni->forPhase = 2;
		curUni->numConv = curNum;
		curUni->anyDouble = toRevQ ? (toRevQ + curOff) : 0;
		curUni->anyBytes = toRev + curOff;
		curUni->anyDoubleB = toRevQ ? (toRevQ + curOffB) : 0;
		curUni->anyBytesB = toRev + curOffB;
		curUni->threadID = usePool->bigAdd(curUni);
		curOff += curNum;
	}
	usePool->bigEnd();
	if(numConv & 0x0001){
		toRev[curOff] = whodunRevCompMap[0x00FF&toRev[curOff]];
	}
}
void MTGeneticSequenceUtilities::wait(MTGeneticSequenceUtilitiesUniform* useUni){
	usePool->bigStart();
	for(unsigned i = 0; i<helpUnis.size(); i++){
		usePool->bigJoin(useUni[i].threadID);
	}
	usePool->bigEnd();
}

SequenceEntry::SequenceEntry(){
	name = 0;
	allocName = 0;
	seq = 0;
	allocSeq = 0;
	qual = 0;
	allocQual = 0;
}
SequenceEntry::~SequenceEntry(){
	if(name){ free(name); }
	if(seq){ free(seq); }
	if(qual){ free(qual); }
}
void SequenceEntry::preallocate(uintptr_t needName, uintptr_t needSeq, int needQual){
	if(needName > allocName){
		if(name){ free(name); }
		name = (char*)malloc(needName);
		allocName = needName;
	}
	if(needSeq > allocSeq){
		if(seq){ free(seq); }
		seq = (char*)malloc(needSeq);
		allocSeq = needSeq;
	}
	if(needQual && (needSeq > allocQual)){
		if(qual){ free(qual); }
		qual = (double*)malloc(needSeq*sizeof(double));
		allocQual = needSeq;
	}
}

SequenceReader::SequenceReader(){}
SequenceReader::~SequenceReader(){
	for(uintptr_t i = 0; i<cache.size(); i++){
		delete(cache[i]);
	}
}
SequenceEntry* SequenceReader::allocate(){
	SequenceEntry* toRet;
	if(cache.size()){
		toRet = cache[cache.size()-1];
		cache.pop_back();
	}
	else{
		toRet = new SequenceEntry();
	}
	toRet->nameLen = 0;
	toRet->shortNameLen = 0;
	toRet->seqLen = 0;
	toRet->haveQual = 0;
	return toRet;
}
void SequenceReader::release(SequenceEntry* toRet){
	cache.push_back(toRet);
}

SequenceWriter::SequenceWriter(){}
SequenceWriter::~SequenceWriter(){}
void SequenceWriter::write(uintptr_t numWrite, SequenceEntry** allOut){
	for(uintptr_t i = 0; i<numWrite; i++){
		write(allOut[i]);
	}
}

RandacSequenceReader::RandacSequenceReader(){}
SequenceEntry* RandacSequenceReader::getEntry(uintmax_t endInd){
	return getEntry(endInd, 0, getEntryLength(endInd));
}

#define FASTAQ_CHUNK_SIZE 0x00100000
#define FASTAQ_MODE_PREFIX 0
#define FASTAQ_MODE_FASTASEQ 1
#define FASTAQ_MODE_FASTQSEQ 2
#define FASTAQ_MODE_FASTQSEP 3
#define FASTAQ_MODE_FASTQQUAL 4

#define IS_ASCII_WHITESPACE(testC) (((testC) == ' ') || ((testC) == '\t') || ((testC) == '\r'))

FastAQSequenceReaderUniform::FastAQSequenceReaderUniform(){}
FastAQSequenceReaderUniform::~FastAQSequenceReaderUniform(){}
void FastAQSequenceReaderUniform::doIt(){
	switch(taskID){
		case 0:
			memcpy(tgtEnt->name, srcLoc, srcLen);
			tgtEnt->shortNameLen = srcLen;
			for(uintptr_t i = 0; i<srcLen; i++){
				if(IS_ASCII_WHITESPACE(srcLoc[i])){
					tgtEnt->shortNameLen = i;
					break;
				}
			}
			break;
		case 1:
			memcpy(tgtEnt->seq + tgtOffset, srcLoc, srcLen);
			break;
		case 2:
			phredMeDo.phredsToLog10Prob(srcLen, srcLoc, tgtEnt->qual + tgtOffset);
			break;
		default:
			return;
	};
}

#define FASTQ_PARSE_BUFFER_SIZE 0x010000

FastAQSequenceReader::FastAQSequenceReader(InStream* mainFrom){
	theStr = mainFrom;
	newlineSpl = new CharacterSplitter();
		newlineSpl->splitOn = '\n';
	newlineStr = new SplitterInStream(mainFrom, newlineSpl, FASTAQ_CHUNK_SIZE);
	curMode = FASTAQ_MODE_PREFIX;
	curBuild = 0;
	usePool = 0;
	useStringOp = 0;
}
FastAQSequenceReader::FastAQSequenceReader(InStream* mainFrom, int numThread, ThreadPool* mainPool){
	theStr = mainFrom;
	newlineSpl = new MultithreadCharacterSplitter(numThread, mainPool);
		newlineSpl->splitOn = '\n';
	newlineStr = new SplitterInStream(mainFrom, newlineSpl, numThread * FASTQ_PARSE_BUFFER_SIZE);
	curMode = FASTAQ_MODE_PREFIX;
	curBuild = 0;
	numThr = numThread;
	usePool = mainPool;
	useStringOp = new MultithreadedStringH(numThread, usePool);
}
FastAQSequenceReader::~FastAQSequenceReader(){
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		usePool->joinTask(liveTask[i]->threadID);
	}
	cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
	for(uintptr_t i = 0; i<waitEnts.size(); i++){ delete(waitEnts[i]); }
	for(uintptr_t i = 0; i<cacheTask.size(); i++){ delete(cacheTask[i]); }
	if(curBuild){ delete(curBuild); }
	delete(newlineStr);
	delete(newlineSpl);
	if(useStringOp){ delete(useStringOp); }
}
SequenceEntry* FastAQSequenceReader::read(){
	if(waitEnts.size()){
		SequenceEntry* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	
	needMoreLoop:
	uintptr_t focusSplit;
	StringSplitData lastR = newlineStr->read();
	if(lastR.numSplits == 0){
		switch(curMode){
			case FASTAQ_MODE_PREFIX:
				goto gotSomethingEnd;
			case FASTAQ_MODE_FASTASEQ:
				waitEnts.push_back(curBuild);
				curBuild = 0;
				curMode = FASTAQ_MODE_PREFIX;
				goto gotSomethingEnd;
			case FASTAQ_MODE_FASTQSEQ:
				throw std::runtime_error("Malformed fastq entry: missing sequence.");
			case FASTAQ_MODE_FASTQSEP:
				throw std::runtime_error("Malformed fastq entry: missing separator.");
			case FASTAQ_MODE_FASTQQUAL:
				throw std::runtime_error("Malformed fastq entry: missing quality.");
			default:
				throw std::runtime_error("Da fuq?");
		};
	}
	//prepare the jobs
	try{
		focusSplit = 0;
		while(focusSplit < lastR.numSplits){
			char* focSplS = lastR.splitStart[focusSplit];
			char* focSplE = lastR.splitEnd[focusSplit];
			switch(curMode){
				case FASTAQ_MODE_PREFIX:
					//look at the first character: @ > ws error
					if(focSplS == focSplE){ focusSplit++; continue; }
					switch(*focSplS){
						case '>': {
							curBuild = allocate();
							curMode = FASTAQ_MODE_FASTASEQ;
							packName(curBuild, focSplS+1, focSplE);
							break;
						}
						case '@':{
							curBuild = allocate();
							curMode = FASTAQ_MODE_FASTQSEQ;
							curBuild->haveQual = 1;
							packName(curBuild, focSplS+1, focSplE);
							break;
						}
						case ' ':
						case '\t':
						case '\r':
						case ';':
							break;
						default:
							throw std::runtime_error("Bad line in fasta.");
					};
					focusSplit++;
					break;
				case FASTAQ_MODE_FASTASEQ:{
						//look for the first line with a @ > or ;
						int wasTerm = 0;
						uintptr_t totalSeqLen = 0;
						uintptr_t nextFocus = focusSplit;
						while(nextFocus < lastR.numSplits){
							char* curSplS = lastR.splitStart[nextFocus];
							char* curSplE = lastR.splitEnd[nextFocus];
							if(curSplS == curSplE){ nextFocus++; continue; }
							if(strchr("@>;", *curSplS)){ wasTerm = 1; break; }
							totalSeqLen += (curSplE - curSplS);
							nextFocus++;
						}
						//allocate space
						uintptr_t fullSeqLen = curBuild->seqLen + totalSeqLen;
						if(fullSeqLen > curBuild->allocSeq){
							char* newSeq = (char*)malloc(fullSeqLen);
							if(useStringOp){
								useStringOp->memcpy(newSeq, curBuild->seq, curBuild->seqLen);
							}
							else{
								memcpy(newSeq, curBuild->seq, curBuild->seqLen);
							}
							if(curBuild->seq){ free(curBuild->seq); }
							curBuild->seq = newSeq;
							curBuild->allocSeq = fullSeqLen;
						}
						//set up the copies
						uintptr_t totalSeqOff = curBuild->seqLen;
						for(uintptr_t curFocus = focusSplit; curFocus < nextFocus; curFocus++){
							char* curSplS = lastR.splitStart[curFocus];
							char* curSplE = lastR.splitEnd[curFocus];
							while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(*curSplS)){curSplS++;}
							while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(curSplE[-1])){curSplE--;}
							packSeq(curBuild, curSplS, curSplE, totalSeqOff);
							totalSeqOff += (curSplE - curSplS);
						}
						curBuild->seqLen = totalSeqOff;
						//prepare to continue
						focusSplit = nextFocus;
						if(wasTerm){
							waitEnts.push_back(curBuild);
							curBuild = 0;
							curMode = FASTAQ_MODE_PREFIX;
						}
					}break;
				case FASTAQ_MODE_FASTQSEQ:{
						char* curSplS = lastR.splitStart[focusSplit];
						char* curSplE = lastR.splitEnd[focusSplit];
						while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(*curSplS)){curSplS++;}
						while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(curSplE[-1])){curSplE--;}
						//allocate space
						uintptr_t fullSeqLen = curSplE - curSplS;
						if(fullSeqLen > curBuild->allocSeq){
							if(curBuild->seq){ free(curBuild->seq); }
							curBuild->seq = (char*)malloc(fullSeqLen);
							curBuild->allocSeq = fullSeqLen;
						}
						//set up the copy
						packSeq(curBuild, curSplS, curSplE, 0);
						curBuild->seqLen = fullSeqLen;
						//and prepare to continue
						curMode = FASTAQ_MODE_FASTQSEP;
						focusSplit++;
					} break;
				case FASTAQ_MODE_FASTQSEP:{
						char* curSplS = lastR.splitStart[focusSplit];
						char* curSplE = lastR.splitEnd[focusSplit];
						if(curSplS == curSplE){
							std::string errMessA("Missing fastq separator in ");
							std::string errMessB(curBuild->name, curBuild->name + std::min((uintptr_t)64, curBuild->shortNameLen));
							throw std::runtime_error(errMessA + errMessB);
						}
						if(*curSplS != '+'){
							std::string errMessA("Missing fastq separator in ");
							std::string errMessB(curBuild->name, curBuild->name + std::min((uintptr_t)64, curBuild->shortNameLen));
							throw std::runtime_error(errMessA + errMessB);
						}
						//continue
						curMode = FASTAQ_MODE_FASTQQUAL;
						focusSplit++;
					} break;
				case FASTAQ_MODE_FASTQQUAL:{
						char* curSplS = lastR.splitStart[focusSplit];
						char* curSplE = lastR.splitEnd[focusSplit];
						while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(*curSplS)){curSplS++;}
						while((curSplS!=curSplE) && IS_ASCII_WHITESPACE(curSplE[-1])){curSplE--;}
						//allocate space
						uintptr_t fullSeqLen = curSplE - curSplS;
						if(fullSeqLen != curBuild->seqLen){ throw std::runtime_error("Quality in fastq not same length as sequence."); }
						if(fullSeqLen > curBuild->allocQual){
							if(curBuild->qual){ free(curBuild->qual); }
							curBuild->qual = (double*)malloc(fullSeqLen * sizeof(double));
							curBuild->allocQual = fullSeqLen;
						}
						//set up the conversion
						packQual(curBuild, curSplS, curSplE, 0);
						//continue
						waitEnts.push_back(curBuild);
						curBuild = 0;
						curMode = FASTAQ_MODE_PREFIX;
						focusSplit++;
					}break;
				default:
					throw std::runtime_error("Da fuq?");
			};
		}
	}catch(std::exception& errE){
		cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
		liveTask.clear();
		throw;
	}
	if(usePool){
		//start the jobs
		uintptr_t jobSID = usePool->addTasks(liveTask.size(), (ThreadTask**)&(liveTask[0]));
		//and wait for the jobs
		usePool->joinTasks(jobSID, jobSID + liveTask.size());
		//and clean up
		cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
		liveTask.clear();
	}
	//see if more is needed
	if(waitEnts.size() == 0){
		goto needMoreLoop;
	}
	
	gotSomethingEnd:
	if(waitEnts.size()){
		SequenceEntry* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	return 0;
}
FastAQSequenceReaderUniform* FastAQSequenceReader::allocUni(){
	if(cacheTask.size()){
		FastAQSequenceReaderUniform* toRet = cacheTask[cacheTask.size()-1];
		cacheTask.pop_back();
		return toRet;
	}
	return new FastAQSequenceReaderUniform();
}
void FastAQSequenceReader::packName(SequenceEntry* packTo, char* namS, char* namE){
	//trim whitespace
	char* rnamS = namS;
	char* rnamE = namE;
	while(rnamS != rnamE){
		if(! IS_ASCII_WHITESPACE(*rnamS) ){break;}
		rnamS++;
	}
	while(rnamS != rnamE){
		if(! IS_ASCII_WHITESPACE(rnamE[-1]) ){break;}
		rnamE--;
	}
	//make the space
	packTo->nameLen = rnamE - rnamS;
	if(packTo->allocName < packTo->nameLen){
		if(packTo->name){ free(packTo->name); }
		packTo->name = (char*)malloc(packTo->nameLen);
		packTo->allocName = packTo->nameLen;
	}
	//set up a copy
	FastAQSequenceReaderUniform* curT = allocUni();
	curT->taskID = 0;
	curT->srcLen = packTo->nameLen;
	curT->srcLoc = rnamS;
	curT->tgtEnt = packTo;
	curT->tgtOffset = 0;
	if(usePool){
		liveTask.push_back(curT);
	}
	else{
		curT->doIt();
		cacheTask.push_back(curT);
	}
}
void FastAQSequenceReader::packSeq(SequenceEntry* packTo, char* seqS, char* seqE, uintptr_t useOff){
	//trim whitespace
	char* rnamS = seqS;
	char* rnamE = seqE;
	//figure out how many threads to use
	uintptr_t seqLen = rnamE - rnamS;
	uintptr_t numUseThr = seqLen / FASTAQ_CHUNK_SIZE;
		numUseThr = usePool ? std::min(numThr, std::max(numUseThr, (uintptr_t)1)) : 1;
	uintptr_t numPerT = seqLen / numUseThr;
	uintptr_t numExtT = seqLen % numUseThr;
	//set up the copies
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numUseThr; i++){
		uintptr_t curNum = numPerT + (i < numExtT);
		FastAQSequenceReaderUniform* curT = allocUni();
		curT->taskID = 1;
		curT->srcLen = curNum;
		curT->srcLoc = rnamS + curOff;
		curT->tgtEnt = packTo;
		curT->tgtOffset = useOff + curOff;
		if(usePool){
			liveTask.push_back(curT);
		}
		else{
			curT->doIt();
			cacheTask.push_back(curT);
		}
		curOff += curNum;
	}
}
void FastAQSequenceReader::packQual(SequenceEntry* packTo, char* qualS, char* qualE, uintptr_t useOff){
	//trim whitespace
	char* rnamS = qualS;
	char* rnamE = qualE;
	//figure out how many threads to use
	uintptr_t seqLen = rnamE - rnamS;
	uintptr_t numUseThr = seqLen / FASTAQ_CHUNK_SIZE;
		numUseThr = usePool ? std::min(numThr, std::max(numUseThr, (uintptr_t)1)) : 1;
	uintptr_t numPerT = seqLen / numUseThr;
	uintptr_t numExtT = seqLen % numUseThr;
	//set up the copies
	uintptr_t curOff = 0;
	for(uintptr_t i = 0; i<numUseThr; i++){
		uintptr_t curNum = numPerT + (i < numExtT);
		FastAQSequenceReaderUniform* curT = allocUni();
		curT->taskID = 2;
		curT->srcLen = curNum;
		curT->srcLoc = rnamS + curOff;
		curT->tgtEnt = packTo;
		curT->tgtOffset = useOff + curOff;
		if(usePool){
			liveTask.push_back(curT);
		}
		else{
			curT->doIt();
			cacheTask.push_back(curT);
		}
		curOff += curNum;
	}
}
void FastAQSequenceReader::close(){}

FastAQSequenceWriterUniform::FastAQSequenceWriterUniform(){}
FastAQSequenceWriterUniform::~FastAQSequenceWriterUniform(){}
void FastAQSequenceWriterUniform::doIt(){
	switch(taskID){
		case 0:
			memcpy(dstLoc, srcEnt->name + srcOffset, dstLen);
			break;
		case 1:
			memcpy(dstLoc, srcEnt->seq + srcOffset, dstLen);
			break;
		case 2:
			phredMeDo.phredsLog10ToScore(dstLen, srcEnt->qual + srcOffset, dstLoc);
			break;
		default:
			return;
	};
}

FastAQSequenceWriter::FastAQSequenceWriter(OutStream* mainTo){
	theStr = mainTo;
	storeAlloc = 0;
	storeHelp = 0;
	usePool = 0;
}
FastAQSequenceWriter::FastAQSequenceWriter(OutStream* mainTo, int numThread, ThreadPool* mainPool){
	theStr = mainTo;
	storeAlloc = FASTAQ_CHUNK_SIZE*numThread;
	storeHelp = (char*)malloc(storeAlloc);
	numThr = numThread;
	usePool = mainPool;
}
FastAQSequenceWriter::~FastAQSequenceWriter(){
	if(storeHelp){ free(storeHelp); }
	for(uintptr_t i = 0; i<cacheTask.size(); i++){ delete(cacheTask[i]); }
}
void FastAQSequenceWriter::write(SequenceEntry* nextOut){
	if(usePool){
		write(1, &nextOut);
		return;
	}
	theStr->writeByte(nextOut->qual ? '@' : '>');
	theStr->writeBytes(nextOut->name, nextOut->nameLen);
	theStr->writeByte('\n');
	theStr->writeBytes(nextOut->seq, nextOut->seqLen);
	theStr->writeByte('\n');
	if(nextOut->qual){
		theStr->writeByte('+');
		theStr->writeByte('\n');
		if(nextOut->seqLen > storeAlloc){
			if(storeHelp){ free(storeHelp); }
			storeAlloc = nextOut->seqLen;
			storeHelp = (char*)malloc(storeAlloc);
		}
		phredMeDo.phredsLog10ToScore(nextOut->seqLen, nextOut->qual, storeHelp);
		theStr->writeBytes(storeHelp, nextOut->seqLen);
		theStr->writeByte('\n');
	}
}
#define FASTAQ_WAIT_REMAINDER \
	for(uintptr_t i = 0; i<liveTask.size(); i++){\
		usePool->bigJoin(liveTask[i]->threadID);\
	}\
	usePool->bigEnd();\
	usePool->bigStart();\
	cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());\
	liveTask.clear();\
	theStr->writeBytes(storeHelp, storeOff);\
	storeOff = 0;
void FastAQSequenceWriter::write(uintptr_t numWrite, SequenceEntry** allOut){
	//do the easy thing if no pool
	if(usePool == 0){
		for(uintptr_t i = 0; i<numWrite; i++){
			write(allOut[i]);
		}
		return;
	}
	//clump together before write
	usePool->bigStart();
	FastAQSequenceWriterUniform* curUni;
	uintptr_t storeOff = 0;
	for(uintptr_t i = 0; i<numWrite; i++){
		SequenceEntry* curSeq = allOut[i];
		//figure out how much is needed for all the stuff: if more, wait out everything and start fresh
		uintptr_t curSeqNeed = 3 + curSeq->nameLen + curSeq->seqLen + (curSeq->haveQual ? (curSeq->seqLen + 3) : 0);
		if(curSeqNeed > (storeAlloc - storeOff)){
			FASTAQ_WAIT_REMAINDER
		}
		if(curSeqNeed > storeAlloc){
			if(storeHelp){ free(storeHelp); }
			storeAlloc = curSeqNeed;
			storeHelp = (char*)malloc(storeAlloc);
		}
		//prefix
			storeHelp[storeOff] = curSeq->haveQual ? '@' : '>';
			storeOff++;
		//pack the name
			curUni = allocUni();
			curUni->taskID = 0;
			curUni->dstLen = curSeq->nameLen;
			curUni->dstLoc = storeHelp + storeOff;
			curUni->srcOffset = 0;
			curUni->srcEnt = curSeq;
			curUni->threadID = usePool->bigAdd(curUni);
			storeOff += curSeq->nameLen;
			storeHelp[storeOff] = '\n';
			storeOff++;
		//pack the sequence
			uintptr_t numUT = std::max((uintptr_t)1, std::min(numThr, curSeq->seqLen / FASTAQ_CHUNK_SIZE));
			uintptr_t numPT = curSeq->seqLen / numUT;
			uintptr_t numET =curSeq->seqLen % numUT;
			uintptr_t curOff = 0;
			for(uintptr_t j = 0; j<numUT; j++){
				uintptr_t curNum = numPT + (j < numET);
				curUni = allocUni();
				curUni->taskID = 1;
				curUni->dstLen = curNum;
				curUni->dstLoc = storeHelp + storeOff;
				curUni->srcOffset = curOff;
				curUni->srcEnt = curSeq;
				curUni->threadID = usePool->bigAdd(curUni);
				curOff += curNum;
				storeOff += curNum;
			}
			storeHelp[storeOff] = '\n';
			storeOff++;
		//pack the quality
			if(curSeq->haveQual){
				storeHelp[storeOff] = '+';
				storeHelp[storeOff+1] = '\n';
				storeOff += 2;
				curOff = 0;
				for(uintptr_t j = 0; j<numUT; j++){
					uintptr_t curNum = numPT + (j < numET);
					curUni = allocUni();
					curUni->taskID = 2;
					curUni->dstLen = curNum;
					curUni->dstLoc = storeHelp + storeOff;
					curUni->srcOffset = curOff;
					curUni->srcEnt = curSeq;
					curUni->threadID = usePool->bigAdd(curUni);
					curOff += curNum;
					storeOff += curNum;
				}
				storeHelp[storeOff] = '\n';
				storeOff++;
			}
	}
	FASTAQ_WAIT_REMAINDER
	usePool->bigEnd();
}
void FastAQSequenceWriter::close(){}
FastAQSequenceWriterUniform* FastAQSequenceWriter::allocUni(){
	FastAQSequenceWriterUniform* toRet;
	if(cacheTask.size()){
		toRet = cacheTask[cacheTask.size()-1];
		cacheTask.pop_back();
	}
	else{
		toRet = new FastAQSequenceWriterUniform();
	}
	liveTask.push_back(toRet);
	return toRet;
}

/**The chunksize for threading.*/
#define GAIL_THREAD_GRAIN 0x000100
/**The block size for gail files*/
#define GAIL_BLOCK_SIZE 0x010000
//gail annotation
//name_len short_name_len have_qual
//seq_len
//name_offset
//seq_offset
//qual_offset
/**The number of bytes an annotation takes up in the file.*/
#define GAIL_ANNOTATION_LEN 40
/**Annotation from a gail file.*/
typedef struct{
	/**The length of the name.*/
	uintptr_t nameLen;
	/**The truncated name length.*/
	uintptr_t shortNameLen;
	/**Whether it has a quality.*/
	uintptr_t haveQual;
	/**The length of the sequence.*/
	uintptr_t seqLen;
	/**The offset of the name.*/
	uintmax_t nameOff;
	/**The offset of the sequence.*/
	uintmax_t seqOff;
	/**The offset of the quality.*/
	uintmax_t qualOff;
} GailAnnotation;
/**
 * Pack a gail annotation to make it ready to write.
 * @param toPack The data to pack.
 * @param packTo The place to pack it.
 */
void whodun_packGailAnnotation(GailAnnotation* toPack, char* packTo){
	int packFlag = (toPack->haveQual ? 1 : 0);
	packTo[0] = toPack->nameLen >> 16;
	packTo[1] = toPack->nameLen >> 8;
	packTo[2] = toPack->nameLen;
	packTo[3] = toPack->shortNameLen >> 16;
	packTo[4] = toPack->shortNameLen >> 8;
	packTo[5] = toPack->shortNameLen;
	packBE16(packFlag, packTo+6);
	packBE64(toPack->seqLen, packTo+8);
	packBE64(toPack->nameOff, packTo+16);
	packBE64(toPack->seqOff, packTo+24);
	packBE64(toPack->qualOff, packTo+32);
}
/**
 * Unpack a gail annotation.
 * @param toUnpack The packed data.
 * @param toStore The place to put it.
 */
void whodun_unpackGailAnnotation(char* toUnpack, GailAnnotation* toStore){
	toStore->nameLen = ((0x00FF&toUnpack[0])<<16) + ((0x00FF&toUnpack[1])<<8) + (0x00FF&toUnpack[2]);
	toStore->shortNameLen = ((0x00FF&toUnpack[3])<<16) + ((0x00FF&toUnpack[4])<<8) + (0x00FF&toUnpack[5]);
	int packFlag = unpackBE16(toUnpack+6);
	toStore->haveQual = (packFlag & 0x01);
	toStore->seqLen = unpackBE64(toUnpack+8);
	toStore->nameOff = unpackBE64(toUnpack+16);
	toStore->seqOff = unpackBE64(toUnpack+24);
	toStore->qualOff = unpackBE64(toUnpack+32);
}
/**
 * Handle allocations for an entry in a gail file.
 * @param toPrep The sequence to allocate for.
 * @param prepFor The entry about to be read.
 */
void whodun_gailPrepareEntry(SequenceEntry* toPrep, GailAnnotation* prepFor){
	if(toPrep->allocName < prepFor->nameLen){
		if(toPrep->name){ free(toPrep->name); }
		toPrep->name = (char*)malloc(prepFor->nameLen);
		toPrep->allocName = prepFor->nameLen;
	}
	if(toPrep->allocSeq < prepFor->seqLen){
		if(toPrep->seq){ free(toPrep->seq); }
		toPrep->seq = (char*)malloc(prepFor->seqLen);
		toPrep->allocSeq = prepFor->seqLen;
	}
	if(prepFor->haveQual && (toPrep->allocQual < prepFor->seqLen)){
		if(toPrep->qual){ free(toPrep->qual); }
		toPrep->qual = (double*)malloc(prepFor->seqLen*sizeof(double));
		toPrep->allocQual = prepFor->seqLen;
	}
	toPrep->nameLen = prepFor->nameLen;
	toPrep->shortNameLen = prepFor->shortNameLen;
	toPrep->seqLen = prepFor->seqLen;
	toPrep->haveQual = prepFor->haveQual;
}

void GailSequenceUniform::doIt(){
	switch(taskStage){
		case 0:
			//pack double to byte
			for(uintptr_t i = 0; i<numVal; i++){
				packBE64(packDbl(doper[i]), boperA + 8*i);
			}
			break;
		case 1:
			//direct byte copy
			memcpy(boperB, boperA, numVal);
			break;
		case 2:
			//unpack double
			for(uintptr_t i = 0; i<numVal; i++){
				doper[i] = unpackDbl(unpackBE64(boperA + 8*i));
			}
			break;
		default:
			throw std::runtime_error("Da fuq...?");
	};
}

GailSequenceReader::GailSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool){
	//get the length of the annotation file
	intmax_t numBlocksT = fileGetSize(fileName);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	if(numBlocksT % GAIL_ANNOTATION_LEN){std::string errMess("Malformed annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	numEntry = numBlocksT / GAIL_ANNOTATION_LEN;
	focusInd = 0;
	//Open it up
	GZipCompressionFactory gailComp;
	std::string baseFN(fileName);
	std::string nameFN = baseFN + ".n";
	std::string seqsFN = baseFN + ".s";
	std::string qualFN = baseFN + ".q";
	std::string nameBN = baseFN + ".n.blk";
	std::string seqsBN = baseFN + ".s.blk";
	std::string qualBN = baseFN + ".q.blk";
	indStr = new FileInStream(baseFN.c_str());
	namStr = new MultithreadBlockCompInStream(nameFN.c_str(), nameBN.c_str(), &gailComp, numThread, mainPool);
	seqStr = new MultithreadBlockCompInStream(seqsFN.c_str(), seqsBN.c_str(), &gailComp, numThread, mainPool);
	qualStr = new MultithreadBlockCompInStream(qualFN.c_str(), qualBN.c_str(), &gailComp, numThread, mainPool);
	//other setup
	numThr = numThread;
	usePool = mainPool;
	loadAlloc = 0;
	loadBuffer = 0;
	annotBuffer = (char*)malloc(numThread * (sizeof(GailAnnotation)+GAIL_ANNOTATION_LEN));
}
GailSequenceReader::~GailSequenceReader(){
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		usePool->joinTask(liveTask[i]->threadID);
	}
	cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
	for(uintptr_t i = 0; i<cacheTask.size(); i++){ delete(cacheTask[i]); }
	delete(indStr);
	delete(namStr);
	delete(seqStr);
	delete(qualStr);
	if(loadBuffer){ free(loadBuffer); }
	free(annotBuffer);
}
SequenceEntry* GailSequenceReader::read(){
	if(waitEnts.size()){
		SequenceEntry* toRet = waitEnts[0];
		waitEnts.pop_front();
		return toRet;
	}
	
	//load a fresh batch of annotations
	GailAnnotation* loadAnnot = (GailAnnotation*)annotBuffer;
	char* actAnnotBuff = (char*)(loadAnnot + numThr);
	uintptr_t numLoad = indStr->readBytes(actAnnotBuff, numThr * GAIL_ANNOTATION_LEN);
	if(numLoad == 0){ return 0; }
	if(numLoad % GAIL_ANNOTATION_LEN){
		throw std::runtime_error("Malformed annotation file.");
	}
	numLoad = numLoad / GAIL_ANNOTATION_LEN;
	for(uintptr_t i = 0; i<numLoad; i++){
		whodun_unpackGailAnnotation(actAnnotBuff + GAIL_ANNOTATION_LEN*i, loadAnnot + i);
	}
	//figure out the needed size, and make the buffer and entities
	uintptr_t needNameSize = 0;
	uintptr_t needSeqSize = 0;
	uintptr_t needQualSize = 0;
	for(uintptr_t i = 0; i<numLoad; i++){
		GailAnnotation* curAnnot = loadAnnot + i;
		SequenceEntry* curEnt = allocate();
		whodun_gailPrepareEntry(curEnt, curAnnot);
		waitEnts.push_back(curEnt);
		needNameSize += curAnnot->nameLen;
		needSeqSize += curAnnot->seqLen;
		if(curAnnot->haveQual){
			needQualSize += 8*curAnnot->seqLen;
		}
	}
	uintptr_t maxNeedSize = std::max(needNameSize, std::max(needSeqSize, needQualSize));
	if(maxNeedSize > loadAlloc){
		if(loadBuffer){ free(loadBuffer); }
		loadBuffer = (char*)malloc(maxNeedSize);
		loadAlloc = maxNeedSize;
	}
	//load the name data
		GailSequenceUniform* curU;
		#define GAIL_ALLOC_UNI \
			if(cacheTask.size()){\
				curU = cacheTask[cacheTask.size()-1];\
				cacheTask.pop_back();\
			}\
			else{\
				curU = new GailSequenceUniform();\
			}\
			liveTask.push_back(curU);
		#define GAIL_FIGURE_THREAD_SPLIT(forLen) \
			uintptr_t useTC = std::max((uintptr_t)1, std::min(numThr, (curEnt->forLen / numThr)));\
			uintptr_t numPT = curEnt->forLen / useTC;\
			uintptr_t numET = curEnt->forLen % useTC;
		uintptr_t curOff = 0;
		namStr->forceReadBytes(loadBuffer, needNameSize);
		usePool->bigStart();
		for(uintptr_t i = 0; i<numLoad; i++){
			SequenceEntry* curEnt = waitEnts[i];
			GAIL_FIGURE_THREAD_SPLIT(nameLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 1;
				curU->numVal = curNum;
				curU->boperA = loadBuffer + curOff;
				curU->boperB = curEnt->name + locOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += curNum;
			}
		}
		for(uintptr_t i = 0; i<liveTask.size(); i++){ usePool->bigJoin(liveTask[i]->threadID); }
		usePool->bigEnd();
		cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
		liveTask.clear();
	//load the sequence data
		curOff = 0;
		seqStr->forceReadBytes(loadBuffer, needSeqSize);
		usePool->bigStart();
		for(uintptr_t i = 0; i<numLoad; i++){
			SequenceEntry* curEnt = waitEnts[i];
			GAIL_FIGURE_THREAD_SPLIT(seqLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 1;
				curU->numVal = curNum;
				curU->boperA = loadBuffer + curOff;
				curU->boperB = curEnt->seq + locOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += curNum;
			}
		}
		for(uintptr_t i = 0; i<liveTask.size(); i++){ usePool->bigJoin(liveTask[i]->threadID); }
		usePool->bigEnd();
		cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
		liveTask.clear();
	//load the quality data
		curOff = 0;
		qualStr->forceReadBytes(loadBuffer, needQualSize);
		usePool->bigStart();
		for(uintptr_t i = 0; i<numLoad; i++){
			SequenceEntry* curEnt = waitEnts[i];
			if(!(curEnt->haveQual)){ continue; }
			GAIL_FIGURE_THREAD_SPLIT(seqLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 2;
				curU->numVal = curNum;
				curU->boperA = loadBuffer + curOff;
				curU->doper = curEnt->qual + locOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += (8*curNum);
			}
		}
		for(uintptr_t i = 0; i<liveTask.size(); i++){ usePool->bigJoin(liveTask[i]->threadID); }
		usePool->bigEnd();
		cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
		liveTask.clear();
	//should be a simple call
	return read();
}
void GailSequenceReader::close(){
	indStr->close();
	namStr->close();
	seqStr->close();
	qualStr->close();
}

RandacGailSequenceReader::RandacGailSequenceReader(const char* fileName){
	//get the length of the annotation file
	intmax_t numBlocksT = fileGetSize(fileName);
	if(numBlocksT < 0){std::string errMess("Problem examining annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	if(numBlocksT % GAIL_ANNOTATION_LEN){std::string errMess("Malformed annotation file "); errMess.append(fileName); throw std::runtime_error(errMess);}
	numEntry = numBlocksT / GAIL_ANNOTATION_LEN;
	//Open it up
	GZipCompressionFactory gailComp;
	std::string baseFN(fileName);
	std::string nameFN = baseFN + ".n";
	std::string seqsFN = baseFN + ".s";
	std::string qualFN = baseFN + ".q";
	std::string nameBN = baseFN + ".n.blk";
	std::string seqsBN = baseFN + ".s.blk";
	std::string qualBN = baseFN + ".q.blk";
	indStr = new FileInStream(baseFN.c_str());
	namStr = new BlockCompInStream(nameFN.c_str(), nameBN.c_str(), &gailComp);
	seqStr = new BlockCompInStream(seqsFN.c_str(), seqsBN.c_str(), &gailComp);
	qualStr = new BlockCompInStream(qualFN.c_str(), qualBN.c_str(), &gailComp);
	focusInd = 0;
	qualityAlloc = 0;
	qualityBuffer = 0;
	needSeek = 0;
}
RandacGailSequenceReader::~RandacGailSequenceReader(){
	delete(indStr);
	delete(namStr);
	delete(seqStr);
	delete(qualStr);
	if(qualityBuffer){ free(qualityBuffer); }
};
SequenceEntry* RandacGailSequenceReader::read(){
	if(focusInd >= numEntry){ return 0; }
	//read the annotation
	if(needSeek){
		indStr->seek(GAIL_ANNOTATION_LEN * focusInd);
	}
	GailAnnotation curAnn;
	{
		char annotBuff[GAIL_ANNOTATION_LEN];
		indStr->forceReadBytes(annotBuff, GAIL_ANNOTATION_LEN);
		whodun_unpackGailAnnotation(annotBuff, &curAnn);
	}
	//seek if necessary
	if(needSeek){
		namStr->seek(curAnn.nameOff);
		seqStr->seek(curAnn.seqOff);
		qualStr->seek(curAnn.qualOff);
	}
	//get the entry ready
	SequenceEntry* toRet = allocate();
	whodun_gailPrepareEntry(toRet, &curAnn);
	if(curAnn.haveQual && (qualityAlloc < curAnn.seqLen)){
		qualityAlloc = curAnn.seqLen;
		if(qualityBuffer){ free(qualityBuffer); }
		qualityBuffer = (char*)malloc(8*qualityAlloc);
	}
	//read
	namStr->forceReadBytes(toRet->name, curAnn.nameLen);
	seqStr->forceReadBytes(toRet->seq, curAnn.seqLen);
	if(curAnn.haveQual){
		qualStr->forceReadBytes(qualityBuffer, 8*curAnn.seqLen);
		for(uintptr_t i = 0; i<curAnn.seqLen; i++){
			toRet->qual[i] = unpackDbl(unpackBE64(qualityBuffer + 8*i));
		}
	}
	//prepare for the next round
	needSeek = 0;
	focusInd++;
	return toRet;
}
void RandacGailSequenceReader::close(){
	indStr->close();
	namStr->close();
	seqStr->close();
	qualStr->close();
}
uintmax_t RandacGailSequenceReader::getNumEntries(){
	return numEntry;
}
uintptr_t RandacGailSequenceReader::getEntryLength(uintmax_t entInd){
	needSeek = 1;
	indStr->seek(GAIL_ANNOTATION_LEN * entInd);
	GailAnnotation curAnn;
	char annotBuff[GAIL_ANNOTATION_LEN];
	indStr->forceReadBytes(annotBuff, GAIL_ANNOTATION_LEN);
	whodun_unpackGailAnnotation(annotBuff, &curAnn);
	return curAnn.seqLen;
}
SequenceEntry* RandacGailSequenceReader::getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase){
	needSeek = 1;
	//read the annotation
	indStr->seek(GAIL_ANNOTATION_LEN * endInd);
	GailAnnotation curAnn;
	{
		char annotBuff[GAIL_ANNOTATION_LEN];
		indStr->forceReadBytes(annotBuff, GAIL_ANNOTATION_LEN);
		whodun_unpackGailAnnotation(annotBuff, &curAnn);
	}
	if(toBase > curAnn.seqLen){ throw std::runtime_error("Getting sequence off the end of the sequence."); }
	if(fromBase > toBase){ throw std::runtime_error("Bad access."); }
	curAnn.seqLen = toBase - fromBase;
	//seek if necessary
	namStr->seek(curAnn.nameOff);
	seqStr->seek(curAnn.seqOff + fromBase);
	if(curAnn.haveQual){
		qualStr->seek(curAnn.qualOff + 8*fromBase);
	}
	//get the entry ready
	SequenceEntry* toRet = allocate();
	whodun_gailPrepareEntry(toRet, &curAnn);
	if(curAnn.haveQual && (qualityAlloc < curAnn.seqLen)){
		qualityAlloc = curAnn.seqLen;
		if(qualityBuffer){ free(qualityBuffer); }
		qualityBuffer = (char*)malloc(8*qualityAlloc);
	}
	//read
	namStr->forceReadBytes(toRet->name, curAnn.nameLen);
	seqStr->forceReadBytes(toRet->seq, curAnn.seqLen);
	if(curAnn.haveQual){
		qualStr->forceReadBytes(qualityBuffer, 8*curAnn.seqLen);
		for(uintptr_t i = 0; i<curAnn.seqLen; i++){
			toRet->qual[i] = unpackDbl(unpackBE64(qualityBuffer + 8*i));
		}
	}
	return toRet;
}

GailSequenceWriter::GailSequenceWriter(const char* fileName){
	qualityAlloc = 0;
	qualityBuffer = 0;
	totNameData = 0;
	totSeqsData = 0;
	totQualData = 0;
	GZipCompressionFactory gailComp;
	std::string baseFN(fileName);
	std::string nameFN = baseFN + ".n";
	std::string seqsFN = baseFN + ".s";
	std::string qualFN = baseFN + ".q";
	std::string nameBN = baseFN + ".n.blk";
	std::string seqsBN = baseFN + ".s.blk";
	std::string qualBN = baseFN + ".q.blk";
	indStr = new FileOutStream(0, baseFN.c_str());
	namStr = new BlockCompOutStream(0, GAIL_BLOCK_SIZE, nameFN.c_str(), nameBN.c_str(), &gailComp);
	seqStr = new BlockCompOutStream(0, GAIL_BLOCK_SIZE, seqsFN.c_str(), seqsBN.c_str(), &gailComp);
	qualStr = new BlockCompOutStream(0, GAIL_BLOCK_SIZE, qualFN.c_str(), qualBN.c_str(), &gailComp);
	usePool = 0;
	numThr = 1;
}
GailSequenceWriter::GailSequenceWriter(const char* fileName, int numThread, ThreadPool* mainPool){
	qualityAlloc = 0;
	qualityBuffer = 0;
	totNameData = 0;
	totSeqsData = 0;
	totQualData = 0;
	GZipCompressionFactory gailComp;
	std::string baseFN(fileName);
	std::string nameFN = baseFN + ".n";
	std::string seqsFN = baseFN + ".s";
	std::string qualFN = baseFN + ".q";
	std::string nameBN = baseFN + ".n.blk";
	std::string seqsBN = baseFN + ".s.blk";
	std::string qualBN = baseFN + ".q.blk";
	indStr = new FileOutStream(0, baseFN.c_str());
	namStr = new MultithreadBlockCompOutStream(0, GAIL_BLOCK_SIZE, nameFN.c_str(), nameBN.c_str(), &gailComp, numThread, mainPool);
	seqStr = new MultithreadBlockCompOutStream(0, GAIL_BLOCK_SIZE, seqsFN.c_str(), seqsBN.c_str(), &gailComp, numThread, mainPool);
	qualStr = new MultithreadBlockCompOutStream(0, GAIL_BLOCK_SIZE, qualFN.c_str(), qualBN.c_str(), &gailComp, numThread, mainPool);
	usePool = mainPool;
	numThr = numThread;
}
GailSequenceWriter::~GailSequenceWriter(){
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		usePool->joinTask(liveTask[i]->threadID);
	}
	cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
	for(uintptr_t i = 0; i<cacheTask.size(); i++){ delete(cacheTask[i]); }
	if(qualityBuffer){ free(qualityBuffer); }
	delete(indStr);
	delete(namStr);
	delete(seqStr);
	delete(qualStr);
}
void GailSequenceWriter::write(SequenceEntry* nextOut){
	if(usePool){
		write(1, &nextOut);
		return;
	}
	//write out the annotation
	GailAnnotation curAnn;
		curAnn.nameLen = nextOut->nameLen;
		curAnn.shortNameLen = nextOut->shortNameLen;
		curAnn.seqLen = nextOut->seqLen;
		curAnn.haveQual = nextOut->haveQual;
		curAnn.nameOff = totNameData;
		curAnn.seqOff = totSeqsData;
		curAnn.qualOff = totQualData;
	{
		char annotBuff[GAIL_ANNOTATION_LEN];
		whodun_packGailAnnotation(&curAnn, annotBuff);
		indStr->writeBytes(annotBuff, GAIL_ANNOTATION_LEN);
	}
	totNameData += curAnn.nameLen;
	totSeqsData += curAnn.seqLen;
	totQualData += (curAnn.haveQual ? (8*curAnn.seqLen) : 0);
	//write out the name and sequence
	namStr->writeBytes(nextOut->name, curAnn.nameLen);
	seqStr->writeBytes(nextOut->seq, curAnn.seqLen);
	if(curAnn.haveQual){
		if(qualityAlloc < 8*curAnn.seqLen){
			qualityAlloc = 8*curAnn.seqLen;
			if(qualityBuffer){ free(qualityBuffer); }
			qualityBuffer = (char*)malloc(qualityAlloc);
		}
		for(uintptr_t i = 0; i<curAnn.seqLen; i++){
			packBE64(packDbl(nextOut->qual[i]), qualityBuffer + 8*i);
		}
		qualStr->writeBytes(qualityBuffer, 8*curAnn.seqLen);
	}
}
void GailSequenceWriter::write(uintptr_t numWrite, SequenceEntry** allOut){
	//do the easy thing if no pool
	if(usePool == 0){
		for(uintptr_t i = 0; i<numWrite; i++){
			write(allOut[i]);
		}
		return;
	}
	GailSequenceUniform* curU;
	//write out the annotations
	uintptr_t totNameNeed = 0;
	uintptr_t totSeqNeed = 0;
	uintptr_t totQualNeed = 0;
	for(uintptr_t i = 0; i<numWrite; i++){
		SequenceEntry* nextOut = allOut[i];
		GailAnnotation curAnn;
		curAnn.nameLen = nextOut->nameLen;
		curAnn.shortNameLen = nextOut->shortNameLen;
		curAnn.seqLen = nextOut->seqLen;
		curAnn.haveQual = nextOut->haveQual;
		curAnn.nameOff = totNameData;
		curAnn.seqOff = totSeqsData;
		curAnn.qualOff = totQualData;
		char annotBuff[GAIL_ANNOTATION_LEN];
		whodun_packGailAnnotation(&curAnn, annotBuff);
		indStr->writeBytes(annotBuff, GAIL_ANNOTATION_LEN);
		totNameData += curAnn.nameLen;
			totNameNeed += curAnn.nameLen;
		totSeqsData += curAnn.seqLen;
			totSeqNeed += curAnn.seqLen;
		totQualData += (curAnn.haveQual ? (8*curAnn.seqLen) : 0);
			totQualNeed += (curAnn.haveQual ? (8*curAnn.seqLen) : 0);
	}
	//pack all the stuff
	uintptr_t needBuffSize = totNameNeed + totSeqNeed + 8*totQualNeed;
	if(needBuffSize > qualityAlloc){
		qualityAlloc = needBuffSize;
		if(qualityBuffer){ free(qualityBuffer); }
		qualityBuffer = (char*)malloc(qualityAlloc);
	}
	uintptr_t curOff = 0;
	usePool->bigStart();
	for(uintptr_t i = 0; i<numWrite; i++){
		SequenceEntry* curEnt = allOut[i];
		{
			GAIL_FIGURE_THREAD_SPLIT(nameLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 1;
				curU->numVal = curNum;
				curU->boperA = curEnt->name + locOff;
				curU->boperB = qualityBuffer + curOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += curNum;
			}
		}
	}
	for(uintptr_t i = 0; i<numWrite; i++){
		SequenceEntry* curEnt = allOut[i];
		{
			GAIL_FIGURE_THREAD_SPLIT(seqLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 1;
				curU->numVal = curNum;
				curU->boperA = curEnt->seq + locOff;
				curU->boperB = qualityBuffer + curOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += curNum;
			}
		}
	}
	for(uintptr_t i = 0; i<numWrite; i++){
		SequenceEntry* curEnt = allOut[i];
		if(curEnt->haveQual){
			GAIL_FIGURE_THREAD_SPLIT(seqLen)
			uintptr_t locOff = 0;
			for(uintptr_t j = 0; j<useTC; j++){
				GAIL_ALLOC_UNI
				uintptr_t curNum = numPT + (j<numET);
				curU->taskStage = 0;
				curU->numVal = curNum;
				curU->boperA = qualityBuffer + curOff;
				curU->doper = curEnt->qual + locOff;
				curU->threadID = usePool->bigAdd(curU);
				locOff += curNum;
				curOff += (8*curNum);
			}
		}
	}
	for(uintptr_t i = 0; i<liveTask.size(); i++){ usePool->bigJoin(liveTask[i]->threadID); }
	usePool->bigEnd();
	cacheTask.insert(cacheTask.end(), liveTask.begin(), liveTask.end());
	liveTask.clear();
	namStr->writeBytes(qualityBuffer, totNameNeed);
	seqStr->writeBytes(qualityBuffer + totNameNeed, totSeqNeed);
	qualStr->writeBytes(qualityBuffer + totNameNeed + totSeqNeed, totQualNeed);
}
void GailSequenceWriter::close(){
	indStr->close();
	namStr->close();
	seqStr->close();
	qualStr->close();
}

#define EXTENSION_IS_RAW_FASTA \
	bool isRawFA = false;\
	isRawFA = isRawFA || strendswith(fileName, ".fasta");\
	isRawFA = isRawFA || strendswith(fileName, ".fa");\
	isRawFA = isRawFA || strendswith(fileName, ".fastq");\
	isRawFA = isRawFA || strendswith(fileName, ".fq");

#define EXTENSION_IS_GZIP_FASTA \
	bool isZipFA = false;\
	isZipFA = isZipFA || strendswith(fileName, ".fasta.gz");\
	isZipFA = isZipFA || strendswith(fileName, ".fa.gz");\
	isZipFA = isZipFA || strendswith(fileName, ".fastq.gz");\
	isZipFA = isZipFA || strendswith(fileName, ".fq.gz");\
	isZipFA = isZipFA || strendswith(fileName, ".fasta.gzip");\
	isZipFA = isZipFA || strendswith(fileName, ".fa.gzip");\
	isZipFA = isZipFA || strendswith(fileName, ".fastq.gzip");\
	isZipFA = isZipFA || strendswith(fileName, ".fq.gzip");

ExtensionSequenceReader::ExtensionSequenceReader(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new FastAQSequenceReader(baseStrs[0]);
			return;
		}
		//anything explicity fasta
		EXTENSION_IS_RAW_FASTA
		if(isRawFA){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new FastAQSequenceReader(baseStrs[0]);
			return;
		}
		//compressed fasta
		EXTENSION_IS_GZIP_FASTA
		if(isZipFA){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new FastAQSequenceReader(baseStrs[0]);
			return;
		}
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new RandacGailSequenceReader(fileName);
			return;
		}
		//TODO extract from SAM
		//TODO extract from BAM
		//fasta is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new FastAQSequenceReader(baseStrs[0]);
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
ExtensionSequenceReader::ExtensionSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			wrapStr = new FastAQSequenceReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//anything explicity fasta
		EXTENSION_IS_RAW_FASTA
		if(isRawFA){
			baseStrs.push_back(new FileInStream(fileName));
			wrapStr = new FastAQSequenceReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//compressed fasta
		EXTENSION_IS_GZIP_FASTA
		if(isZipFA){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new FastAQSequenceReader(baseStrs[0], numThread, mainPool);
			return;
		}
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new GailSequenceReader(fileName, numThread, mainPool);
			return;
		}
		//TODO extract from SAM
		//TODO extract from BAM
		//fasta is the default
		baseStrs.push_back(new FileInStream(fileName));
		wrapStr = new FastAQSequenceReader(baseStrs[0], numThread, mainPool);
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
ExtensionSequenceReader::~ExtensionSequenceReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
SequenceEntry* ExtensionSequenceReader::read(){
	return wrapStr->read();
}
void ExtensionSequenceReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
SequenceEntry* ExtensionSequenceReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionSequenceReader::release(SequenceEntry* toRet){
	wrapStr->release(toRet);
}

ExtensionRandacSequenceReader::ExtensionRandacSequenceReader(const char* fileName){
	try{
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new RandacGailSequenceReader(fileName);
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
ExtensionRandacSequenceReader::ExtensionRandacSequenceReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new RandacGailSequenceReader(fileName);
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
ExtensionRandacSequenceReader::~ExtensionRandacSequenceReader(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
SequenceEntry* ExtensionRandacSequenceReader::read(){
	return wrapStr->read();
}
void ExtensionRandacSequenceReader::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}
SequenceEntry* ExtensionRandacSequenceReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionRandacSequenceReader::release(SequenceEntry* toRet){
	wrapStr->release(toRet);
}
uintmax_t ExtensionRandacSequenceReader::getNumEntries(){
	return wrapStr->getNumEntries();
}
uintptr_t ExtensionRandacSequenceReader::getEntryLength(uintmax_t entInd){
	return wrapStr->getEntryLength(entInd);
}
SequenceEntry* ExtensionRandacSequenceReader::getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase){
	return wrapStr->getEntry(endInd, fromBase, toBase);
}
SequenceEntry* ExtensionRandacSequenceReader::getEntry(uintmax_t endInd){
	return wrapStr->getEntry(endInd);
}

ExtensionSequenceWriter::ExtensionSequenceWriter(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new FastAQSequenceWriter(baseStrs[0]);
			return;
		}
		//anything explicity fasta
		EXTENSION_IS_RAW_FASTA
		if(isRawFA){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new FastAQSequenceWriter(baseStrs[0]);
			return;
		}
		//compressed fasta
		EXTENSION_IS_GZIP_FASTA
		if(isZipFA){
			throw std::runtime_error("Cannot block compress a fasta.");
		}
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new GailSequenceWriter(fileName);
			return;
		}
		//TODO extract from SAM
		//TODO extract from BAM
		//fasta is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new FastAQSequenceWriter(baseStrs[0]);
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
ExtensionSequenceWriter::ExtensionSequenceWriter(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			wrapStr = new FastAQSequenceWriter(baseStrs[0], numThread, mainPool);
			return;
		}
		//anything explicity fasta
		EXTENSION_IS_RAW_FASTA
		if(isRawFA){
			baseStrs.push_back(new FileOutStream(0, fileName));
			wrapStr = new FastAQSequenceWriter(baseStrs[0], numThread, mainPool);
			return;
		}
		//compressed fasta
		EXTENSION_IS_GZIP_FASTA
		if(isZipFA){
			throw std::runtime_error("Cannot block compress a fasta.");
		}
		//gail
		if(strendswith(fileName, ".gail")){
			wrapStr = new GailSequenceWriter(fileName, numThread, mainPool);
			return;
		}
		//TODO extract from SAM
		//TODO extract from BAM
		//fasta is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		wrapStr = new FastAQSequenceWriter(baseStrs[0], numThread, mainPool);
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
ExtensionSequenceWriter::~ExtensionSequenceWriter(){
	if(wrapStr){ delete(wrapStr); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		delete(baseStrs[i]);
	}
}
void ExtensionSequenceWriter::write(SequenceEntry* nextOut){
	wrapStr->write(nextOut);
}
void ExtensionSequenceWriter::write(uintptr_t numWrite, SequenceEntry** allOut){
	wrapStr->write(numWrite, allOut);
}
void ExtensionSequenceWriter::close(){
	if(wrapStr){ wrapStr->close(); }
	for(uintptr_t i = 0; i<baseStrs.size(); i++){
		baseStrs[i]->close();
	}
}

void whodun::getGeneticSequenceExtensions(int modeWrite, std::set<std::string>* toFill){
	toFill->insert(".fa");
	toFill->insert(".fasta");
	toFill->insert(".fq");
	toFill->insert(".fastq");
	toFill->insert(".gail");
	if(!modeWrite){
		toFill->insert(".fa.gz");
		toFill->insert(".fasta.gz");
		toFill->insert(".fq.gz");
		toFill->insert(".fastq.gz");
		toFill->insert(".fa.gzip");
		toFill->insert(".fasta.gzip");
		toFill->insert(".fq.gzip");
		toFill->insert(".fastq.gzip");
	}
}

void whodun::getRandacGeneticSequenceExtensions(std::set<std::string>* toFill){
	toFill->insert(".gail");
}

RandacSequenceCache::RandacSequenceCache(uintptr_t maxCacheSize, uintptr_t cacheChunk, RandacSequenceReader* useSource){
	maxCache = maxCacheSize / cacheChunk;
	cacheLine = cacheChunk;
	useSrc = useSource;
	saveNumEnt = useSource->getNumEntries();
}
RandacSequenceCache::~RandacSequenceCache(){
	std::map< std::pair<uintmax_t,uintptr_t>, SequenceEntry* >::iterator cacIt;
	for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
		useSrc->release(cacIt->second);
	}
}
SequenceEntry* RandacSequenceCache::read(){
	SequenceEntry* toRet;
	cacheLock.lockWrite();
	try{
		toRet = useSrc->read();
	}catch(std::runtime_error& errE){
		cacheLock.unlockWrite(); throw;
	}
	cacheLock.unlockWrite();
	return toRet;
}
void RandacSequenceCache::close(){}
SequenceEntry* RandacSequenceCache::allocate(){
	allocLock.lock();
	SequenceEntry* toRet = useSrc->allocate();
	allocLock.unlock();
	return toRet;
}
void RandacSequenceCache::release(SequenceEntry* toRet){
	allocLock.lock();
	useSrc->release(toRet);
	allocLock.unlock();
}
uintmax_t RandacSequenceCache::getNumEntries(){
	return saveNumEnt;
}
uintptr_t RandacSequenceCache::getEntryLength(uintmax_t entInd){
	bool haveEnt = false;
	uintptr_t toRet;
	
	cacheLock.lockRead();
	std::map<uintmax_t,uintptr_t>::iterator slIt = saveLens.find(entInd);
	if(slIt != saveLens.end()){
		haveEnt = true;
		toRet = slIt->second;
	}
	cacheLock.unlockRead();
	
	if(!haveEnt){
		cacheLock.lockWrite();
		try{
			toRet = useSrc->getEntryLength(entInd);
		}catch(std::exception& errE){ cacheLock.unlockWrite(); throw; }
		saveLens[entInd] = toRet;
		cacheLock.unlockWrite();
	}
	return toRet;
}
SequenceEntry* RandacSequenceCache::getEntry(uintmax_t endInd, uintptr_t fromBase, uintptr_t toBase){
	SequenceEntry* toRet = allocate();
	
	uintptr_t curFrom = fromBase;
	char* curFill;
	double* curFillQ;
	int firstGo = 1;
	uintptr_t endLength = getEntryLength(endInd);
	int haveWrite = 0;
	
	cacheLock.lockRead();
	while(curFrom < toBase){
		uintptr_t realOff = curFrom % cacheLine;
		uintptr_t realFromI = curFrom - realOff;
		uintptr_t realToI = realFromI + cacheLine; realToI = std::min(realToI, endLength);
		SequenceEntry* curEntBlock;
		std::pair<uintmax_t,uintptr_t> seqKey(endInd,realFromI);
		std::map< std::pair<uintmax_t,uintptr_t>, SequenceEntry* >::iterator cacIt;
		cacIt = saveSeqs.find(seqKey);
		if(cacIt == saveSeqs.end()){
			if(!haveWrite){
				cacheLock.unlockRead();
				cacheLock.lockWrite();
				haveWrite = 1;
				//release if too much stored
				if(saveSeqs.size() >= maxCache){
					for(cacIt = saveSeqs.begin(); cacIt != saveSeqs.end(); cacIt++){
						useSrc->release(cacIt->second);
					}
					saveSeqs.clear();
					saveLens.clear();
				}
			}
			//load in the new data
			try{
				curEntBlock = useSrc->getEntry(endInd, realFromI, realToI);
			}catch(std::exception& errE){ cacheLock.unlockWrite(); throw; }
			saveSeqs[seqKey] = curEntBlock;
		}
		else{
			curEntBlock = cacIt->second;
		}
		if(firstGo){
			toRet->preallocate(curEntBlock->nameLen, toBase - fromBase, curEntBlock->haveQual);
			memcpy(toRet->name, curEntBlock->name, curEntBlock->nameLen);
			toRet->nameLen = curEntBlock->nameLen;
			toRet->shortNameLen = curEntBlock->shortNameLen;
			toRet->seqLen = toBase - fromBase;
			toRet->haveQual = curEntBlock->haveQual;
			curFill = toRet->seq;
			curFillQ = toRet->qual;
			firstGo = 0;
		}
		uintptr_t curCopy = std::min((uintptr_t)(cacheLine - realOff), (uintptr_t)(toBase - curFrom));
		memcpy(curFill, curEntBlock->seq + realOff, curCopy);
		if(toRet->haveQual){
			memcpy(curFillQ, curEntBlock->qual + realOff, curCopy*sizeof(double));
		}
		curFrom = realToI;
		curFill += curCopy;
		curFillQ += curCopy;
	}
	if(haveWrite){
		cacheLock.unlockWrite();
	}
	else{
		cacheLock.unlockRead();
	}
	return toRet;
}

SequenceStringSource::SequenceStringSource(RandacSequenceReader* useSource){
	useSrc = useSource;
}
SequenceStringSource::~SequenceStringSource(){}
uintmax_t SequenceStringSource::numStrings(){
	uintmax_t toRet = useSrc->getNumEntries();
	return toRet;
}
uintmax_t SequenceStringSource::stringLength(uintmax_t strInd){
	uintmax_t toRet = useSrc->getEntryLength(strInd);
	return toRet;
}
void SequenceStringSource::getString(uintmax_t strInd, uintmax_t fromC, uintmax_t toC, char* saveC){
	SequenceEntry* toWork = useSrc->getEntry(strInd, fromC, toC);
	memcpy(saveC, toWork->seq, toC - fromC);
	useSrc->release(toWork);
}

