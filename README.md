# fast-deltaSVM

## Overview
This repository contains code for a batch-based version, deemed FastSK-batch, of [FastSK](https://github.com/QData/FastSK). FastSK is a fast and accurate gkm-SVM based SVM classifer for sequences. Our implementation provides an improved algorithm for implementing gkm-SVM string kernel calculations. 

The original version of FastSK computes the entire kernel at once and computes the kernel function between all pairs of test sequences, which is infeasible with a large number of test sequences, such as when scoring all gmers for a delta-svm calculation, and unnecessary as the kernel function between test sequences is not used for prediction purposes. FastSK-Batch attempts to solve this by first computing the entire kernel for only training sequences and fitting the SVM using this kernel using the original FastSK algorithm. Once the SVM has been fitted, the test sequences are split into batches and the kernel is computed only between test sequences in the current batch and train sequences. Notably, the kernel is not calculated between pairs of test sequences as done in the original FastSK kernel calculation, which lowers memory usage and increases the speed of the algorithm. Note however, that the batch-based approach is slower than the original FastSK algorithm if the entire kernel can fit into memory. 

### Results
The results between FastSK-Batch and FastSK are summarized below on the EP300 dataset, for which the kernel fits into memory. 

  | g | m| Threads | Algorithm        | Batchsize | Time | Accuracy | AUROC |
  |---|---|---------|-----------------|-----------|------|----------|-------|
  | 10 | 6 |    4   | FastSK          |   -       | 1:28 | 92.9     | 0.978747 |
  | 10 | 6 |    4   | FastSK-Batch    |   2000    | 2:53 | 92.9     | 0.978747 |

To test the performance on a large number of test sequences, the EP300 dataset was used as training data and all gmers of length 9 were used as testing data. The results of FastSK-Batch, LS-GKM, and FastSK-Batch-Naive, which uses the original FastSK kernel calculation on batches of the test sequences, are summarized below. Note that LS-GKM is much faster. 

  | g | m| Threads | Algorithm           | Batchsize | Time | Peak Memory |
  |---|---|--------|---------------------|-----------|------ |-------------|
  | 8 | 4 | 4      | FastSK-Batch        | 20000     | 2:54  | 1.3 GB |
  | 8 | 4 | 4      | FastSK-Batch-Naive  | 20000     | 3:24  | 7.5 GB |
  | 8 | 4 | 4      | FastSK-Batch        | 50000     | 2:30  | 2.7GB |
  | 8 | 4 | 4      | FastSK-Batch-Naive  | 50000     | Fault due to memory constraints  | - |
  | 8 | 4 | 4      | LS-GKM              | -         | 0:15  | <1 GB |

### Possible Improvements to FastSK-Batch
The kernel calculation for FastSK-Batch may be able to be further improved, but the majority of the time is now spent predicting sequences rather than computing the kernel. The prediction code could be spead up through multi-threading, but is unlikely to have a large enough impact to decrease the runtime to be on par with LS-GKM.  FastSK-Batch is also not able to normalize the kernel as FastSK does since the kernel is not computed between any pair of test sequences. Currently, the kernel is simply unnormalized. 


## Wrapper Info
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
