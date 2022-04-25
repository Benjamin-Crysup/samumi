#include "samumi.h"

#include <map>
#include <set>
#include <algorithm>

#define UMI_AMBIGUITY_TRASH 0
#define UMI_AMBIGUITY_BIGGER 1
#define UMI_AMBIGUITY_SEPARATE 2
#define HAPLOTYPE_AMBIGUITY_TRASH 0
#define HAPLOTYPE_AMBIGUITY_IGNORE 1
#define HAPLOTYPE_AMBIGUITY_SEPARATE 2

//TODO multithread

UmiHammingMergeArgumentParser::UmiHammingMergeArgumentParser(){
	outFileN = 0;
	inFileN = 0;
	numThread = 1;
	threadGrain = 1024;
	mergeHam = 1;
	umiAmbigMode = UMI_AMBIGUITY_TRASH;
	hapAmbigMode = HAPLOTYPE_AMBIGUITY_TRASH;
	defOutFN[0] = '-'; defOutFN[1] = 0;
	myMainDoc = "Usage: samumi hammrg [OPTION] [FILE]\n"
		"Attempt to merge close umis.\n"
		"The OPTIONS are:\n";
	myVersionDoc = "SammedUmi hammrg 1.0";
	myCopyrightDoc = "Copyright (C) 2021 UNT HSC Center for Human Identification";
	mySummary = "Merge close umis.";
	whodun::ArgumentParserStrMeta outMeta("Output File");
		outMeta.isFile = true;
		outMeta.fileWrite = true;
		whodun::getTextTableExtensions(1, &(outMeta.fileExts));
		addStringOption("--out", &outFileN, 0, "    Specify the output file.\n    --out File.tsv\n", &outMeta);
	whodun::ArgumentParserStrMeta inMeta("Input File");
		inMeta.isFile = true;
		whodun::getTextTableExtensions(0, &(inMeta.fileExts));
		addStringOption("--in", &inFileN, 0, "    Specify the input file.\n    --in File.tsv\n", &inMeta);
	whodun::ArgumentParserStrMeta dumpMeta("Trash File");
		dumpMeta.isFile = true;
		dumpMeta.fileWrite = true;
		whodun::getTextTableExtensions(1, &(dumpMeta.fileExts));
		addStringOption("--dump", &dumpFileN, 0, "    Specify the file to write dropped reads to.\n    --dump File.tsv\n", &dumpMeta);
	whodun::ArgumentParserIntMeta threadMeta("Threads");
		addIntegerOption("--thread", &numThread, 0, "    How many threads to use.\n    --thread 1\n", &threadMeta);
	whodun::ArgumentParserIntMeta hamMeta("Hamming Distance");
		addIntegerOption("--ham", &mergeHam, 0, "    The maximum distance to merge.\n    --ham 1\n", &hamMeta);
	whodun::ArgumentParserEnumMeta umiKillMeta("UMI Kill on Ambiguity");
		umiKillMeta.enumGroup = "UmiAmbig";
		addEnumerationFlag("--umiambiguitykill", &umiAmbigMode, UMI_AMBIGUITY_TRASH, "    If there are multiple candidate merge umis, drop the ambiguous group.\n", &umiKillMeta);
	whodun::ArgumentParserEnumMeta umiBigMeta("UMI Bigger Ambiguity");
		umiBigMeta.enumGroup = "UmiAmbig";
		addEnumerationFlag("--umiambiguitybig", &umiAmbigMode, UMI_AMBIGUITY_BIGGER, "    If there are multiple candidate merge umis, pick the larger group.\n", &umiBigMeta);
	whodun::ArgumentParserEnumMeta umiSkipMeta("UMI Keep on Ambiguity");
		umiSkipMeta.enumGroup = "UmiAmbig";
		addEnumerationFlag("--umiambiguitynone", &umiAmbigMode, UMI_AMBIGUITY_SEPARATE, "    If there are multiple candidate merge umis, do not merge.\n", &umiSkipMeta);
	whodun::ArgumentParserEnumMeta hapKillMeta("Haplotype Kill on Mismatch");
		hapKillMeta.enumGroup = "HapAmbig";
		addEnumerationFlag("--hapdiffkill", &hapAmbigMode, HAPLOTYPE_AMBIGUITY_TRASH, "    If nearby umis have different haplotypes, drop the smaller group.\n", &hapKillMeta);
	whodun::ArgumentParserEnumMeta hapIgnoreMeta("Haplotype Ignore Mismatch");
		hapIgnoreMeta.enumGroup = "HapAmbig";
		addEnumerationFlag("--hapdiffignore", &hapAmbigMode, HAPLOTYPE_AMBIGUITY_IGNORE, "    Merge all nearby UMIs: don't bother comparing haplotypes.\n", &hapIgnoreMeta);
	whodun::ArgumentParserEnumMeta hapSkipMeta("Haplotype Keep on Mismatch");
		hapSkipMeta.enumGroup = "HapAmbig";
		addEnumerationFlag("--hapdiffnone", &hapAmbigMode, HAPLOTYPE_AMBIGUITY_SEPARATE, "    If nearby umis have different haplotypes, do not merge.\n", &hapSkipMeta);
}
UmiHammingMergeArgumentParser::~UmiHammingMergeArgumentParser(){}
int UmiHammingMergeArgumentParser::posteriorCheck(){
	if(!outFileN || (strlen(outFileN)==0)){
		outFileN = defOutFN;
	}
	if(!inFileN || (strlen(inFileN)==0)){
		inFileN = defOutFN;
	}
	if(!dumpFileN || (strlen(dumpFileN)==0)){
		dumpFileN = 0;
	}
	if(numThread <= 0){
		argumentError = "Need at least one thread.";
		return 1;
	}
	if(mergeHam < 0){
		argumentError = "Hamming distance must be positive.";
		return 1;
	}
	return 0;
}
void UmiHammingMergeArgumentParser::runThing(){
	std::vector<EntryReadData*> allData;
	try{
		whodun::SizePtrString emptyStr = {0, 0};
		//load
		whodun::ThreadPool useThread(numThread);
		fullReadEntryReadData(inFileN, &allData, numThread, &useThread, threadGrain);
		//figure the merges
		std::set< std::pair<std::string,std::string> > allDrop;
		std::map< std::pair<std::string,std::string>,std::string > allMerge;
		{
			std::set<UmiFamilyMarker> localDrop;
			std::map<UmiFamilyMarker,whodun::SizePtrString> localMerge;
			//get the neighborhood
				UmiFamilyNeighborhood neigh(allData.size(), allData.size() ? &(allData[0]) : (EntryReadData**)0, numThread, &useThread);
			//count how many are in each family
				std::vector< std::pair<uintptr_t,UmiFamilyMarker> > familyCount;
				std::vector<PrimerUmiAlleleMarker>::iterator endIt = neigh.allUmiEntries.end();
				std::vector<PrimerUmiAlleleMarker>::iterator curLowIt = neigh.allUmiEntries.begin();
				while(curLowIt != endIt){
					std::vector<PrimerUmiAlleleMarker>::iterator curHigIt = std::upper_bound(curLowIt, endIt, *curLowIt, familyOnlyCompareFunc);
					UmiFamilyMarker curFam(curLowIt->first, curLowIt->second);
					familyCount.push_back( std::pair<uintptr_t,UmiFamilyMarker>(curHigIt - curLowIt, curFam) );
					curLowIt = curHigIt;
				}
				std::sort(familyCount.begin(), familyCount.end());
			//do any merges
				std::set<UmiFamilyMarker> nearFams;
				std::vector<uintptr_t> nearInds;
				std::set<UmiFamilyMarker> nearBigFams;
				for(uintptr_t famI = 0; famI < familyCount.size(); famI++){
					//find the family
						PrimerUmiAlleleMarker smallFamLook(familyCount[famI].second.first, familyCount[famI].second.second, emptyStr);
						std::vector<PrimerUmiAlleleMarker>::iterator smallFamS = std::lower_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), smallFamLook, familyOnlyCompareFunc);
						std::vector<PrimerUmiAlleleMarker>::iterator smallFamE = std::upper_bound(smallFamS, neigh.allUmiEntries.end(), smallFamLook, familyOnlyCompareFunc);
						uintptr_t smallFamSize = smallFamE - smallFamS;
					//figure the primary haplotype
						uintptr_t smallPCnt = 0;
						whodun::SizePtrString smallWinHap = emptyStr;
						std::vector<PrimerUmiAlleleMarker>::iterator curSmallIt = smallFamS;
						while(curSmallIt != smallFamE){
							std::vector<PrimerUmiAlleleMarker>::iterator nxtSmallIt = std::upper_bound(curSmallIt, smallFamE, *curSmallIt);
							uintptr_t curPCnt = nxtSmallIt - curSmallIt;
							if(curPCnt > smallPCnt){
								smallPCnt = curPCnt;
								smallWinHap = curSmallIt->third;
							}
							curSmallIt = nxtSmallIt;
						}
					//find any nearby
						nearFams.clear();
						nearInds.clear();
						neigh.findUmiWithinRange(smallFamLook.first, smallFamLook.second, mergeHam, &nearInds);
						for(uintptr_t i = 0; i<nearInds.size(); i++){
							EntryReadData* curEnt = allData[nearInds[i]];
							whodun::SizePtrString bigPrim = {curEnt->canonPrimer.size(), curEnt->canonPrimer.size() ? &(curEnt->canonPrimer[0]) : (char*)0};
							whodun::SizePtrString bigUmi = {curEnt->canonUMI.size(), curEnt->canonUMI.size() ? &(curEnt->canonUMI[0]) : (char*)0};
							nearFams.insert(UmiFamilyMarker(bigPrim, bigUmi));
						}
					//remove this family from the nearby set
						nearFams.erase(familyCount[famI].second);
						if(nearFams.size() == 0){ continue; }
					//remove smaller/handled families from the nearby set
						nearBigFams.clear();
						uintptr_t biggestFamSize = 0;
						UmiFamilyMarker biggestFam;
						for(std::set<UmiFamilyMarker>::iterator curFIt = nearFams.begin(); curFIt != nearFams.end(); curFIt++){
							PrimerUmiAlleleMarker bigFamLook(curFIt->first, curFIt->second, emptyStr);
							std::vector<PrimerUmiAlleleMarker>::iterator bigFamS = std::lower_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), bigFamLook, familyOnlyCompareFunc);
							std::vector<PrimerUmiAlleleMarker>::iterator bigFamE = std::upper_bound(bigFamS, neigh.allUmiEntries.end(), bigFamLook, familyOnlyCompareFunc);
							uintptr_t curFamSize = bigFamE - bigFamS;
							if(curFamSize < smallFamSize){ continue; }
							if(localDrop.find(*curFIt) != localDrop.end()){ continue; }
							if(localMerge.find(*curFIt) != localMerge.end()){ continue; }
							nearBigFams.insert(*curFIt);
							if(curFamSize > biggestFamSize){
								biggestFamSize = curFamSize;
								biggestFam = *curFIt;
							}
						}
						if(nearBigFams.size() == 0){ continue; }
					//handle umi set ambiguity
						if(nearBigFams.size() > 1){
							if(umiAmbigMode == UMI_AMBIGUITY_TRASH){
								localDrop.insert(familyCount[famI].second);
								continue;
							}
							else if(umiAmbigMode == UMI_AMBIGUITY_SEPARATE){
								continue;
							}
						}
					//find the haplotype of the largest
						PrimerUmiAlleleMarker bigFamLook(biggestFam.first, biggestFam.second, emptyStr);
						std::vector<PrimerUmiAlleleMarker>::iterator bigFamS = std::lower_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), bigFamLook, familyOnlyCompareFunc);
						std::vector<PrimerUmiAlleleMarker>::iterator bigFamE = std::upper_bound(bigFamS, neigh.allUmiEntries.end(), bigFamLook, familyOnlyCompareFunc);
						uintptr_t bigPCnt = 0;
						whodun::SizePtrString bigWinHap = emptyStr;
						std::vector<PrimerUmiAlleleMarker>::iterator curBigIt = bigFamS;
						while(curBigIt != bigFamE){
							std::vector<PrimerUmiAlleleMarker>::iterator nxtSmallIt = std::upper_bound(curBigIt, bigFamE, *curBigIt);
							uintptr_t curPCnt = nxtSmallIt - curBigIt;
							if(curPCnt > bigPCnt){
								bigPCnt = curPCnt;
								bigWinHap = curBigIt->third;
							}
							curBigIt = nxtSmallIt;
						}
					//handle haplotype issues
						if((smallWinHap.len != bigWinHap.len) || memcmp(smallWinHap.txt, bigWinHap.txt, smallWinHap.len)){
							if(hapAmbigMode == HAPLOTYPE_AMBIGUITY_TRASH){
								localDrop.insert(familyCount[famI].second);
								continue;
							}
							else if(hapAmbigMode == HAPLOTYPE_AMBIGUITY_SEPARATE){
								continue;
							}
						}
					//mark the winner
						localMerge[familyCount[famI].second] = biggestFam.second;
				}
			//make new memory
				for(std::set<UmiFamilyMarker>::iterator curD = localDrop.begin(); curD != localDrop.end(); curD++){
					std::string primN(curD->first.txt, curD->first.txt + curD->first.len);
					std::string umiN(curD->second.txt, curD->second.txt + curD->second.len);
					allDrop.insert( std::pair<std::string,std::string>(primN,umiN) );
				}
				for(std::map<UmiFamilyMarker,whodun::SizePtrString>::iterator curM = localMerge.begin(); curM != localMerge.end(); curM++){
					std::string primN(curM->first.first.txt, curM->first.first.txt + curM->first.first.len);
					std::string umiN(curM->first.second.txt, curM->first.second.txt + curM->first.second.len);
					std::string umiAlt(curM->second.txt, curM->second.txt + curM->second.len);
					allMerge[std::pair<std::string,std::string>(primN,umiN)] = umiAlt;
				}
		}
		//apply the merges
		std::vector<EntryReadData*> allSurvive;
		std::vector<EntryReadData*> allDead;
		std::pair<std::string,std::string> lookForPU;
		for(uintptr_t i = 0; i<allData.size(); i++){
			EntryReadData* curDat = allData[i];
			lookForPU.first.clear();
				lookForPU.first.insert(lookForPU.first.end(), curDat->canonPrimer.begin(), curDat->canonPrimer.end());
			lookForPU.second.clear();
				lookForPU.second.insert(lookForPU.second.end(), curDat->canonUMI.begin(), curDat->canonUMI.end());
			if(allDrop.find(lookForPU) != allDrop.end()){
				allDead.push_back(curDat);
				continue;
			}
			std::map< std::pair<std::string,std::string>,std::string >::iterator mrgIt = allMerge.find(lookForPU);
			if(mrgIt != allMerge.end()){
				int isDrop = 0;
				while(true){
					lookForPU.second.clear();
					lookForPU.second.insert(lookForPU.second.end(), mrgIt->second.begin(), mrgIt->second.end());
					if(allDrop.find(lookForPU) != allDrop.end()){
						isDrop = 1;
						break;
					}
					std::map< std::pair<std::string,std::string>,std::string >::iterator nxtIt = allMerge.find(lookForPU);
					if(nxtIt == allMerge.end()){ break; }
					mrgIt = nxtIt;
				}
				if(isDrop){
					allDead.push_back(curDat);
					continue;
				}
				curDat->canonUMI.clear();
				curDat->canonUMI.insert(curDat->canonUMI.end(), mrgIt->second.begin(), mrgIt->second.end());
			}
			allSurvive.push_back(curDat);
		}
		//resort and repack the data
		std::vector<EntryReadData*> allSort(allSurvive.size());
		if(allSurvive.size()){
			UmiFamilyNeighborhood neigh(allSurvive.size(), &(allSurvive[0]), numThread, &useThread);
			for(uintptr_t i = 0; i<allSurvive.size(); i++){
				allSort[i] = allSurvive[neigh.umiEntryInds[i]];
			}
		}
		//output
		fullWriteEntryReadData(outFileN, &allSort, numThread, &useThread, threadGrain);
		if(dumpFileN){
			fullWriteEntryReadData(dumpFileN, &allDead, numThread, &useThread, threadGrain);
		}
	}catch(std::exception& errE){ for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); } throw; }
	for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); }
}

