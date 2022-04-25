library(tidyverse)
library(ranger)

cargs = commandArgs(trailingOnly=TRUE)

for(targ in cargs){
	if((targ == "--help") | (targ == "-h") | (targ == "/?")){
		print("Train a random forest for alelle data.")
		print("Uses positional arguments.")
		print("The first argument is where to save the model file.")
		print("The second argument is the minimum number of UMI families to accept.")
		print("The following arguments are the files to train on.")
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

outArg = cargs[[1]]
minUMIs = strtoi(cargs[[2]])
origTab = filter(read_tsv(cargs[[3]]), UMI_Count >= minUMIs)
for (i in seq_along(cargs)){
	if(i>3){
		newTab = filter(read_tsv(cargs[[i]]), UMI_Count >= minUMIs)
		origTab = rbind(origTab, newTab)
	}
}
rfModel = ranger(Correct ~ Primer + UMI_Count + Log10Error + Total_Primary_Read_Count + Bulk_Purity + Primary_Length + UMI_per_Locus_Total_UMI + Reads_per_Locus_Total_Reads + F0_Primary_Reads + F0_Purity + F0_MaxMinQ_UMI + F0_Best_PNoSeqErr + F1_Primary_Reads + F1_Purity + F1_MaxMinQ_UMI + F1_Best_PNoSeqErr + F2_Primary_Reads + F2_Purity + F2_MaxMinQ_UMI + F2_Best_PNoSeqErr + F3_Primary_Reads + F3_Purity + F3_MaxMinQ_UMI + F3_Best_PNoSeqErr + F4_Primary_Reads + F4_Purity + F4_MaxMinQ_UMI + F4_Best_PNoSeqErr, data = origTab, probability=TRUE, mtry=9, splitrule="gini", min.node.size=5)
saveRDS(rfModel, outArg)


