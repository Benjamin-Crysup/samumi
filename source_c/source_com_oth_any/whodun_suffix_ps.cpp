#include "whodun_suffix.h"

using namespace whodun;

bool whodun::internalSuffixRankCompare(void* unif, void* itemA, void* itemB){
	//compare ranks first
	uintmax_t rankA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_RANK);
	uintmax_t rankB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_RANK);
	if(rankA < rankB){
		return true;
	}
	else if(rankA > rankB){
		return false;
	}
	//next ranks next
	uintmax_t nextA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
	uintmax_t nextB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_NEXT);
	if(nextA < nextB){
		return true;
	}
	else if(nextA > nextB){
		return false;
	}
	//otherwise, impose a total ordering (string index)
	uintmax_t sindA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	uintmax_t sindB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_STRING);
	if(sindA < sindB){
		return true;
	}
	else if(sindA > sindB){
		return false;
	}
	//as a fallback, character index: should never
	uintmax_t charA = unpackBE64(((char*)itemA) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	uintmax_t charB = unpackBE64(((char*)itemB) + WHODUN_SUFFIX_ENTRY_OFF_CHAR);
	return charA < charB;
}
