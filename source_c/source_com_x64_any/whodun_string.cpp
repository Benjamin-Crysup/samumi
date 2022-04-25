#include "whodun_string.h"

#include <string.h>

using namespace whodun;

size_t whodun::memcspn(const char* str1, size_t numB1, const char* str2, size_t numB2){
	if(numB1 == 0){ return numB1; }
	if(numB2 == 0){ return numB1; }
	if(numB2 < 8){
		uint64_t bvecSearch = 0;
		uint64_t bvecMask = -1;
		for(size_t i = 0; i<numB2; i++){
			uint64_t curBt = 0x00FF & str2[i];
			bvecSearch |= (curBt << (i<<3));
			bvecMask = (bvecMask << 8);
		}
		//bvecSearch = str2[0] << 0 | str2[1] << 8 ...
		//bvecMask = FFFFFFFF << 8*numB2
		//str1[i] in bvecSearch = anyByteZero(((0x0101010101010101 * str1[i])^bvecSearch)|bvecMask)
		//anyByteZero(v) = (v-0x0101010101010101) & ~v & 0x8080808080808080
		const char* endPt;
		asm volatile (
			"movq $0x0101010101010101, %%r8\n"
			"movq $0x8080808080808080, %%r9\n"
			"memcspn_rep%=:\n"
			"xorq %%r10, %%r10\n"
			"movb (%%rax), %%r10b\n"
			"imulq %%r8, %%r10\n"
			"xorq %%rcx, %%r10\n"
			"orq %%rdx, %%r10\n"
			"movq %%r10, %%r11\n"
			"subq %%r8, %%r10\n"
			"notq %%r11\n"
			"andq %%r11, %%r10\n"
			"andq %%r9, %%r10\n"
			"jnz memcspn_done%=\n"
			"addq $1, %%rax\n"
			"addq $-1, %%rbx\n"
			"jnz memcspn_rep%=\n"
			"memcspn_done%=:\n"
			"xorq %%r10, %%r10\n"
		: "=a" (endPt)
		: "a" (str1), "b" (numB1), "c" (bvecSearch), "d" (bvecMask)
		: "cc", "r8", "r9", "r10", "r11"
		);
		return endPt - str1;
	}
	{
		for(size_t curCS = 0; curCS < numB1; curCS++){
			for(size_t i = 0; i<numB2; i++){
				if(str1[curCS] == str2[i]){
					return curCS;
				}
			}
		}
		return numB1;
	}
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
		;
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
			return (char*)(str1+curCS);
		}
	}
	return 0;
}

/**
 * This will swap byte by byte.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numBts The number of bytes to swap.
 */
void memswap_direct_byte(char* arrA, char* arrB, size_t numBts){
	//just swap byte by byte
	asm volatile(
		"memswap_byte_rep%=:\n"
		"movb (%%rax), %%r8b\n"
		"movb (%%rbx), %%r9b\n"
		"movb %%r8b, (%%rbx)\n"
		"movb %%r9b, (%%rax)\n"
		"addq $1, %%rax\n"
		"addq $1, %%rbx\n"
		"addq $-1, %%rcx\n"
		"jnz memswap_byte_rep%=\n"
	:
	: "a" (arrA), "b" (arrB), "c" (numBts)
	: "cc", "memory", "r8", "r9"
	);
}

/**
 * This will swap word by word.
 * @param arrA THe first array.
 * @param arrB The second array.
 * @param numWord The number of words to swap.
 */
void memswap_direct_word(char* arrA, char* arrB, size_t numWord){
	//just swap word by word
	asm volatile(
		"memswap_word_rep%=:\n"
		"movq (%%rax), %%r8\n"
		"movq (%%rbx), %%r9\n"
		"movq %%r8, (%%rbx)\n"
		"movq %%r9, (%%rax)\n"
		"addq $8, %%rax\n"
		"addq $8, %%rbx\n"
		"addq $-1, %%rcx\n"
		"jnz memswap_word_rep%=\n"
	:
	: "a" (arrA), "b" (arrB), "c" (numWord)
	: "cc", "memory", "r8", "r9"
	);
}

void whodun::memswap(char* arrA, char* arrB, size_t numBts){
	if(numBts == 0){
		return;
	}
	if(numBts < 8){
		memswap_direct_byte(arrA, arrB, numBts);
		return;
	}
	uintptr_t charAAddr = (uintptr_t)arrA;
	uintptr_t charBAddr = (uintptr_t)arrB;
	uintptr_t charAMa = charAAddr & 0x07;
	uintptr_t charBMa = charBAddr & 0x07;
	char* workA = arrA;
	char* workB = arrB;
	size_t leftBts = numBts;
	if(charAMa == charBMa){
		//try to align
		if(charAMa){
			charAMa = 8 - charAMa;
			memswap_direct_byte(workA, workB, charAMa);
			workA += charAMa;
			workB += charAMa;
			leftBts -= charAMa;
		}
	}
	//move the words
	size_t numWords = leftBts >> 3;
	if(numWords){
		memswap_direct_word(workA, workB, numWords);
		size_t tmpBytes = numWords << 3;
		workA += tmpBytes;
		workB += tmpBytes;
		leftBts -= tmpBytes;
	}
	//move the leftovers
	if(leftBts){
		memswap_direct_byte(workA, workB, leftBts);
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
	asm volatile (
		"bswapq %%rcx\n"
		"movq %%rcx, (%%rdx)\n"
	:
	: "c" (toPrep), "d" (toBuffer)
	: "cc", "memory"
	);
}
void whodun::packBE32(uint32_t toPrep, char* toBuffer){
	asm volatile (
		"bswapl %%ecx\n"
		"movl %%ecx, (%%rdx)\n"
	:
	: "c" (toPrep), "d" (toBuffer)
	: "cc", "memory"
	);
}
void whodun::packBE16(uint16_t toPrep, char* toBuffer){
	asm volatile (
		"bswapl %%ecx\n"
		"shrl $16,%%ecx\n"
		"movw %%cx, (%%rdx)\n"
	:
	: "c" (toPrep), "d" (toBuffer)
	: "cc", "memory"
	);
}

void whodun::packLE64(uint64_t toPrep, char* toBuffer){
	*((uint64_t*)toBuffer) = toPrep;
}
void whodun::packLE32(uint32_t toPrep, char* toBuffer){
	*((uint32_t*)toBuffer) = toPrep;
}
void whodun::packLE16(uint16_t toPrep, char* toBuffer){
	*((uint16_t*)toBuffer) = toPrep;
}

uint64_t whodun::unpackBE64(const char* toBuffer){
	uint64_t toRet = 0;
	asm volatile (
		"movq (%%rcx), %%rax\n"
		"bswapq %%rax\n"
	: "=a" (toRet)
	: "c" (toBuffer)
	: "cc"
	);
	return toRet;
}
uint32_t whodun::unpackBE32(const char* toBuffer){
	uint32_t toRet = 0;
	asm volatile (
		"movl (%%rcx), %%eax\n"
		"bswapl %%eax\n"
	: "=a" (toRet)
	: "c" (toBuffer)
	: "cc"
	);
	return toRet;
}
uint16_t whodun::unpackBE16(const char* toBuffer){
	uint16_t toRet = 0;
	asm volatile (
		"movw (%%rcx), %%ax\n"
		"bswapl %%eax\n"
		"shrl $16,%%eax\n"
	: "=a" (toRet)
	: "c" (toBuffer)
	: "cc"
	);
	return toRet;
}

uint64_t whodun::unpackLE64(const char* toBuffer){
	return *((uint64_t*)toBuffer);
}
uint32_t whodun::unpackLE32(const char* toBuffer){
	return *((uint32_t*)toBuffer);
}
uint16_t whodun::unpackLE16(const char* toBuffer){
	return *((uint16_t*)toBuffer);
}

uint64_t whodun::packDbl(double toPack){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	return tmpSave.saveI;
}
uint32_t whodun::packFlt(float toPack){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveF = toPack;
	return tmpSave.saveI;
}

double whodun::unpackDbl(uint64_t toPack){
	union {
		uint64_t saveI;
		double saveF;
	} tmpSave;
	tmpSave.saveI = toPack;
	return tmpSave.saveF;
}
float whodun::unpackFlt(uint32_t toPack){
	union {
		uint32_t saveI;
		float saveF;
	} tmpSave;
	tmpSave.saveI = toPack;
	return tmpSave.saveF;
}

