#include "samumi.h"

#include <algorithm>

FamilyPackArgumentParser::FamilyPackArgumentParser(){
	outFileN = 0;
	inFileN = 0;
	numThread = 1;
	threadGrain = 1024;
	defOutFN[0] = '-'; defOutFN[1] = 0;
	myMainDoc = "Usage: samumi fampack [OPTION] [FILE]\n"
		"Pack reads into families.\n"
		"The OPTIONS are:\n";
	myVersionDoc = "SammedUmi fampack 1.0";
	myCopyrightDoc = "Copyright (C) 2021 UNT HSC Center for Human Identification";
	mySummary = "Pack read level data into families.";
	whodun::ArgumentParserStrMeta outMeta("Output File");
		outMeta.isFile = true;
		outMeta.fileWrite = true;
		whodun::getTextTableExtensions(1, &(outMeta.fileExts));
		addStringOption("--out", &outFileN, 0, "    Specify the output file.\n    --out File.tsv\n", &outMeta);
	whodun::ArgumentParserStrMeta inMeta("Input File");
		inMeta.isFile = true;
		whodun::getTextTableExtensions(0, &(inMeta.fileExts));
		addStringOption("--in", &inFileN, 0, "    Specify the input file.\n    --in File.tsv\n", &inMeta);
	whodun::ArgumentParserIntMeta threadMeta("Threads");
		addIntegerOption("--thread", &numThread, 0, "    How many threads to use.\n    --thread 1\n", &threadMeta);
}
FamilyPackArgumentParser::~FamilyPackArgumentParser(){}
int FamilyPackArgumentParser::posteriorCheck(){
	if(!outFileN || (strlen(outFileN)==0)){
		outFileN = defOutFN;
	}
	if(!inFileN || (strlen(inFileN)==0)){
		inFileN = defOutFN;
	}
	if(numThread <= 0){
		argumentError = "Need at least one thread.";
		return 1;
	}
	return 0;
}
void FamilyPackArgumentParser::runThing(){
	std::vector<EntryReadData*> allData;
	try{
		//load
		whodun::ThreadPool useThread(numThread);
		fullReadEntryReadData(inFileN, &allData, numThread, &useThread, threadGrain);
		//pack Primers, UMIs and haplotypes
		std::vector<EntryReadData*> allSort(allData.size());
		if(allData.size()){
			UmiFamilyNeighborhood neigh(allData.size(), &(allData[0]), numThread, &useThread);
			for(uintptr_t i = 0; i<allData.size(); i++){
				allSort[i] = allData[neigh.umiEntryInds[i]];
			}
		}
		//output
		fullWriteEntryReadData(outFileN, &allSort, numThread, &useThread, threadGrain);
	}catch(std::exception& errE){ for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); } }
	for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); }
}
