#include "whodun_tree.h"

using namespace whodun;

TreeNode::~TreeNode(){}
Tree::~Tree(){}
Partitioner::~Partitioner(){}

void* PartitionTreeNode::getPayload(){
	return this;
}
uintptr_t PartitionTreeNode::numChildren(){
	if(leftSubI){ return 2; }
	return 0;
}
TreeNode* PartitionTreeNode::getChild(uintptr_t indC){
	if(indC == 0){
		return &(myTree->allNodes[leftSubI]);
	}
	else{
		return &(myTree->allNodes[rightSubI]);
	}
}
TreeNode* PartitionTreeNode::getParent(){
	if(parI){
		return &(myTree->allNodes[parI]);
	}
	else{
		return 0;
	}
}

PartitionTree::PartitionTree(Partitioner* toPartition){
	toPart = toPartition;
}
PartitionTree::~PartitionTree(){}
void PartitionTree::buildTree(uintptr_t numData, void** theData){
	allNodes.clear();
	allSplits.clear();
	partData.clear();
		for(uintptr_t i = 0; i<numData; i++){ partData.push_back(theData[i]); }
	uintptr_t splitSize = toPart->splitSize();
	tmpStack.push_back( triple<uintptr_t,uintptr_t,uintptr_t>(0, numData, 0) );
	while(tmpStack.size()){
		triple<uintptr_t,uintptr_t,uintptr_t> curSV = tmpStack[tmpStack.size()-1];
		tmpStack.pop_back();
		uintptr_t splitI = allSplits.size();
		uintptr_t newNI = allNodes.size();
		uintptr_t parI = curSV.third;
		PartitionTreeNode newNode;
		newNode.parI = parI;
		newNode.myTree = this;
		newNode.splitI = splitI;
		newNode.myDInd = curSV.first;
		newNode.leftSubI = 0;
		newNode.rightSubI = 0;
		allSplits.resize(allSplits.size() + splitSize);
		void* curSplD = &(allSplits[splitI]);
		if(toPart->allocateSplit(curSV.second - curSV.first, curSV.first + &(partData[0]), curSplD)){
			//pack all the spans at the front of the line
			intptr_t packL = curSV.first;
			intptr_t packH = curSV.second - 1;
			while(packL <= packH){
				if(toPart->splitDir(curSplD, partData[packL])==0){
					packL++;
				}
				else{
					void* tmpSave = partData[packL];
					partData[packL] = partData[packH];
					partData[packH] = tmpSave;
					packH--;
				}
			}
			newNode.numData = packL - curSV.first;
			//pack the lows and highs
			uintptr_t negFI = packL;
			packH = curSV.second - 1;
			while(packL <= packH){
				if(toPart->splitDir(curSplD, partData[packL])<0){
					packL++;
				}
				else{
					void* tmpSave = partData[packL];
					partData[packL] = partData[packH];
					partData[packH] = tmpSave;
					packH--;
				}
			}
			//add new things to the stack
			tmpStack.push_back( triple<uintptr_t,uintptr_t,uintptr_t>(packL, curSV.second, newNI) );
			tmpStack.push_back( triple<uintptr_t,uintptr_t,uintptr_t>(negFI, packL, newNI) );
		}
		else{
			newNode.numData = curSV.second - curSV.first;
			allSplits.resize(splitI);
		}
		allNodes.push_back(newNode);
		if(allNodes[parI].leftSubI){
			allNodes[parI].rightSubI = newNI;
		}
		else{
			allNodes[parI].leftSubI = newNI; //a little bit of trickery for the root node: that's also why this is at the end
		}
	}
}
TreeNode* PartitionTree::getRoot(){
	if(allNodes.size()){
		return &(allNodes[0]);
	}
	return 0;
}

