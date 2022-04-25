library(tidyverse)
library(ranger)

cargs = commandArgs(trailingOnly=TRUE)

for(targ in cargs){
	if((targ == "--help") | (targ == "-h") | (targ == "/?")){
		print("Train a random forest for UMI family data.")
		print("Uses positional arguments.")
		print("The first argument is where to save the model file.")
		print("The second argument is the minimum number of reads to accept.")
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
minReads = strtoi(cargs[[2]])
origTab = filter(read_tsv(cargs[[3]]), Reads >= minReads)
for (i in seq_along(cargs)){
	if(i>3){
		newTab = filter(read_tsv(cargs[[i]]), Reads >= minReads)
		origTab = rbind(origTab, newTab)
	}
}
rfModel = ranger(Correct ~ Primer + Reads + Primary_Length + Nearby_UMI + Nearby_UMI_Same + Run_Type + Phred_MaxMin_UMI + Phred_MaxMin_Primer + Phred_MaxMin_Anchor + Phred_MaxMin_Common + Phred_MaxMin_Primary + Phred_MaxMin_Alternate + Phred_Aggregate_Best_Primary + Phred_Aggregate + Reads_per_Locus_Total_Reads + Reads_per_Person_Total_Reads + Primary_Proportion + Secondary_Proportion + Secondary_Proportion_per_Primary_Proportion + Hap_Per_Read + Length_Diff + Length_Diff_mod_Period + Nearby_Read_Ratio + Nearby_Primary_Ratio + Nearby_UMI_Length_Diff + Nearby_UMI_Length_Diff_mod_Period, data = origTab, probability=TRUE, mtry=9, splitrule="gini", min.node.size=5)
saveRDS(rfModel, outArg)




