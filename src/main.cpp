#include "fastsk.hpp"
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.hpp"

using namespace std;

int help() {
    printf("\nUsage: fastsk [options] <trainingFile> <testFile> <dictionaryFile> <labelsFile>\n");
    printf("FLAGS WITH ARGUMENTS\n");
    printf("\t g : gmer length; length of substrings (allowing up to m mismatches) used to compare sequences. Constraints: 0 < g < 20\n");
    printf("\t m : maximum number of mismatches when comparing two gmers. Constraints: 0 <= m < g\n");
    printf("\t t : (optional) number of threads to use. Set to 1 to not multithread kernel computation\n");
    printf("\t C : (optional) SVM C parameter. Default is 1.0\n");
    printf("\t r : (optional) Kernel type. Must be linear (default), fastsk, or rbf\n");
    printf("\t I : (optional) Maximum number of iterations. Default 100. The number of mismatch positions to sample when running the approximation algorithm.\n");
    printf("\t b : (optional) Batch size for FastSK-batch. The number of testing sequences to use in a batch to compute the kernel and predict.\n");
    printf("NO ARGUMENT FLAGS\n");
    printf("\t a : (optional) Approximation. If set, the fast approximation algorithm will be used to compute the kernel function\n");
    printf("\t q : (optional) Quiet mode. If set, Kernel computation and SVM training info won't be printed.\n");
    printf("ORDERED PARAMETERS\n");
    printf("\t trainingFile : set of training examples in FASTA format\n");
    printf("\t testingFile : set of testing examples in FASTA format\n");
    printf("\t dictionaryFile : (optional) file containing the alphabet of characters that appear in the sequences. If not provided, a dictionary will be inferred from the training file.\n");
    printf("\n");
    printf("\nExample usage:\n");
    printf("\t./fastsk -g 8 -m 4 -t 4 -C 0.01 1.1.train.fasta 1.1.test.fasta protein_dictionary.txt\n\n");

    return 1;
}

int main(int argc, char* argv[]) {
    int quiet = 0;
    string train_file;
    string test_file;
    string dictionary_file;

    // Kernel function params
    int g = -1;
    int m = -1;
    int t = 20;
    bool approx = false;
    int max_iters = 100;
    int batch_size = 0;
    double delta = 0.025;
    bool skip_variance = false;
    string kernel_type = "linear";

    // SVM params
    double C = 1.0;
    double nu = 1;
    double eps = 1;

    int c;
    while ((c = getopt(argc, argv, "g:m:t:I:b:C:r:aq")) != -1) {
        switch (c) {
            case 'g':
                g = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 't':
                t = atoi(optarg);
                break;
            case 'I':
                t = atoi(optarg);
                break;
            case 'b':
                batch_size = atoi(optarg);
            case 'C':
                C = atof(optarg);
                break;
            case 'a':
                approx = true;
                break;
            case 'r':
                kernel_type = optarg;
                break;
            case 'q':
                quiet = 1;
                break;
            break;
        }
    }

    if (g == -1) {
        printf("Must provide a value for the g parameter\n");
        return help();
    }
    if (m == -1) {
        printf("Must provide a value for the m parameter\n");
        return help();
    }
    if (kernel_type != "linear" && kernel_type != "fastsk" && kernel_type != "rbf") {
        printf("kernel must be linear, fastsk, or rbf.\n");
        return help();
    }

    int arg_num = optind;

    if (arg_num < argc) {
        train_file = argv[arg_num++];
    } else {
        printf("Train data file required\n");
        return help();
    }
    if (arg_num < argc) {
        test_file = argv[arg_num++];
    } else {
        printf("Test data file required\n");
        return help();
    }
    if (arg_num < argc) {
        dictionary_file = argv[arg_num++];
    }

    FastSK* fastsk = new FastSK(g, m, t, approx, delta, max_iters, skip_variance);


    // FastSK //
    if (batch_size <= 0) {
        fastsk->compute_kernel(train_file, test_file, dictionary_file);
        fastsk->fit(C, nu, eps, kernel_type);
        fastsk->score("auc", "auc_file_one_shot.txt");
    } 
    // Batch-based Versions //
    else {
        DataReader* data_reader = new DataReader(train_file, dictionary_file);
        bool train = true;

        data_reader->read_data(train_file, train);
        data_reader->read_data(test_file, !train);
        vector<vector<int> > train_seq = data_reader->train_seq;
        vector<vector<int> > test_seq = data_reader->test_seq;
        int* train_labels = data_reader->train_labels.data();
        int* test_labels = data_reader->test_labels.data();

        // FastSK-Batch //
        fastsk->batch_score(train_seq, test_seq, train_labels, test_labels, batch_size, C, nu, eps, kernel_type);

        // FastSK-Batch-Naive //
        // fastsk->compute_train(train_seq, train_labels);
        // fastsk->fit(C, nu, eps, kernel_type);

        // int i = 0;
        // while (i < (int) test_seq.size()) {
        //     vector<vector<int> > test_batch;

        //     for (int j = 0; j < min((int) test_seq.size() - i, batch_size); j++) {
        //         test_batch.push_back(test_seq[i + j]);
        //     }
        //     fastsk->compute_kernel(train_seq, test_batch, train_labels, test_labels + i);
        //     fastsk->score("auc", "auc.txt");
        //     fastsk->free_kernel();
        //     i += batch_size;
        // }
    }
}
