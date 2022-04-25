#include "samumi.h"

#include <algorithm>

/**Actually run the fuzzy find.*/
class FuzzfindUniform : public whodun::ThreadTask{
public:
	/**Set up.*/
	FuzzfindUniform();
	/**Tear down*/
	~FuzzfindUniform();
	void doIt();
	/**
	 * Do pairs in a bam file.
	 * @param read1 The first read.
	 * @param read2 The second read.
	 */
	void doPairB(whodun::CRBSAMFileContents* read1, whodun::CRBSAMFileContents* read2);
	/**
	 * Do singles in a bam file.
	 * @param read1 The first read.
	 */
	void doSingB(whodun::CRBSAMFileContents* read1);
	/**
	 * Do pairs in a fastq file.
	 * @param read1 The first read.
	 * @param read2 The second read.
	 */
	void doPairF(whodun::SequenceEntry* read1, whodun::SequenceEntry* read2);
	/**
	 * Do singles in a fastq file.
	 * @param read1 The first read.
	 */
	void doSingF(whodun::SequenceEntry* read1);
	/**
	 * Do a pair of entries.
	 * @param readName The name of the sequence.
	 * @param read1Seq The sequence of read 1.
	 * @param read1Qual The phred scores of read 1.
	 * @param read2Seq The sequence of read 2.
	 * @param read2Qual The phred scores of read 2.
	 */
	void doPair(whodun::SizePtrString readName, whodun::SizePtrString read1Seq, whodun::SizePtrString read1Qual, whodun::SizePtrString read2Seq, whodun::SizePtrString read2Qual);
	/**
	 * Do a single entry.
	 * @param readName The name of the sequence.
	 * @param read1Seq The sequence of read 1.
	 * @param read1Qual The phred scores of read 1.
	 */
	void doSing(whodun::SizePtrString readName, whodun::SizePtrString read1Seq, whodun::SizePtrString read1Qual);
	/**Where to start work for bam files.*/
	whodun::CRBSAMFileContents** mangleBamS;
	/**Where to end work for bam files.*/
	whodun::CRBSAMFileContents** mangleBamE;
	/**Where to start work for reverse sequences in bam files: null for single sequences.*/
	whodun::CRBSAMFileContents** mangleBamRS;
	/**Where to start work for bam files.*/
	whodun::SequenceEntry** mangleFastS;
	/**Where to end work for bam files.*/
	whodun::SequenceEntry** mangleFastE;
	/**Where to start work for reverse sequences in bam files: null for single sequences.*/
	whodun::SequenceEntry** mangleFastRS;
	/**The argument storage.*/
	FuzzFinderArgumentParser* argCom;
	/**The primers.*/
	PrimerSet* allPrime;
	/**The results.*/
	std::vector<whodun::TextTableRow*> allOut;
	/**Cache for results.*/
	std::vector<whodun::TextTableRow*> cacheOut;
	/**A place to build results.*/
	EntryReadData resBuild;
	/**Do the reverse complement.*/
	whodun::GeneticSequenceUtilities doRC;
	/**The place to pack the results.*/
	whodun::TextTableRow** endFill;
	/**Temporary storage for quality.*/
	std::vector<char> tmpQuality;
};
FuzzfindUniform::FuzzfindUniform(){}
FuzzfindUniform::~FuzzfindUniform(){
	cacheOut.insert(cacheOut.end(), allOut.begin(), allOut.end());
	allOut.clear();
	for(uintptr_t i = 0; i<cacheOut.size(); i++){ delete(cacheOut[i]); }
}
void FuzzfindUniform::doIt(){
	whodun::CRBSAMFileContents** curMB = mangleBamS;
	whodun::CRBSAMFileContents** curMRB = mangleBamRS;
	while(curMB != mangleBamE){
		if(*curMRB){
			doPairB(*curMB, *curMRB);
		}
		else{
			doSingB(*curMB);
		}
		curMB++;
		curMRB++;
	}
	whodun::SequenceEntry** curMF = mangleFastS;
	whodun::SequenceEntry** curMRF = mangleFastRS;
	while(curMF != mangleFastE){
		if(*curMRF){
			doPairF(*curMF, *curMRF);
		}
		else{
			doSingF(*curMF);
		}
		curMF++;
		curMRF++;
	}
	if(endFill){
		if(allOut.size()){
			memcpy(endFill, &(allOut[0]), allOut.size()*sizeof(whodun::TextTableRow*));
		}
		cacheOut.insert(cacheOut.end(), allOut.begin(), allOut.end());
		allOut.clear();
	}
}
void FuzzfindUniform::doPairB(whodun::CRBSAMFileContents* read1, whodun::CRBSAMFileContents* read2){
	//WLOG read1 is R1
	if((read1->flag & WHODUN_SAM_FLAG_LAST) && (read2->flag & WHODUN_SAM_FLAG_FIRST)){
		doPairB(read2, read1);
		return;
	}
	//if weird pair, skip
	if(!(read1->flag & WHODUN_SAM_FLAG_FIRST) || !(read2->flag & WHODUN_SAM_FLAG_LAST)){
		return;
	}
	//undo any reverse complements
	if(read1->flag & WHODUN_SAM_FLAG_SEQREVCOMP){
		doRC.reverseComplement(read1->seq.len, read1->seq.txt, 0);
		std::reverse(read1->qual.txt, read1->qual.txt + read1->qual.len);
	}
	if(read2->flag & WHODUN_SAM_FLAG_SEQREVCOMP){
		doRC.reverseComplement(read2->seq.len, read2->seq.txt, 0);
		std::reverse(read2->qual.txt, read2->qual.txt + read2->qual.len);
	}
	doPair(read1->name, read1->seq, read1->qual, read2->seq, read2->qual);
}
void FuzzfindUniform::doSingB(whodun::CRBSAMFileContents* read1){
	doSing(read1->name, read1->seq, read1->qual);
	doRC.reverseComplement(read1->seq.len, read1->seq.txt, 0);
	std::reverse(read1->qual.txt, read1->qual.txt + read1->qual.len);
	doSing(read1->name, read1->seq, read1->qual);
}
void FuzzfindUniform::doPairF(whodun::SequenceEntry* read1, whodun::SequenceEntry* read2){
	tmpQuality.resize(read1->seqLen + read2->seqLen + 1);
	if(read1->haveQual){
		doRC.phredsLog10ToScore(read1->seqLen, read1->qual, &(tmpQuality[0]));
	}
	if(read2->haveQual){
		doRC.phredsLog10ToScore(read2->seqLen, read2->qual, &(tmpQuality[read1->seqLen]));
	}
	whodun::SizePtrString read1Name = {read1->nameLen, read1->name};
	whodun::SizePtrString read1Seq = {read1->seqLen, read1->seq};
	whodun::SizePtrString read1Qual = {read1->haveQual ? read1->seqLen : 0, &(tmpQuality[0])};
	whodun::SizePtrString read2Seq = {read2->seqLen, read2->seq};
	whodun::SizePtrString read2Qual = {read2->haveQual ? read2->seqLen : 0, &(tmpQuality[read1->seqLen])};
	doPair(read1Name, read1Seq, read1Qual, read2Seq, read2Qual);
}
void FuzzfindUniform::doSingF(whodun::SequenceEntry* read1){
	tmpQuality.resize(read1->seqLen + 1);
	if(read1->haveQual){
		doRC.phredsLog10ToScore(read1->seqLen, read1->qual, &(tmpQuality[0]));
	}
	whodun::SizePtrString read1Name = {read1->nameLen, read1->name};
	whodun::SizePtrString read1Seq = {read1->seqLen, read1->seq};
	whodun::SizePtrString read1Qual = {read1->haveQual ? read1->seqLen : 0, &(tmpQuality[0])};
	doSing(read1Name, read1Seq, read1Qual);
	doRC.reverseComplement(read1Seq.len, read1Seq.txt, 0);
	std::reverse(read1Qual.txt, read1Qual.txt + read1Qual.len);
	doSing(read1Name, read1Seq, read1Qual);
}
void FuzzfindUniform::doPair(whodun::SizePtrString readName, whodun::SizePtrString read1Seq, whodun::SizePtrString read1Qual, whodun::SizePtrString read2Seq, whodun::SizePtrString read2Qual){
	//look for the common sequence in read 2
	uintptr_t umiLen = argCom->umiLen;
	char* commonS = argCom->commonSeq;
	uintptr_t commonSLen = strlen(commonS);
	char* commonLoc = fuzzyFind(commonS, commonS + commonSLen, read2Seq.txt, read2Seq.txt + read2Seq.len, argCom->commonFuzz);
	if(commonLoc == 0){ return; }
	if((commonLoc - read2Seq.txt) < (intptr_t)umiLen){ return; }
	char* umiLoc = commonLoc - umiLen;
	//run through the primers
	for(uintptr_t i = 0; i<allPrime->allPrimer.size(); i++){
		PrimerData* curPrim = &(allPrime->allPrimer[i]);
		//figure out which to search for
		char* cprimeP; char* cprimePE;
		char* cprimeA; char* cprimeAE;
		if(curPrim->isRC){
			cprimeP = &(curPrim->primerRC[0]);
				cprimePE = cprimeP + curPrim->primerRC.size();
			cprimeA = &(curPrim->anchorRC[0]);
				cprimeAE = cprimeA + curPrim->anchorRC.size();
		}
		else{
			cprimeP = &(curPrim->primer[0]);
				cprimePE = cprimeP + curPrim->primer.size();
			cprimeA = &(curPrim->anchor[0]);
				cprimeAE = cprimeA + curPrim->anchor.size();
		}
		//look for the primer and anchor
		char* primerLoc = fuzzyFind(cprimeP, cprimePE, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->primerFuzz);
			if(!primerLoc){ continue; }
		char* anchorLoc = fuzzyFind(cprimeA, cprimeAE, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->anchorFuzz);
			if(!anchorLoc){ continue; }
			if(anchorLoc < primerLoc){ continue; }
			if((anchorLoc - primerLoc) < (cprimePE - cprimeP)){ continue; }
		//prepare the data
		resBuild.clear();
		resBuild.qname.insert(resBuild.qname.end(), readName.txt, readName.txt + readName.len);
		resBuild.canonPrimer.insert(resBuild.canonPrimer.end(), curPrim->primer.begin(), curPrim->primer.end());
		resBuild.canonLocus.insert(resBuild.canonLocus.end(), curPrim->locus.begin(), curPrim->locus.end());
		resBuild.canonUMI.insert(resBuild.canonUMI.end(), umiLoc, umiLoc + umiLen);
		resBuild.canonSequence.insert(resBuild.canonSequence.end(), primerLoc + (cprimePE - cprimeP), anchorLoc);
		resBuild.sequence.insert(resBuild.sequence.end(), primerLoc + (cprimePE - cprimeP), anchorLoc);
		resBuild.primer.insert(resBuild.primer.end(), primerLoc, primerLoc + (cprimePE - cprimeP));
		resBuild.anchor.insert(resBuild.anchor.end(), anchorLoc, anchorLoc + (cprimeAE - cprimeA));
		resBuild.umi.insert(resBuild.umi.end(), umiLoc, umiLoc + umiLen);
		resBuild.common.insert(resBuild.common.end(), commonLoc, commonLoc + commonSLen);
		if(read1Qual.len){
			resBuild.sequencePhred.insert(resBuild.sequencePhred.end(), read1Qual.txt + (cprimePE - cprimeP) + (primerLoc - read1Seq.txt), read1Qual.txt + (anchorLoc - read1Seq.txt));
			resBuild.primerPhred.insert(resBuild.primerPhred.end(), read1Qual.txt + (primerLoc - read1Seq.txt), read1Qual.txt + (primerLoc - read1Seq.txt) + (cprimePE - cprimeP));
			resBuild.anchorPhred.insert(resBuild.anchorPhred.end(), read1Qual.txt + (anchorLoc - read1Seq.txt), read1Qual.txt + (anchorLoc - read1Seq.txt) + (cprimeAE - cprimeA));
		}
		if(read2Qual.len){
			resBuild.umiPhred.insert(resBuild.umiPhred.end(), read2Qual.txt + (umiLoc - read2Seq.txt), read2Qual.txt + (umiLoc - read2Seq.txt) + umiLen);
			resBuild.commonPhred.insert(resBuild.commonPhred.end(), read2Qual.txt + (commonLoc - read2Seq.txt), read2Qual.txt + (commonLoc - read2Seq.txt) + commonSLen);
		}
		//reverse complement if need be
		if(curPrim->isRC){
			if(resBuild.sequence.size()){
				doRC.reverseComplement(resBuild.canonSequence.size(), &(resBuild.canonSequence[0]), 0);
				doRC.reverseComplement(resBuild.sequence.size(), &(resBuild.sequence[0]), 0);
				std::reverse(resBuild.sequencePhred.begin(), resBuild.sequencePhred.end());
			}
			if(resBuild.primer.size()){
				doRC.reverseComplement(resBuild.primer.size(), &(resBuild.primer[0]), 0);
				std::reverse(resBuild.primerPhred.begin(), resBuild.primerPhred.end());
			}
			if(resBuild.anchor.size()){
				doRC.reverseComplement(resBuild.anchor.size(), &(resBuild.anchor[0]), 0);
				std::reverse(resBuild.anchorPhred.begin(), resBuild.anchorPhred.end());
			}
		}
		//pack it up
		whodun::TextTableRow* nextOut;
		if(cacheOut.size()){
			nextOut = cacheOut[cacheOut.size()-1];
			cacheOut.pop_back();
		}
		else{
			nextOut = new whodun::TextTableRow();
		}
		resBuild.dump(nextOut);
		allOut.push_back(nextOut);
	}
}
void FuzzfindUniform::doSing(whodun::SizePtrString readName, whodun::SizePtrString read1Seq, whodun::SizePtrString read1Qual){
	//look for the common sequence (in both directions)
	uintptr_t umiLen = argCom->umiLen;
	uintptr_t commonSLen = strlen(argCom->commonSeq);
	char* commonLocF = fuzzyFind(argCom->commonSeq, argCom->commonSeq + commonSLen, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->commonFuzz);
	char* commonLocR = fuzzyFind(&(argCom->commonSeqR[0]), &(argCom->commonSeqR[0]) + commonSLen, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->commonFuzz);
	//run through the primers
	for(uintptr_t i = 0; i<allPrime->allPrimer.size(); i++){
		PrimerData* curPrim = &(allPrime->allPrimer[i]);
		//look for the sequence on the forward strand (period)
		char* cprimeP = &(curPrim->primer[0]);
			char* cprimePE = cprimeP + curPrim->primer.size();
			char* primerLoc = fuzzyFind(cprimeP, cprimePE, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->primerFuzz);
			if(!primerLoc){ continue; }
		char* cprimeA = &(curPrim->anchor[0]);
			char* cprimeAE = cprimeA + curPrim->anchor.size();
			char* anchorLoc = fuzzyFind(cprimeA, cprimeAE, read1Seq.txt, read1Seq.txt + read1Seq.len, argCom->anchorFuzz);
			if(!anchorLoc){ continue; }
		//how to interpret depends on isRC
		if(curPrim->isRC){
			//commonF anchor primer
			if(!commonLocF){ continue; }
			if((commonLocF - read1Seq.txt) < (intptr_t)umiLen){ return; }
			if((anchorLoc - commonLocF) < (intptr_t)commonSLen){ continue; }
			if((primerLoc - anchorLoc) < (cprimeAE - cprimeA)){ continue; }
			//umi and common sequence are good as is, read 1 stuff is backwards (but should remain so)
			resBuild.clear();
			resBuild.qname.insert(resBuild.qname.end(), readName.txt, readName.txt + readName.len);
			resBuild.canonPrimer.insert(resBuild.canonPrimer.end(), curPrim->primer.begin(), curPrim->primer.end());
			resBuild.canonLocus.insert(resBuild.canonLocus.end(), curPrim->locus.begin(), curPrim->locus.end());
			resBuild.canonUMI.insert(resBuild.canonUMI.end(), commonLocF - umiLen, commonLocF);
			resBuild.canonSequence.insert(resBuild.canonSequence.end(), anchorLoc + (cprimeAE - cprimeA), primerLoc);
			resBuild.sequence.insert(resBuild.sequence.end(), anchorLoc + (cprimeAE - cprimeA), primerLoc);
			resBuild.umi.insert(resBuild.umi.end(), commonLocF - umiLen, commonLocF);
			resBuild.primer.insert(resBuild.primer.end(), primerLoc, primerLoc + (cprimePE - cprimeP));
			resBuild.anchor.insert(resBuild.anchor.end(), anchorLoc, anchorLoc + (cprimeAE - cprimeA));
			resBuild.common.insert(resBuild.common.end(), commonLocF, commonLocF + commonSLen);
			if(read1Qual.len){
				resBuild.umiPhred.insert(resBuild.umiPhred.end(), read1Qual.txt + ((commonLocF - umiLen) - read1Seq.txt), read1Qual.txt + (commonLocF - read1Seq.txt));
				resBuild.primerPhred.insert(resBuild.primerPhred.end(), read1Qual.txt + (primerLoc - read1Seq.txt), read1Qual.txt + (primerLoc - read1Seq.txt) + (cprimePE - cprimeP));
				resBuild.anchorPhred.insert(resBuild.anchorPhred.end(), read1Qual.txt + (anchorLoc - read1Seq.txt), read1Qual.txt + (anchorLoc - read1Seq.txt) + (cprimeAE - cprimeA));
				resBuild.sequencePhred.insert(resBuild.sequencePhred.end(), read1Qual.txt + ((anchorLoc + (cprimeAE - cprimeA)) - read1Seq.txt), read1Qual.txt + (primerLoc - read1Seq.txt));
				resBuild.commonPhred.insert(resBuild.commonPhred.end(), read1Qual.txt + (commonLocF - read1Seq.txt), read1Qual.txt + commonSLen + (commonLocF - read1Seq.txt));
			}
		}
		else{
			//primer anchor commonR
			if(!commonLocR){ continue; }
			if((anchorLoc - primerLoc) < (cprimePE - cprimeP)){ continue; }
			if((commonLocR - anchorLoc) < (cprimeAE - cprimeA)){ continue; }
			if(((read1Seq.txt + read1Seq.len) - commonLocR) < (intptr_t)(commonSLen + umiLen)){ continue; }
			//read 2 stuff needs reverse complement, read 1 good
			resBuild.clear();
			resBuild.qname.insert(resBuild.qname.end(), readName.txt, readName.txt + readName.len);
			resBuild.canonPrimer.insert(resBuild.canonPrimer.end(), curPrim->primer.begin(), curPrim->primer.end());
			resBuild.canonLocus.insert(resBuild.canonLocus.end(), curPrim->locus.begin(), curPrim->locus.end());
			resBuild.canonUMI.insert(resBuild.canonUMI.end(), commonLocR + commonSLen, commonLocR + commonSLen + umiLen);
			resBuild.canonSequence.insert(resBuild.canonSequence.end(), primerLoc + (cprimePE - cprimeP), anchorLoc);
			resBuild.sequence.insert(resBuild.sequence.end(), primerLoc + (cprimePE - cprimeP), anchorLoc);
			resBuild.umi.insert(resBuild.umi.end(), commonLocR + commonSLen, commonLocR + commonSLen + umiLen);
			resBuild.primer.insert(resBuild.primer.end(), primerLoc, primerLoc + (cprimePE - cprimeP));
			resBuild.anchor.insert(resBuild.anchor.end(), anchorLoc, anchorLoc + (cprimeAE - cprimeA));
			resBuild.common.insert(resBuild.common.end(), commonLocR, commonLocR + commonSLen);
			if(read1Qual.len){
				resBuild.umiPhred.insert(resBuild.umiPhred.end(), read1Qual.txt + ((commonLocR + commonSLen) - read1Seq.txt), read1Qual.txt + ((commonLocR + commonSLen + umiLen) - read1Seq.txt));
				resBuild.primerPhred.insert(resBuild.primerPhred.end(), read1Qual.txt + (primerLoc - read1Seq.txt), read1Qual.txt + (primerLoc - read1Seq.txt) + (cprimePE - cprimeP));
				resBuild.anchorPhred.insert(resBuild.anchorPhred.end(), read1Qual.txt + (anchorLoc - read1Seq.txt), read1Qual.txt + (anchorLoc - read1Seq.txt) + (cprimeAE - cprimeA));
				resBuild.sequencePhred.insert(resBuild.sequencePhred.end(), read1Qual.txt + ((primerLoc + (cprimePE - cprimeP)) - read1Seq.txt), read1Qual.txt + (anchorLoc - read1Seq.txt));
				resBuild.commonPhred.insert(resBuild.commonPhred.end(), read1Qual.txt + (commonLocR - read1Seq.txt), read1Qual.txt + ((commonLocR + commonSLen) - read1Seq.txt));
			}
			//reverse complement
			if(resBuild.umi.size()){
				doRC.reverseComplement(resBuild.umi.size(), &(resBuild.umi[0]), 0);
				std::reverse(resBuild.umiPhred.begin(), resBuild.umiPhred.end());
			}
			if(resBuild.common.size()){
				doRC.reverseComplement(resBuild.common.size(), &(resBuild.common[0]), 0);
				std::reverse(resBuild.commonPhred.begin(), resBuild.commonPhred.end());
			}
		}
		//pack it up
		whodun::TextTableRow* nextOut;
		if(cacheOut.size()){
			nextOut = cacheOut[cacheOut.size()-1];
			cacheOut.pop_back();
		}
		else{
			nextOut = new whodun::TextTableRow();
		}
		resBuild.dump(nextOut);
		allOut.push_back(nextOut);
	}
}

FuzzFinderArgumentParser::FuzzFinderArgumentParser(){
	outFileN = 0;
	inFileN = 0;
	inForwN = 0;
	inBackN = 0;
	primerFileN = 0;
	commonSeq = 0;
	umiLen = 12;
	primerFuzz = 1;
	anchorFuzz = 1;
	commonFuzz = 1;
	numThread = 1;
	threadGrain = 1024;
	fastqInter = false;
	defOutFN[0] = '-'; defOutFN[1] = 0;
	myMainDoc = "Usage: samumi fuzzfind [OPTION] [FILE]\n"
		"Find relevant information from reads in a UMI sequence run.\n"
		"The OPTIONS are:\n";
	myVersionDoc = "SammedUmi fuzzfind 1.0";
	myCopyrightDoc = "Copyright (C) 2021 UNT HSC Center for Human Identification";
	mySummary = "Look for UMI backed bracketed haplotypes.";
	whodun::ArgumentParserStrMeta outMeta("Output File");
		outMeta.isFile = true;
		outMeta.fileWrite = true;
		whodun::getTextTableExtensions(1, &(outMeta.fileExts));
		addStringOption("--out", &outFileN, 0, "    Specify the output file.\n    --out File.tsv\n", &outMeta);
	whodun::ArgumentParserStrMeta inMeta("Input File");
		inMeta.isFile = true;
		whodun::getCRBSamExtensions(0, &(inMeta.fileExts));
		addStringOption("--in", &inFileN, 0, "    Specify the input file.\n    --in File.sam\n", &inMeta);
	whodun::ArgumentParserStrMeta inFMeta("Forward Strand Input");
		inFMeta.isFile = true;
		whodun::getGeneticSequenceExtensions(0, &(inFMeta.fileExts));
		addStringOption("--forw", &inForwN, 0, "    Specify the file with the forward reads.\n    --forw File.fq\n", &inFMeta);
	whodun::ArgumentParserStrMeta inBMeta("Reverse Strand Input");
		inBMeta.isFile = true;
		whodun::getGeneticSequenceExtensions(0, &(inBMeta.fileExts));
		addStringOption("--back", &inBackN, 0, "    Specify the file with the reverse reads.\n    --back File.fq\n", &inBMeta);
	whodun::ArgumentParserStrMeta primerMeta("Primer File");
		primerMeta.isFile = true;
		whodun::getTextTableExtensions(0, &(primerMeta.fileExts));
		addStringOption("--primer", &primerFileN, 0, "    Specify the primer file.\n    --primer File.tsv\n", &primerMeta);
	whodun::ArgumentParserStrMeta comMeta("Common Sequence");
		addStringOption("--common", &commonSeq, 0, "    Specify the common sequence on read 2.\n    --common ATTGGAGTCCT\n", &comMeta);
	whodun::ArgumentParserIntMeta umilMeta("UMI Length");
		addIntegerOption("--umi", &umiLen, 0, "    Specify the length of the UMIs.\n    --umi 12\n", &umilMeta);
	whodun::ArgumentParserIntMeta cfuzzMeta("Common Fuzz");
		addIntegerOption("--cfuzz", &commonFuzz, 0, "    Specify the number of mismatches in the common sequence.\n    --cfuzz 1\n", &cfuzzMeta);
	whodun::ArgumentParserIntMeta pfuzzMeta("Primer Fuzz");
		addIntegerOption("--pfuzz", &primerFuzz, 0, "    Specify the number of mismatches in the primer sequence.\n    --pfuzz 1\n", &pfuzzMeta);
	whodun::ArgumentParserIntMeta afuzzMeta("Anchor Fuzz");
		addIntegerOption("--afuzz", &anchorFuzz, 0, "    Specify the number of mismatches in the anchor sequence.\n    --afuzz 1\n", &afuzzMeta);
	whodun::ArgumentParserIntMeta threadMeta("Threads");
		addIntegerOption("--thread", &numThread, 0, "    How many threads to use.\n    --thread 1\n", &threadMeta);
	whodun::ArgumentParserBoolMeta interMeta("FastQ Interleaved");
		addBooleanFlag("--interleave", &fastqInter, 1, "    Whether the forward fastq also has the reverse reads interleaved.\n", &interMeta);
}
FuzzFinderArgumentParser::~FuzzFinderArgumentParser(){}
int FuzzFinderArgumentParser::posteriorCheck(){
	if(numThread <= 0){
		argumentError = "Need at least one thread.";
		return 1;
	}
	if(primerFuzz < 0){
		argumentError = "Mismatch allowance must be non-negative.";
		return 1;
	}
	if(anchorFuzz < 0){
		argumentError = "Mismatch allowance must be non-negative.";
		return 1;
	}
	if(commonFuzz < 0){
		argumentError = "Mismatch allowance must be non-negative.";
		return 1;
	}
	if(umiLen < 0){
		argumentError = "Cannot have negative length UMIs.";
		return 1;
	}
	if(!primerFileN || (strlen(primerFileN)==0)){
		argumentError = "Need to specify the primer data.";
		return 1;
	}
	if(!commonSeq || (strlen(commonSeq)==0)){
		argumentError = "Need to specify the common sequence.";
		return 1;
	}
	if(!outFileN || (strlen(outFileN)==0)){
		outFileN = defOutFN;
	}
	if(inForwN){
		if(inFileN){
			argumentError = "Cannot both have a sam and fastq file for input.";
			return 1;
		}
		if(fastqInter && inBackN){
			argumentError = "Reverse reads cannot be both interleaved and in a second file.";
			return 1;
		}
	}
	else if(!inFileN || (strlen(inFileN)==0)){
		inFileN = defOutFN;
	}
	whodun::GeneticSequenceUtilities doRC;
	commonSeqR.insert(commonSeqR.end(), commonSeq, commonSeq + strlen(commonSeq));
	doRC.reverseComplement(commonSeqR.size(), &(commonSeqR[0]), 0);
	return 0;
}
void FuzzFinderArgumentParser::runThing(){
	whodun::ThreadPool useThread(numThread);
	PrimerSet allPrime(primerFileN);
	whodun::CRBSAMFileReader* inSam = 0;
	whodun::SequenceReader* inFA = 0;
	whodun::SequenceReader* inFB = 0;
	whodun::TextTableWriter* outF = 0;
	std::vector<FuzzfindUniform> allUni(numThread);
	std::vector<whodun::ThreadTask*> passUni;
	for(uintptr_t i = 0; i<allUni.size(); i++){
		passUni.push_back(&(allUni[i]));
		allUni[i].mangleBamS = 0;
		allUni[i].mangleBamE = 0;
		allUni[i].mangleBamRS = 0;
		allUni[i].mangleFastS = 0;
		allUni[i].mangleFastE = 0;
		allUni[i].mangleFastRS = 0;
		allUni[i].argCom = this;
		allUni[i].allPrime = &allPrime;
	}
	std::vector<whodun::TextTableRow*> endDump;
	int isMore = 1;
	try{
		outF = new whodun::ExtensionTextTableWriter(outFileN, numThread, &useThread);
		if(inFileN){
			inSam = new whodun::ExtensionCRBSAMReader(inFileN, numThread, &useThread);
			std::vector<whodun::CRBSAMFileContents*> bamF(numThread*threadGrain);
			std::vector<whodun::CRBSAMFileContents*> bamR(numThread*threadGrain);
			uintptr_t bamIndex = 0;
			whodun::CRBSAMPairedEndCache bamPair;
			while(isMore){
				//load
				uintptr_t numLoad = 0;
				while(numLoad < bamF.size()){
					whodun::CRBSAMFileContents* curD = inSam->read();
					if(!curD){
						isMore = 0;
						break;
					}
					if(curD->lastReadHead || !bamPair.entryIsPrimary(curD)){
						inSam->release(curD);
						continue;
					}
					if(bamPair.entryNeedPair(curD)){
						if(bamPair.havePair(curD)){
							whodun::CRBSAMFileContents* pairD = bamPair.getPair(curD).second;
							bamF[numLoad] = curD;
							bamR[numLoad] = pairD;
							numLoad++;
						}
						else{
							bamPair.waitForPair(bamIndex, curD);
							bamIndex++;
						}
					}
					else{
						bamF[numLoad] = curD;
						bamR[numLoad] = 0;
						numLoad++;
						bamIndex++;
					}
				}
				//run finds
				whodun::CRBSAMFileContents** bigS = &(bamF[0]);
				whodun::CRBSAMFileContents** bigRS = &(bamR[0]);
				uintptr_t numPT = numLoad / numThread;
				uintptr_t numET = numLoad % numThread;
				uintptr_t curOff = 0;
				for(uintptr_t i = 0; i<allUni.size(); i++){
					uintptr_t curNum = numPT + (i < numET);
					FuzzfindUniform* curU = &(allUni[i]);
					curU->endFill = 0;
					curU->mangleBamS = bigS + curOff;
					curU->mangleBamRS = bigRS + curOff;
					curOff += curNum;
					curU->mangleBamE = bigS + curOff;
				}
				uintptr_t thrId = useThread.addTasks(numThread, &(passUni[0]));
				useThread.joinTasks(thrId, thrId + numThread);
				//pack and output finds
				#define PACK_AND_OUTPUT_FINDS(toNotDo) \
					uintptr_t totalOut = 1;\
					for(uintptr_t i = 0; i<allUni.size(); i++){\
						totalOut += allUni[i].allOut.size();\
					}\
					if(totalOut > endDump.size()){ endDump.resize(totalOut); }\
					totalOut = 0;\
					whodun::TextTableRow** tabOutS = &(endDump[0]);\
					for(uintptr_t i = 0; i<allUni.size(); i++){\
						FuzzfindUniform* curU = &(allUni[i]);\
						toNotDo \
						curU->endFill = tabOutS + totalOut;\
						totalOut += curU->allOut.size();\
					}\
					thrId = useThread.addTasks(numThread, &(passUni[0]));\
					useThread.joinTasks(thrId, thrId + numThread);\
					outF->write(totalOut, tabOutS);
				PACK_AND_OUTPUT_FINDS(curU->mangleBamS = 0; curU->mangleBamE = 0;)
				//reclaim
				for(uintptr_t i = 0; i<numLoad; i++){
					inSam->release(bamF[i]);
					if(bamR[i]){ inSam->release(bamR[i]); }
				}
			}
			//complain about stuff off the end
			std::vector<uintptr_t> outstandID;
			std::vector<whodun::CRBSAMFileContents*> outstandEnt;
			bamPair.getOutstanding(&outstandID, &outstandEnt);
			for(uintptr_t i = 0; i<outstandEnt.size(); i++){
				std::cerr << outstandEnt[i]->name << "\tPair Missing\tQuery marked as paired, but no pair present." << std::endl;
				inSam->release(outstandEnt[i]);
			}
		}
		else if(inBackN){
			inFA = new whodun::ExtensionSequenceReader(inForwN, numThread, &useThread);
			inFB = new whodun::ExtensionSequenceReader(inBackN, numThread, &useThread);
			std::vector<whodun::SequenceEntry*> fastF(numThread*threadGrain);
			std::vector<whodun::SequenceEntry*> fastR(numThread*threadGrain);
			while(isMore){
				//load
				uintptr_t numLoad = 0;
				while(numLoad < fastF.size()){
					whodun::SequenceEntry* curF = inFA->read();
					whodun::SequenceEntry* curR = inFB->read();
					if((curF == 0) != (curR == 0)){
						if(curF){ inFA->release(curF); }
						if(curR){ inFB->release(curR); }
						throw std::runtime_error("Supposedly paired files have different lengths.");
					}
					if(!curF){
						isMore = 0;
						break;
					}
					fastF[numLoad] = curF;
					fastR[numLoad] = curR;
					numLoad++;
				}
				//run the finds
				whodun::SequenceEntry** bigS = &(fastF[0]);
				whodun::SequenceEntry** bigRS = &(fastR[0]);
				uintptr_t numPT = numLoad / numThread;
				uintptr_t numET = numLoad % numThread;
				uintptr_t curOff = 0;
				for(uintptr_t i = 0; i<allUni.size(); i++){
					uintptr_t curNum = numPT + (i < numET);
					FuzzfindUniform* curU = &(allUni[i]);
					curU->endFill = 0;
					curU->mangleFastS = bigS + curOff;
					curU->mangleFastRS = bigRS + curOff;
					curOff += curNum;
					curU->mangleFastE = bigS + curOff;
				}
				uintptr_t thrId = useThread.addTasks(numThread, &(passUni[0]));
				useThread.joinTasks(thrId, thrId + numThread);
				//pack and output
				PACK_AND_OUTPUT_FINDS(curU->mangleFastS = 0; curU->mangleFastE = 0;)
				//reclaim
				for(uintptr_t i = 0; i<numLoad; i++){
					inFA->release(fastF[i]);
					inFB->release(fastR[i]);
				}
			}
		}
		else if(fastqInter){
			inFA = new whodun::ExtensionSequenceReader(inForwN, numThread, &useThread);
			std::vector<whodun::SequenceEntry*> fastF(numThread*threadGrain);
			std::vector<whodun::SequenceEntry*> fastR(numThread*threadGrain);
			while(isMore){
				//load
				uintptr_t numLoad = 0;
				while(numLoad < fastF.size()){
					whodun::SequenceEntry* curF = inFA->read();
					if(!curF){
						isMore = 0;
						break;
					}
					whodun::SequenceEntry* curR = inFA->read();
					if(!curR){
						inFA->release(curF);
						throw std::runtime_error("Supposedly interleaved file has odd length.");
					}
					fastF[numLoad] = curF;
					fastR[numLoad] = curR;
					numLoad++;
				}
				//run the finds
				whodun::SequenceEntry** bigS = &(fastF[0]);
				whodun::SequenceEntry** bigRS = &(fastR[0]);
				uintptr_t numPT = numLoad / numThread;
				uintptr_t numET = numLoad % numThread;
				uintptr_t curOff = 0;
				for(uintptr_t i = 0; i<allUni.size(); i++){
					uintptr_t curNum = numPT + (i < numET);
					FuzzfindUniform* curU = &(allUni[i]);
					curU->endFill = 0;
					curU->mangleFastS = bigS + curOff;
					curU->mangleFastRS = bigRS + curOff;
					curOff += curNum;
					curU->mangleFastE = bigS + curOff;
				}
				uintptr_t thrId = useThread.addTasks(numThread, &(passUni[0]));
				useThread.joinTasks(thrId, thrId + numThread);
				//pack and output
				PACK_AND_OUTPUT_FINDS(curU->mangleFastS = 0; curU->mangleFastE = 0;)
				//reclaim
				for(uintptr_t i = 0; i<numLoad; i++){
					inFA->release(fastF[i]);
					inFA->release(fastR[i]);
				}
			}
		}
		else{
			inFA = new whodun::ExtensionSequenceReader(inForwN, numThread, &useThread);
			std::vector<whodun::SequenceEntry*> fastF(numThread*threadGrain);
			std::vector<whodun::SequenceEntry*> fastR(numThread*threadGrain);
			while(isMore){
				//load
				uintptr_t numLoad = 0;
				while(numLoad < fastF.size()){
					whodun::SequenceEntry* curF = inFA->read();
					if(!curF){
						isMore = 0;
						break;
					}
					fastF[numLoad] = curF;
					fastR[numLoad] = 0;
					numLoad++;
				}
				//run the finds
				whodun::SequenceEntry** bigS = &(fastF[0]);
				whodun::SequenceEntry** bigRS = &(fastR[0]);
				uintptr_t numPT = numLoad / numThread;
				uintptr_t numET = numLoad % numThread;
				uintptr_t curOff = 0;
				for(uintptr_t i = 0; i<allUni.size(); i++){
					uintptr_t curNum = numPT + (i < numET);
					FuzzfindUniform* curU = &(allUni[i]);
					curU->endFill = 0;
					curU->mangleFastS = bigS + curOff;
					curU->mangleFastRS = bigRS + curOff;
					curOff += curNum;
					curU->mangleFastE = bigS + curOff;
				}
				uintptr_t thrId = useThread.addTasks(numThread, &(passUni[0]));
				useThread.joinTasks(thrId, thrId + numThread);
				//pack and output
				PACK_AND_OUTPUT_FINDS(curU->mangleFastS = 0; curU->mangleFastE = 0;)
				//reclaim
				for(uintptr_t i = 0; i<numLoad; i++){
					inFA->release(fastF[i]);
				}
			}
		}
	}
	catch(std::exception& errE){
		if(inSam){ inSam->close(); delete(inSam); }
		if(inFA){ inFA->close(); delete(inFA); }
		if(inFB){ inFB->close(); delete(inFB); }
		if(outF){ outF->close(); delete(outF); }
		throw;
	}
	if(inSam){ inSam->close(); delete(inSam); }
	if(inFA){ inFA->close(); delete(inFA); }
	if(inFB){ inFB->close(); delete(inFB); }
	if(outF){ outF->close(); delete(outF); }
}

