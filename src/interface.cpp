
#include <Rcpp.h>
#include <string>
#include "fastsk.hpp"
using namespace Rcpp;

//' FastSK: A Fast and Accurate GKM-SVM
//'
//' @name fastsk_compute_kernel
//' @description Computes the kernel for a GKM-SVM
//' @param train_file A FASTA file containing training sequences and their label
//' @param test_file A FASTA file containing testing sequences and their label
//' @param g The length of the substrings used to compare sequences. Constraints: \code{0 < g < 20}
//' @param m The maximum number of mismatches when comparing two gmers Constraints: \code{0 <= m < g}
//' @param t The number of threads to used. Default is 1
//' @param approx A boolean; if set to true, then the fast approximation algorithm is used to compute the kernel. Default is False
//' @param delta Default is 0.025
//' @param max_iters The maximum number of iterations to run. Default is 100
//' @param skip_variance Default is False
//' @param dictionary_file A file containing the alphabet of characters appearing in the sequences. If not provided, the dictionary will be inferred
//' @export
// [[Rcpp::export]]
void fastsk_compute_kernel(std::string train_file, std::string test_file, std::string out_file, int g, int m,
                        int t=1, bool approx=false, double delta=0.025, int max_iters=100, bool skip_variance=false,
                        std::string dictionary_file="") {

    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);
    fastsk->compute_kernel(train_file, test_file, dictionary_file);
    fastsk->save_kernel(out_file);
}

//' FastSK: A Fast and Accurate GKM-SVM
//'
//' @name fastsk_train_and_score
//' @description Trains a fast and accurate gkm-svm and scores on test sequences
//' @param train_file A FASTA file containing training sequences and their label
//' @param test_file A FASTA file containing testing sequences and their label
//' @param g The length of the substrings used to compare sequences. Constraints: \code{0 < g < 20}
//' @param m The maximum number of mismatches when comparing two gmers Constraints: \code{0 <= m < g}
//' @param t The number of threads to used. Default is 1
//' @param approx A boolean; if set to true, then the fast approximation algorithm is used to compute the kernel. Default is False
//' @param delta Default is 0.025
//' @param max_iters The maximum number of iterations to run. Default is 100
//' @param skip_variance Default is False
//' @param C SVM C parameter. Default is 1.0
//' @param nu SVM nu parameter. Default is 1.0
//' @param eps SVM epsilon parameter. Defult is 1.0
//' @param kernel_type The kernel type to used. Must be one of linear (default), fastsk, or rbf
//' @param dictionary_file A file containing the alphabet of characters appearing in the sequences. If not provided, the dictionary will be inferred
//' @param metric The metric to score the test sequences with. Must be one of auc (default) or accuracy
//' @param metric_file A filepath to write the test sequence predictions. Default is auc_file.txt
//' @export
// [[Rcpp::export]]
void fastsk_train_and_score(std::string train_file, std::string test_file, int g, int m,
                        int t=1, bool approx=false, double delta=0.025, int max_iters=100,
                        bool skip_variance=false, double C=1.0, double nu=1.0, double eps=1.0,
                        std::string kernel_type="linear", std::string dictionary_file="",
                        std::string metric="auc", std::string metric_file="auc_file.txt") {


    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);
    fastsk->compute_kernel(train_file, test_file, dictionary_file);
    fastsk->fit(C, nu, eps, kernel_type);
    fastsk->score(metric, metric_file);
}
