#include "whodun_gen_coord.h"

#include <string.h>

using namespace whodun;

bool whodun::coordinateCompareFunc(const CoordinateRange& itemA, const CoordinateRange& itemB){
	if(itemA.start != itemB.start){
		return itemA.start < itemB.start;
	}
	return itemA.end < itemB.end;
}

BedFileReader::BedFileReader(TextTableReader* readFrom){
	std::vector<char> nullTerm;
	std::vector<uintptr_t> textOffs;
	TextTableRow* curRow = readFrom->read();
	while(curRow){
		if(curRow->numEntries < 3){ continue; }
		if(curRow->entrySizes[0] && (curRow->curEntries[0][0] == '#')){ continue; }
		textOffs.push_back(saveText.size());
		saveText.insert(saveText.end(), curRow->curEntries[0], curRow->curEntries[0] + curRow->entrySizes[0]);
		CoordinateRange curAdd;
		nullTerm.clear(); nullTerm.insert(nullTerm.end(), curRow->curEntries[1], curRow->curEntries[1] + curRow->entrySizes[1]); nullTerm.push_back(0);
			curAdd.start = atol(&(nullTerm[0]));
		nullTerm.clear(); nullTerm.insert(nullTerm.end(), curRow->curEntries[2], curRow->curEntries[2] + curRow->entrySizes[2]); nullTerm.push_back(0);
			curAdd.end = atol(&(nullTerm[0]));
		locs.push_back(curAdd);
		readFrom->release(curRow);
		curRow = readFrom->read();
	}
	saveText.push_back(0);
	textOffs.push_back(saveText.size());
	for(uintptr_t i = 1; i<textOffs.size(); i++){
		SizePtrString curTxt;
		curTxt.txt = &(saveText[0]) + textOffs[i-1];
		curTxt.len = textOffs[i] - textOffs[i-1];
		chromosomes.push_back(curTxt);
	}
}
BedFileReader::~BedFileReader(){}

UCSCCoordinateParse::UCSCCoordinateParse(SizePtrString ucsc){
	#define NUM_BUFF_SIZE (8*sizeof(uintptr_t)+8)
	char numBuff[NUM_BUFF_SIZE];
	//figure out the chromosome
	char* colChar = (char*)memchr(ucsc.txt, ':', ucsc.len);
	if(!colChar){
		lastErr = "Malformed coordinate: no colon present.";
		return;
	}
	ref.len = colChar - ucsc.txt;
	ref.txt = ucsc.txt;
	//find the numbers
	SizePtrString firstNum = {ucsc.len - (ref.len+1), colChar + 1};
	SizePtrString secondNum = {0, colChar};
	char* minChar = (char*)memchr(firstNum.txt , '-', firstNum.len);
	if(minChar){
		firstNum.len = minChar - firstNum.txt;
		secondNum.txt = minChar + 1;
		secondNum.len = (ucsc.txt + ucsc.len) - secondNum.txt;
	}
	//parse the numbers
	firstNum = trim(firstNum);
	if(memspn(firstNum.txt, firstNum.len, "0123456789", 10) != firstNum.len){
		lastErr = "Malformed coordinate: illegal character found.";
		return;
	}
	uintptr_t firstNumL = std::min(firstNum.len, (uintptr_t)(NUM_BUFF_SIZE-1));
	memcpy(numBuff, firstNum.txt, firstNumL);
	numBuff[firstNumL] = 0;
	bounds.start = atol(numBuff);
	if(secondNum.len){
		secondNum = trim(secondNum);
		if(memspn(secondNum.txt, secondNum.len, "0123456789", 10) != secondNum.len){
			lastErr = "Malformed coordinate: illegal character found.";
			return;
		}
		uintptr_t secondNumL = std::min(secondNum.len, (uintptr_t)(NUM_BUFF_SIZE-1));
		memcpy(numBuff, secondNum.txt, secondNumL);
		numBuff[secondNumL] = 0;
		bounds.end = atol(numBuff);
	}
	else{
		bounds.end = bounds.start + 1;
	}
	//clean up the reference
	ref = trim(ref);
}

CigarCoordinateParse::CigarCoordinateParse(uintptr_t pos, SizePtrString cigar){
	startPos = pos;
	saveCig = cigar;
}
uintptr_t CigarCoordinateParse::getBases(){
	uintptr_t totNumB = 0;
	uintptr_t curOpCnt = 0;
	for(uintptr_t i = 0; i<saveCig.len; i++){
		char curC = saveCig.txt[i];
		if((curC >= 48) && (curC <= 57)){
			curOpCnt = (curOpCnt*10) + (curC - 48);
		}
		else if((curC == 'M') || (curC == 'I') || (curC == 'S') || (curC == '=') || (curC == 'X')){
			totNumB += curOpCnt;
			curOpCnt = 0;
		}
		else{
			curOpCnt = 0;
		}
	}
	return curOpCnt;
}
CoordinateRange CigarCoordinateParse::getBounds(){
	CoordinateRange toRet = {startPos,startPos};
	uintptr_t curOpCnt = 0;
	for(uintptr_t i = 0; i<saveCig.len; i++){
		char curC = saveCig.txt[i];
		if((curC >= 48) && (curC <= 57)){
			curOpCnt = (curOpCnt*10) + (curC - 48);
		}
		else if((curC == 'M') || (curC == 'D') || (curC == 'N') || (curC == '=') || (curC == 'X')){
			toRet.end += curOpCnt;
			curOpCnt = 0;
		}
		else{
			curOpCnt = 0;
		}
	}
	return toRet;
}
std::pair<uintptr_t,uintptr_t> CigarCoordinateParse::getCoordinates(intptr_t* saveArr){
	uintptr_t curRef = startPos;
	uintptr_t curRead = 0;
	int seenAction = 0;
	uintptr_t softClipStart = 0;
	uintptr_t softClipEnd = 0;
	int seenHardEnd = 0;
	uintptr_t curOpCnt = 0;
	for(uintptr_t i = 0; i<saveCig.len; i++){
		char curC = saveCig.txt[i];
		if((curC >= 48) && (curC <= 57)){
			curOpCnt = (curOpCnt*10) + (curC - 48);
			continue;
		}
		if(curOpCnt == 0){
			throw std::runtime_error("Cigar operation missing count.");
		}
		switch(curC){
			case 'M':
			case '=':
			case 'X':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Match operation in cigar string after clipping."); }
				seenAction = 1;
				for(uintptr_t i = 0; i<curOpCnt; i++){
					saveArr[curRead] = curRef;
					curRef++;
					curRead++;
				}
				break;
			case 'I':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Insertion operation in cigar string after clipping."); }
				seenAction = 1;
				for(uintptr_t i = 0; i<curOpCnt; i++){
					saveArr[curRead] = -1;
					curRead++;
				}
				break;
			case 'D':
			case 'N':
				if(softClipEnd || seenHardEnd){ throw std::runtime_error("Deletion operation in cigar string after clipping."); }
				seenAction = 1;
				curRef += curOpCnt;
				break;
			case 'S':
				if(seenHardEnd){ throw std::runtime_error("Cannot soft clip after hard clip."); }
				if(seenAction){ softClipEnd += curOpCnt; } else{ softClipStart += curOpCnt; }
				for(uintptr_t i = 0; i<curOpCnt; i++){
					saveArr[curRead] = -1;
					curRead++;
				}
				break;
			case 'H':
				if(seenAction || softClipStart){ seenHardEnd = 1; }
				break;
			case 'P':
				//I do not care
				break;
			default:
				throw std::runtime_error("Unknown cigar operation.");
		}
		curOpCnt = 0;
	}
	if(curOpCnt){
		throw std::runtime_error("Cigar operation count missing operation.");
	}
	return std::pair<uintptr_t,uintptr_t>(softClipStart, softClipEnd);
}


