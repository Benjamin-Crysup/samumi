#include "samumi.h"

#include <math.h>
#include <algorithm>

#include "whodun_stat_util.h"
#include "whodun_gen_sequence.h"

UmiSummarizeArgumentParser::UmiSummarizeArgumentParser(){
	outFileN = 0;
	inFileN = 0;
	numThread = 1;
	threadGrain = 1024;
	mergeHam = 1;
	readTagInfo = 0;
	primerFileN = 0;
	defOutFN[0] = '-'; defOutFN[1] = 0;
	myMainDoc = "Usage: samumi famsum [OPTION] [FILE]\n"
		"Summarize data for UMIs.\n"
		"The OPTIONS are:\n";
	myVersionDoc = "SammedUmi famsum 1.0";
	myCopyrightDoc = "Copyright (C) 2021 UNT HSC Center for Human Identification";
	mySummary = "Summarize UMI families.";
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
	whodun::ArgumentParserIntMeta hamMeta("Hamming Distance");
		addIntegerOption("--ham", &mergeHam, 0, "    The maximum distance to note.\n    --ham 1\n", &hamMeta);
	whodun::ArgumentParserStrMeta typeMeta("Run Type");
		addStringOption("--type", &readTagInfo, 0, "    Specify the run type.\n    --type UDI\n", &typeMeta);
	whodun::ArgumentParserStrMeta primerMeta("Primer File");
		primerMeta.isFile = true;
		whodun::getTextTableExtensions(0, &(primerMeta.fileExts));
		addStringOption("--primer", &primerFileN, 0, "    Specify the primer file.\n    --primer File.tsv\n", &primerMeta);
}
UmiSummarizeArgumentParser::~UmiSummarizeArgumentParser(){}
int UmiSummarizeArgumentParser::posteriorCheck(){
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
	if(mergeHam < 0){
		argumentError = "Hamming distance must be positive.";
		return 1;
	}
	if(!readTagInfo || (strlen(readTagInfo)==0)){
		argumentError = "Need a tag for this run.";
		return 1;
	}
	if(!primerFileN || (strlen(primerFileN)==0)){
		argumentError = "Need to specify the primer data.";
		return 1;
	}
	return 0;
}
void UmiSummarizeArgumentParser::runThing(){
	//load the primer data
	PrimerSet allPrime(primerFileN);
	std::map<whodun::SizePtrString,uintptr_t> locusPeriods;
	for(uintptr_t i = 0; i<allPrime.allPrimer.size(); i++){
		if(allPrime.allPrimer[i].primer.size()){
			whodun::SizePtrString curLocN = {allPrime.allPrimer[i].primer.size(), &(allPrime.allPrimer[i].primer[0])};
			locusPeriods[curLocN] = allPrime.allPrimer[i].period;
		}
	}
	//set up the load
	whodun::SizePtrString emptyStr = {0,0};
	std::vector<EntryReadData*> allData;
	whodun::ThreadPool useThread(numThread);
	whodun::ExtensionTextTableWriter outF(outFileN, numThread, &useThread);
	try{
		char numMarshalBuf[8*sizeof(intmax_t) + 8*sizeof(double) + 8];
		//output the main header
		std::string allHeadText;
		std::vector<uintptr_t> allHeadLoc;
		std::vector<uintptr_t> allHeadLen;
		#define ADD_HEADER_ZT_ENTRY(headTxt) allHeadLoc.push_back(allHeadText.size()); allHeadText.append(headTxt); allHeadLen.push_back(strlen(headTxt));
		#define ADD_HEADER_INT_ENTRY(newEnt) sprintf(numMarshalBuf, "%jd", (intmax_t)(newEnt)); allHeadLoc.push_back(allHeadText.size()); allHeadText.append(numMarshalBuf); allHeadLen.push_back(strlen(numMarshalBuf));
		#define ADD_HEADER_DBL_ENTRY(newEnt) sprintf(numMarshalBuf, "%.9e", (double)(newEnt)); allHeadLoc.push_back(allHeadText.size()); allHeadText.append(numMarshalBuf); allHeadLen.push_back(strlen(numMarshalBuf));
		#define ADD_HEADER_SPS_ENTRY(headTxt) allHeadLoc.push_back(allHeadText.size()); allHeadText.insert(allHeadText.end(), headTxt.txt, headTxt.txt + headTxt.len); allHeadLen.push_back(headTxt.len);
		#define ADD_HEADER_CV_ENTRY(headTxt) allHeadLoc.push_back(allHeadText.size()); allHeadText.insert(allHeadText.end(), headTxt.begin(), headTxt.end()); allHeadLen.push_back(headTxt.size());
		#define DUMP_ENTRY(toTTR) \
			toTTR.ensureCapacity(allHeadLoc.size(), allHeadText.size()+1);\
			toTTR.numEntries = allHeadLen.size();\
			strcpy(toTTR.entryStore, allHeadText.c_str());\
			for(uintptr_t i = 0; i<allHeadLoc.size(); i++){\
				toTTR.curEntries[i] = toTTR.entryStore + allHeadLoc[i];\
				toTTR.entrySizes[i] = allHeadLen[i];\
			}
		{
			ADD_HEADER_ZT_ENTRY("Locus")
			ADD_HEADER_ZT_ENTRY("Primer")
			ADD_HEADER_ZT_ENTRY("UMI")
			ADD_HEADER_ZT_ENTRY("Reads") //number of reads for this family
			ADD_HEADER_ZT_ENTRY("Period") //motif size for this primer
			ADD_HEADER_ZT_ENTRY("Locus_Total_Reads") //total number of reads for the primer
			ADD_HEADER_ZT_ENTRY("Person_Total_Reads") //total number of reads in the file.
			ADD_HEADER_ZT_ENTRY("Primary_Allele") //The winning allele
			ADD_HEADER_ZT_ENTRY("Primary_Read_Count") //number of reads of primary haplotype
			ADD_HEADER_ZT_ENTRY("Primary_Length") //length of primary haplotype
			ADD_HEADER_ZT_ENTRY("Secondary_Read_Count") //number of reads with secondary haplotype
			ADD_HEADER_ZT_ENTRY("Secondary_Length") //length of secondary haplotype
			ADD_HEADER_ZT_ENTRY("Hap_Count") //number of haplotypes in family
			ADD_HEADER_ZT_ENTRY("Nearby_UMI") //whether there is a neighbor
			ADD_HEADER_ZT_ENTRY("Nearby_UMI_Seq") //UMI sequence of neighbor
			ADD_HEADER_ZT_ENTRY("Nearby_Read") //number of reads for neighbor umi
			ADD_HEADER_ZT_ENTRY("Nearby_Primary_Count") //the number of times this UMI family primary shows up in neighbor UMI
			ADD_HEADER_ZT_ENTRY("Nearby_UMI_Same") //whether the two UMI families have the same primary
			ADD_HEADER_ZT_ENTRY("Nearby_UMI_Length") //the length of neighbor primary
			ADD_HEADER_ZT_ENTRY("Run_Type") //a tag for this particular run
			
			//quality metrics
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_UMI") //max-min score of the UMI
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_Primer") //max-min score of the primer
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_Anchor") //max-min score of the anchor
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_Common") //max-min score of the common
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_Primary") //max-min score of the primary allele
			ADD_HEADER_ZT_ENTRY("Phred_MaxMin_Alternate") //max-min score of the alternate alleles
			ADD_HEADER_ZT_ENTRY("Phred_Aggregate_Best_Primary") //best prod(1-p_err) for primary allele
			ADD_HEADER_ZT_ENTRY("Phred_Aggregate_Best_Alternate") //best prod(1-p_err) for alternate allele
			ADD_HEADER_ZT_ENTRY("Phred_Aggregate") //aggregate score of the primary (p(no nuc wrong)=prod(1-pi), p(one match good & all mis bad) = (1 - prod(p match bad)) * prod(p mis bad))
			
			//mangled stuff
			ADD_HEADER_ZT_ENTRY("Reads_per_Locus_Total_Reads")
			ADD_HEADER_ZT_ENTRY("Reads_per_Person_Total_Reads")
			ADD_HEADER_ZT_ENTRY("Primary_Proportion")
			ADD_HEADER_ZT_ENTRY("Secondary_Proportion")
			ADD_HEADER_ZT_ENTRY("Secondary_Proportion_per_Primary_Proportion")
			ADD_HEADER_ZT_ENTRY("Hap_Per_Read")
			ADD_HEADER_ZT_ENTRY("Length_Diff")
			ADD_HEADER_ZT_ENTRY("Length_Diff_mod_Period")
			ADD_HEADER_ZT_ENTRY("Nearby_Read_Ratio")
			ADD_HEADER_ZT_ENTRY("Nearby_Primary_Ratio")
			ADD_HEADER_ZT_ENTRY("Nearby_UMI_Length_Diff")
			ADD_HEADER_ZT_ENTRY("Nearby_UMI_Length_Diff_mod_Period")
			
			whodun::TextTableRow headDump;
			DUMP_ENTRY(headDump);
			outF.write(&headDump);
		}
		//load
		//make output space
		std::vector<whodun::TextTableRow> baseDump(numThread * threadGrain);
		std::vector<whodun::TextTableRow*> endDump(baseDump.size());
			for(uintptr_t i = 0; i<endDump.size(); i++){ endDump[i] = &(baseDump[i]); }
		fullReadEntryReadData(inFileN, &allData, numThread, &useThread, threadGrain);
		//get the neighborhood
		UmiFamilyNeighborhood neigh(allData.size(), allData.size() ? &(allData[0]) : (EntryReadData**)0, numThread, &useThread);
		//figure out how many are present per locus (also note the total)
		uintptr_t personTotalReads = neigh.allUmiEntries.size();
		std::map<whodun::SizePtrString,uintptr_t> primerTotalReads;
		std::vector<PrimerUmiAlleleMarker>::iterator begIt = neigh.allUmiEntries.begin();
		std::vector<PrimerUmiAlleleMarker>::iterator endIt = neigh.allUmiEntries.end();
		std::vector<PrimerUmiAlleleMarker>::iterator curLowIt = begIt;
		while(curLowIt != endIt){
			std::vector<PrimerUmiAlleleMarker>::iterator curHigIt = std::upper_bound(curLowIt, endIt, *curLowIt, primerOnlyCompareFunc);
			primerTotalReads[curLowIt->first] = (curHigIt - curLowIt);
			curLowIt = curHigIt;
		}
		//run down the families
		uintptr_t numRow = 0;
		std::map<whodun::SizePtrString,uintptr_t> allAlleles;
		std::vector< std::pair<uintptr_t,whodun::SizePtrString> > allAlleleCnts;
		std::vector<uintptr_t> nearInds;
		std::map<whodun::SizePtrString,uintptr_t> allUmis;
		std::vector<double> phredConv;
		whodun::GeneticSequenceUtilities phredConvM;
		curLowIt = begIt;
		while(curLowIt != endIt){
			#define AMALGAMATE_ALLELE_DATA(fromIt, toIt) \
				allAlleles.clear();\
				for(std::vector<PrimerUmiAlleleMarker>::iterator tmpIt = fromIt; tmpIt != toIt; tmpIt++){\
					allAlleles[tmpIt->third]++;\
				}\
				allAlleleCnts.clear();\
				for(std::map<whodun::SizePtrString,uintptr_t>::iterator curM = allAlleles.begin(); curM!=allAlleles.end(); curM++){\
					allAlleleCnts.push_back(std::pair<uintptr_t,whodun::SizePtrString>(curM->second, curM->first));\
				}\
				std::sort(allAlleleCnts.begin(), allAlleleCnts.end());\
				std::reverse(allAlleleCnts.begin(), allAlleleCnts.end());
			//basic setup
			allHeadText.clear();
			allHeadLoc.clear();
			allHeadLen.clear();
			EntryReadData* curDat = allData[neigh.umiEntryInds[curLowIt - begIt]];
			ADD_HEADER_CV_ENTRY(curDat->canonLocus);
			ADD_HEADER_SPS_ENTRY(curLowIt->first);
			ADD_HEADER_SPS_ENTRY(curLowIt->second);
			
			std::vector<PrimerUmiAlleleMarker>::iterator curHigIt = std::upper_bound(curLowIt, endIt, *curLowIt, familyOnlyCompareFunc);
			uintptr_t readCount = curHigIt - curLowIt;
			ADD_HEADER_INT_ENTRY(readCount);
			
			if(locusPeriods.find(curLowIt->first) == locusPeriods.end()){
				throw std::runtime_error(std::string("Period not found for primer: ") + std::string(curLowIt->first.txt, curLowIt->first.txt + curLowIt->first.len));
			}
			uintptr_t primerPeriod = locusPeriods[curLowIt->first];
			ADD_HEADER_INT_ENTRY(primerPeriod);
			
			uintptr_t locusReadCount = primerTotalReads[curLowIt->first];
			ADD_HEADER_INT_ENTRY(locusReadCount);
			ADD_HEADER_INT_ENTRY(personTotalReads);
			
			AMALGAMATE_ALLELE_DATA(curLowIt,curHigIt)
			uintptr_t topAlleleCnt = allAlleleCnts[0].first;
			whodun::SizePtrString topAllele = allAlleleCnts[0].second;
			uintptr_t altAlleleCnt = (allAlleleCnts.size() > 1) ? allAlleleCnts[1].first : 0;
			whodun::SizePtrString altAllele = {0,0};
			uintptr_t altAlleleLen = topAllele.len;
			if(allAlleleCnts.size() > 1){ altAllele = allAlleleCnts[1].second; altAlleleLen = altAllele.len; }
			uintptr_t hapCount = allAlleleCnts.size();
			ADD_HEADER_SPS_ENTRY(topAllele);
			ADD_HEADER_INT_ENTRY(topAlleleCnt);
			ADD_HEADER_INT_ENTRY(topAllele.len);
			ADD_HEADER_INT_ENTRY(altAlleleCnt);
			ADD_HEADER_INT_ENTRY(altAlleleLen);
			ADD_HEADER_INT_ENTRY(hapCount);
			
			nearInds.clear();
			allUmis.clear();
			int isNearUmi = 0;
			uintptr_t altUmiReads = 0;
			whodun::SizePtrString altUmi = {0,0};
			neigh.findUmiWithinRange(curLowIt->first, curLowIt->second, mergeHam, &nearInds);
			for(uintptr_t i = 0; i<nearInds.size(); i++){
				EntryReadData* altDat = allData[nearInds[i]];
				whodun::SizePtrString curUmi = {0,0};
				if(altDat->canonUMI.size()){ curUmi = {altDat->canonUMI.size(), &(altDat->canonUMI[0])}; }
				allUmis[curUmi]++;
			}
			allUmis.erase(curLowIt->second);
			for(std::map<whodun::SizePtrString,uintptr_t>::iterator curM = allUmis.begin(); curM != allUmis.end(); curM++){
				if(curM->second > altUmiReads){
					isNearUmi = 1;
					altUmiReads = curM->second;
					altUmi = curM->first;
				}
			}
			if(isNearUmi){
				ADD_HEADER_ZT_ENTRY("TRUE");
			}
			else{
				ADD_HEADER_ZT_ENTRY("FALSE");
			}
			ADD_HEADER_SPS_ENTRY(altUmi);
			ADD_HEADER_INT_ENTRY(altUmiReads);
			
			uintptr_t nearUMIPrimaryPresent = 0;
			if(isNearUmi){
				PrimerUmiAlleleMarker nearMark(curLowIt->first, altUmi, topAllele);
				std::vector<PrimerUmiAlleleMarker>::iterator altHapS = std::lower_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), nearMark);
				std::vector<PrimerUmiAlleleMarker>::iterator altHapE = std::upper_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), nearMark);
				nearUMIPrimaryPresent = altHapE - altHapS;
			}
			ADD_HEADER_INT_ENTRY(nearUMIPrimaryPresent);
			
			int nearUmiSameHap = 1;
			uintptr_t altUmiLength = topAllele.len;
			if(isNearUmi){
				PrimerUmiAlleleMarker nearMark(curLowIt->first, altUmi, emptyStr);
				std::vector<PrimerUmiAlleleMarker>::iterator altHapS = std::lower_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), nearMark, familyOnlyCompareFunc);
				std::vector<PrimerUmiAlleleMarker>::iterator altHapE = std::upper_bound(neigh.allUmiEntries.begin(), neigh.allUmiEntries.end(), nearMark, familyOnlyCompareFunc);
				AMALGAMATE_ALLELE_DATA(altHapS,altHapE)
				whodun::SizePtrString altUmiHap = allAlleleCnts[0].second;
				altUmiLength = altUmiHap.len;
				nearUmiSameHap = (altUmiHap.len == topAllele.len) && (memcmp(altUmiHap.txt, topAllele.txt, topAllele.len)==0);
			}
			if(nearUmiSameHap){
				ADD_HEADER_ZT_ENTRY("TRUE");
			}
			else{
				ADD_HEADER_ZT_ENTRY("FALSE");
			}
			ADD_HEADER_INT_ENTRY(altUmiLength);
			
			ADD_HEADER_ZT_ENTRY(readTagInfo)
			
			uintptr_t curMaxMinUmi = 34;
			uintptr_t curMaxMinPrim = 34;
			uintptr_t curMaxMinAnch = 34;
			uintptr_t curMaxMinCom = 34;
			uintptr_t curMaxMinTop = 34;
			uintptr_t curMaxMinSec = 34;
			double bestPhredAggMat = -1.0 / 0.0;
			double bestPhredAggMis = -1.0 / 0.0;
			double curPhredAggMat = 0.0;
			double curPhredAggMis = 0.0;
			for(std::vector<PrimerUmiAlleleMarker>::iterator curLookIt = curLowIt; curLookIt != curHigIt; curLookIt++){
				EntryReadData* focDat = allData[neigh.umiEntryInds[curLookIt - begIt]];
				int isMatch = (focDat->canonSequence.size() == topAllele.len);
				if(isMatch && topAllele.len){
					isMatch = memcmp(topAllele.txt, &(focDat->canonSequence[0]), topAllele.len)==0;
				}
				//find minima and aggregate
				if(focDat->umiPhred.size()){
					uintptr_t curMinP = 255;
					for(uintptr_t i = 0; i<focDat->umiPhred.size(); i++){
						curMinP = std::min(curMinP, (uintptr_t)(0x00FF & focDat->umiPhred[i]));
					}
					curMaxMinUmi = std::max(curMaxMinUmi, curMinP);
				}
				if(focDat->primerPhred.size()){
					uintptr_t curMinP = 255;
					for(uintptr_t i = 0; i<focDat->primerPhred.size(); i++){
						curMinP = std::min(curMinP, (uintptr_t)(0x00FF & focDat->primerPhred[i]));
					}
					curMaxMinPrim = std::max(curMaxMinPrim, curMinP);
				}
				if(focDat->anchorPhred.size()){
					uintptr_t curMinP = 255;
					for(uintptr_t i = 0; i<focDat->anchorPhred.size(); i++){
						curMinP = std::min(curMinP, (uintptr_t)(0x00FF & focDat->anchorPhred[i]));
					}
					curMaxMinAnch = std::max(curMaxMinUmi, curMinP);
				}
				if(focDat->commonPhred.size()){
					uintptr_t curMinP = 255;
					for(uintptr_t i = 0; i<focDat->commonPhred.size(); i++){
						curMinP = std::min(curMinP, (uintptr_t)(0x00FF & focDat->commonPhred[i]));
					}
					curMaxMinCom = std::max(curMaxMinUmi, curMinP);
				}
				double log10E = log10(exp(1.0));
				if(focDat->sequencePhred.size()){
					uintptr_t curMinP = 255;
					for(uintptr_t i = 0; i<focDat->sequencePhred.size(); i++){
						curMinP = std::min(curMinP, (uintptr_t)(0x00FF & focDat->sequencePhred[i]));
					}
					phredConv.resize(focDat->sequencePhred.size());
					phredConvM.phredsToLog10Prob(focDat->sequencePhred.size(), &(focDat->sequencePhred[0]), &(phredConv[0]));
					double curSeqProb = 0.0;
					for(uintptr_t i = 0; i<phredConv.size(); i++){
						curSeqProb += whodun::logOneMinusExp(-phredConv[i]/log10E);
					}
					if(isMatch){
						curMaxMinTop = std::max(curMaxMinTop, curMinP);
						bestPhredAggMat = std::max(bestPhredAggMat, curSeqProb);
						curPhredAggMat += whodun::logOneMinusExp(-curSeqProb);
					}
					else{
						curMaxMinSec = std::max(curMaxMinSec, curMinP);
						bestPhredAggMis = std::max(bestPhredAggMis, curSeqProb);
						curPhredAggMis += whodun::logOneMinusExp(-curSeqProb);
					}
				}
			}
			bestPhredAggMat = bestPhredAggMat / log(10.0);
			bestPhredAggMis = bestPhredAggMis / log(10.0);
			double curPhredAgg = whodun::logOneMinusExp(-curPhredAggMat) + curPhredAggMis;
				curPhredAgg = curPhredAgg / log(10.0);
			curMaxMinUmi -= 33;
			curMaxMinPrim -= 33;
			curMaxMinAnch -= 33;
			curMaxMinCom -= 33;
			curMaxMinTop -= 33;
			curMaxMinSec -= 33;
			ADD_HEADER_INT_ENTRY(curMaxMinUmi);
			ADD_HEADER_INT_ENTRY(curMaxMinPrim);
			ADD_HEADER_INT_ENTRY(curMaxMinAnch);
			ADD_HEADER_INT_ENTRY(curMaxMinCom);
			ADD_HEADER_INT_ENTRY(curMaxMinTop);
			ADD_HEADER_INT_ENTRY(curMaxMinSec);
			ADD_HEADER_DBL_ENTRY(bestPhredAggMat);
			ADD_HEADER_DBL_ENTRY(bestPhredAggMis);
			ADD_HEADER_DBL_ENTRY(curPhredAgg);
			
			double readsPerLocus = readCount * 1.0 / locusReadCount;
			double readsPerPerson = readCount * 1.0 / personTotalReads;
			ADD_HEADER_DBL_ENTRY(readsPerLocus);
			ADD_HEADER_DBL_ENTRY(readsPerPerson);
			
			double topAlleleRat = topAlleleCnt * 1.0 / readCount;
			double altAlleleRat = altAlleleCnt * 1.0 / readCount;
			double alleleRatRat = altAlleleRat / topAlleleRat;
			double hapRat = hapCount * 1.0 / readCount;
			ADD_HEADER_DBL_ENTRY(topAlleleRat);
			ADD_HEADER_DBL_ENTRY(altAlleleRat);
			ADD_HEADER_DBL_ENTRY(alleleRatRat);
			ADD_HEADER_DBL_ENTRY(hapRat);
			
			intptr_t lenDiff = topAllele.len - altAlleleLen;
			intptr_t lenMod = (lenDiff < 0) ? -lenDiff : lenDiff;
				lenMod = lenMod % primerPeriod;
			ADD_HEADER_INT_ENTRY(lenDiff);
			ADD_HEADER_INT_ENTRY(lenMod);
			
			double altUmiRat = altUmiReads * 1.0 / readCount;
			double altUmiPrimRat = nearUMIPrimaryPresent * 1.0 / readCount;
			intptr_t altUmiLenDiff = topAllele.len - altUmiLength;
			intptr_t altUmiLenMod = (altUmiLenDiff < 0) ? -altUmiLenDiff : altUmiLenDiff;
				altUmiLenMod = altUmiLenMod % primerPeriod;
			ADD_HEADER_DBL_ENTRY(altUmiRat);
			ADD_HEADER_DBL_ENTRY(altUmiPrimRat);
			ADD_HEADER_INT_ENTRY(altUmiLenDiff);
			ADD_HEADER_INT_ENTRY(altUmiLenMod);
			
			DUMP_ENTRY(baseDump[numRow]);
			numRow++;
			if(numRow == baseDump.size()){
				outF.write(numRow, &endDump[0]);
				numRow = 0;
			}
			curLowIt = curHigIt;
		}
		if(numRow){ outF.write(numRow, &endDump[0]); }
	}catch(std::exception& errE){ outF.close(); for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); } throw; }
	outF.close();
	for(uintptr_t i = 0; i<allData.size(); i++){ delete(allData[i]); }
}

