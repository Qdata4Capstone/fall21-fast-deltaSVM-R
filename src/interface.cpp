
#include <Rcpp.h>
#include <string>
#include "fastsk.hpp"
using namespace Rcpp;

// [[Rcpp::export]]
void compute_kernel(std::string train_file, std::string test_file, std::string out_file, int g, int m, 
                        int t=1, bool approx=false, double delta=0.025, int max_iters=100, bool skip_variance=false, 
                        std::string dictionary_file="") {

    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);
    fastsk->compute_kernel(train_file, test_file, dictionary_file);
    fastsk->save_kernel(out_file);
}




// [[Rcpp::export]]
void train(std::string kernel_file, std::string train_file, std::string test_file, std::string out_file, 
                int g, int m, int t=1, bool approx=false, double delta=0.025, int max_iters=100, bool skip_variance=false, 
                double C=1.0, double nu=1.0, double eps=1.0, std::string kernel_type="linear") {

    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);
    fastsk->fit(C, nu, eps, kernel_type);
}

// [[Rcpp::export]]
double score() {

    return 0;
}

// [[Rcpp::export]]
void train_and_score(std::string train_file, std::string test_file, int g, int m, 
                        int t=1, bool approx=false, double delta=0.025, int max_iters=100, 
                        bool skip_variance=false, double C=1.0, double nu=1.0, double eps=1.0, 
                        std::string kernel_type="linear", std::string dictionary_file="", 
                        std::string metric="auc") {
 
    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);
    fastsk->compute_kernel(train_file, test_file, dictionary_file);
    fastsk->fit(C, nu, eps, kernel_type);
    fastsk->score(metric);
}


