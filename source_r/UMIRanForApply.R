library(tidyverse)
library(ranger)

cargs = commandArgs(trailingOnly=TRUE)
useModFN = cargs[[1]]
origTab = cargs[[2]]
outFN = cargs[[3]]
minReads = strtoi(cargs[[4]])

if(origTab == "-"){
	origTab = file("stdin")
}
origTab = filter(read_tsv(origTab), Reads >= minReads)

if(nrow(origTab)==0){
	endTab = mutate(origTab, pCorrect = 0)
} else {
	rfModel = readRDS(useModFN)
	useInd = 1
	if(rfModel$forest$class.values[[1]] == 0){
		useInd = 2
	}
	predTab = predict(rfModel, origTab)$predictions[,useInd]
	endTab = mutate(origTab, pCorrect = predTab)
}

if(outFN == "-"){
	writeLines(format_tsv(endTab), stdout())
} else {
	write_tsv(endTab, outFN)
}
