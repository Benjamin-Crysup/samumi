#include "whodun_thread.h"

#include <string.h>
#include <iostream>

using namespace whodun;

void ThreadPoolLoopTask::doIt(){
	std::string errMess;
	mainPool->taskMut.lock();
	while(mainPool->poolLive){
		uintptr_t nextDoID = mainPool->nextDoID;
		uintptr_t lookInd = nextDoID - mainPool->idOffset;
		if(lookInd < mainPool->openTasks.size()){
			//get a task
			ThreadTask* nextUni = mainPool->openTasks.getItem(lookInd)->taskDo;
			mainPool->nextDoID++;
			mainPool->taskMut.unlock();
			
			//run the task
			int wasErr = 0;
			try{
				nextUni->doIt();
			}catch(std::exception& err){
				wasErr = 1;
				errMess = err.what();
			}
			
			mainPool->taskMut.lock();
			//let things know it finished
			lookInd = nextDoID - mainPool->idOffset;
			ThreadPoolTaskInfo* curTaskP = mainPool->openTasks.getItem(lookInd);
			if(wasErr){
				curTaskP->taskDo->wasErr = 1;
				curTaskP->taskDo->errMess = (char*)malloc(errMess.size()+1);
				strcpy(curTaskP->taskDo->errMess, errMess.c_str());
			}
			curTaskP->isDone = 1;
			if(curTaskP->waitCond){ curTaskP->waitCond->signal(); }
		}
		else{
			mainPool->taskCond.wait();
		}
	}
	mainPool->taskMut.unlock();
}

ThreadPool::ThreadPool(int numThread) : taskCond(&taskMut){
	poolLive = true;
	numThr = numThread;
	idOffset = 0;
	nextDoID = 0;
	uniStore.resize(numThread);
	for(int i = 0; i<numThread; i++){
		uniStore[i].mainPool = this;
		liveThread.push_back(new OSThread(&(uniStore[i])));
	}
}

ThreadPool::~ThreadPool(){
	taskMut.lock();
		poolLive = false;
		taskCond.broadcast();
	taskMut.unlock();
	for(unsigned i = 0; i<liveThread.size(); i++){
		liveThread[i]->join();
		delete(liveThread[i]);
	}
	if(openTasks.size()){
		std::cerr << "Killing pool with stuff in the queue" << std::endl;
		std::terminate();
	}
	for(uintptr_t i = 0; i<saveConds.size(); i++){
		delete(saveConds[i]);
	}
}

uintptr_t ThreadPool::addTask(ThreadTask* toDo){
	taskMut.lock();
		uintptr_t useID = idOffset + openTasks.size();
		ThreadPoolTaskInfo* newT = openTasks.pushBack(1);
		*newT = {toDo, (OSCondition*)0, 0, 0};
		taskCond.signal();
	taskMut.unlock();
	return useID;
}

uintptr_t ThreadPool::addTasks(uintptr_t numAdd, ThreadTask** toDo){
	taskMut.lock();
	
	uintptr_t firstID = idOffset + openTasks.size();
	
	uintptr_t numLeft = numAdd;
	ThreadTask** nextAdd = toDo;
	while(numLeft){
		uintptr_t numPush = std::max((uintptr_t)1, std::min(numLeft, openTasks.pushBackSpan()));
		ThreadPoolTaskInfo* newT = openTasks.pushBack(numPush);
		for(uintptr_t i = 0; i<numPush; i++){
			newT[i] = {nextAdd[i], (OSCondition*)0, 0, 0};
		}
		numLeft -= numPush;
		nextAdd += numPush;
	}
	
	taskCond.broadcast();
	taskMut.unlock();
	return firstID;
}

void ThreadPool::joinTask(uintptr_t taskID){
	taskMut.lock();
		uintptr_t lookInd = taskID - idOffset;
		ThreadPoolTaskInfo* curTaskP = openTasks.getItem(lookInd);
		//wait for it to finish
		if(!(curTaskP->isDone)){
			if(curTaskP->waitCond){ throw std::runtime_error("Cannot join on the same task twice."); }
			if(saveConds.size()){
				curTaskP->waitCond = saveConds[saveConds.size()-1];
				saveConds.pop_back();
			}
			else{
				curTaskP->waitCond = new OSCondition(&taskMut);
			}
			while(!(curTaskP->isDone)){
				curTaskP->waitCond->wait();
				lookInd = taskID - idOffset;
				curTaskP = openTasks.getItem(lookInd);
			}
		}
		//note the join
		curTaskP->hasJoin = 1;
		//and remove all joined tasks from the queue
		while(openTasks.size()){
			uintptr_t numLook = openTasks.popFrontSpan();
			ThreadPoolTaskInfo* curF = openTasks.getItem(0);
			ThreadPoolTaskInfo* curT = curF;
			for(uintptr_t i = 0; i<numLook; i++){
				if(!(curT->hasJoin)){ break; }
				OSCondition* waitCond = curT->waitCond;
				if(waitCond){ saveConds.push_back(waitCond); }
				curT++;
			}
			uintptr_t numPop = curT - curF;
			if(numPop){
				openTasks.popFront(numPop);
				idOffset += numPop;
			}
			if(numPop != numLook){ break; }
		}
	taskMut.unlock();
}
void ThreadPool::joinTasks(uintptr_t fromID, uintptr_t toID){
	bigStart();
	for(uintptr_t i = fromID; i<toID; i++){
		bigJoin(i);
	}
	bigEnd();
}

void ThreadPool::bigStart(){
	taskMut.lock();
	bigAnyJoined = 0;
	bigAnyAdded = 0;
}
uintptr_t ThreadPool::bigAdd(ThreadTask* toDo){
	bigAnyAdded = 1;
	uintptr_t useID = idOffset + openTasks.size();
	ThreadPoolTaskInfo* newT = openTasks.pushBack(1);
	*newT = {toDo, (OSCondition*)0, 0, 0};
	return useID;
}
void ThreadPool::bigJoin(uintptr_t taskID){
	bigAnyJoined = 1;
	uintptr_t lookInd = taskID - idOffset;
	ThreadPoolTaskInfo* curTaskP = openTasks.getItem(lookInd);
	//wait for it to finish
	if(!(curTaskP->isDone)){
		if(curTaskP->waitCond){ throw std::runtime_error("Cannot join on the same task twice."); }
		if(bigAnyAdded){
			bigAnyAdded = 0;
			taskCond.broadcast();
		}
		if(saveConds.size()){
			curTaskP->waitCond = saveConds[saveConds.size()-1];
			saveConds.pop_back();
		}
		else{
			curTaskP->waitCond = new OSCondition(&taskMut);
		}
		while(!(curTaskP->isDone)){
			curTaskP->waitCond->wait();
			lookInd = taskID - idOffset;
			curTaskP = openTasks.getItem(lookInd);
		}
	}
	//note the join
	curTaskP->hasJoin = 1;
}
void ThreadPool::bigEnd(){
	if(bigAnyAdded){ taskCond.broadcast(); }
	if(bigAnyJoined){
		//remove all joined tasks from the queue
		while(openTasks.size()){
			uintptr_t numLook = openTasks.popFrontSpan();
			ThreadPoolTaskInfo* curF = openTasks.getItem(0);
			ThreadPoolTaskInfo* curT = curF;
			for(uintptr_t i = 0; i<numLook; i++){
				if(!(curT->hasJoin)){ break; }
				OSCondition* waitCond = curT->waitCond;
				if(waitCond){ saveConds.push_back(waitCond); }
				curT++;
			}
			uintptr_t numPop = curT - curF;
			if(numPop){
				openTasks.popFront(numPop);
				idOffset += numPop;
			}
			if(numPop != numLook){ break; }
		}
	}
	taskMut.unlock();
}

void ParallelForLoopUniform::doIt(){
	mainLoop->doRange(threadInd, fromI, toI);
}

ParallelForLoop::ParallelForLoop(uintptr_t numThread){
	allUni.resize(numThread);
	for(uintptr_t i = 0; i<numThread; i++){
		allUni[i].mainLoop = this;
		allUni[i].threadInd = i;
		passUni.push_back(&(allUni[i]));
	}
}

ParallelForLoop::~ParallelForLoop(){}

void ParallelForLoop::doIt(ThreadPool* inPool){
	uintptr_t numPT = (endI - startI) / allUni.size();
	uintptr_t numET = (endI - startI) % allUni.size();
	uintptr_t curOff = startI;
	for(uintptr_t i = 0; i<allUni.size(); i++){
		allUni[i].fromI = curOff;
		curOff += (numPT + (i < numET));
		allUni[i].toI = curOff;
	}
	uintptr_t uniI = inPool->addTasks(allUni.size(), &(passUni[0]));
	inPool->joinTasks(uniI, uniI + allUni.size());
}

void ParallelForLoop::doRange(uintptr_t threadInd, uintptr_t fromI, uintptr_t toI){
	for(uintptr_t i = fromI; i<toI; i++){
		doSingle(threadInd, i);
	}
}

ReadWriteLock::ReadWriteLock() : waitW(&countLock), waitR(&countLock){
	numR = 0;
	numW = 0;
}
ReadWriteLock::~ReadWriteLock(){}
void ReadWriteLock::lockRead(){
	countLock.lock();
	while(numW){ waitR.wait(); }
	numR++;
	countLock.unlock();
}
void ReadWriteLock::unlockRead(){
	countLock.lock();
	numR--;
	if(numR == 0){ waitW.broadcast(); }
	countLock.unlock();
}
void ReadWriteLock::lockWrite(){
	countLock.lock();
	numW++;
	while(numR){ waitW.wait(); }
}
void ReadWriteLock::unlockWrite(){
	numW--;
	if(numW == 0){ waitR.broadcast(); }
	countLock.unlock();
}

