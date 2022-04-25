#include "whodun_suffix.h"

using namespace whodun;

bool whodun::internalSuffixRankCompare(void* unif, void* itemA, void* itemB){
	bool toRet;
	//inline assembly
	asm volatile (
		"xorq %%rax, %%rax\n"
		//rank
		"movq 24(%%rcx), %%r8\n"
		"movq 24(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		//next-rank
		"movq 32(%%rcx), %%r8\n"
		"movq 32(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		//string
		"movq (%%rcx), %%r8\n"
		"movq (%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"jne endCSOTest%=\n"
		//character
		"movq 8(%%rcx), %%r8\n"
		"movq 8(%%rdx), %%r9\n"
		"bswapq %%r8\n"
		"bswapq %%r9\n"
		"subq %%r8, %%r9\n"
		"setg %%al\n"
		"endCSOTest%=:\n"
	: "=a" (toRet)
	: "c" (itemA), "d" (itemB)
	: "r8", "r9", "cc"
	);
	return toRet;
}
