#include "whodun_gen_table_align.h"

#include <string.h>
#include <algorithm>

using namespace whodun;

//TODO trim

CRBSAMFileContents::CRBSAMFileContents(){
	entryStore = 0;
	entryAlloc = 0;
}
CRBSAMFileContents::~CRBSAMFileContents(){
	if(entryStore){ free(entryStore); }
}
void CRBSAMFileContents::ensureCapacity(uintptr_t numChar){
	if(numChar > entryAlloc){
		entryAlloc = numChar;
		if(entryStore){ free(entryStore); }
		entryStore = (char*)malloc(entryAlloc);
	}
}
CRBSAMFileContents& CRBSAMFileContents::operator=(const CRBSAMFileContents& toCopy){
	lastReadHead = toCopy.lastReadHead;
	char* nextStore;
	#define CRBSAMCOPYSTRENT(entName)\
		entName.len = toCopy.entName.len;\
		entName.txt = nextStore;\
		memcpy(nextStore, toCopy.entName.txt, toCopy.entName.len);\
		nextStore += toCopy.entName.len;
	if(lastReadHead){
		ensureCapacity(toCopy.headerLine.len);
		nextStore = entryStore;
		CRBSAMCOPYSTRENT(headerLine)
	}
	else{
		uintptr_t totNeed = toCopy.name.len;
			totNeed += toCopy.reference.len;
			totNeed += toCopy.cigar.len;
			totNeed += toCopy.nextReference.len;
			totNeed += toCopy.seq.len;
			totNeed += toCopy.qual.len;
			totNeed += toCopy.extra.len;
		ensureCapacity(totNeed);
		nextStore = entryStore;
		CRBSAMCOPYSTRENT(name);
		flag = toCopy.flag;
		CRBSAMCOPYSTRENT(reference);
		pos = toCopy.pos;
		mapq = toCopy.mapq;
		CRBSAMCOPYSTRENT(cigar);
		CRBSAMCOPYSTRENT(nextReference);
		nextPos = toCopy.nextPos;
		tempLen = toCopy.tempLen;
		CRBSAMCOPYSTRENT(seq);
		CRBSAMCOPYSTRENT(qual);
		CRBSAMCOPYSTRENT(extra);
	}
	return *this;
}

CRBSAMFileReader::CRBSAMFileReader(){}
CRBSAMFileReader::~CRBSAMFileReader(){
	for(uintptr_t i = 0; i<cache.size(); i++){
		delete(cache[i]);
	}
}
CRBSAMFileContents* CRBSAMFileReader::allocate(){
	CRBSAMFileContents* toRet;
	if(cache.size()){
		toRet = cache[cache.size()-1];
		cache.pop_back();
	}
	else{
		toRet = new CRBSAMFileContents();
	}
	return toRet;
}
void CRBSAMFileReader::release(CRBSAMFileContents* toRet){
	cache.push_back(toRet);
}

CRBSAMFileWriter::CRBSAMFileWriter(){}
CRBSAMFileWriter::~CRBSAMFileWriter(){}
void CRBSAMFileWriter::write(uintptr_t numWrite, CRBSAMFileContents** allOut){
	for(uintptr_t i = 0; i<numWrite; i++){
		write(allOut[i]);
	}
}

/**Silently drop an entry.*/
#define CRBSAM_SILENT_ELIDE -1
/**A bad entry was present.*/
#define CRBSAM_MALFORMED_ENTRY -2

CRBSAMUniform::CRBSAMUniform(){
	backSpl.splitOn = '\t';
}
CRBSAMUniform::~CRBSAMUniform(){}
void CRBSAMUniform::doIt(){
	switch(actDo){
		case 0:{
			//convert from asText to asParse
			TextTableRow* asText = samConv.asText;
			CRBSAMFileContents* asParse = samConv.asParse;
			asText->trim();
			//skip obviously invalids
			if(asText->numEntries == 0){
				asParse->lastReadHead = CRBSAM_SILENT_ELIDE;
				return;
			}
			if(memspn(asText->curEntries[0], asText->entrySizes[0], " \t\r\n", 4) == asText->entrySizes[0]){
				asParse->lastReadHead = CRBSAM_SILENT_ELIDE;
				return;
			}
			//pack the line on the thing
			uintptr_t lineLen = asText->numEntries;
			for(uintptr_t i = 0; i<asText->numEntries; i++){
				lineLen += asText->entrySizes[i];
			}
			asParse->ensureCapacity(lineLen);
			lineLen = 0;
			for(uintptr_t i = 0; i<asText->numEntries; i++){
				memcpy(asParse->entryStore + lineLen, asText->curEntries[i], asText->entrySizes[i]);
				lineLen += asText->entrySizes[i];
				asParse->entryStore[lineLen] = 0;
				lineLen++;
			}
			lineLen = 0;
			#define SAM_READ_PARSE_TEXT(varNam, tsvInd) \
				asParse->varNam.len = asText->entrySizes[tsvInd];\
				asParse->varNam.txt = asParse->entryStore + lineLen;\
				lineLen += asText->entrySizes[tsvInd]; lineLen++;\
				if((asParse->varNam.len == 1) && (asParse->varNam.txt[0] == '*')){\
					asParse->varNam.len = 0;\
				}
			#define SAM_READ_PARSE_INT(varNam, tsvInd) \
				asParse->varNam = atol(asParse->entryStore + lineLen);\
				lineLen += asText->entrySizes[tsvInd]; lineLen++;
			#define SAM_READ_SALVAGE_REST(varName, tsvInd) \
				asParse->varName.len = 0;\
				asParse->varName.txt = asParse->entryStore + lineLen;\
				for(uintptr_t i = tsvInd; i<asText->numEntries; i++){\
					asParse->varName.len += asText->entrySizes[i];\
					lineLen += asText->entrySizes[i];\
					asParse->entryStore[lineLen] = '\t';\
					lineLen++;\
					if(i > tsvInd){ asParse->varName.len++; }\
				}
			//see if it is a header
			if(asText->entrySizes[0] && (asText->curEntries[0][0] == '@')){
				asParse->lastReadHead = 1;
				SAM_READ_SALVAGE_REST(headerLine, 0)
				return;
			}
			//parse the entry
			if(asText->numEntries < 11){
				asParse->lastReadHead = CRBSAM_MALFORMED_ENTRY;
				return;
			}
			asParse->lastReadHead = 0;
			lineLen = 0;
			SAM_READ_PARSE_TEXT(name, 0)
			SAM_READ_PARSE_INT(flag, 1)
			SAM_READ_PARSE_TEXT(reference, 2)
			SAM_READ_PARSE_INT(pos, 3) asParse->pos--;
			SAM_READ_PARSE_INT(mapq, 4)
			SAM_READ_PARSE_TEXT(cigar, 5)
			SAM_READ_PARSE_TEXT(nextReference, 6)
			SAM_READ_PARSE_INT(nextPos, 7) asParse->nextPos--;
			SAM_READ_PARSE_INT(tempLen, 8)
			SAM_READ_PARSE_TEXT(seq, 9)
			SAM_READ_PARSE_TEXT(qual, 10)
			SAM_READ_SALVAGE_REST(extra, 11)
		break;}
		case 1:{
			//convert from asParse to asText
			TextTableRow* asText = samConv.asText;
			CRBSAMFileContents* asParse = samConv.asParse;
			asText->numEntries = 0;
			uintptr_t curOff = 0;
			if(asParse->lastReadHead){
				//split the header
				backSpl.splitAll(asParse->headerLine.len, asParse->headerLine.txt);
				asText->ensureCapacity(backSpl.res.numSplits, asParse->headerLine.len);
			}
			else{
				//split the extra
				backSpl.splitAll(asParse->extra.len, asParse->extra.txt);
				//figure out how much space is necessary
				uintptr_t numNeedC = asParse->name.len;
					numNeedC += 8*sizeof(intptr_t);
					numNeedC += asParse->reference.len;
					numNeedC += 8*sizeof(intptr_t);
					numNeedC += 8*sizeof(intptr_t);
					numNeedC += asParse->cigar.len;
					numNeedC += asParse->nextReference.len;
					numNeedC += 8*sizeof(intptr_t);
					numNeedC += 8*sizeof(intptr_t);
					numNeedC += asParse->seq.len;
					numNeedC += asParse->qual.len;
					numNeedC += asParse->extra.len;
				asText->ensureCapacity(11 + backSpl.res.numSplits, numNeedC + 12);
				asText->numEntries = 0;
				uintptr_t curOff = 0;
				#define SAM_DUMP_WRITE_TEXT(fromTxt) \
					asText->curEntries[asText->numEntries] = asText->entryStore + curOff;\
					if(fromTxt.len == 0){\
						asText->entrySizes[asText->numEntries] = 1;\
						asText->curEntries[asText->numEntries][0] = '*';\
					}\
					else{\
						asText->entrySizes[asText->numEntries] = fromTxt.len;\
						memcpy(asText->curEntries[asText->numEntries], fromTxt.txt, fromTxt.len);\
					}\
					curOff += asText->entrySizes[asText->numEntries];\
					asText->numEntries++;
				#define SAM_DUMP_WRITE_INT(fromVal) \
					asText->curEntries[asText->numEntries] = asText->entryStore + curOff;\
					sprintf(asText->curEntries[asText->numEntries], "%jd", (intmax_t)(fromVal));\
					asText->entrySizes[asText->numEntries] = strlen(asText->curEntries[asText->numEntries]);\
					curOff += asText->entrySizes[asText->numEntries];\
					asText->numEntries++;
				SAM_DUMP_WRITE_TEXT(asParse->name)
				SAM_DUMP_WRITE_INT(asParse->flag)
				SAM_DUMP_WRITE_TEXT(asParse->reference)
				SAM_DUMP_WRITE_INT(asParse->pos + 1)
				SAM_DUMP_WRITE_INT(asParse->mapq)
				SAM_DUMP_WRITE_TEXT(asParse->cigar)
				SAM_DUMP_WRITE_TEXT(asParse->nextReference)
				SAM_DUMP_WRITE_INT(asParse->nextPos + 1)
				SAM_DUMP_WRITE_INT(asParse->tempLen)
				SAM_DUMP_WRITE_TEXT(asParse->seq)
				SAM_DUMP_WRITE_TEXT(asParse->qual)
			}
			for(uintptr_t i = 0; i<backSpl.res.numSplits; i++){
				char* curS = backSpl.res.splitStart[i];
				char* curE = backSpl.res.splitEnd[i];
				if(curS == curE){ continue; }
				asText->entrySizes[asText->numEntries] = curE - curS;
				asText->curEntries[asText->numEntries] = asText->entryStore + curOff;
				memcpy(asText->curEntries[asText->numEntries], curS, curE - curS);
				curOff += (curE - curS);
				asText->numEntries++;
			}
		break;}
		case 2:{
			bamConv.lastErr = 0;
			SizePtrString asData = bamConv.asData;
			CRBSAMFileContents* asParse = bamConv.asParse;
			asParse->lastReadHead = 0;
			//setup get
				uintptr_t curI = 0;
				uintptr_t nxtI;
				#define BAM_READ_BYTE(toVar, errMess) \
					if((curI + 1) > asData.len){ bamConv.lastErr = errMess; return; }\
					toVar = 0x00FF & asData.txt[curI];\
					curI++;
				#define BAM_READ_I16(toVar, errMess) \
					if((curI + 2) > asData.len){ bamConv.lastErr = errMess; return; }\
					toVar = unpackLE16(asData.txt + curI);\
					curI += 2;
				#define BAM_READ_I32(toVar, errMess) \
					if((curI + 4) > asData.len){ bamConv.lastErr = errMess; return; }\
					toVar = unpackLE32(asData.txt + curI);\
					curI += 4;
				#define BAM_SIGN_SAVE_BYTE(toVar) if(toVar & 0x0080){ toVar = toVar | (((uint64_t)-1) << 8); }
				#define BAM_SIGN_SAVE_SHORT(toVar) if(toVar & 0x008000){ toVar = toVar | (((uint64_t)-1) << 16); }
				#define BAM_SIGN_SAVE_INT(toVar) if(toVar & 0x80000000){ toVar = toVar | (((uint64_t)-1) << 32); }
			//get the constant crap
				int32_t refInd; BAM_READ_I32(refInd, "Reference index missing.") BAM_SIGN_SAVE_INT(refInd)
				int32_t entryPos; BAM_READ_I32(entryPos, "Position missing.") BAM_SIGN_SAVE_INT(entryPos) asParse->pos = entryPos;
				uintptr_t namLen; BAM_READ_BYTE(namLen, "Name length missing."); namLen = 0x00FF & namLen;
				BAM_READ_BYTE(asParse->mapq, "Map quality missing.");
				curI+=2; //I don't care about the index bin
				uintptr_t numCigOp; BAM_READ_I16(numCigOp, "Number of cigar operations missing.")
				BAM_READ_I16(asParse->flag, "SAM flag missing.")
				uintptr_t seqLen; BAM_READ_I32(seqLen, "Sequence length missing.")
				int32_t nrefInd; BAM_READ_I32(nrefInd, "Next reference index missing.") BAM_SIGN_SAVE_INT(nrefInd)
				intptr_t nextPos; BAM_READ_I32(nextPos, "Next position missing.") BAM_SIGN_SAVE_INT(nextPos) asParse->nextPos = nextPos;
				int32_t entryTempLen; BAM_READ_I32(entryTempLen, "Template length missing.") BAM_SIGN_SAVE_INT(entryTempLen) asParse->tempLen = entryTempLen;
			//get the relevant references
				std::string* refName = 0;
				if(refInd >= 0){
					if((uintptr_t)refInd >= bamConv.refNames->size()){ bamConv.lastErr = "BAM entry has bad reference index."; return; }
					refName = &((*(bamConv.refNames))[refInd]);
				}
				std::string* nrefName = 0;
				if(nrefInd >= 0){
					if((uintptr_t)nrefInd >= bamConv.refNames->size()){ bamConv.lastErr = "BAM entry has bad reference index."; return; }
					nrefName = &((*(bamConv.refNames))[nrefInd]);
				}
			//get the size
				uintptr_t needSizeL = 5*asData.len;
					needSizeL += (refName ? refName->size() : 0);
					needSizeL += (nrefName ? nrefName->size() : 0);
				asParse->ensureCapacity(needSizeL);
				uintptr_t packOff = 0;
			//pack reference
				asParse->reference.len = 0;
				if(refName){
					asParse->reference.len = refName->size();
					asParse->reference.txt = asParse->entryStore + packOff;
					memcpy(asParse->entryStore + packOff, refName->c_str(), refName->size());
					packOff += refName->size();
				}
			//pack next reference
				asParse->nextReference.len = 0;
				if(nrefName){
					asParse->nextReference.len = nrefName->size();
					asParse->nextReference.txt = asParse->entryStore + packOff;
					memcpy(asParse->entryStore + packOff, nrefName->c_str(), nrefName->size());
					packOff += nrefName->size();
				}
			//pack name
				if((curI + namLen) > asData.len){bamConv.lastErr = "Bam Entry name truncated."; return; }
				asParse->name.len = std::max((intptr_t)namLen-1, (intptr_t)0);
				asParse->name.txt = asParse->entryStore + packOff;
				memcpy(asParse->entryStore + packOff, asData.txt + curI, namLen);
				packOff += namLen;
				curI += namLen;
			//mark the cigar to return to
				uintptr_t cigRetI = curI;
				curI += 4*numCigOp;
			//sequence
				const char* seqBaseMap = "=ACMGRSVTWYHKDBN";
				nxtI = curI + ((seqLen + 1)/2);
				if(nxtI > asData.len){ bamConv.lastErr = "BAM entry sequence incomplete."; return; }
				asParse->seq.len = seqLen;
				asParse->seq.txt = asParse->entryStore + packOff;
				for(uintptr_t i = 0; i<seqLen; i+=2){
					char curBt = asData.txt[curI + (i>>1)];
					asParse->seq.txt[i] = seqBaseMap[0x0F & (curBt >> 4)];
				}
				for(uintptr_t i = 1; i<seqLen; i+=2){
					char curBt = asData.txt[curI + (i>>1)];
					asParse->seq.txt[i] = seqBaseMap[0x0F & curBt];
				}
				packOff += seqLen;
				curI = nxtI;
			//quality
				nxtI = curI + seqLen;
				if(nxtI > asData.len){ bamConv.lastErr = "BAM entry quality incomplete."; return; }
				asParse->qual.txt = asParse->entryStore + packOff;
				asParse->qual.len = seqLen;
				for(uintptr_t i = 0; i<seqLen; i++){
					int curQV = 0x00FF & asData.txt[curI+i];
					if(curQV == 0x00FF){
						asParse->qual.len = 0;
						break;
					}
					asParse->qual.txt[i] = (curQV + 33);
				}
				packOff += seqLen;
				curI = nxtI;
			//extra crap (keep an eye out for cigar)
				char numConvBuffer[64+4*sizeof(uintmax_t)];
				uintptr_t cigExtS = 0;
				uintptr_t cigExtE = 0;
				int numT = 0;
				asParse->extra.len = 0;
				asParse->extra.txt = asParse->entryStore + packOff;
				while(curI < asData.len){
					char tagA; BAM_READ_BYTE(tagA, "Missing tag name.")
					char tagB; BAM_READ_BYTE(tagB, "Missing tag name.")
					char tagTp; BAM_READ_BYTE(tagTp, "Missing tag type.");
					#define BAM_TAG_PUSH_BYTE(toPush) asParse->extra.txt[asParse->extra.len] = (toPush); asParse->extra.len++; packOff++;
					#define BAM_TAG_PUSH_STR(toPushS, toPushE) memcpy(asParse->extra.txt + asParse->extra.len, toPushS, (toPushE - toPushS)); asParse->extra.len += (toPushE - toPushS); packOff += (toPushE - toPushS);
					#define BAM_TAG_NMTPOUT(overTp) \
						if(numT){BAM_TAG_PUSH_BYTE('\t')}\
						BAM_TAG_PUSH_BYTE(tagA)\
						BAM_TAG_PUSH_BYTE(tagB)\
						BAM_TAG_PUSH_BYTE(':')\
						BAM_TAG_PUSH_BYTE(overTp)\
						BAM_TAG_PUSH_BYTE(':')
					switch(tagTp){
						case 'A': {
								BAM_TAG_NMTPOUT(tagTp)
								char curGet; BAM_READ_BYTE(curGet, "Missing tag data (char).")
								BAM_TAG_PUSH_BYTE(curGet);
								numT = 1;
						break;}
						case 'c': {
								BAM_TAG_NMTPOUT(tagTp)
								char curGet; BAM_READ_BYTE(curGet, "Missing tag data (byte).") BAM_SIGN_SAVE_BYTE(curGet)
								int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
						break;}
						case 'C':
							{
								BAM_TAG_NMTPOUT(tagTp)
								unsigned char curGet; BAM_READ_BYTE(curGet, "Missing tag data (byte).")
								int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 's':
							{
								BAM_TAG_NMTPOUT(tagTp)
								int16_t curGet; BAM_READ_I16(curGet, "Missing tag data (short).") BAM_SIGN_SAVE_SHORT(curGet)
								int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 'S':
							{
								BAM_TAG_NMTPOUT(tagTp)
								uint16_t curGet; BAM_READ_I16(curGet, "Missing tag data (short).")
								int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 'i':
							{
								BAM_TAG_NMTPOUT(tagTp)
								int32_t curGet; BAM_READ_I32(curGet, "Missing tag data (int).") BAM_SIGN_SAVE_INT(curGet)
								int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 'I':
							{
								BAM_TAG_NMTPOUT(tagTp)
								uint32_t curGet; BAM_READ_I32(curGet, "Missing tag data (int).")
								int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 'f':
							{
								BAM_TAG_NMTPOUT(tagTp)
								uint32_t curGet; BAM_READ_I32(curGet, "Missing tag data (float).")
								int numC = sprintf(numConvBuffer, "%.6e", (double)unpackFlt(curGet));
								BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
								numT = 1;
							}
							break;
						case 'Z':
							{
								BAM_TAG_NMTPOUT(tagTp)
								char curGet; BAM_READ_BYTE(curGet, "Missing tag text.")
								while(curGet){
									BAM_TAG_PUSH_BYTE(curGet);
									BAM_READ_BYTE(curGet, "Missing tag text.")
								}
								numT = 1;
							}
							break;
						case 'H':
							{
								BAM_TAG_NMTPOUT(tagTp)
								char curGet; BAM_READ_BYTE(curGet, "Missing tag hex data.")
								while(curGet){
									BAM_TAG_PUSH_BYTE(curGet);
									BAM_READ_BYTE(curGet, "Missing tag hex data.")
									BAM_TAG_PUSH_BYTE(curGet);
									BAM_READ_BYTE(curGet, "Missing tag hex data.")
								}
								numT = 1;
							}
							break;
						case 'B': {
								char subTp; BAM_READ_BYTE(subTp, "Missing array type.")
								#define TAG_ARR_NMTPOUT(overTp, underTp) \
									if(numT){BAM_TAG_PUSH_BYTE('\t')}\
									BAM_TAG_PUSH_BYTE(tagA)\
									BAM_TAG_PUSH_BYTE(tagB)\
									BAM_TAG_PUSH_BYTE(':')\
									BAM_TAG_PUSH_BYTE(overTp)\
									BAM_TAG_PUSH_BYTE(':')\
									BAM_TAG_PUSH_BYTE(underTp)
								uint32_t arrLen; BAM_READ_I32(arrLen, "Missing array length.")
								switch(subTp){
									case 'c': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												char curGet; BAM_READ_BYTE(curGet, "Missing array data (byte).") BAM_SIGN_SAVE_BYTE(curGet)
												int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									case 'C': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												unsigned char curGet; BAM_READ_BYTE(curGet, "Missing array data (byte).")
												int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									case 's': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												int16_t curGet; BAM_READ_I16(curGet, "Missing array data (short).") BAM_SIGN_SAVE_SHORT(curGet)
												int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									case 'S': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												uint16_t curGet; BAM_READ_I16(curGet, "Missing array data (short).")
												int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									case 'i': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												int32_t curGet; BAM_READ_I32(curGet, "Missing array data (int).") BAM_SIGN_SAVE_INT(curGet)
												int numC = sprintf(numConvBuffer, "%jd", (intmax_t)curGet);
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									case 'I': {
											if((tagA == 'C') && (tagB == 'G')){
												cigExtS = curI;
												curI += 4*arrLen;
												cigExtE = curI;
												if(curI > asData.len){ bamConv.lastErr = "BAM entry cigar extension truncated."; return; }
											}
											else{
												TAG_ARR_NMTPOUT(tagTp, subTp)
												for(uintptr_t i = 0; i<arrLen; i++){
													BAM_TAG_PUSH_BYTE(',');
													uint32_t curGet; BAM_READ_I32(curGet, "Missing array data (int).")
													int numC = sprintf(numConvBuffer, "%ju", (uintmax_t)curGet);
													BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
												}
												numT = 1;
											}
									break;}
									case 'f': {
											TAG_ARR_NMTPOUT(tagTp, subTp)
											for(uintptr_t i = 0; i<arrLen; i++){
												BAM_TAG_PUSH_BYTE(',');
												uint32_t curGet; BAM_READ_I32(curGet, "Missing array data (float).")
												int numC = sprintf(numConvBuffer, "%.6e", (double)unpackFlt(curGet));
												BAM_TAG_PUSH_STR(numConvBuffer, numConvBuffer + numC)
											}
											numT = 1;
									break;}
									default:
										bamConv.lastErr = "Unknown array type code.";
										return;
								}
						break;}
						default:
							bamConv.lastErr = "Unknown extra field type code.";
							return;
					}
				}
			//cigar
				asParse->cigar.len = 0;
				asParse->cigar.txt = asParse->entryStore + packOff;
				uintptr_t realNumOp;
				if(cigExtS != cigExtE){
					curI = cigExtS;
					realNumOp = (cigExtE - cigExtS) >> 2;
				}
				else{
					curI = cigRetI;
					realNumOp = numCigOp;
				}
				const char* cigOpMap = "MIDNSHP=X*******";
				for(uintptr_t i = 0; i<realNumOp; i++){
					uintmax_t cigOpPack; BAM_READ_I32(cigOpPack, "Missing cigar operation.")
					int istrlen = sprintf(numConvBuffer, "%ju", (cigOpPack >> 4));
					memcpy(asParse->cigar.txt + asParse->cigar.len, numConvBuffer, istrlen);
					asParse->cigar.len += istrlen;
					asParse->cigar.txt[asParse->cigar.len] = cigOpMap[cigOpPack & 0x0F];
					asParse->cigar.len++;
				}
		break;}
		default:
			throw std::runtime_error("Da fuq???");
	};
}

SAMFileReader::SAMFileReader(TextTableReader* baseStr){
	theStr = baseStr;
	numThr = 1;
	usePool = 0;
	subEnts.resize(1);
	liveTask.resize(1);
	passTask.push_back(&(liveTask[0]));
}
SAMFileReader::SAMFileReader(TextTableReader* baseStr, int numThread, ThreadPool* mainPool){
	theStr = baseStr;
	numThr = numThread;
	usePool = mainPool;
	subEnts.resize(5*numThr);
	liveTask.resize(subEnts.size());
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		passTask.push_back(&(liveTask[i]));
	}
}
SAMFileReader::~SAMFileReader(){
	for(uintptr_t i = 0; i<waitEnts.size(); i++){ delete(waitEnts[i]); }
}
CRBSAMFileContents* SAMFileReader::read(){
	tailRecurTgt:
	if(waitEnts.size()){
		CRBSAMFileContents* toRet = waitEnts[0];
		waitEnts.pop_front();
		if(toRet->lastReadHead == CRBSAM_SILENT_ELIDE){
			release(toRet);
			goto tailRecurTgt;
		}
		if(toRet->lastReadHead == CRBSAM_MALFORMED_ENTRY){
			release(toRet);
			throw std::runtime_error("Entry in sam file malformed.");
		}
		return toRet;
	}
	
	//read up to 5x numThr table entries
	uintptr_t numGood = 0;
	while(numGood < subEnts.size()){
		subEnts[numGood] = theStr->read();
		if(subEnts[numGood]){
			numGood++;
		}
		else{
			break;
		}
	}
	if(numGood == 0){
		return 0;
	}
	//run the conversions
	for(uintptr_t i = 0; i<numGood; i++){
		CRBSAMUniform* curU = passTask[i];
		curU->actDo = 0;
		curU->samConv.asText = subEnts[i];
		curU->samConv.asParse = allocate();
		waitEnts.push_back(curU->samConv.asParse);
	}
	if(usePool){
		uintptr_t fromI = usePool->addTasks(numGood, (ThreadTask**)&(passTask[0]));
		usePool->joinTasks(fromI, fromI + numGood);
	}
	else{
		liveTask[0].doIt();
	}
	//and return
	for(uintptr_t i = 0; i<numGood; i++){
		theStr->release(subEnts[i]);
	}
	return read();
}
void SAMFileReader::close(){}

SAMFileWriter::SAMFileWriter(TextTableWriter* baseStr){
	theStr = baseStr;
	numThr = 1;
	usePool = 0;
	subEnts.resize(1);
	liveTask.resize(subEnts.size());
	for(uintptr_t i = 0; i<subEnts.size(); i++){
		passTask.push_back(&(liveTask[i]));
		passEnts.push_back(&(subEnts[i]));
	}
}
SAMFileWriter::SAMFileWriter(TextTableWriter* baseStr, int numThread, ThreadPool* mainPool){
	theStr = baseStr;
	numThr = numThread;
	usePool = mainPool;
	subEnts.resize(5*numThr);
	liveTask.resize(subEnts.size());
	for(uintptr_t i = 0; i<subEnts.size(); i++){
		passTask.push_back(&(liveTask[i]));
		passEnts.push_back(&(subEnts[i]));
	}
}
SAMFileWriter::~SAMFileWriter(){}
void SAMFileWriter::write(CRBSAMFileContents* nextOut){
	write(1, &nextOut);
}
void SAMFileWriter::write(uintptr_t numWrite, CRBSAMFileContents** allOut){
	uintptr_t leftW = numWrite;
	CRBSAMFileContents** nextW = allOut;
	while(leftW){
		uintptr_t curNumW = std::min(leftW, subEnts.size());
		//set em up
		for(uintptr_t i = 0; i<curNumW; i++){
			liveTask[i].actDo = 1;
			liveTask[i].samConv.asText = passEnts[i];
			liveTask[i].samConv.asParse = nextW[i];
		}
		//run em
		if(usePool){
			uintptr_t fromI = usePool->addTasks(curNumW, (ThreadTask**)&(passTask[0]));
			usePool->joinTasks(fromI, fromI + curNumW);
		}
		else{
			liveTask[0].doIt();
		}
		//write em
		theStr->write(curNumW, &(passEnts[0]));
		//and move on
		leftW -= curNumW;
		nextW += curNumW;
	}
}
void SAMFileWriter::close(){}

#define BAM_WS_CHARS " \r\t\n"
#define BAM_WS_COUNT 4
BAMFileReader::BAMFileReader(InStream* toParse){
	fromSrc = toParse;
	parseHeader();
	numThr = 1;
	usePool = 0;
	useCopy = 0;
	stageAlloc = 128;
	stageArena = (char*)malloc(stageAlloc);
	copyArena = (char*)malloc(stageAlloc);
	stageSize = 0;
	liveTask.resize(1);
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		passTask.push_back(&(liveTask[i]));
	}
}
BAMFileReader::BAMFileReader(InStream* toParse, int numThread, ThreadPool* mainPool){
	fromSrc = toParse;
	parseHeader();
	numThr = numThread;
	usePool = mainPool;
	useCopy = new MultithreadedStringH(numThread, mainPool);
	stageAlloc = 128;
	stageArena = (char*)malloc(stageAlloc);
	copyArena = (char*)malloc(stageAlloc);
	stageSize = 0;
	liveTask.resize(5*numThr);
	for(uintptr_t i = 0; i<liveTask.size(); i++){
		passTask.push_back(&(liveTask[i]));
	}
}
void BAMFileReader::parseHeader(){
	//FIND THE MAGIC
		char tmpBuff[4];
		if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("No magic in BAM file."); }
		if(memcmp(tmpBuff, "BAM\1", 4)!=0){ throw std::runtime_error("Bad magic in BAM file."); }
	//load in the header
		if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing header length."); }
		uint32_t headLen = unpackLE32(tmpBuff);
		std::vector<char> tempStore(headLen);
		if(fromSrc->readBytes(&(tempStore[0]), headLen) != headLen){ throw std::runtime_error("BAM file missing header data."); }
	//load in the references
		std::vector<char> nameTmp;
		if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference data."); }
		uint32_t numRef = unpackLE32(tmpBuff);
		refNames.resize(numRef);
		for(uint32_t i = 0; i<numRef; i++){
			if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference name length."); }
			uint32_t rnameLen = unpackLE32(tmpBuff);
			nameTmp.resize(rnameLen);
			if(fromSrc->readBytes(&(nameTmp[0]), rnameLen) != rnameLen){ throw std::runtime_error("BAM file missing reference name."); }
			if(rnameLen){ nameTmp.pop_back(); } //get rid of the null at the end
			refNames[i].insert(refNames[i].end(), nameTmp.begin(), nameTmp.end());
			if(fromSrc->readBytes(tmpBuff, 4) != 4){ throw std::runtime_error("BAM file missing reference length."); }
			//don't really care about the reference length: not my problem
		}
	//split up the header
		try{
			uintptr_t tempO = 0;
			while(tempO < tempStore.size()){
				//skip any whitespace
				uintptr_t skipWS = memspn(&(tempStore[tempO]), tempStore.size()-tempO, BAM_WS_CHARS, BAM_WS_COUNT);
				if(skipWS){
					tempO += skipWS;
					continue;
				}
				//current thing had better be an @
				if(tempStore[tempO] != '@'){ throw std::runtime_error("BAM header entry malformed."); }
				//get the header text
				const char* headEntS = &(tempStore[tempO]);
				const char* headEntE = (const char*)memchr(headEntS, '\n', tempStore.size()-tempO);
					headEntE = headEntE ? headEntE : (&(tempStore[0]) + tempStore.size());
				//pack it up
				uintptr_t headLen = headEntE - headEntS;
				tempO += headLen;
				CRBSAMFileContents* toRet = allocate();
				toRet->ensureCapacity(headLen);
				toRet->lastReadHead = 1;
				toRet->headerLine.len = headLen;
				toRet->headerLine.txt = toRet->entryStore;
				memcpy(toRet->entryStore, headEntS, headLen);
				waitEnts.push_back(toRet);
			}
		}catch(std::exception& errE){
			for(uintptr_t i = 0; i<waitEnts.size(); i++){
				delete(waitEnts[i]);
			}
			waitEnts.clear();
			throw;
		}
}
BAMFileReader::~BAMFileReader(){
	for(uintptr_t i = 0; i<waitEnts.size(); i++){
		delete(waitEnts[i]);
	}
	if(useCopy){ delete(useCopy); }
	if(stageArena){ free(stageArena); }
	if(copyArena){ free(copyArena); }
}
CRBSAMFileContents* BAMFileReader::read(){
	if(waitEnts.size()){
		CRBSAMFileContents* toRet = waitEnts[0];
		//TODO
		waitEnts.pop_front();
		return toRet;
	}
	
	refillArena:
	//fill the arena
	uintptr_t arenaRead = stageAlloc - stageSize;
	uintptr_t numRead = fromSrc->readBytes(stageArena + stageSize, arenaRead);
	stageSize += numRead;
	if(stageSize == 0){ return 0; }
	//need the entry size: should be present
	if(stageSize < 4){ throw std::runtime_error("Truncated BAM file: missing entry size."); }
	uintptr_t curFocI = 0;
	uintptr_t curThrI = 0;
	while(curThrI < liveTask.size()){
		if((stageSize - curFocI) < 4){ break; }
		uint32_t entLen = unpackLE32(stageArena + curFocI);
		if((curFocI + entLen + 4) > stageSize){ break; }
		CRBSAMUniform* curU = passTask[curThrI];
		curU->actDo = 2;
		curU->bamConv.asData.len = entLen;
		curU->bamConv.asData.txt = stageArena + curFocI + 4;
		curU->bamConv.asParse = allocate();
		waitEnts.push_back(curU->bamConv.asParse);
		curU->bamConv.refNames = &refNames;
		curFocI = curFocI + entLen + 4;
		curThrI++;
	}
	//run if anything present
	if(curThrI){
		if(usePool){
			uintptr_t fromI = usePool->addTasks(curThrI, (ThreadTask**)&(passTask[0]));
			usePool->joinTasks(fromI, fromI + curThrI);
		}
		else{
			liveTask[0].doIt();
		}
		for(uintptr_t i = 0; i<curThrI; i++){
			if(liveTask[i].bamConv.lastErr){
				throw std::runtime_error(liveTask[i].bamConv.lastErr);
			}
		}
	}
	//repack the trailing data
	if(curThrI < liveTask.size()){
		stageAlloc = stageAlloc << 1;
		free(copyArena);
		copyArena = (char*)malloc(stageAlloc);
	}
	stageSize = stageSize - curFocI;
	if(useCopy){
		useCopy->memcpy(copyArena, stageArena + curFocI, stageSize);
	}
	else{
		memcpy(copyArena, stageArena + curFocI, stageSize);
	}
	if(curThrI < liveTask.size()){
		free(stageArena);
		stageArena = (char*)malloc(stageAlloc);
	}
	char* tmpArena = copyArena;
	copyArena = stageArena;
	stageArena = tmpArena;
	//if NOTHING read, retry
	if(curThrI == 0){ goto refillArena; }
	//simple recall
	return read();
}
void BAMFileReader::close(){}

void whodun::getCRBSamExtensions(int modeWrite, std::set<std::string>* toFill){
	toFill->insert(".sam");
	if(!modeWrite){
		toFill->insert(".bam");
	}
}

ExtensionCRBSAMReader::ExtensionCRBSAMReader(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			baseTsvStrs.push_back(new TSVTableReader(baseStrs[0]));
			wrapStr = new SAMFileReader(baseTsvStrs[0]);
			return;
		}
		//anything explicitly sam
		if(strendswith(fileName,".sam")){
			baseStrs.push_back(new FileInStream(fileName));
			baseTsvStrs.push_back(new TSVTableReader(baseStrs[0]));
			wrapStr = new SAMFileReader(baseTsvStrs[0]);
			return;
		}
		//bam
		if(strendswith(fileName,".bam")){
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new BAMFileReader(baseStrs[0]);
			return;
		}
		//sam is the default
		baseStrs.push_back(new FileInStream(fileName));
		baseTsvStrs.push_back(new TSVTableReader(baseStrs[0]));
		wrapStr = new SAMFileReader(baseTsvStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i;
		i = baseTsvStrs.size();
		while(i){
			i--;
			baseTsvStrs[i]->close();
			delete(baseTsvStrs[i]);
		}
		i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionCRBSAMReader::ExtensionCRBSAMReader(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleInStream());
			baseTsvStrs.push_back(new TSVTableReader(baseStrs[0], numThread, mainPool));
			wrapStr = new SAMFileReader(baseTsvStrs[0], numThread, mainPool);
			return;
		}
		//anything explicitly sam
		if(strendswith(fileName,".sam")){
			baseStrs.push_back(new FileInStream(fileName));
			baseTsvStrs.push_back(new TSVTableReader(baseStrs[0], numThread, mainPool));
			wrapStr = new SAMFileReader(baseTsvStrs[0], numThread, mainPool);
			return;
		}
		//bam
		if(strendswith(fileName,".bam")){
			//TODO multithread block compressed gzip input
			baseStrs.push_back(new GZipInStream(fileName));
			wrapStr = new BAMFileReader(baseStrs[0]);
			return;
		}
		//sam is the default
		baseStrs.push_back(new FileInStream(fileName));
		baseTsvStrs.push_back(new TSVTableReader(baseStrs[0], numThread, mainPool));
		wrapStr = new SAMFileReader(baseTsvStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i;
		i = baseTsvStrs.size();
		while(i){
			i--;
			baseTsvStrs[i]->close();
			delete(baseTsvStrs[i]);
		}
		i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionCRBSAMReader::~ExtensionCRBSAMReader(){
	uintptr_t i;
	delete(wrapStr);
	i = baseTsvStrs.size();
	while(i){
		i--;
		delete(baseTsvStrs[i]);
	}
	i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
CRBSAMFileContents* ExtensionCRBSAMReader::read(){
	return wrapStr->read();
}
void ExtensionCRBSAMReader::close(){
	wrapStr->close();
	uintptr_t i;
	i = baseTsvStrs.size();
	while(i){
		i--;
		baseTsvStrs[i]->close();
	}
	i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}
CRBSAMFileContents* ExtensionCRBSAMReader::allocate(){
	return wrapStr->allocate();
}
void ExtensionCRBSAMReader::release(CRBSAMFileContents* toRet){
	wrapStr->release(toRet);
}

ExtensionCRBSAMWriter::ExtensionCRBSAMWriter(const char* fileName){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0]));
			wrapStr = new SAMFileWriter(baseTsvStrs[0]);
			return;
		}
		//anything explicitly sam
		if(strendswith(fileName,".sam")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0]));
			wrapStr = new SAMFileWriter(baseTsvStrs[0]);
			return;
		}
		//TODO bam
		//sam is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0]));
		wrapStr = new SAMFileWriter(baseTsvStrs[0]);
	}
	catch(std::exception& errE){
		uintptr_t i;
		i = baseTsvStrs.size();
		while(i){
			i--;
			baseTsvStrs[i]->close();
			delete(baseTsvStrs[i]);
		}
		i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionCRBSAMWriter::ExtensionCRBSAMWriter(const char* fileName, int numThread, ThreadPool* mainPool){
	try{
		// - is for stdin
		if(strcmp(fileName, "-")==0){
			baseStrs.push_back(new ConsoleOutStream());
			baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0], numThread, mainPool));
			wrapStr = new SAMFileWriter(baseTsvStrs[0], numThread, mainPool);
			return;
		}
		//anything explicitly sam
		if(strendswith(fileName,".sam")){
			baseStrs.push_back(new FileOutStream(0, fileName));
			baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0], numThread, mainPool));
			wrapStr = new SAMFileWriter(baseTsvStrs[0], numThread, mainPool);
			return;
		}
		//TODO bam
		//sam is the default
		baseStrs.push_back(new FileOutStream(0, fileName));
		baseTsvStrs.push_back(new TSVTableWriter(baseStrs[0], numThread, mainPool));
		wrapStr = new SAMFileWriter(baseTsvStrs[0], numThread, mainPool);
	}
	catch(std::exception& errE){
		uintptr_t i;
		i = baseTsvStrs.size();
		while(i){
			i--;
			baseTsvStrs[i]->close();
			delete(baseTsvStrs[i]);
		}
		i = baseStrs.size();
		while(i){
			i--;
			baseStrs[i]->close();
			delete(baseStrs[i]);
		}
		throw;
	}
}
ExtensionCRBSAMWriter::~ExtensionCRBSAMWriter(){
	uintptr_t i;
	delete(wrapStr);
	i = baseTsvStrs.size();
	while(i){
		i--;
		delete(baseTsvStrs[i]);
	}
	i = baseStrs.size();
	while(i){
		i--;
		delete(baseStrs[i]);
	}
}
void ExtensionCRBSAMWriter::write(CRBSAMFileContents* nextOut){
	wrapStr->write(nextOut);
}
void ExtensionCRBSAMWriter::write(uintptr_t numWrite, CRBSAMFileContents** allOut){
	wrapStr->write(numWrite, allOut);
}
void ExtensionCRBSAMWriter::close(){
	wrapStr->close();
	uintptr_t i;
	i = baseTsvStrs.size();
	while(i){
		i--;
		baseTsvStrs[i]->close();
	}
	i = baseStrs.size();
	while(i){
		i--;
		baseStrs[i]->close();
	}
}

#define WHODUN_SAM_HEAD_META_SORT_UNK_TXT "unknown"
#define WHODUN_SAM_HEAD_META_SORT_NONE_TXT "unsorted"
#define WHODUN_SAM_HEAD_META_SORT_NAME_TXT "queryname"
#define WHODUN_SAM_HEAD_META_SORT_COORD_TXT "coordinate"
#define WHODUN_SAM_HEAD_META_GROUP_NONE_TXT "none"
#define WHODUN_SAM_HEAD_META_GROUP_NAME_TXT "query"
#define WHODUN_SAM_HEAD_META_GROUP_REF_TXT "reference"
#define WHODUN_SAM_HEAD_REF_TOP_LINE_TXT "linear"
#define WHODUN_SAM_HEAD_REF_TOP_CIRCLE_TXT "circular"

CRBSAMHeaderParser::CRBSAMHeaderParser(){
	subSplit.splitOn = '\t';
}
CRBSAMHeaderParser::~CRBSAMHeaderParser(){}
int CRBSAMHeaderParser::parseHeader(CRBSAMFileContents* toParse){
	#define MAX_MEM_INT 32
	char memIntBuff[MAX_MEM_INT+1];
	uintptr_t memIntSize;
	#define PARSE_MEM_INT(strS, strE, saveIn) \
		memIntSize = std::min((uintptr_t)MAX_MEM_INT, (uintptr_t)(strE - strS));\
		memcpy(memIntBuff, strS, memIntSize);\
		memIntBuff[memIntSize] = 0;\
		saveIn = atol(memIntBuff);
	#define PARSE_HEAD_SAVE_STRING(winS, winE, saveIn)\
		(saveIn).len = (winE) - (winS);\
		(saveIn).txt = (winS);
	if(!(toParse->lastReadHead)){
		headType = 0;
		return headType;
	}
	line = toParse->headerLine;
	subSplit.splitAll(line.len, line.txt);
	uintptr_t firstLen = (subSplit.res.splitEnd[0] - subSplit.res.splitStart[0]);
	if(firstLen != 3){
		headType = WHODUN_SAM_HEAD_OTHER;
		return headType;
	}
	if(memcmp(subSplit.res.splitStart[0], "@HD", 3)==0){
		headType = WHODUN_SAM_HEAD_HD;
		//init to default
		headHD.majorVersion = 1;
		headHD.minorVersion = 0;
		headHD.sortOrder = WHODUN_SAM_HEAD_META_SORT_UNK;
		headHD.groupOrder = WHODUN_SAM_HEAD_META_GROUP_NONE;
		headHD.subSortOrder.len = 0;
		//parse
		for(uintptr_t i = 1; i<subSplit.res.numSplits; i++){
			char* curS = subSplit.res.splitStart[i];
			char* curE = subSplit.res.splitEnd[i];
			if(curE - curS < 3){ continue; }
			if(memcmp(curS, "VN:", 3)==0){
				char* majS = curS + 3;
				char* majE = (char*)memchr(majS, '.', curE - majS);
				char* minS = 0;
				char* minE = 0;
				if(majE == 0){
					majE = curE;
				}
				else{
					minS = majE + 1;
					minE = curE;
				}
				PARSE_MEM_INT(majS, majE, headHD.majorVersion)
				PARSE_MEM_INT(minS, minE, headHD.minorVersion)
			}
			else if(memcmp(curS, "SO:", 3)==0){
				char* valS = curS + 3;
				uintptr_t valLen = curE - valS;
				if((valLen==strlen(WHODUN_SAM_HEAD_META_SORT_UNK_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_SORT_UNK_TXT, valLen)==0)){
					headHD.sortOrder = WHODUN_SAM_HEAD_META_SORT_UNK;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_META_SORT_NONE_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_SORT_NONE_TXT, valLen)==0)){
					headHD.sortOrder = WHODUN_SAM_HEAD_META_SORT_NONE;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_META_SORT_NAME_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_SORT_NAME_TXT, valLen)==0)){
					headHD.sortOrder = WHODUN_SAM_HEAD_META_SORT_NAME;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_META_SORT_COORD_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_SORT_COORD_TXT, valLen)==0)){
					headHD.sortOrder = WHODUN_SAM_HEAD_META_SORT_COORD;
				}
			}
			else if(memcmp(curS, "GO:", 3)==0){
				char* valS = curS + 3;
				uintptr_t valLen = curE - valS;
				if((valLen==strlen(WHODUN_SAM_HEAD_META_GROUP_NONE_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_GROUP_NONE_TXT, valLen)==0)){
					headHD.groupOrder = WHODUN_SAM_HEAD_META_GROUP_NONE;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_META_GROUP_NAME_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_GROUP_NAME_TXT, valLen)==0)){
					headHD.groupOrder = WHODUN_SAM_HEAD_META_GROUP_NAME;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_META_GROUP_REF_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_META_GROUP_REF_TXT, valLen)==0)){
					headHD.groupOrder = WHODUN_SAM_HEAD_META_GROUP_REF;
				}
			}
			else if(memcmp(curS, "SS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headHD.subSortOrder)
			}
		}
	}
	else if(memcmp(subSplit.res.splitStart[0], "@SQ", 3)==0){
		headType = WHODUN_SAM_HEAD_SQ;
		//init to default
		headSQ.name.len = 0;
		headSQ.len = 0;
		headSQ.isAlt = 0;
		headSQ.altDesc.len = 0;
		headSQ.altNames.len = 0;
		headSQ.genAssID.len = 0;
		headSQ.desc.len = 0;
		headSQ.md5.len = 0;
		headSQ.species.len = 0;
		headSQ.topology = WHODUN_SAM_HEAD_REF_TOP_LINE;
		headSQ.url.len = 0;
		//parse
		for(uintptr_t i = 1; i<subSplit.res.numSplits; i++){
			char* curS = subSplit.res.splitStart[i];
			char* curE = subSplit.res.splitEnd[i];
			if(curE - curS < 3){ continue; }
			if(memcmp(curS, "SN:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.name)
			}
			else if(memcmp(curS, "LN:", 3)==0){
				PARSE_MEM_INT(curS+3, curE, headSQ.len)
			}
			else if(memcmp(curS, "AH:", 3)==0){
				headSQ.isAlt = 1;
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.altDesc)
			}
			else if(memcmp(curS, "AN:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.altNames)
			}
			else if(memcmp(curS, "AS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.genAssID)
			}
			else if(memcmp(curS, "DS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.desc)
			}
			else if(memcmp(curS, "M5:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.md5)
			}
			else if(memcmp(curS, "SP:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.species)
			}
			else if(memcmp(curS, "TP:", 3)==0){
				char* valS = curS + 3;
				uintptr_t valLen = curE - valS;
				if((valLen==strlen(WHODUN_SAM_HEAD_REF_TOP_LINE_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_REF_TOP_LINE_TXT, valLen)==0)){
					headSQ.topology = WHODUN_SAM_HEAD_REF_TOP_LINE;
				}
				else if((valLen==strlen(WHODUN_SAM_HEAD_REF_TOP_CIRCLE_TXT)) && (memcmp(valS, WHODUN_SAM_HEAD_REF_TOP_CIRCLE_TXT, valLen)==0)){
					headSQ.topology = WHODUN_SAM_HEAD_REF_TOP_CIRCLE;
				}
			}
			else if(memcmp(curS, "UR:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headSQ.url)
			}
		}
	}
	else if(memcmp(subSplit.res.splitStart[0], "@RG", 3)==0){
		headType = WHODUN_SAM_HEAD_RG;
		//init to default
		headRG.id.len = 0;
		headRG.barcode.len = 0;
		headRG.center.len = 0;
		headRG.desc.len = 0;
		headRG.date.len = 0;
		headRG.flow.len = 0;
		headRG.key.len = 0;
		headRG.library.len = 0;
		headRG.progData.len = 0;
		headRG.insertSize = 0;
		headRG.platform.len = 0;
		headRG.platformModel.len = 0;
		headRG.platformUnit.len = 0;
		headRG.sampleName.len = 0;
		//parse
		for(uintptr_t i = 1; i<subSplit.res.numSplits; i++){
			char* curS = subSplit.res.splitStart[i];
			char* curE = subSplit.res.splitEnd[i];
			if(curE - curS < 3){ continue; }
			if(memcmp(curS, "ID:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.id)
			}
			else if(memcmp(curS, "BC:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.barcode)
			}
			else if(memcmp(curS, "CN:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.center)
			}
			else if(memcmp(curS, "DS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.desc)
			}
			else if(memcmp(curS, "DT:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.date)
			}
			else if(memcmp(curS, "FO:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.flow)
			}
			else if(memcmp(curS, "KS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.key)
			}
			else if(memcmp(curS, "LB:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.library)
			}
			else if(memcmp(curS, "PG:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.progData)
			}
			else if(memcmp(curS, "PI:", 3)==0){
				PARSE_MEM_INT(curS+3, curE, headRG.insertSize)
			}
			else if(memcmp(curS, "PL:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.platform)
			}
			else if(memcmp(curS, "PM:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.platformModel)
			}
			else if(memcmp(curS, "PU:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.platformUnit)
			}
			else if(memcmp(curS, "SM:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headRG.sampleName)
			}
		}
	}
	else if(memcmp(subSplit.res.splitStart[0], "@PG", 3)==0){
		headType = WHODUN_SAM_HEAD_PG;
		//init to default
		headPG.id.len = 0;
		headPG.name.len = 0;
		headPG.comLine.len = 0;
		headPG.prevID.len = 0;
		headPG.desc.len = 0;
		headPG.version.len = 0;
		//parse
		for(uintptr_t i = 1; i<subSplit.res.numSplits; i++){
			char* curS = subSplit.res.splitStart[i];
			char* curE = subSplit.res.splitEnd[i];
			if(curE - curS < 3){ continue; }
			if(memcmp(curS, "ID:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.id)
			}
			else if(memcmp(curS, "PN:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.name)
			}
			else if(memcmp(curS, "CL:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.comLine)
			}
			else if(memcmp(curS, "PP:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.prevID)
			}
			else if(memcmp(curS, "DS:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.desc)
			}
			else if(memcmp(curS, "VN:", 3)==0){
				PARSE_HEAD_SAVE_STRING(curS+3, curE, headPG.version)
			}
		}
	}
	else{
		headType = WHODUN_SAM_HEAD_OTHER;
	}
	return headType;
}

CRBSAMExtraParser::CRBSAMExtraParser(){
	subSplit.splitOn = '\t';
}
CRBSAMExtraParser::~CRBSAMExtraParser(){}
void CRBSAMExtraParser::parseExtra(CRBSAMFileContents* toParse){
	char memIntBuff[MAX_MEM_INT+1];
	uintptr_t memIntSize;
	subSplit.splitAll(toParse->extra.len, toParse->extra.txt);
	//initialize
	memset(&(extDatP), 0, sizeof(CRBSAMExtraEntryDataPresent));
	//parse
	for(uintptr_t i = 1; i<subSplit.res.numSplits; i++){
		char* curS = subSplit.res.splitStart[i];
		char* curE = subSplit.res.splitEnd[i];
		if(curE - curS < 5){ continue; }
		if(memcmp(curS, "AM:i:", 5)==0){
			extDatP.minTempMapQuality = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.minTempMapQuality)
		}
		else if(memcmp(curS, "AS:i:", 5)==0){
			extDatP.alignScore = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.alignScore)
		}
		else if(memcmp(curS, "BQ:Z:", 5)==0){
			extDatP.alignQualityOffset = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.alignQualityOffset)
		}
		else if(memcmp(curS, "CC:Z:", 5)==0){
			extDatP.nextHitReference = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.nextHitReference)
		}
		else if(memcmp(curS, "CP:i:", 5)==0){
			extDatP.nextHitCoord = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.nextHitCoord)
		}
		else if(memcmp(curS, "E2:Z:", 5)==0){
			extDatP.alternateBaseCalls = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.alternateBaseCalls)
		}
		else if(memcmp(curS, "FI:i:", 5)==0){
			extDatP.templateIndex = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.templateIndex)
		}
		else if(memcmp(curS, "FS:Z:", 5)==0){
			extDatP.segmentSuffix = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.segmentSuffix)
		}
		else if(memcmp(curS, "H0:i:", 5)==0){
			extDatP.hitCountPerfect = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.hitCountPerfect)
		}
		else if(memcmp(curS, "H1:i:", 5)==0){
			extDatP.hitCountByOne = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.hitCountByOne)
		}
		else if(memcmp(curS, "H2:i:", 5)==0){
			extDatP.hitCountByTwo = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.hitCountByTwo)
		}
		else if(memcmp(curS, "HI:i:", 5)==0){
			extDatP.hitIndex = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.hitIndex)
		}
		else if(memcmp(curS, "IH:i:", 5)==0){
			extDatP.numAlignmentsIH = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.numAlignmentsIH)
		}
		else if(memcmp(curS, "MC:Z:", 5)==0){
			extDatP.pairCigar = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.pairCigar)
		}
		else if(memcmp(curS, "MD:Z:", 5)==0){
			extDatP.referenceDiff = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.referenceDiff)
		}
		else if(memcmp(curS, "MQ:i:", 5)==0){
			extDatP.pairMapQuality = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.pairMapQuality)
		}
		else if(memcmp(curS, "NH:i:", 5)==0){
			extDatP.numAlignmentsNH = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.numAlignmentsNH)
		}
		else if(memcmp(curS, "NM:i:", 5)==0){
			extDatP.numDifference = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.numDifference)
		}
		else if(memcmp(curS, "PQ:Z:", 5)==0){
			extDatP.mappedPhred = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.mappedPhred)
		}
		else if(memcmp(curS, "Q2:Z:", 5)==0){
			extDatP.pairQuality = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.pairQuality)
		}
		else if(memcmp(curS, "R2:Z:", 5)==0){
			extDatP.pairSequence = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.pairSequence)
		}
		else if(memcmp(curS, "SA:Z:", 5)==0){
			extDatP.chimericAlts = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.chimericAlts)
		}
		else if(memcmp(curS, "SM:i:", 5)==0){
			extDatP.unpairedMapQuality = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.unpairedMapQuality)
		}
		else if(memcmp(curS, "TC:i:", 5)==0){
			extDatP.templateCount = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.templateCount)
		}
		else if(memcmp(curS, "TS:A:", 5)==0){
			if(curE - curS < 6){ continue; }
			extDatP.strand = 1;
			extDat.strand = curS[5];
		}
		else if(memcmp(curS, "U2:Z:", 5)==0){
			extDatP.alternateBaseQuals = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.alternateBaseQuals)
		}
		else if(memcmp(curS, "UQ:i:", 5)==0){
			extDatP.segmentQual = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.segmentQual)
		}
		else if(memcmp(curS, "RG:Z:", 5)==0){
			extDatP.readGroup = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.readGroup)
		}
		else if(memcmp(curS, "LB:Z:", 5)==0){
			extDatP.library = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.library)
		}
		else if(memcmp(curS, "PG:Z:", 5)==0){
			extDatP.program = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.program)
		}
		else if(memcmp(curS, "PU:Z:", 5)==0){
			extDatP.platformUnit = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.platformUnit)
		}
		else if(memcmp(curS, "BC:Z:", 5)==0){
			extDatP.barcodeSeq = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.barcodeSeq)
		}
		else if(memcmp(curS, "QT:Z:", 5)==0){
			extDatP.barcodeQual = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.barcodeQual)
		}
		else if(memcmp(curS, "CB:Z:", 5)==0){
			extDatP.cellID = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.cellID)
		}
		else if(memcmp(curS, "CR:Z:", 5)==0){
			extDatP.cellSeq = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.cellSeq)
		}
		else if(memcmp(curS, "CY:Z:", 5)==0){
			extDatP.cellQual = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.cellQual)
		}
		else if(memcmp(curS, "MI:Z:", 5)==0){
			extDatP.umiID = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.umiID)
		}
		else if(memcmp(curS, "OX:Z:", 5)==0){
			extDatP.umiSeq = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.umiSeq)
		}
		else if(memcmp(curS, "BZ:Z:", 5)==0){
			extDatP.umiQual = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.umiQual)
		}
		else if(memcmp(curS, "RX:Z:", 5)==0){
			extDatP.umiRawSeq = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.umiRawSeq)
		}
		else if(memcmp(curS, "QX:Z:", 5)==0){
			extDatP.umiRawQual = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.umiRawQual)
		}
		else if(memcmp(curS, "OA:Z:", 5)==0){
			extDatP.previousAlignments = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.previousAlignments)
		}
		else if(memcmp(curS, "OQ:Z:", 5)==0){
			extDatP.previousQuality = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.previousQuality)
		}
		else if(memcmp(curS, "CT:Z:", 5)==0){
			extDatP.readAnnotations = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.readAnnotations)
		}
		else if(memcmp(curS, "PT:Z:", 5)==0){
			extDatP.readPaddedAnnotations = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.readPaddedAnnotations)
		}
		else if(memcmp(curS, "CM:i:", 5)==0){
			extDatP.colorEditDistance = 1;
			PARSE_MEM_INT(curS+5, curE, extDat.colorEditDistance)
		}
		else if(memcmp(curS, "CS:Z:", 5)==0){
			extDatP.colorSeq = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.colorSeq)
		}
		else if(memcmp(curS, "CQ:Z:", 5)==0){
			extDatP.colorQual = 1;
			PARSE_HEAD_SAVE_STRING(curS+5, curE, extDat.colorQual)
		}
		else if(curE - curS < 6){ continue; }
		else if(memcmp(curS, "FZ:B:S", 6)==0){
			extDatP.flowIntensity = 1;
			PARSE_HEAD_SAVE_STRING(curS+6, curE, extDat.flowIntensity)
		}
	}
}

CRBSAMPairedEndCache::CRBSAMPairedEndCache(){}
CRBSAMPairedEndCache::~CRBSAMPairedEndCache(){}
bool CRBSAMPairedEndCache::entryIsPrimary(CRBSAMFileContents* toExamine){
	return (toExamine->flag & (WHODUN_SAM_FLAG_SECONDARY | WHODUN_SAM_FLAG_SUPPLEMENT)) == 0;
}
bool CRBSAMPairedEndCache::entryIsAlign(CRBSAMFileContents* toExamine){
	if(toExamine->flag & WHODUN_SAM_FLAG_SEGUNMAP){
		return false;
	}
	//will only handle up to pairs: triples and more not entertained
	if((toExamine->flag & (WHODUN_SAM_FLAG_FIRST | WHODUN_SAM_FLAG_LAST)) == (WHODUN_SAM_FLAG_FIRST | WHODUN_SAM_FLAG_LAST)){
		return false;
	}
	if(toExamine->pos < 0){
		return false;
	}
	uintptr_t cigSize = toExamine->cigar.len;
	if(cigSize == 0){
		return false;
	}
	//if M=X in the thing
	char* curLook = toExamine->cigar.txt;
	if(memchr(curLook, 'M', cigSize) || memchr(curLook, '=', cigSize) || memchr(curLook, 'X', cigSize)){
		return true;
	}
	return false;
}
bool CRBSAMPairedEndCache::entryNeedPair(CRBSAMFileContents* toExamine){
	return toExamine->flag & WHODUN_SAM_FLAG_MULTSEG;
}
bool CRBSAMPairedEndCache::havePair(CRBSAMFileContents* toExamine){
	return (waitingPair.find(toExamine->name) != waitingPair.end());
}
std::pair<uintptr_t,CRBSAMFileContents*> CRBSAMPairedEndCache::getPair(CRBSAMFileContents* forAln){
	std::map<SizePtrString, uintptr_t, bool(*)(SizePtrString,SizePtrString)>::iterator namIdIt = waitingPair.find(forAln->name);
	uintptr_t pairID = namIdIt->second;
	std::map<uintptr_t, CRBSAMFileContents* >::iterator pairIt = pairData.find(pairID);
	CRBSAMFileContents* pairDat = pairIt->second;
	waitingPair.erase(namIdIt);
	pairData.erase(pairIt);
	return std::pair<uintptr_t,CRBSAMFileContents*>(pairID,pairDat);
}
void CRBSAMPairedEndCache::waitForPair(uintptr_t alnID, CRBSAMFileContents* forAln){
	waitingPair[forAln->name] = alnID;
	pairData[alnID] = forAln;
}
void CRBSAMPairedEndCache::getOutstanding(std::vector<uintptr_t>* saveID, std::vector<CRBSAMFileContents*>* saveAln){
	std::map<SizePtrString, uintptr_t, bool(*)(SizePtrString,SizePtrString)>::iterator namIdIt = waitingPair.begin();
	while(namIdIt != waitingPair.end()){
		saveID->push_back(namIdIt->second);
		saveAln->push_back(pairData[namIdIt->second]);
		namIdIt++;
	}
	waitingPair.clear();
	pairData.clear();
}

