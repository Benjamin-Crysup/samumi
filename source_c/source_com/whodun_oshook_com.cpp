#include "whodun_oshook.h"

#include <string.h>
#include <iostream>
#include <exception>
#include <stdexcept>

using namespace whodun;

OutStream::OutStream(){
	isClosed = 0;
}
OutStream::~OutStream(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}
void OutStream::writeBytes(const char* toW, uintptr_t numW){
	for(uintptr_t i = 0; i<numW; i++){ writeByte(toW[i]); }
}
void OutStream::writeZTBytes(const char* toW){
	writeBytes(toW, strlen(toW));
}
void OutStream::flush(){}

#define READ_ALL_BUFFER_SIZE 1024

InStream::InStream(){
	isClosed = 0;
}
InStream::~InStream(){
	if(!isClosed){ std::cerr << "Need to close a stream before destruction." << std::endl; std::terminate(); }
}
uintptr_t InStream::readBytes(char* toR, uintptr_t numR){
	for(uintptr_t i = 0; i<numR; i++){
		int curR = readByte();
		if(curR < 0){
			return i;
		}
		toR[i] = curR;
	}
	return numR;
}
void InStream::forceReadBytes(char* toR, uintptr_t numR){
	uintptr_t numRead = readBytes(toR, numR);
	if(numRead != numR){ throw std::runtime_error("Truncated stream."); }
}
void InStream::readAll(std::vector<char>* toFill){
	char inpBuff[READ_ALL_BUFFER_SIZE];
	uintptr_t numRead = readBytes(inpBuff, READ_ALL_BUFFER_SIZE);
	while(numRead){
		toFill->insert(toFill->end(), inpBuff, inpBuff + READ_ALL_BUFFER_SIZE);
	}
}

ConcatenatedInStream::ConcatenatedInStream(InStream* strA, InStream* strB){
	waitStreams.push_back(strB);
	waitStreams.push_back(strA);
}
ConcatenatedInStream::ConcatenatedInStream(uintptr_t numStr, InStream** theStrs){
	uintptr_t i = numStr;
	while(i){
		i--;
		waitStreams.push_back(theStrs[i]);
	}
}
ConcatenatedInStream::~ConcatenatedInStream(){}
uintptr_t ConcatenatedInStream::readBytes(char* toR, uintptr_t numR){
	uintptr_t totalR = 0;
	char* leftD = toR;
	uintptr_t leftN = numR;
	
	moreToRead:
	InStream* curStr = waitStreams[waitStreams.size()-1];
	uintptr_t curRead = curStr->readBytes(leftD, leftN);
	totalR += curRead;
	leftD += curRead;
	leftN -= curRead;
	if(leftN){
		waitStreams.pop_back();
		if(waitStreams.size()){ goto moreToRead; }
	}
	return totalR;
}
int ConcatenatedInStream::readByte(){
	char toRet;
	uintptr_t numR = readBytes(&toRet, 1);
	if(numR){
		return 0x00FF & toRet;
	}
	else{
		return -1;
	}
}
void ConcatenatedInStream::close(){isClosed = 1;}

ConcatenatedRandaccInStream::ConcatenatedRandaccInStream(RandaccInStream* strA, RandaccInStream* strB){
	curF = 0;
	curI = 0;
	uintmax_t curOff = 0;
	wrapped.push_back(strA);
	lowInds.push_back(curOff);
	curOff += strA->size();
	highInds.push_back(curOff);
	wrapped.push_back(strB);
	lowInds.push_back(curOff);
	curOff += strB->size();
	highInds.push_back(curOff);
}
ConcatenatedRandaccInStream::ConcatenatedRandaccInStream(uintptr_t numStr, RandaccInStream** theStrs){
	curF = 0;
	curI = 0;
	uintmax_t curOff = 0;
	for(uintptr_t i = 0; i<numStr; i++){
		RandaccInStream* curStr = theStrs[i];
		wrapped.push_back(curStr);
		lowInds.push_back(curOff);
		curOff += curStr->size();
		highInds.push_back(curOff);
	}
}
ConcatenatedRandaccInStream::~ConcatenatedRandaccInStream(){}
uintptr_t ConcatenatedRandaccInStream::readBytes(char* toR, uintptr_t numR){
	uintptr_t totalR = 0;
	char* leftD = toR;
	uintptr_t leftN = numR;
	
	moreToRead:
	if(curF < wrapped.size()){
		InStream* curStr = wrapped[curF];
		uintptr_t curRead = curStr->readBytes(leftD, leftN);
		totalR += curRead;
		leftD += curRead;
		leftN -= curRead;
		if(leftN){
			curF++;
			if(curF < wrapped.size()){ wrapped[curF]->seek(0); }
			goto moreToRead;
		}
	}
	curI += totalR;
	return totalR;
}
int ConcatenatedRandaccInStream::readByte(){
	char toRet;
	uintptr_t numR = readBytes(&toRet, 1);
	if(numR){
		return 0x00FF & toRet;
	}
	else{
		return -1;
	}
}
void ConcatenatedRandaccInStream::close(){isClosed = 1;}
uintmax_t ConcatenatedRandaccInStream::tell(){ return curI; }
void ConcatenatedRandaccInStream::seek(uintmax_t toLoc){
	curI = toLoc;
	if(lowInds.size()){
		//figure out which file
		uintptr_t newF = std::lower_bound(lowInds.begin(), lowInds.end(), toLoc) - lowInds.begin();
		curF = newF;
		wrapped[curF]->seek(toLoc - lowInds[curF]);
	}
}
uintmax_t ConcatenatedRandaccInStream::size(){
	if(wrapped.size()){
		return highInds[highInds.size()-1];
	}
	return 0;
}

SubrangeRandaccInStream::SubrangeRandaccInStream(RandaccInStream* toWrap, uintmax_t wrapFrom, uintmax_t wrapTo){
	wrapped = toWrap;
	curI = wrapFrom;
	lowI = wrapFrom;
	higI = wrapTo;
	wrapped->seek(lowI);
}
SubrangeRandaccInStream::~SubrangeRandaccInStream(){}
uintptr_t SubrangeRandaccInStream::readBytes(char* toR, uintptr_t numR){
	uintmax_t maxRead = higI - curI;
	if(maxRead < numR){
		return readBytes(toR, maxRead);
	}
	curI += numR;
	return wrapped->readBytes(toR, numR);
}
int SubrangeRandaccInStream::readByte(){
	char toRet;
	uintptr_t numR = readBytes(&toRet, 1);
	if(numR){
		return 0x00FF & toRet;
	}
	else{
		return -1;
	}
}
void SubrangeRandaccInStream::close(){isClosed = 1;}
uintmax_t SubrangeRandaccInStream::tell(){
	return curI - lowI;
}
void SubrangeRandaccInStream::seek(uintmax_t toLoc){
	curI = lowI + toLoc;
	if(curI > higI){ throw std::runtime_error("Seek is out of bounds."); }
	wrapped->seek(curI);
}
uintmax_t SubrangeRandaccInStream::size(){
	return higI - lowI;
}

MemoryInStream::MemoryInStream(uintptr_t numBytes, const char* theBytes){
	numBts = numBytes;
	theBts = theBytes;
	nextBt = 0;
}
MemoryInStream::~MemoryInStream(){}
uintmax_t MemoryInStream::tell(){
	return nextBt;
}
void MemoryInStream::seek(uintmax_t toLoc){
	nextBt = toLoc;
}
uintmax_t MemoryInStream::size(){
	return numBts;
}
int MemoryInStream::readByte(){
	if(nextBt >= numBts){
		return -1;
	}
	int toRet = 0x00FF & theBts[nextBt];
	nextBt++;
	return toRet;
}
uintptr_t MemoryInStream::readBytes(char* toR, uintptr_t numR){
	uintptr_t numLeft = numBts - nextBt;
	uintptr_t numGet = std::min(numLeft, numR);
	memcpy(toR, theBts + nextBt, numGet);
	nextBt += numGet;
	return numGet;
}
//TODO allow threading
void MemoryInStream::close(){isClosed = 1;}

ConsoleOutStream::ConsoleOutStream(){}
ConsoleOutStream::~ConsoleOutStream(){}
void ConsoleOutStream::writeByte(int toW){
	fputc(toW, stdout);
}
void ConsoleOutStream::writeBytes(const char* toW, uintptr_t numW){
	fwrite(toW, 1, numW, stdout);
}
void ConsoleOutStream::close(){isClosed = 1;};

ConsoleErrStream::ConsoleErrStream(){}
ConsoleErrStream::~ConsoleErrStream(){}
void ConsoleErrStream::writeByte(int toW){
	fputc(toW, stderr);
}
void ConsoleErrStream::close(){isClosed = 1;};

ConsoleInStream::ConsoleInStream(){}
ConsoleInStream::~ConsoleInStream(){}
int ConsoleInStream::readByte(){
	return fgetc(stdin);
}
uintptr_t ConsoleInStream::readBytes(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, stdin);
}
void ConsoleInStream::close(){isClosed = 1;};

FileOutStream::FileOutStream(int append, const char* fileName){
	if(append){
		baseFile = fopen(fileName, "ab");
	}
	else{
		baseFile = fopen(fileName, "wb");
	}
	if(baseFile == 0){
		isClosed = 1;
		throw std::runtime_error("Could not open file " + myName);
	}
	myName = fileName;
}
FileOutStream::~FileOutStream(){}
void FileOutStream::writeByte(int toW){
	if(fputc(toW, baseFile) < 0){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void FileOutStream::writeBytes(const char* toW, uintptr_t numW){
	if(fwrite(toW, 1, numW, baseFile) != numW){
		throw std::runtime_error("Problem writing file " + myName);
	}
}
void FileOutStream::close(){
	isClosed = 1;
	if(baseFile){
		int probClose = fclose(baseFile);
		baseFile = 0;
		if(probClose){ throw std::runtime_error("Problem closing file."); }
	}
}

FileInStream::FileInStream(const char* fileName){
	myName = fileName;
	intmax_t testSize = fileGetSize(fileName);
	if(testSize < 0){ isClosed = 1; throw std::runtime_error("Could not open file " + myName); }
	fileSize = testSize;
	baseFile = fopen(fileName, "rb");
	if(baseFile == 0){ isClosed = 1; throw std::runtime_error("Could not open file " + myName); }
}
FileInStream::~FileInStream(){}
uintmax_t FileInStream::tell(){
	intmax_t curPos = fileTellFutureProof(baseFile);
	if(curPos < 0){ throw std::runtime_error("Problem getting position of file " + myName); }
	return curPos;
}
void FileInStream::seek(uintmax_t toLoc){
	int wasP = fileSeekFutureProof(baseFile, toLoc, SEEK_SET);
	if(wasP){ throw std::runtime_error("Problem seeking in file " + myName); }
}
uintmax_t FileInStream::size(){
	return fileSize;
}
int FileInStream::readByte(){
	int toR = fgetc(baseFile);
	if((toR < 0) && ferror(baseFile)){
		throw std::runtime_error("Problem reading file " + myName);
	}
	return toR;
}
uintptr_t FileInStream::readBytes(char* toR, uintptr_t numR){
	return fread(toR, 1, numR, baseFile);
}
void FileInStream::close(){
	isClosed = 1;
	if(baseFile){
		int probClose = fclose(baseFile);
		baseFile = 0;
		if(probClose){ throw std::runtime_error("Problem closing file."); }
	}
}

ThreadTask::ThreadTask(){
	wasErr = 0;
	errMess = 0;
}

ThreadTask::~ThreadTask(){
	if(errMess){ free(errMess); }
}

void whodun_actualThreadFunc(void* threadUni){
	OSThread* mainUni = (OSThread*)threadUni;
	ThreadTask* baseUni = mainUni->saveDo;
	try{
		baseUni->doIt();
	}catch(std::exception& err){
		baseUni->wasErr = 1;
		const char* errMess = err.what();
		baseUni->errMess = (char*)malloc(strlen(errMess)+1);
		strcpy(baseUni->errMess, errMess);
	}
}

OSThread::OSThread(ThreadTask* toDo){
	saveDo = toDo;
	threadHandle = threadStart(whodun_actualThreadFunc, this);
	if(!threadHandle){
		throw std::runtime_error("Problem starting thread.");
	}
}

OSThread::~OSThread(){
	if(threadHandle){ std::cerr << "You MUST join on a thread before its destruction." << std::endl; std::terminate(); }
}

void OSThread::join(){
	threadJoin(threadHandle);
	threadHandle = 0;
}

OSMutex::OSMutex(){
	myMut = mutexMake();
}

OSMutex::~OSMutex(){
	mutexKill(myMut);
}

void OSMutex::lock(){
	mutexLock(myMut);
}

void OSMutex::unlock(){
	mutexUnlock(myMut);
}

OSCondition::OSCondition(OSMutex* baseMut){
	saveMut = baseMut->myMut;
	myCond = conditionMake(saveMut);
}

OSCondition::~OSCondition(){
	conditionKill(myCond);
}

void OSCondition::wait(){
	conditionWait(saveMut, myCond);
}

void OSCondition::signal(){
	conditionSignal(saveMut, myCond);
}

void OSCondition::broadcast(){
	conditionBroadcast(saveMut, myCond);
}

OSDLLSO::OSDLLSO(const char* dllName){
	myDLL = dllLoadIn(dllName);
	if(myDLL == 0){
		throw std::runtime_error("Could not load dll.");
	}
}

OSDLLSO::~OSDLLSO(){
	dllUnload(myDLL);
}

void* OSDLLSO::get(const char* locName){
	return dllGetLocation(myDLL, locName);
}

/**Write to stdin of a process.*/
class SubprocessStdinStream : public OutStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStdinStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStdinStream();
	
	void writeByte(int toW);
	void writeBytes(const char* toW, uintptr_t numW);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};
SubprocessStdinStream::SubprocessStdinStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStdinStream::~SubprocessStdinStream(){}
void SubprocessStdinStream::writeByte(int toW){
	char realW = toW;
	writeBytes(&realW, 1);
}
void SubprocessStdinStream::writeBytes(const char* toW, uintptr_t numW){
	subprocInWrite(baseProc->saveProc, toW, numW);
}
void SubprocessStdinStream::close(){
	subprocInFinish(baseProc->saveProc);
	isClosed = 1;
}

/**Read from stdout of a process.*/
class SubprocessStdoutStream : public InStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStdoutStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStdoutStream();
	
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};
SubprocessStdoutStream::SubprocessStdoutStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStdoutStream::~SubprocessStdoutStream(){}
int SubprocessStdoutStream::readByte(){
	char realR;
	if(readBytes(&realR, 1)){
		return 0x00FF & realR;
	}
	else{
		return -1;
	}
}
uintptr_t SubprocessStdoutStream::readBytes(char* toR, uintptr_t numR){
	return subprocOutRead(baseProc->saveProc, toR, numR);
}
void SubprocessStdoutStream::close(){
	isClosed = 1;
}

/**Read from stdout of a process.*/
class SubprocessStderrStream : public InStream{
public:
	/**
	 * Basic setup.
	 * @param baseP The base process.
	 */
	SubprocessStderrStream(SubProcess* baseP);
	/**Clean up and close.*/
	~SubprocessStderrStream();
	
	int readByte();
	uintptr_t readBytes(char* toR, uintptr_t numR);
	void close();
	
	/**The underlying process*/
	SubProcess* baseProc;
};
SubprocessStderrStream::SubprocessStderrStream(SubProcess* baseP){
	baseProc = baseP;
}
SubprocessStderrStream::~SubprocessStderrStream(){}
int SubprocessStderrStream::readByte(){
	char realR;
	if(readBytes(&realR, 1)){
		return 0x00FF & realR;
	}
	else{
		return -1;
	}
}
uintptr_t SubprocessStderrStream::readBytes(char* toR, uintptr_t numR){
	return subprocErrRead(baseProc->saveProc, toR, numR);
}
void SubprocessStderrStream::close(){
	isClosed = 1;
}

SubProcess::SubProcess(SubProcessArguments* theArgs){
	saveProc = subprocStart(theArgs);
	if(saveProc == 0){
		throw std::runtime_error("Problem starting process.");
	}
	haveWait = 0;
	this->stdIn = 0;
	this->stdOut = 0;
	this->stdErr = 0;
	if(theArgs->stdinMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdIn = new SubprocessStdinStream(this);
	}
	if(theArgs->stdoutMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdOut = new SubprocessStdoutStream(this);
	}
	if(theArgs->stderrMeans == WHODUN_SUBPROCESS_PIPE){
		this->stdErr = new SubprocessStderrStream(this);
	}
}
SubProcess::~SubProcess(){
	if(saveProc == 0){
		std::cerr << "You MUST wait on a process before terminating.";
		std::terminate();
	}
}
int SubProcess::wait(){
	if(this->stdIn){ delete(this->stdIn); this->stdIn = 0; }
	if(this->stdOut){ delete(this->stdOut); this->stdOut = 0; }
	if(this->stdErr){ delete(this->stdErr); this->stdErr = 0; }
	retCode = subprocWait(saveProc);
	haveWait = 1;
	return retCode;
}

