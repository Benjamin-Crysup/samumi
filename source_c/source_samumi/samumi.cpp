#include "samumi.h"

#include <string>
#include <algorithm>

int main(int argc, char** argv){
	//make the possible actions
		std::map<std::string,SamumiArgumentParser*> allActs;
		FuzzFinderArgumentParser act000; allActs["fuzzfind"] = &act000;
		FamilyPackArgumentParser act001; allActs["fampack"] = &act001;
		UmiHammingMergeArgumentParser act002; allActs["hammrg"] = &act002;
		UmiSummarizeArgumentParser act003; allActs["famsum"] = &act003;
		//TODO
	//simple help
		if((argc <= 1) || (strcmp(argv[1],"--help")==0) || (strcmp(argv[1],"-h")==0) || (strcmp(argv[1],"/?")==0)){
			std::cout << "Usage: samumi action OPTIONS" << std::endl;
			std::cout << "UMI mangling utilities." << std::endl;
			std::cout << "Possible actions are:" << std::endl;
			for(std::map<std::string,SamumiArgumentParser*>::iterator actIt = allActs.begin(); actIt != allActs.end(); actIt++){
				std::cout << actIt->first << std::endl;
				std::cout << actIt->second->mySummary << std::endl;
			}
			return 0;
		}
		if(strcmp(argv[1], "--version")==0){
			std::cout << "SAMmed UMI 0.0" << std::endl;
			std::cout << "Copyright (C) 2021 UNT Center for Human Identification" << std::endl;
			std::cout << "License LGPLv3+: GNU LGPL version 3 or later\n    <https://www.gnu.org/licenses/lgpl-3.0.html>\nThis is free software: you are free to change and redistribute it.\n";
			std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
			return 0;
		}
	//pick the action and parse arguments
		if(allActs.find(argv[1]) == allActs.end()){
			std::cerr << "Unknown action " << argv[1] << std::endl;
			return 1;
		}
		SamumiArgumentParser* theAct = allActs[argv[1]];
		if(theAct->parseArguments(argc-2, argv+2, &std::cout) < 0){
			std::cerr << theAct->argumentError << std::endl;
			return 1;
		}
		if(theAct->needRun == 0){ return 0; }
	//run the stupid thing
		try{
			theAct->runThing();
		}catch(std::exception& err){
			std::cerr << err.what() << std::endl;
			return 1;
		}
	return 0;
}

SamumiArgumentParser::~SamumiArgumentParser(){}

TSVHeaderMap::TSVHeaderMap(whodun::TextTableRow* headLine){
	for(uintptr_t i = 0; i<headLine->numEntries; i++){
		whodun::SizePtrString curEnt = {headLine->entrySizes[i], headLine->curEntries[i]};
		curEnt = whodun::trim(curEnt);
		if(curEnt.len == 0){ continue; }
		headOrder[std::string(curEnt.txt, curEnt.txt + curEnt.len)] = i;
	}
}
TSVHeaderMap::~TSVHeaderMap(){}

PrimerData::PrimerData(){}
PrimerData::~PrimerData(){}
void PrimerData::parsePrimerData(whodun::TextTableRow* loadF){
	locus.clear(); chrom.clear(); primer.clear(); anchor.clear();
	if(loadF->numEntries != 7){
		throw std::runtime_error("Bad entry in primer data.");
	}
	loadF->trim();
	locus.insert(locus.end(), loadF->curEntries[0], loadF->curEntries[0] + loadF->entrySizes[0]);
	chrom.insert(chrom.end(), loadF->curEntries[1], loadF->curEntries[1] + loadF->entrySizes[1]);
	std::string tmpPos(loadF->curEntries[2], loadF->curEntries[2] + loadF->entrySizes[2]);
		position = atol(tmpPos.c_str());
	std::string tmpRC(loadF->curEntries[3], loadF->curEntries[3] + loadF->entrySizes[3]);
		isRC = atol(tmpRC.c_str());
	primer.insert(primer.end(), loadF->curEntries[4], loadF->curEntries[4] + loadF->entrySizes[4]);
	anchor.insert(anchor.end(), loadF->curEntries[5], loadF->curEntries[5] + loadF->entrySizes[5]);
	primerRC.clear(); primerRC.insert(primerRC.end(), primer.begin(), primer.end());
		doRC.reverseComplement(primer.size(), &(primerRC[0]), 0);
	anchorRC.clear(); anchorRC.insert(anchorRC.end(), anchor.begin(), anchor.end());
		doRC.reverseComplement(anchor.size(), &(anchorRC[0]), 0);
	std::string tmpPer(loadF->curEntries[6], loadF->curEntries[6] + loadF->entrySizes[6]);
		period = atol(tmpPer.c_str());
}
std::ostream& operator<<(std::ostream& os, PrimerData const & toOut){
	os << std::string(toOut.locus.begin(), toOut.locus.end());
	os << "\t";
	os << std::string(toOut.primer.begin(), toOut.primer.end());
	os << "\t";
	os << std::string(toOut.anchor.begin(), toOut.anchor.end());
	return os;
}
PrimerSet::PrimerSet(){}
PrimerSet::PrimerSet(whodun::TextTableReader* loadFrom){
	parse(loadFrom);
}
PrimerSet::~PrimerSet(){}
PrimerSet::PrimerSet(const char* fileName){
	whodun::ExtensionTextTableReader inF(fileName);
	try{
		parse(&inF);
	}
	catch(std::exception& errE){
		inF.close(); throw;
	}
	inF.close();
}
void PrimerSet::parse(whodun::TextTableReader* loadFrom){
	//TODO header line
	PrimerData curLoad;
	whodun::TextTableRow* curR = loadFrom->read();
	while(curR){
		{
			if(curR->numEntries < 6){ goto continueTgt; }
			curLoad.parsePrimerData(curR);
			allPrimer.push_back(curLoad);
		}
		continueTgt:
		loadFrom->release(curR);
		curR = loadFrom->read();
	}
}

const char* fuzzyFind(const char* lookForS, const char* lookForE, const char* lookInS, const char* lookInE, uintptr_t maxFuzz){
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
char* fuzzyFind(const char* lookForS, const char* lookForE, char* lookInS, char* lookInE, uintptr_t maxFuzz){
	const char* lookInSTmp = lookInS;
	const char* lookInETmp = lookInE;
	return (char*)fuzzyFind(lookForS, lookForE, lookInSTmp, lookInETmp, maxFuzz);
}

EntryReadData::EntryReadData(){}
EntryReadData::~EntryReadData(){}
void EntryReadData::clear(){
	qname.clear();
	canonPrimer.clear();
	canonLocus.clear();
	canonUMI.clear();
	canonSequence.clear();
	sequence.clear();
	sequencePhred.clear();
	umi.clear();
	umiPhred.clear();
	primer.clear();
	primerPhred.clear();
	anchor.clear();
	anchorPhred.clear();
	common.clear();
	commonPhred.clear();
}
void EntryReadData::parse(whodun::TextTableRow* toParse){
	toParse->trim();
	whodun::SizePtrString curStr;
	#define ENTRY_READ_PARSE(toVar, fromInd) \
		toVar.clear();\
		curStr.txt = toParse->curEntries[fromInd];\
		curStr.len = toParse->entrySizes[fromInd];\
		curStr = whodun::trim(curStr);\
		toVar.insert(toVar.end(), curStr.txt, curStr.txt + curStr.len);
	ENTRY_READ_PARSE(qname, 0)
	ENTRY_READ_PARSE(canonPrimer, 1)
	ENTRY_READ_PARSE(canonLocus, 2)
	ENTRY_READ_PARSE(canonUMI, 3)
	ENTRY_READ_PARSE(canonSequence, 4)
	ENTRY_READ_PARSE(sequence, 5)
	ENTRY_READ_PARSE(sequencePhred, 6)
	ENTRY_READ_PARSE(umi, 7)
	ENTRY_READ_PARSE(umiPhred, 8)
	ENTRY_READ_PARSE(primer, 9)
	ENTRY_READ_PARSE(primerPhred, 10)
	ENTRY_READ_PARSE(anchor, 11)
	ENTRY_READ_PARSE(anchorPhred, 12)
	ENTRY_READ_PARSE(common, 13)
	ENTRY_READ_PARSE(commonPhred, 14)
}
void EntryReadData::dump(whodun::TextTableRow* toDump){
	uintptr_t totSize = 0;
		totSize += qname.size();
		totSize += canonPrimer.size();
		totSize += canonLocus.size();
		totSize += canonUMI.size();
		totSize += canonSequence.size();
		totSize += sequence.size();
		totSize += sequencePhred.size();
		totSize += umi.size();
		totSize += umiPhred.size();
		totSize += primer.size();
		totSize += primerPhred.size();
		totSize += anchor.size();
		totSize += anchorPhred.size();
		totSize += common.size();
		totSize += commonPhred.size();
	toDump->ensureCapacity(15, totSize);
	toDump->numEntries = 15;
	uintptr_t curOff = 0;
	#define ENTRY_WRITE_DUMP(toVar, fromInd) \
		toDump->entrySizes[fromInd] = toVar.size();\
		toDump->curEntries[fromInd] = toDump->entryStore + curOff;\
		if(toVar.size()){\
			memcpy(toDump->entryStore + curOff, &(toVar[0]), toVar.size());\
			curOff += toVar.size();\
		}
	ENTRY_WRITE_DUMP(qname, 0)
	ENTRY_WRITE_DUMP(canonPrimer, 1)
	ENTRY_WRITE_DUMP(canonLocus, 2)
	ENTRY_WRITE_DUMP(canonUMI, 3)
	ENTRY_WRITE_DUMP(canonSequence, 4)
	ENTRY_WRITE_DUMP(sequence, 5)
	ENTRY_WRITE_DUMP(sequencePhred, 6)
	ENTRY_WRITE_DUMP(umi, 7)
	ENTRY_WRITE_DUMP(umiPhred, 8)
	ENTRY_WRITE_DUMP(primer, 9)
	ENTRY_WRITE_DUMP(primerPhred, 10)
	ENTRY_WRITE_DUMP(anchor, 11)
	ENTRY_WRITE_DUMP(anchorPhred, 12)
	ENTRY_WRITE_DUMP(common, 13)
	ENTRY_WRITE_DUMP(commonPhred, 14)
}

EntryReadDataBulkParse::EntryReadDataBulkParse(uintptr_t numThread) : ParallelForLoop(numThread){}
void EntryReadDataBulkParse::doSingle(uintptr_t threadInd, uintptr_t ind){
	allStore[ind]->parse(allParse[ind]);
}

EntryReadDataBulkDump::EntryReadDataBulkDump(uintptr_t numThread) : ParallelForLoop(numThread){}
void EntryReadDataBulkDump::doSingle(uintptr_t threadInd, uintptr_t ind){
	allStore[ind]->dump(allParse[ind]);
}

void fullReadEntryReadData(const char* fileName, std::vector<EntryReadData*>* allData, uintptr_t numThread, whodun::ThreadPool* useThread, uintptr_t threadGrain){
	whodun::ExtensionTextTableReader inF(fileName, numThread, useThread);
	try{
		std::vector<whodun::TextTableRow*> baseLoad(numThread * threadGrain);
		EntryReadDataBulkParse doParse(numThread);
			doParse.allParse = &(baseLoad[0]);
			doParse.startI = 0;
		int isMore = 1;
		while(isMore){
			uintptr_t numLoad = 0;
			while(numLoad < baseLoad.size()){
				baseLoad[numLoad] = inF.read();
				if(!baseLoad[numLoad]){ isMore = 0; break; }
				numLoad++;
			}
			if(numLoad == 0){ break; }
			uintptr_t fromInd = allData->size();
			for(uintptr_t i = 0; i<numLoad; i++){
				allData->push_back(new EntryReadData());
			}
			doParse.allStore = &((*allData)[fromInd]);
			doParse.endI = numLoad;
			doParse.doIt(useThread);
		}
	}catch(std::exception& errE){ inF.close(); throw; }
	inF.close();
}

void fullWriteEntryReadData(const char* fileName, std::vector<EntryReadData*>* allData, uintptr_t numThread, whodun::ThreadPool* useThread, uintptr_t threadGrain){
	whodun::ExtensionTextTableWriter outF(fileName, numThread, useThread);
	if(allData->size() == 0){ outF.close(); return; }
	try{
		std::vector<whodun::TextTableRow> realDump(numThread * threadGrain);
		std::vector<whodun::TextTableRow*> baseDump(numThread * threadGrain);
			for(uintptr_t i = 0; i<realDump.size(); i++){
				baseDump[i] = &(realDump[i]);
			}
		EntryReadDataBulkDump doDump(numThread);
			doDump.allParse = &(baseDump[0]);
			doDump.startI = 0;
		uintptr_t nextOut = 0;
		while(nextOut < allData->size()){
			doDump.allStore = &((*allData)[nextOut]);
			uintptr_t numOut = std::min(baseDump.size(), (allData->size() - nextOut));
			doDump.endI = numOut;
			doDump.doIt(useThread);
			outF.write(numOut, &(baseDump[0]));
			nextOut += numOut;
		}
	}catch(std::exception& errE){ outF.close(); throw; }
	outF.close();
}

bool primerOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB){
	return entA.first < entB.first;
}
bool umiOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB){
	return entA.second < entB.second;
}
bool familyOnlyCompareFunc(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB){
	if(entA.first < entB.first){ return true; }
	if(entB.first < entA.first){ return false; }
	return entA.second < entB.second;
}
bool UmiCharacterCompareFunc::operator()(const PrimerUmiAlleleMarker& entA, const PrimerUmiAlleleMarker& entB){
	if(entB.second.len <= charInd){
		return false;
	}
	if(entA.second.len <= charInd){
		return true;
	}
	return entA.second.txt[charInd] < entB.second.txt[charInd];
}

uintptr_t hammingDistance(whodun::SizePtrString strA, whodun::SizePtrString strB){
	uintptr_t comDist = std::min(strA.len, strB.len);
	uintptr_t curHam = std::max(strA.len, strB.len) - comDist;
	for(uintptr_t i = 0; i<comDist; i++){
		if(strA.txt[i] != strB.txt[i]){
			curHam++;
		}
	}
	return curHam;
}

UmiFamilyNeighborhood::UmiFamilyNeighborhood(uintptr_t numReads, EntryReadData** allReads, uintptr_t numThread, whodun::ThreadPool* useThread){
	numReadData = numReads;
	allReadData = allReads;
	//TODO multithread
	std::vector< std::pair<PrimerUmiAlleleMarker, uintptr_t> > allPack(numReads);
	for(uintptr_t i = 0; i<numReads; i++){
		allPack[i].second = i;
		EntryReadData* curDat = allReads[i];
		PrimerUmiAlleleMarker* curID = &(allPack[i].first);
		curID->first = {curDat->canonPrimer.size(), curDat->canonPrimer.size() ? &(curDat->canonPrimer[0]) : (char*)0};
		curID->second = {curDat->canonUMI.size(), curDat->canonUMI.size() ? &(curDat->canonUMI[0]) : (char*)0};
		curID->third = {curDat->canonSequence.size(), curDat->canonSequence.size() ? &(curDat->canonSequence[0]) : (char*)0};
	}
	std::sort(allPack.begin(), allPack.end());
	allUmiEntries.resize(numReads);
	umiEntryInds.resize(numReads);
	for(uintptr_t i = 0; i<numReads; i++){
		allUmiEntries[i] = allPack[i].first;
		umiEntryInds[i] = allPack[i].second;
	}
	//get the families
	/*
	std::vector<PrimerUmiAlleleMarker>::iterator begIt = allUmiEntries.begin();
	std::vector<PrimerUmiAlleleMarker>::iterator endIt = allUmiEntries.end();
	std::vector<PrimerUmiAlleleMarker>::iterator curLowIt = begIt;
	while(curLowIt != endIt){
		std::vector<PrimerUmiAlleleMarker>::iterator curHigIt = std::upper_bound(curLowIt, endIt, *curLowIt, familyOnlyCompareFunc);
		familyStarts.push_back(curLowIt - begIt);
		familyEnds.push_back(curHigIt - begIt);
		curLowIt = curHigIt;
	}
	*/
}
UmiFamilyNeighborhood::~UmiFamilyNeighborhood(){}
void UmiFamilyNeighborhood::findUmiWithinRange(whodun::SizePtrString primer, whodun::SizePtrString onUmi, uintptr_t maxHam, std::vector<uintptr_t>* storeInds){
	PrimerUmiAlleleMarker workMark;
	workMark.first = primer;
	workMark.second = onUmi;
	UmiCharacterCompareFunc charComp;
	//find the primer bounds
	std::vector<PrimerUmiAlleleMarker>::iterator primerStart = std::lower_bound(allUmiEntries.begin(), allUmiEntries.end(), workMark, primerOnlyCompareFunc);
	std::vector<PrimerUmiAlleleMarker>::iterator primerEnd = std::upper_bound(allUmiEntries.begin(), allUmiEntries.end(), workMark, primerOnlyCompareFunc);
	//set up a stack
	std::vector< std::vector<PrimerUmiAlleleMarker>::iterator > stackBoundS;
	std::vector< std::vector<PrimerUmiAlleleMarker>::iterator > stackBoundE;
	std::vector<uintptr_t> stackDists;
	std::vector<uintptr_t> stackInds;
	#define STACK_PUSH(bndS, bndE, cDist, cChar) stackBoundS.push_back(bndS); stackBoundE.push_back(bndE); stackDists.push_back(cDist); stackInds.push_back(cChar);
	STACK_PUSH(primerStart, primerEnd, 0, 0)
	//run through the stack
	while(stackBoundS.size()){
		//get the current frame
			uintptr_t curSI = stackBoundS.size() - 1;
			std::vector<PrimerUmiAlleleMarker>::iterator curBndS = stackBoundS[curSI];
			std::vector<PrimerUmiAlleleMarker>::iterator curBndE = stackBoundE[curSI];
			uintptr_t curDist = stackDists[curSI];
			uintptr_t curChar = stackInds[curSI];
			stackBoundS.pop_back();
			stackBoundE.pop_back();
			stackDists.pop_back();
			stackInds.pop_back();
		//quit if too far away
			if(curDist > maxHam){ continue; }
			if(curBndS == curBndE){ continue; }
		//if no more characters, report all within
			if(curChar >= onUmi.len){
				for(std::vector<PrimerUmiAlleleMarker>::iterator curIt = curBndS; curIt != curBndE; curIt++){
					if(hammingDistance(onUmi, curIt->second) <= maxHam){
						storeInds->push_back(umiEntryInds[curIt - allUmiEntries.begin()]);
					}
				}
				continue;
			}
		//figure out where to look next
			charComp.charInd = curChar;
			std::vector<PrimerUmiAlleleMarker>::iterator nxtBndS = std::lower_bound(curBndS, curBndE, workMark, charComp);
			std::vector<PrimerUmiAlleleMarker>::iterator nxtBndE = std::upper_bound(curBndS, curBndE, workMark, charComp);
			STACK_PUSH(curBndS, nxtBndS, curDist + 1, curChar + 1)
			STACK_PUSH(nxtBndE, curBndE, curDist + 1, curChar + 1)
			STACK_PUSH(nxtBndS, nxtBndE, curDist, curChar + 1)
	}
}


