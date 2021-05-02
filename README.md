# fast-deltaSVM

## Overview
This repository is the R wrapper for [FastSK](https://github.com/QData/FastSK). FastSK is a fast and accurate gkm-SVM based SVM classifer for sequences. Our implementation provides an improved algorithm for implementing gkm-SVM string kernel calculations.

We also have a Python package and C++ source code [available for use](https://github.com/QData/FastSK).

## Installation 
You can install the R package with the following commands in an R console:
```
install.packages("devtools")
devtools::install_github("QData/fast-deltaSVM")
library(FastGKMSVM)
```

## Usage
We provide two functions `fastsk_compute_kernel` and `fastsk_train_and_score`. The function `fastsk_compute_kernel` computes the train and test kernels from the given sequences while `fastsk_train_and_score` computes the train and test kernels and scores the given test sequences. 

### Inputs
Both functions require a training and testing file in FASTA format. The the description line must be either a 1 to denote a positive sequence or a 0 to denote a negative sequence. For example,

```
>0
AAAA...
>1
CCCC...
```
The sequences may contain both lowercase and capaitalized letters. 

TODO:
Add script to convert from LS-GKM input format to ours
Add script to run with .bed files

### Outputs
`fastsk_compute_kernel` will output one file containing the full kernel matrix.

`fastsk_train_and_score` will output one file where each line contains the correct label followed by the predicted probability that the sequence is a postitive sample. The predictions are in the same order as the given training file. 


### Parameter Documentation
 Full parameter level documentation can be seen by running the following commands in an R console:
```
help(fastsk_compute_kernel)
help(fastsk_train_and_score)
```

### A Simple Example
To get started, we'll use the example dataset in the data folder. Download the two files in the [data](https://github.com/QData/fast-deltaSVM/tree/main/data) folder then open an R console and run the following to train on the training sequences, score the test sequences, and write the predictions to `pred.out`:
```
library(FastGKMSVM)
fastsk_train_and_score("EP300.train.fasta", "EP300.test.fasta", 10, 3, metric_file="pred.out")
```

To only compute the kernel and write it to `kernel.out` without training the SVM, run:
```
library(FastGKMSVM)
fastsk_compute_kernel("EP300.train.fasta", "EP300.test.fasta", 10, 3, kernel_file="test.out")
```

## Development
The [Rcpp package](https://cran.r-project.org/web/packages/Rcpp/index.html) was used to integrate the C++ source code into R and [roxygen2](https://cran.r-project.org/web/packages/roxygen2/index.html) was used to generate documentation. 
