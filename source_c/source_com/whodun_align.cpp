#include "whodun_align.h"

#include <algorithm>

using namespace whodun;

LinearPairwiseSequenceAlignment::LinearPairwiseSequenceAlignment(){}
LinearPairwiseSequenceAlignment::~LinearPairwiseSequenceAlignment(){}
int LinearPairwiseSequenceAlignment::findAlignmentScores(int numFind, intptr_t* storeCost, intptr_t minScore, intptr_t maxDupDeg){
	std::greater<intptr_t> compMeth;
	unsigned numFindU = numFind;
	uintptr_t numFound = 0;
	intptr_t curMin = minScore;
	intptr_t realLim = minScore;
	startFuzzyIteration(realLim, maxDupDeg, numFind);
	while(getNextAlignment()){
		intptr_t* tsIt = std::lower_bound(storeCost, storeCost + numFound, alnScore, compMeth);
		uintptr_t tsInd = tsIt - storeCost;
		if((tsInd < numFound) && (*tsIt == alnScore)){ continue; }
		if(tsInd == numFindU){ continue; }
		if(numFound == numFindU){ numFound--; }
		memmove(tsIt + 1, tsIt, (numFound - tsInd)*sizeof(intptr_t));
		*tsIt = alnScore;
		numFound++;
		if((numFound == numFindU) && (storeCost[numFindU-1] > curMin)){
			curMin = storeCost[numFindU-1];
			realLim = curMin + 1;
			updateMinimumScore(realLim);
		}
	}
	return numFound;
}

AlignCostAffine::AlignCostAffine(){
	allMMCost = 0;
	numLiveChars = 0;
	openRefCost = 0;
	extendRefCost = 0;
	closeRefCost = 0;
	openReadCost = 0;
	extendReadCost = 0;
	closeReadCost = 0;
}
AlignCostAffine::~AlignCostAffine(){
	if(allMMCost){free(allMMCost);}
}

std::ostream& whodun::operator<<(std::ostream& os, const AlignCostAffine& toOut){
	os << toOut.openRefCost << " " << toOut.extendRefCost << " " << toOut.closeRefCost << " ";
	os << toOut.openReadCost << " " << toOut.extendReadCost << " " << toOut.closeReadCost;
	os << " " << toOut.numLiveChars;
	for(int i = 0; i<256; i++){
		if(toOut.charMap[i]){
			os << " " << i;
		}
	}
	for(int i = 1; i<toOut.numLiveChars; i++){
		for(int j = 1; j<toOut.numLiveChars; j++){
			os << " " << toOut.allMMCost[i][j];
		}
	}
	return os;
}

PositionalCostTable::PositionalCostTable(){
	allocSpace = 0;
	allCosts = 0;
}
PositionalCostTable::~PositionalCostTable(){
	if(allCosts){ free(allCosts); }
}
void PositionalCostTable::allocate(uintptr_t numRef, uintptr_t numRead){
	uintptr_t numTot = (numRef+1) * (numRead+1);
	if(numTot > allocSpace){
		if(allCosts){ free(allCosts); }
		allCosts = (PositionCost*)malloc(numTot*sizeof(PositionCost));
		allocSpace = numTot;
	}
	numCutsRef = numRef;
	numCutsRead = numRead;
}

ReferencePositionalCostTable::ReferencePositionalCostTable(){}
ReferencePositionalCostTable::~ReferencePositionalCostTable(){}
void ReferencePositionalCostTable::parse(uintptr_t numEntry, TextTableRow** theEnts){
	allCosts.resize(numEntry);
	allRanges.clear();
	std::string tmpAsciiSto;
	#define PARSE_FUNKY_INT(srcLen, srcTxt, saveLoc) \
		tmpAsciiSto.clear();\
		tmpAsciiSto.insert(tmpAsciiSto.end(), srcTxt, srcTxt + srcLen);\
		saveLoc = atol(tmpAsciiSto.c_str());
	for(uintptr_t i = 0; i<numEntry; i++){
		TextTableRow* curR = theEnts[i];
		if(curR->numEntries < 9){ throw std::runtime_error("Too few entries in cost table."); }
		AlignCostAffine* curF = &(allCosts[i]);
		intptr_t fromLoc; PARSE_FUNKY_INT(curR->entrySizes[0], curR->curEntries[0], fromLoc)
		intptr_t toLoc; PARSE_FUNKY_INT(curR->entrySizes[1], curR->curEntries[1], toLoc)
		if((fromLoc < 0) || (toLoc < 0) || (toLoc < fromLoc)){ throw std::runtime_error("Bad bounds."); }
		if(fromLoc == toLoc){ continue; }
		allRanges.push_back(triple<intptr_t,intptr_t,uintptr_t>(fromLoc,toLoc,i));
		PARSE_FUNKY_INT(curR->entrySizes[2], curR->curEntries[2], curF->openRefCost)
		PARSE_FUNKY_INT(curR->entrySizes[3], curR->curEntries[3], curF->extendRefCost)
		PARSE_FUNKY_INT(curR->entrySizes[4], curR->curEntries[4], curF->closeRefCost)
		PARSE_FUNKY_INT(curR->entrySizes[5], curR->curEntries[5], curF->openReadCost)
		PARSE_FUNKY_INT(curR->entrySizes[6], curR->curEntries[6], curF->extendReadCost)
		PARSE_FUNKY_INT(curR->entrySizes[7], curR->curEntries[7], curF->closeReadCost)
		int numChar; PARSE_FUNKY_INT(curR->entrySizes[8], curR->curEntries[8], numChar)
		if(numChar < 0){ throw std::runtime_error("Negative character count in cost file."); }
		if(curR->numEntries < (uintptr_t)(numChar*numChar + numChar + 9)){ throw std::runtime_error("Too few entries in cost table."); }
		//set up a blank map
		if(numChar > curF->numLiveChars){
			if(curF->allMMCost){ free(curF->allMMCost); }
			curF->allMMCost = (int**)malloc((numChar+1)*sizeof(int*) + (numChar+1)*(numChar+1)*sizeof(int));
			int* finTgt = (int*)(curF->allMMCost + (numChar+1));
			for(int j = 0; j<=numChar; j++){
				curF->allMMCost[j] = finTgt;
				finTgt += (numChar+1);
			}
		}
		for(int j = 0; j<256; j++){
			curF->charMap[j] = numChar;
		}
		//parse the character set
		int curLookI = 9;
		for(int j = 0; j<numChar; j++){
			int curC; PARSE_FUNKY_INT(curR->entrySizes[curLookI], curR->curEntries[curLookI], curC)
			if((curC < 0) || (curC > 255)){ throw std::runtime_error("Bad character specification."); }
			if(curF->charMap[curC] != numChar){ throw std::runtime_error("Duplicate character specification."); }
			curF->charMap[curC] = j;
			curLookI++;
		}
		//parse costs
		int worstScore = 1 << (sizeof(int)*8 - 1);
		for(int j = 0; j<numChar; j++){
			for(int k = 0; k<numChar; k++){
				int curS; PARSE_FUNKY_INT(curR->entrySizes[curLookI], curR->curEntries[curLookI], curS)
				curF->allMMCost[j][k] = curS;
				curLookI++;
			}
			curF->allMMCost[j][numChar] = worstScore;
		}
		//fill in missing
		for(int k = 0; k<=numChar; k++){
			curF->allMMCost[numChar][k] = worstScore;
		}
	}
	std::sort(allRanges.begin(), allRanges.end());
	//make sure there are no overlaps
	for(uintptr_t i = 1; i<allRanges.size(); i++){
		if(allRanges[i-1].second > allRanges[i].first){
			throw std::runtime_error("Overlapping ranges present.");
		}
	}
}
void ReferencePositionalCostTable::fill(PositionalCostTable* toFill, uintptr_t refOffset, SizePtrString refString, uintptr_t readOffset, SizePtrString readString, AlignCostAffine* defCost){
	toFill->allocate(refString.len, readString.len);
	//find the entry for the previous base in the reference
	intptr_t curLook = (refOffset - 1);
	uintptr_t curRanI;
	{
		uintptr_t ranLI = 0;
		uintptr_t ranHI = allRanges.size();
		while(ranHI != ranLI){
			uintptr_t ranMI = ranLI + ((ranHI - ranLI)/2);
			if(curLook < allRanges[ranMI].second){
				ranHI = ranMI;
			}
			else{
				ranLI = ranMI + 1;
			}
		}
		curRanI = ranLI;
	}
	//fill in
	PositionCost* curCost = toFill->allCosts;
	//special case for the reference prefix
	AlignCostAffine* useC = defCost;
	if((curRanI < allRanges.size()) && (curLook >= allRanges[curRanI].first)){
		useC = &(allCosts[allRanges[curRanI].third]);
	}
	for(uintptr_t j = 0; j<=readString.len; j++){
		curCost->eatCost = 0;
		curCost->openRefCost = useC->openRefCost;
		curCost->extendRefCost = useC->extendRefCost;
		curCost->closeRefCost = useC->closeRefCost;
		curCost->openReadCost = useC->openReadCost;
		curCost->extendReadCost = useC->extendReadCost;
		curCost->closeReadCost = useC->closeReadCost;
		curCost++;
	}
	curLook++;
	if((curRanI < allRanges.size()) && (curLook >= allRanges[curRanI].second)){
		curRanI++;
	}
	//the rest of the reference
	for(uintptr_t i = 0; i<refString.len; i++){
		useC = defCost;
		if((curRanI < allRanges.size()) && (curLook >= allRanges[curRanI].first)){
			useC = &(allCosts[allRanges[curRanI].third]);
		}
		int* curCharCostTab = useC->allMMCost[useC->charMap[0x00FF & refString.txt[i]]];
		//prefix of read
		curCost->eatCost = 0;
		curCost->openRefCost = useC->openRefCost;
		curCost->extendRefCost = useC->extendRefCost;
		curCost->closeRefCost = useC->closeRefCost;
		curCost->openReadCost = useC->openReadCost;
		curCost->extendReadCost = useC->extendReadCost;
		curCost->closeReadCost = useC->closeReadCost;
		curCost++;
		//fill in the read
		for(uintptr_t j = 0; j<readString.len; j++){
			curCost->eatCost = curCharCostTab[useC->charMap[0x00FF & readString.txt[j]]];
			curCost->openRefCost = useC->openRefCost;
			curCost->extendRefCost = useC->extendRefCost;
			curCost->closeRefCost = useC->closeRefCost;
			curCost->openReadCost = useC->openReadCost;
			curCost->extendReadCost = useC->extendReadCost;
			curCost->closeReadCost = useC->closeReadCost;
			curCost++;
		}
		curLook++;
		if((curRanI < allRanges.size()) && (curLook >= allRanges[curRanI].second)){
			curRanI++;
		}
	}
}

#define PDMAKE_TABLES \
	intptr_t** matchTable = costTable + (seqA.len + 1);\
	intptr_t** matchMatchTable = matchTable + (seqA.len + 1);\
	intptr_t** matchSkipATable = matchMatchTable + (seqA.len + 1);\
	intptr_t** matchSkipBTable = matchSkipATable + (seqA.len + 1);\
	intptr_t** skipATable = matchSkipBTable + (seqA.len + 1);\
	intptr_t** skipAMatchTable = skipATable + (seqA.len + 1);\
	intptr_t** skipASkipATable = skipAMatchTable + (seqA.len + 1);\
	intptr_t** skipASkipBTable = skipASkipATable + (seqA.len + 1);\
	intptr_t** skipBTable = skipASkipBTable + (seqA.len + 1);\
	intptr_t** skipBMatchTable = skipBTable + (seqA.len + 1);\
	intptr_t** skipBSkipATable = skipBMatchTable + (seqA.len + 1);\
	intptr_t** skipBSkipBTable = skipBSkipATable + (seqA.len + 1);\
	intptr_t* costLinear = (intptr_t*)(skipBSkipBTable + (seqA.len + 1)); ((void)(costLinear));

#define ALIGN_NEED_MATCH_MATCH 1
#define ALIGN_NEED_MATCH_SKIPA 2
#define ALIGN_NEED_MATCH_SKIPB 4
#define ALIGN_NEED_SKIPA_MATCH 8
#define ALIGN_NEED_SKIPA_SKIPA 16
#define ALIGN_NEED_SKIPA_SKIPB 32
#define ALIGN_NEED_SKIPB_MATCH 64
#define ALIGN_NEED_SKIPB_SKIPA 128
#define ALIGN_NEED_SKIPB_SKIPB 256
#define ALIGN_NEED_MATCH (ALIGN_NEED_MATCH_MATCH | ALIGN_NEED_MATCH_SKIPA | ALIGN_NEED_MATCH_SKIPB)
#define ALIGN_NEED_SKIPA (ALIGN_NEED_SKIPA_MATCH | ALIGN_NEED_SKIPA_SKIPA | ALIGN_NEED_SKIPA_SKIPB)
#define ALIGN_NEED_SKIPB (ALIGN_NEED_SKIPB_MATCH | ALIGN_NEED_SKIPB_SKIPA | ALIGN_NEED_SKIPB_SKIPB)

/**
 * Guards a direction entry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @return The guarded paths to go down.
 */
intptr_t whodun_focusStackGuardFlag(intptr_t fI, intptr_t fJ, intptr_t dirFlag){
	intptr_t liveDirs = dirFlag;
	//guard here
	bool fI0 = fI == 0; bool fIg0 = fI > 0; bool fI1 = fI == 1; bool fIg1 = fI > 1;
	bool fJ0 = fJ == 0; bool fJg0 = fJ > 0; bool fJ1 = fJ == 1; bool fJg1 = fJ > 1;
	//match match	: (i==1 && j==1) || (i>1 && j>1)
	if(!((fIg1 && fJg1) || (fI1 && fJ1)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_MATCH; }
	//match skipa	: j>0 && i>1
	if(!(fJg0 && fIg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_SKIPA; }
	//match skipb	: i>0 && j>1
	if(!(fIg0 && fJg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_MATCH_SKIPB; }
	//skipa match	: (i>1 && j>0) || (i==1 && j==0)
	if(!((fIg1 && fJg0) || (fI1 && fJ0)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_MATCH; }
	//skipa skipa	: (i>1)
	if(!(fIg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_SKIPA; }
	//skipa skipb	: i>0 && j>0
	if(!(fIg0 && fJg0))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPA_SKIPB; }
	//skipb match	: (i>0 && j>1) || (i==0 && j==1)
	if(!((fIg0 && fJg1) || (fI0 && fJ1)))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_MATCH; }
	//skipb skipa	: i>0 && j>0
	if(!(fIg0 && fJg0))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_SKIPA; }
	//skipb skipb	: (j>1)
	if(!(fJg1))
		{ liveDirs = liveDirs & ~ALIGN_NEED_SKIPB_SKIPB; }
	return liveDirs;
}

/**
 * Make an initializer for a PositionDependentAGLPFocusStackEntry.
 * @param fI The i index.
 * @param fJ The j index.
 * @param dirFlag The paths to go down.
 * @param pathSc The score of the path this is on.
 * @param howG How this node was arrived at. 0 for start.
 */
#define NEW_FOCUS_STACK_ENTRY(fI, fJ, dirFlag, pathSc, howG) (AGLPFocusStackEntry){fI, fJ, whodun_focusStackGuardFlag(fI,fJ,dirFlag), pathSc, howG, 0}
/**
 * Sorting method for stack starting locations.
 * @param itemA The first item.
 * @param itemB The second item.
 * @return Whether itemA's score should come before itemB. (better scores go later).
 */
bool stackStartPositionSortFunction(const AGLPFocusStackEntry& itemA, const AGLPFocusStackEntry& itemB){
	return itemA.pathScore < itemB.pathScore;
}

PositionDependentCostLPSA::PositionDependentCostLPSA(int numSeqEnds, PositionalCostTable* useCost){
	aInds = 0;
	bInds = 0;
	numEnds = numSeqEnds;
	cost = useCost;
	costTable = 0;
	saveSize = 0;
	alnStack = 0;
	alnStackAlloc = 0;
}
PositionDependentCostLPSA::~PositionDependentCostLPSA(){
	if(aInds){ free(aInds); }
	if(bInds){ free(bInds); }
	if(costTable){ free(costTable); }
	if(alnStack){ free(alnStack); }
}
void PositionDependentCostLPSA::alignSequences(SizePtrString refString, SizePtrString readString){
	alignSequences(refString, readString, numEnds, cost);
}
void PositionDependentCostLPSA::alignSequences(SizePtrString refString, SizePtrString readString, int numSeqEnds, PositionalCostTable* useCost){
	//save the new stuff
		numEnds = numSeqEnds;
		cost = useCost;
		seqA = refString;
		seqB = readString;
	//allocate the stupid thing
		uintptr_t numTabEnts = (seqA.len+1)*(seqB.len+1);
		uintptr_t numLineEnts = (seqB.len+1);
		uintptr_t numPtrEnts = (seqA.len+1);
		uintptr_t numAlSing = numTabEnts*sizeof(intptr_t) + numPtrEnts*sizeof(intptr_t*);
		uintptr_t totNumAlloc = numAlSing*13;
		if(totNumAlloc > saveSize){
			if(costTable){ free(costTable); }
			costTable = (intptr_t**)malloc(totNumAlloc);
			saveSize = totNumAlloc;
		}
		PDMAKE_TABLES
		uintptr_t numSetLin = numPtrEnts * 13;
		for(uintptr_t i = 0; i<numSetLin; i++){
			costTable[i] = costLinear;
			costLinear += numLineEnts;
		}
	//get some commons
		intptr_t worstScore = -1; worstScore = worstScore << (8*sizeof(intptr_t)-1);
		intptr_t negToZero = (numEnds == 0)-1;
		intptr_t startIJ;
		switch(numEnds){
			case 0: startIJ = 0; break;
			case 2: startIJ = 0; break;
			case 4: startIJ = -1; break;
			default:
				throw std::runtime_error("Da fuq?");
		};
		intptr_t skipSets = startIJ & worstScore;
		uintptr_t costRowStride = seqB.len + 1;
	//helpful space for variables
		intptr_t scoreDiff;
		intptr_t diffSign;
		intptr_t scoreMax;
		PositionCost* curCost;
		PositionCost* skbCost;
		PositionCost* skaCost;
		PositionCost* matCost;
	//helpful code pieces
		#define NEGATIVE_GUARD(forVal) forVal = (negToZero | ((forVal < 0)-1)) & forVal;
		#define GET_MAX_THREE(itemA,itemB,itemC) \
			scoreMax = itemA;\
			scoreDiff = (scoreMax - itemB);\
			diffSign = scoreDiff >> (8*sizeof(intptr_t)-1);\
			scoreMax = scoreMax - (scoreDiff & diffSign);\
			scoreDiff = (scoreMax - itemC);\
			diffSign = scoreDiff >> (8*sizeof(intptr_t)-1);\
			scoreMax = scoreMax - (scoreDiff & diffSign);
	//fill in the stupid thing
	intptr_t i; intptr_t lenA = seqA.len;
	intptr_t j; intptr_t lenB = seqB.len;
	curCost = cost->allCosts;
	//short circuit if either has length zero
		if(seqA.len == 0){
			i = 0;
			j = 0;{
				matchMatchTable[i][j] = 0;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = 0;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = 0;
				curCost++;
			}
			j = 1; {
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				intptr_t winBM = startIJ & (matchTable[i][j-1] + curCost->openReadCost + curCost->extendReadCost);
				skipBMatchTable[i][j] = winBM;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = winBM;
				costTable[i][j] = winBM;
				curCost++;
			}
			for(j = 2; j<=lenB; j++){
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				intptr_t winBB = startIJ & (skipBTable[i][j-1] + curCost->extendReadCost);
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = winBB;
				skipBTable[i][j] = winBB;
				costTable[i][j] = winBB;
				curCost++;
			}
			return;
		}
		if(seqB.len == 0){
			j = 0;
			i = 0; {
				matchMatchTable[i][j] = 0;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = 0;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = 0;
				curCost++;
			}
			i = 1; {
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				intptr_t winAM = startIJ & (matchTable[i-1][j] + curCost->openRefCost + curCost->extendRefCost);
				skipAMatchTable[i][j] = winAM;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = winAM;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = winAM;
				curCost++;
			}
			for(i = 2; i<=lenA; i++){
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				intptr_t winAA = startIJ & (skipATable[i-1][j] + curCost->extendRefCost);
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = winAA;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = winAA;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = winAA;
				curCost++;
			}
			return;
		}
	//fill in the table
	//without matching anything in the reference
		i = 0; {
			j = 0; {
				matchMatchTable[i][j] = 0;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = 0;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = 0;
				curCost++;
			}
			j = 1; {
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				intptr_t winBM = startIJ & (matchTable[i][j-1] + curCost->openReadCost + curCost->extendReadCost);
				skipBMatchTable[i][j] = winBM;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = winBM;
				costTable[i][j] = winBM;
				curCost++;
			}
			for(j = 2; j<=lenB; j++){
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = skipSets;
				intptr_t winBB = startIJ & (skipBTable[i][j-1] + curCost->extendReadCost);
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = winBB;
				skipBTable[i][j] = winBB;
				costTable[i][j] = winBB;
				curCost++;
			}
		}
	//consumed only one thing in the reference
		i = 1; {
			j = 0; {
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				intptr_t winAM = startIJ & (matchTable[i-1][j] + curCost->openRefCost + curCost->extendRefCost);
				skipAMatchTable[i][j] = winAM;
				skipASkipATable[i][j] = skipSets;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = winAM;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = winAM;
				curCost++;
			}
			j = 1; {
				skbCost = curCost - 1;
				skaCost = curCost - costRowStride;
				intptr_t winMM = matchTable[i-1][j-1] + curCost->eatCost;
					NEGATIVE_GUARD(winMM)
				matchMatchTable[i][j] = winMM;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = winMM;
				intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeReadCost) + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAB)
				skipAMatchTable[i][j] = worstScore;
				skipASkipATable[i][j] = worstScore;
				skipASkipBTable[i][j] = winAB;
				skipATable[i][j] = winAB;
				intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeRefCost) + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBA)
				skipBMatchTable[i][j] = worstScore;
				skipBSkipATable[i][j] = winBA;
				skipBSkipBTable[i][j] = worstScore;
				skipBTable[i][j] = winBA;
				GET_MAX_THREE(winMM, winAB, winBA)
				costTable[i][j] = scoreMax;
				curCost++;
				matCost = skaCost;
				skaCost++;
				skbCost++;
			}
			for(j = 2; j<=lenB; j++){
				intptr_t winMB = skipBTable[i-1][j-1] + (startIJ & matCost->closeReadCost) + curCost->eatCost;
					NEGATIVE_GUARD(winMB)
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = winMB;
				matchTable[i][j] = winMB;
				intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeReadCost) + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAB)
				skipAMatchTable[i][j] = worstScore;
				skipASkipATable[i][j] = worstScore;
				skipASkipBTable[i][j] = winAB;
				skipATable[i][j] = winAB;
				intptr_t winBM = matchTable[i][j-1] + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBM)
				intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeRefCost) + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBA)
				intptr_t winBB = skipBTable[i][j-1] + curCost->extendReadCost;
					NEGATIVE_GUARD(winBB)
				skipBMatchTable[i][j] = winBM;
				skipBSkipATable[i][j] = winBA;
				skipBSkipBTable[i][j] = winBB;
				GET_MAX_THREE(winBM,winBA,winBB)
				skipBTable[i][j] = scoreMax;
				GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
				costTable[i][j] = scoreMax;
				curCost++;
				matCost++;
				skaCost++;
				skbCost++;
			}
		}
	//enough to do the full monty
		for(i = 2; i<=lenA; i++){
			j = 0; {
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = worstScore;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = worstScore;
				intptr_t winAA = startIJ & (skipATable[i-1][j] + curCost->extendRefCost);
				skipAMatchTable[i][j] = skipSets;
				skipASkipATable[i][j] = winAA;
				skipASkipBTable[i][j] = skipSets;
				skipATable[i][j] = winAA;
				skipBMatchTable[i][j] = skipSets;
				skipBSkipATable[i][j] = skipSets;
				skipBSkipBTable[i][j] = skipSets;
				skipBTable[i][j] = skipSets;
				costTable[i][j] = winAA;
				skbCost = curCost;
				matCost = curCost - costRowStride;
				curCost++;
				skaCost = curCost - costRowStride;
			}
			j = 1; {
				intptr_t winMA = skipATable[i-1][j-1] + (startIJ & matCost->closeRefCost) + curCost->eatCost;
					NEGATIVE_GUARD(winMA)
				matchMatchTable[i][j] = worstScore;
				matchSkipATable[i][j] = winMA;
				matchSkipBTable[i][j] = worstScore;
				matchTable[i][j] = winMA;
				intptr_t winAM = matchTable[i-1][j] + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAM)
				intptr_t winAA = skipATable[i-1][j] + curCost->extendRefCost;
					NEGATIVE_GUARD(winAA)
				intptr_t winAB = skipBTable[i-1][j] + (startIJ & skaCost->closeReadCost) + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAB)
				skipAMatchTable[i][j] = winAM;
				skipASkipATable[i][j] = winAA;
				skipASkipBTable[i][j] = winAB;
				GET_MAX_THREE(winAM,winAA,winAB)
				skipATable[i][j] = scoreMax;
				skipBMatchTable[i][j] = worstScore;
				intptr_t winBA = skipATable[i][j-1] + (startIJ & skbCost->closeRefCost) + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBA)
				skipBSkipATable[i][j] = winBA;
				skipBSkipBTable[i][j] = worstScore;
				skipBTable[i][j] = winBA;
				GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
				costTable[i][j] = scoreMax;
				curCost++;
				matCost++;
				skaCost++;
				skbCost++;
			}
			for(j = 2; j<=lenB; j++){
				intptr_t winMM = matchTable[i-1][j-1] + curCost->eatCost;
					NEGATIVE_GUARD(winMM)
				intptr_t winMA = skipATable[i-1][j-1] + matCost->closeRefCost + curCost->eatCost;
					NEGATIVE_GUARD(winMA)
				intptr_t winMB = skipBTable[i-1][j-1] + matCost->closeReadCost + curCost->eatCost;
					NEGATIVE_GUARD(winMB)
				matchMatchTable[i][j] = winMM;
				matchSkipATable[i][j] = winMA;
				matchSkipBTable[i][j] = winMB;
				GET_MAX_THREE(winMM, winMA, winMB)
				matchTable[i][j] = scoreMax;
				intptr_t winAM = matchTable[i-1][j] + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAM)
				intptr_t winAA = skipATable[i-1][j] + curCost->extendRefCost;
					NEGATIVE_GUARD(winAA)
				intptr_t winAB = skipBTable[i-1][j] + skaCost->closeReadCost + curCost->openRefCost + curCost->extendRefCost;
					NEGATIVE_GUARD(winAB)
				skipAMatchTable[i][j] = winAM;
				skipASkipATable[i][j] = winAA;
				skipASkipBTable[i][j] = winAB;
				GET_MAX_THREE(winAM, winAA, winAB)
				skipATable[i][j] = scoreMax;
				intptr_t winBM = matchTable[i][j-1] + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBM)
				intptr_t winBA = skipATable[i][j-1] + skbCost->closeRefCost + curCost->openReadCost + curCost->extendReadCost;
					NEGATIVE_GUARD(winBA)
				intptr_t winBB = skipBTable[i][j-1] + curCost->extendReadCost;
					NEGATIVE_GUARD(winBB)
				skipBMatchTable[i][j] = winBM;
				skipBSkipATable[i][j] = winBA;
				skipBSkipBTable[i][j] = winBB;
				GET_MAX_THREE(winBM, winBA, winBB)
				skipBTable[i][j] = scoreMax;
				GET_MAX_THREE(matchTable[i][j], skipATable[i][j], skipBTable[i][j])
				costTable[i][j] = scoreMax;
				curCost++;
				matCost++;
				skaCost++;
				skbCost++;
			}
		}
}
void PositionDependentCostLPSA::startOptimalIteration(){
	allScore.clear();
	allScoreSeen.clear();
	maxDupDeg = 0;
	maxNumScore = 1;
	findStartingLocations();
	if(waitingStart.size()){
		uintptr_t eraseTo = std::lower_bound(waitingStart.begin(), waitingStart.end(), waitingStart[waitingStart.size()-1], stackStartPositionSortFunction) - waitingStart.begin();
		waitingStart.erase(waitingStart.begin(), waitingStart.begin() + eraseTo);
		minScore = waitingStart[0].pathScore;
	}
	uintptr_t needAlloc = (seqA.len + seqB.len + 2);
	if(needAlloc > alnStackAlloc){
		if(alnStack){ free(alnStack); }
		alnStackAlloc = needAlloc;
		alnStack = (AGLPFocusStackEntry*)malloc(sizeof(AGLPFocusStackEntry) * alnStackAlloc);
		
		if(aInds){ free(aInds); }
		if(bInds){ free(bInds); }
		aInds = (intptr_t*)malloc(needAlloc * sizeof(intptr_t));
		bInds = (intptr_t*)malloc(needAlloc * sizeof(intptr_t));
	}
	alnStackSize = 0;
	iterateNextPathStep();
}
void PositionDependentCostLPSA::startFuzzyIteration(intptr_t minimumScore, intptr_t maximumDupDeg, intptr_t maximumNumScore){
	allScore.clear();
	allScoreSeen.clear();
	minScore = minimumScore;
	maxDupDeg = maximumDupDeg;
	maxNumScore = maxNumScore;
	findStartingLocations();
	uintptr_t eraseTo = 0;
	while(eraseTo < waitingStart.size()){
		if(waitingStart[eraseTo].pathScore >= minScore){ break; }
		eraseTo++;
	}
	waitingStart.erase(waitingStart.begin(), waitingStart.begin() + eraseTo);
	uintptr_t needAlloc = (seqA.len + seqB.len + 2);
	if(needAlloc > alnStackAlloc){
		if(alnStack){ free(alnStack); }
		alnStackAlloc = needAlloc;
		alnStack = (AGLPFocusStackEntry*)malloc(sizeof(AGLPFocusStackEntry) * alnStackAlloc);
		
		if(aInds){ free(aInds); }
		if(bInds){ free(bInds); }
		aInds = (intptr_t*)malloc(needAlloc * sizeof(intptr_t));
		bInds = (intptr_t*)malloc(needAlloc * sizeof(intptr_t));
	}
	alnStackSize = 0;
	iterateNextPathStep();
}
void PositionDependentCostLPSA::updateMinimumScore(intptr_t newMin){
	minScore = newMin;
}
void PositionDependentCostLPSA::findStartingLocations(){
	waitingStart.clear();
	PDMAKE_TABLES
	intptr_t lenA = seqA.len;
	intptr_t lenB = seqB.len;
	PositionCost* curCost;
	switch(numEnds){
		case 4:
			curCost = cost->allCosts + seqA.len*(seqB.len+1) + seqB.len;
			waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, lenB, ALIGN_NEED_MATCH, matchTable[lenA][lenB], 0) );
			if(lenA){ waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, lenB, ALIGN_NEED_SKIPA, skipATable[lenA][lenB] + curCost->closeRefCost, 0) ); }
			if(lenB){ waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, lenB, ALIGN_NEED_SKIPB, skipBTable[lenA][lenB] + curCost->closeReadCost, 0) ); }
			break;
		case 2:
			if(lenA == 0){ break; }
			if(lenB == 0){ break; }
			curCost = cost->allCosts + seqA.len*(seqB.len+1);
			for(intptr_t j = 0; j<=lenB; j++){
				if(j){ waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, j, ALIGN_NEED_MATCH, matchTable[lenA][j], 0) ); }
				waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, j, ALIGN_NEED_SKIPA, skipATable[lenA][j] + (j ? curCost->closeRefCost : 0), 0) );
				//if(j){ waitingStart.push_back( NEW_FOCUS_STACK_ENTRY(lenA, j, ALIGN_NEED_SKIPB, skipBTable[lenA][j] + curCost->closeReadCost) ); }
				curCost++;
			}
			curCost = cost->allCosts + seqB.len;
			for(intptr_t i = 0; i<lenA; i++){
				if(i){ waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, lenB, ALIGN_NEED_MATCH, matchTable[i][lenB], 0)); }
				//if(i){ waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, lenB, ALIGN_NEED_SKIPA, skipATable[i][lenB] + curCost->closeRefCost)); }
				waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, lenB, ALIGN_NEED_SKIPB, skipBTable[i][lenB] + (i ? curCost->closeReadCost : 0), 0));
				curCost += (seqB.len + 1);
			}
			break;
		case 0:
			curCost = cost->allCosts;
			for(intptr_t i = 0; i<=lenA; i++){
				for(intptr_t j = 0; j<=lenB; j++){
					if(i&&j){ waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, j, ALIGN_NEED_MATCH, matchTable[i][j], 0)); }
					if(i && j && (j<lenB)){ waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, j, ALIGN_NEED_SKIPA, skipATable[i][j] + curCost->closeRefCost, 0)); }
					if(j && i && (i<lenA)){ waitingStart.push_back(NEW_FOCUS_STACK_ENTRY(i, j, ALIGN_NEED_SKIPB, skipBTable[i][j] + curCost->closeReadCost, 0)); }
					curCost++;
				}
			}
			break;
		default:
			throw std::runtime_error("Da fuq?");
	};
	std::stable_sort(waitingStart.begin(), waitingStart.end(), stackStartPositionSortFunction);
}
int PositionDependentCostLPSA::getNextAlignment(){
	std::greater<intptr_t> compMeth;
	std::vector<intptr_t>::iterator insLoc;
	bool needPreClose = numEnds == 0;
	bool gotThing = false;
	while(!gotThing){
		if(hasPath()){
			//check getting the thing
			if(needPreClose){
				if(currentPathEndScore() >= minScore){
					gotThing = true;
					dumpPath();
					alnScore = currentPathEndScore();
				}
			}
			else if(pathTerminal()){
				if(currentPathScore() >= minScore){
					gotThing = true;
					dumpPath();
					alnScore = currentPathScore();
				}
			}
			//abandon if score too low
			if(currentPathScore() < minScore){
				abandonPath();
				continue;
			}
			//check for abandon given it's not sufficiently new
			if(maxDupDeg && lastIterationChangedPath()){
				intptr_t curPS = currentPathScore();
				//try to add
				insLoc = std::lower_bound(allScore.begin(), allScore.end(), curPS, compMeth);
				uintptr_t insInd = insLoc - allScore.begin();
				if(insInd >= allScore.size()){
					allScore.push_back(curPS);
					allScoreSeen.push_back(0);
				}
				else if(allScore[insInd] != curPS){
					allScore.insert(allScore.begin() + insInd, curPS);
					allScoreSeen.insert(allScoreSeen.begin() + insInd, 0);
				}
				if(allScore.size() > (uintptr_t)maxNumScore){
					minScore = allScore[maxNumScore-1];
				}
				//check for abandon
				if(allScoreSeen[insInd] >= maxDupDeg){
					abandonPath();
					continue;
				}
				allScoreSeen[insInd]++;
			}
			//move to next
			iterateNextPathStep();
		}
		else{
			return 0;
		}
	}
	return 1;
}
bool PositionDependentCostLPSA::hasPath(){
	return alnStackSize != 0;
}
bool PositionDependentCostLPSA::pathTerminal(){
	PDMAKE_TABLES
	AGLPFocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
	intptr_t pi = curFoc->focI;
	intptr_t pj = curFoc->focJ;
	switch(numEnds){
		case 0:
			if(curFoc->liveDirs & ALIGN_NEED_SKIPA){
				return (skipATable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH){
				return (matchTable[pi][pj] == 0);
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB){
				return (skipBTable[pi][pj] == 0);
			}
			else{
				return (costTable[pi][pj] == 0);
			}
		case 2:
			return (pi == 0) || (pj == 0);
		case 4:
			return (pi == 0) && (pj == 0);
		default:
			throw std::runtime_error("Da fuq?");
	}
	return 0;
}
intptr_t PositionDependentCostLPSA::currentPathScore(){
	return (alnStack + alnStackSize - 1)->pathScore;
}
intptr_t PositionDependentCostLPSA::currentPathEndScore(){
	return dieHereScore;
}
void PositionDependentCostLPSA::iterateNextPathStep(){
	PDMAKE_TABLES
	bool negToZero = numEnds == 0;
	//if it hit an end, draw back (do not go forward)
	if(alnStackSize && pathTerminal()){
		alnStackSize--;
	}
	//might need to do the following multiple times.
	tailRecursionTgt:
	lastIterChange = false;
	while(alnStackSize && ((alnStack + alnStackSize - 1)->liveDirs == 0)){
		alnStackSize--;
	}
	if(alnStackSize == 0){
		if(waitingStart.size()){
			*alnStack = waitingStart[waitingStart.size()-1];
			alnStackSize = 1;
			waitingStart.pop_back();
			dieHereScore = 0;
			lastIterChange = true;
		}
		else{
			return;
		}
	}
	else{
		AGLPFocusStackEntry* curFoc = (alnStack + alnStackSize - 1);
		intptr_t li = curFoc->focI;
		intptr_t lj = curFoc->focJ;
		#define DO_PUSH(curDirFlag, lookTable, checkTable, offI, offJ, nextDir) \
			curFoc->liveDirs = curFoc->liveDirs & (~curDirFlag);\
			if(negToZero && (lookTable[li][lj] == 0)){ goto tailRecursionTgt; }\
			intptr_t newScore = curFoc->pathScore + lookTable[li][lj] - checkTable[li][lj];\
			if(newScore != curFoc->pathScore){ lastIterChange = true; }\
				else{ lastIterChange = curFoc->seenDef; curFoc->seenDef = 1; }\
			*(alnStack + alnStackSize) = NEW_FOCUS_STACK_ENTRY(li - offI, lj - offJ, nextDir, newScore, curDirFlag);\
			alnStackSize++;
		#define POST_PUSH_ZCHECK(nextTable, offI, offJ, taction, faction) \
			if(negToZero){\
				intptr_t ptabVal = nextTable[li-offI][lj-offJ];\
				if(ptabVal > 0){\
					taction\
				}\
				else{\
					faction\
				}\
			}
		#define POST_PUSH_TACTION_MATCH dieHereScore = newScore - ptabVal;
		#define POST_PUSH_FACTION_MATCH dieHereScore = newScore;
		#define POST_PUSH_FACTION_SKIP(offI, offJ) \
			if((li-offI) && (lj-offJ)){\
				alnStackSize--;\
				goto tailRecursionTgt;\
			}\
			else{\
				dieHereScore = newScore;\
			}
		#define POST_PUSH_TACTION_SKIP_OPEN(specCost) \
			PositionCost* curCost = cost->allCosts + li*(seqB.len+1) + lj;\
			dieHereScore = newScore - ptabVal + curCost->specCost;
		#define POST_PUSH_TACTION_SKIP_CLOSE(offI, offJ, specCost) \
			PositionCost* othCost = cost->allCosts + (li-offI)*(seqB.len+1) + (lj-offJ);\
			dieHereScore = newScore - ptabVal + -(othCost->specCost);
		if(curFoc->liveDirs & ALIGN_NEED_SKIPA){
			if(curFoc->liveDirs & ALIGN_NEED_SKIPA_SKIPA){
				DO_PUSH(ALIGN_NEED_SKIPA_SKIPA, skipASkipATable, skipATable, 1, 0, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 1, 0, POST_PUSH_TACTION_SKIP_OPEN(openRefCost), POST_PUSH_FACTION_SKIP(1,0))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPA_MATCH){
				DO_PUSH(ALIGN_NEED_SKIPA_MATCH, skipAMatchTable, skipATable, 1, 0, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 1, 0, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPA_SKIPB){
				DO_PUSH(ALIGN_NEED_SKIPA_SKIPB, skipASkipBTable, skipATable, 1, 0, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 1, 0, POST_PUSH_TACTION_SKIP_CLOSE(1,0,closeReadCost), POST_PUSH_FACTION_SKIP(1,0))
			}
		}
		else if(curFoc->liveDirs & ALIGN_NEED_MATCH){
			if(curFoc->liveDirs & ALIGN_NEED_MATCH_SKIPA){
				DO_PUSH(ALIGN_NEED_MATCH_SKIPA, matchSkipATable, matchTable, 1, 1, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 1, 1, POST_PUSH_TACTION_SKIP_CLOSE(1,1,closeRefCost), POST_PUSH_FACTION_SKIP(1,1))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH_MATCH){
				DO_PUSH(ALIGN_NEED_MATCH_MATCH, matchMatchTable, matchTable, 1, 1, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 1, 1, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_MATCH_SKIPB){
				DO_PUSH(ALIGN_NEED_MATCH_SKIPB, matchSkipBTable, matchTable, 1, 1, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 1, 1, POST_PUSH_TACTION_SKIP_CLOSE(1,1,closeReadCost), POST_PUSH_FACTION_SKIP(1,1))
			}
		}
		else if(curFoc->liveDirs & ALIGN_NEED_SKIPB){
			if(curFoc->liveDirs & ALIGN_NEED_SKIPB_SKIPA){
				DO_PUSH(ALIGN_NEED_SKIPB_SKIPA, skipBSkipATable, skipBTable, 0, 1, ALIGN_NEED_SKIPA)
				POST_PUSH_ZCHECK(skipATable, 0, 1, POST_PUSH_TACTION_SKIP_CLOSE(0,1,closeRefCost), POST_PUSH_FACTION_SKIP(0,1))
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB_MATCH){
				DO_PUSH(ALIGN_NEED_SKIPB_MATCH, skipBMatchTable, skipBTable, 0, 1, ALIGN_NEED_MATCH)
				POST_PUSH_ZCHECK(matchTable, 0, 1, POST_PUSH_TACTION_MATCH, POST_PUSH_FACTION_MATCH)
			}
			else if(curFoc->liveDirs & ALIGN_NEED_SKIPB_SKIPB){
				DO_PUSH(ALIGN_NEED_SKIPB_SKIPB, skipBSkipBTable, skipBTable, 0, 1, ALIGN_NEED_SKIPB)
				POST_PUSH_ZCHECK(skipBTable, 0, 1, POST_PUSH_TACTION_SKIP_OPEN(openReadCost), POST_PUSH_FACTION_SKIP(0,1))
			}
		}
	}
}
void PositionDependentCostLPSA::abandonPath(){
	alnStackSize--;
	iterateNextPathStep();
}
void PositionDependentCostLPSA::dumpPath(){
	alnLength = alnStackSize;
	int getInd = alnStackSize-1;
	for(int i = 0; i<alnStackSize; i++){
		AGLPFocusStackEntry* curEnt = alnStack + getInd;
		aInds[i] = curEnt->focI;
		bInds[i] = curEnt->focJ;
		getInd--;
	}
}
bool PositionDependentCostLPSA::lastIterationChangedPath(){
	return lastIterChange;
}


