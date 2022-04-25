library(tidyverse)
library(ranger)

cargs = commandArgs(trailingOnly=TRUE)

for(targ in cargs){
	if((targ == "--help") | (targ == "-h") | (targ == "/?")){
		print("Apply a random forest to UMI family data.")
		print("Uses positional arguments.")
		print("The first argument is the model file.")
		print("The second argument is the input table: - is stdin.")
		print("The thirid argument is the output table: - is stdout.")
		print("The fourth argument is the minimum number of reads to accept.")
		quit(save="no",status=0)
	}
	if(targ == "--version"){
        print("SAMmed UMI 0.0")
        print("Copyright (C) 2021 UNT Center for Human Identification")
        print("License LGPLv3+: GNU LGPL version 3 or later")
        print("    <https://www.gnu.org/licenses/lgpl-3.0.html>")
        print("This is free software: you are free to change and redistribute it.")
        print("There is NO WARRANTY, to the extent permitted by law.")
		quit(save="no",status=0)
	}
}

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
