#include "whodun_string.h"

using namespace whodun;

size_t whodun::memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				return curCS;
			}
		}
	}
	return numB1;
}

size_t whodun::memspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	for(size_t curCS = 0; curCS < numB1; curCS++){
		for(size_t i = 0; i<numB2; i++){
			if(str1[curCS] == str2[i]){
				goto wasGut;
			}
		}
		return curCS;
		wasGut:
	}
	return numB1;
}

char* whodun::memmem(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB2 > numB1){
		return 0;
	}
	size_t maxCheck = (numB1 - numB2) + 1;
	for(size_t curCS = 0; curCS < maxCheck; curCS++){
		if(memcmp(str1+curCS, str2, numB2) == 0){
			return str1+curCS;
		}
	}
	return 0;
}

void whodun::memswap(char* arrA, char* arrB, size_t numBts){
	for(int i = 0; i<numBts; i++){
		char tmp = arrA[i];
		arrA[i] = arrB[i];
		arrB[i] = tmp;
	}
}

size_t whodun::memdiff(const char* str1, size_t numB1, const char* str2, size_t numB2){
	size_t toRet = 0;
	while(numB1 && numB2){
		if(*str1 != *str2){
			return toRet;
		}
		toRet++;
		str1++;
		numB1--;
		str2++;
		numB2--;
	}
	return toRet;
}

void whodun::packBE64(uint64_t toPrep, char* toBuffer){
	toBuffer[7] = toPrep & 0x00FF;
	toBuffer[6] = (toPrep>>8) & 0x00FF;
	toBuffer[5] = (toPrep>>16) & 0x00FF;
	toBuffer[4] = (toPrep>>24) & 0x00FF;
	toBuffer[3] = (toPrep>>32) & 0x00FF;
	toBuffer[2] = (toPrep>>40) & 0x00FF;
	toBuffer[1] = (toPrep>>48) & 0x00FF;
	toBuffer[0] = (toPrep>>56) & 0x00FF;
}
void whodun::packBE32(uint32_t toPrep, char* toBuffer){
	toBuffer[3] = toPrep & 0x00FF;
	toBuffer[2] = (toPrep>>8) & 0x00FF;
	toBuffer[1] = (toPrep>>16) & 0x00FF;
	toBuffer[0] = (toPrep>>24) & 0x00FF;
}
void whodun::packBE16(uint16_t toPrep, char* toBuffer){
	toBuffer[1] = toPrep & 0x00FF;
	toBuffer[0] = (toPrep>>8) & 0x00FF;
}

void whodun::packLE64(uint64_t toPrep, char* toBuffer){
	toBuffer[0] = toPrep & 0x00FF;
	toBuffer[1] = (toPrep>>8) & 0x00FF;
	toBuffer[2] = (toPrep>>16) & 0x00FF;
	toBuffer[3] = (toPrep>>24) & 0x00FF;
	toBuffer[4] = (toPrep>>32) & 0x00FF;
	toBuffer[5] = (toPrep>>40) & 0x00FF;
	toBuffer[6] = (toPrep>>48) & 0x00FF;
	toBuffer[7] = (toPrep>>56) & 0x00FF;
}
void whodun::packLE32(uint32_t toPrep, char* toBuffer){
	toBuffer[0] = toPrep & 0x00FF;
	toBuffer[1] = (toPrep>>8) & 0x00FF;
	toBuffer[2] = (toPrep>>16) & 0x00FF;
	toBuffer[3] = (toPrep>>24) & 0x00FF;
}
void whodun::packLE16(uint16_t toPrep, char* toBuffer){
	toBuffer[0] = toPrep & 0x00FF;
	toBuffer[1] = (toPrep>>8) & 0x00FF;
}

uint64_t whodun::unpackBE64(const char* toBuffer){
	uint64_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[2]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[3]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[4]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[5]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[6]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[7]);
	return toRet;
}
uint32_t whodun::unpackBE32(const char* toBuffer){
	uint32_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[2]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[3]);
	return toRet;
}
uint16_t whodun::unpackBE16(const char* toBuffer){
	uint16_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	return toRet;
}

uint64_t whodun::unpackLE64(const char* toBuffer){
	uint64_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[7]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[6]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[5]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[4]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[3]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[2]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	return toRet;
}
uint32_t whodun::unpackLE32(const char* toBuffer){
	uint32_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[3]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[2]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	return toRet;
}
uint16_t whodun::unpackLE16(const char* toBuffer){
	uint16_t toRet = 0;
	toRet = (toRet << 8) + (0x00FF & toBuffer[1]);
	toRet = (toRet << 8) + (0x00FF & toBuffer[0]);
	return toRet;
}

uint64_t whodun::packDbl(double toPack){
	//assume same endianess
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveF = toConv;
	return tmpSave.saveI;
}
uint32_t whodun::packFlt(float toPack){
	//assume same endianess
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveF = toConv;
	return tmpSave.saveI;
}

double whodun::unpackDbl(uint64_t toPack){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveI = toConv;
	return tmpSave.saveF;
}
float whodun::unpackFlt(uint32_t toPack){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveI = toConv;
	return tmpSave.saveF;
}

