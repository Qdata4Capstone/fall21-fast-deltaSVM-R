#include "fastsk.hpp"
#include "fastsk_kernel.hpp"
#include "utils.hpp"
#include "shared.h"
#include "libsvm-code/svm.h"

#include <vector>
#include <string>
#include <set>
#include <math.h>
#include <cstring>
#include <assert.h>
#include <map>
// #include <Rcpp.h>
#include <iostream>

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

using namespace std;

FastSK::FastSK(int g, int m, int t, bool approx, double delta, int max_iters, bool skip_variance) {
    this->g = g;
    this->m = m;
    this->k = g - m;
    this->num_threads = t;
    this->approx = approx;
    this->delta = delta;
    this->max_iters = max_iters;
    this->skip_variance = skip_variance;
}

void FastSK::free_kernel() {
    free(this->K);
}

void FastSK::compute_kernel(const string train_file, const string test_file) {
    const string dictionary_file = "";
    this->compute_kernel(train_file, test_file, dictionary_file);
}

void FastSK::compute_kernel(const string train_file, const string test_file, const string dictionary_file) {
    // Read in the sequences from the two files and convert them vectors
    DataReader* data_reader = new DataReader(train_file, dictionary_file);
    bool train = true;

    data_reader->read_data(train_file, train);
    data_reader->read_data(test_file, !train);
    vector<vector<int> > train_seq = data_reader->train_seq;
    vector<vector<int> > test_seq = data_reader->test_seq;

    this->train_labels = data_reader->train_labels.data();
    this->test_labels = data_reader->test_labels.data();

    this->compute_kernel(train_seq, test_seq);
}
void FastSK::compute_kernel(vector<vector<int> > Xtrain, vector<vector<int> > Xtest, int *train_labels, int *test_labels) {
    this->train_labels = train_labels;
    this->test_labels = test_labels;

    this->compute_kernel(Xtrain, Xtest);
}

void FastSK::compute_kernel(vector<vector<int> > Xtrain, vector<vector<int> > Xtest) {
    // Given sequences already in numerical form, compute the kernel matrix
    vector<int> lengths;
    int shortest_train = Xtrain[0].size();
    for (unsigned long i = 0; i < Xtrain.size(); i++) {
        int len = Xtrain[i].size();
        if (len < shortest_train) {
            shortest_train = len;
        }
        lengths.push_back(len);
    }
    int shortest_test = Xtest[0].size();
    for (unsigned long i = 0; i < Xtest.size(); i++) {
        int len = Xtest[i].size();
        if (len < shortest_test) {
            shortest_test = len;
        }
        lengths.push_back(len);
    }

    std::cout << "Length of shortest train sequence: " << shortest_train << endl;
    std::cout << "Length of shortest test sequence: " << shortest_test << endl;

    if (this->g > shortest_train) {
        g_greater_than_shortest_train(this->g, shortest_train);
    }
    if (this->g > shortest_test) {
        g_greater_than_shortest_test(this->g, shortest_test);
    }

    long int n_str_train = Xtrain.size();
    long int n_str_test = Xtest.size();
    long int total_str = n_str_train + n_str_test;

    this->n_str_train = n_str_train;
    this->n_str_test = n_str_test;
    this->total_str = total_str;

    int **S = (int **) malloc(total_str * sizeof(int*));

    set<int> dict;
    dict.insert(0);
    for (int i = 0; i < n_str_train; i++) {
        S[i] = Xtrain[i].data();
        for (int j = 0; j < lengths[i]; j++) {
            dict.insert(Xtrain[i][j]);
        }
    }
    for (int i = 0; i < n_str_test; i++) {
        S[n_str_train + i] = Xtest[i].data();
        for (int j = 0; j < lengths[n_str_train + i]; j++) {
            dict.insert(Xtest[i][j]);
        }
    }
    int dict_size = dict.size();
    std::cout << "Dictionary size = " << dict_size << " (+1 for unknown char)." << endl;

    /*Extract g-mers*/
    Features* features = extractFeatures(S, lengths, total_str, g);
    int nfeat = (*features).n;
    int *feat = (*features).features;
    if (!this->quiet) {
        printf("g = %d, k = %d, %d features\n", this->g, this->k, nfeat);
    }

    kernel_params params;
    params.g = g;
    params.k = k;
    params.m = m;
    params.n_str_train = n_str_train;
    params.n_str_test = n_str_test;
    params.total_str = total_str;
    params.n_str_pairs = (total_str / (double) 2) * (total_str + 1);
    params.features = features;
    params.dict_size = dict_size;
    params.num_threads = this->num_threads;
    params.num_mutex = this->num_mutex;
    params.quiet = this->quiet;
    params.approx = this->approx;
    params.delta = this->delta;
    params.max_iters = this->max_iters;
    params.skip_variance = this->skip_variance;

    KernelFunction* kernel_function = new KernelFunction(&params);
    double *K = kernel_function->compute_kernel();
    free(features);

    this->K = K;
    this->stdevs = kernel_function->stdevs;
}

void FastSK::compute_train(vector<vector<int> > Xtrain, int *train_labels) {
    this->train_labels = train_labels;
    this->compute_train(Xtrain);
}

void FastSK::compute_train(vector<vector<int> > Xtrain) {
    vector<int> lengths;
    int shortest_train = Xtrain[0].size();
    for (int i = 0; i < Xtrain.size(); i++) {
        int len = Xtrain[i].size();
        if (len < shortest_train) {
            shortest_train = len;
        }
        lengths.push_back(len);
    }

    if (this->g > shortest_train) {
        g_greater_than_shortest_train(this->g, shortest_train);
    }

    long int n_str_train = Xtrain.size();
    long int n_str_test = 0;
    long int total_str = n_str_train + n_str_test;

    this->n_str_train = n_str_train;
    this->n_str_test = n_str_test;

    int **S = (int **) malloc(total_str * sizeof(int*));

    set<int> dict;
    dict.insert(0);
    for (int i = 0; i < n_str_train; i++) {
        S[i] = Xtrain[i].data();
        for (int j = 0; j < lengths[i]; j++) {
            dict.insert(Xtrain[i][j]);
        }
    }

    int dict_size = dict.size();
    std::cout << "Dictionary size = " << dict_size << " (+1 for unknown char)." << endl;

    /*Extract g-mers*/
    Features* features = extractFeatures(S, lengths, total_str, g);
    int nfeat = (*features).n;
    int *feat = (*features).features;
    if (!this->quiet) {
        printf("g = %d, k = %d, %d features\n", this->g, this->k, nfeat);
    }

    kernel_params params;
    params.g = g;
    params.k = k;
    params.m = m;
    params.n_str_train = n_str_train;
    params.n_str_test = n_str_test;
    params.total_str = total_str;
    params.n_str_pairs = (total_str / (double) 2) * (total_str + 1);
    params.features = features;
    params.dict_size = dict_size;
    params.num_threads = this->num_threads;
    params.num_mutex = this->num_mutex;
    params.quiet = this->quiet;
    params.approx = this->approx;
    params.delta = this->delta;
    params.max_iters = this->max_iters;
    params.skip_variance = this->skip_variance;

    KernelFunction* kernel_function = new KernelFunction(&params);
    double *K = kernel_function->compute_kernel();

    this->K = K;
    this->stdevs = kernel_function->stdevs;
    this->nfeat = nfeat;
}

void FastSK::batch_score(vector<vector<int> > Xtrain, vector<vector<int> > Xtest, int* train_labels, int* test_labels, int batch_size, double C, double nu, double eps, const string kernel_type) {

    this->train_labels = train_labels;
    this->compute_train(Xtrain);
    this->fit(C, nu, eps, kernel_type);

    this->train_labels = train_labels;

    int i = 0; 
    while (i < (int) Xtest.size()) {
        vector<vector<int> > test_batch;

        for (int j = 0; j < min((int) Xtest.size() - i, batch_size); j++) {
            test_batch.push_back(Xtest[i + j]);
        }
        this->test_labels = (test_labels + i);
        this->compute_kernel_batch(Xtrain, test_batch); 
        this->predict("auc");

        i += batch_size;
    }
    
}

void FastSK::compute_kernel_batch(vector<vector<int> > Xtrain, vector<vector<int>> Xbatch) {

    vector<int> sv_lengths;
    vector<int> test_lengths;

    int shortest_train = Xtrain[0].size();
    for (unsigned long i = 0; i < Xtrain.size(); i++) {
        int len = Xtrain[i].size();
        if (len < shortest_train) {
            shortest_train = len;
        }
        sv_lengths.push_back(len);
    }
    int shortest_test = Xbatch[0].size();
    for (unsigned long i = 0; i < Xbatch.size(); i++) {
        int len = Xbatch[i].size();
        if (len < shortest_test) {
            shortest_test = len;
        }
        test_lengths.push_back(len);
    }

    cout << "Length of shortest train sequence: " << shortest_train << endl;
    cout << "Length of shortest test sequence: " << shortest_test << endl;

    if (this->g > shortest_train) {
        g_greater_than_shortest_train(this->g, shortest_train);
    }
    if (this->g > shortest_test) {
        g_greater_than_shortest_test(this->g, shortest_test);
    }

    vector<string> train_seqs;
    for (int i=0; i < (int) Xtrain.size(); i++) {
        string train_seq;
        for (int j = 0; j < (int) Xtrain[i].size(); j++) {
            train_seq.append(to_string(Xtrain[i][j]));
        }
        train_seqs.push_back(train_seq);
    }

    cout << "Done obtaining training sequences\n";

    int n_train_feat = 0;
    for (int i = 0; i < (int) sv_lengths.size(); i++) {
        n_train_feat += (sv_lengths[i] >= this->g) ? (sv_lengths[i] - this->g + 1) : 0;
    }

    cout << "Counted " << n_train_feat << " features\n";

    int *train_groups = (int *) malloc(n_train_feat * sizeof(int));
    string *train_features = new string[n_train_feat];

    int c = 0;
    for (int i = 0; i < (int) train_seqs.size(); i++) {
        for (int j = 0; j < (int) train_seqs[i].size() - this->g + 1; j++) {
            train_features[c] = train_seqs[i].substr(j, this->g);
            train_groups[c] = i;
            c++;
        }
    }
    if (n_train_feat != c) {
        printf("Something is wrong...\n");
    } else {
        printf("Done with train sequences...\n");
    }

    vector<string> test_sequences;
    for (int i=0; i < (int) Xbatch.size(); i++) {
        string sequence;
        for (int j = 0; j < (int) Xbatch[i].size(); j++) {
            sequence.append(to_string(Xbatch[i][j]));
        }
        test_sequences.push_back(sequence);
    }


    int n_test_feat = 0;
    for (int i = 0; i < (int) Xbatch.size(); i++) {
        n_test_feat += (test_lengths[i] >= this->g) ? (test_lengths[i] - this->g + 1) : 0;
    }


    printf("Number of features: %d\n", n_test_feat);
    int *test_groups = (int *) malloc(n_test_feat * sizeof(int));
    string *test_features = new string[n_test_feat];

    c = 0;
    for (int i = 0; i < (int) test_sequences.size(); i++) {
        for (int j = 0; j < (int) test_sequences[i].size() - this->g + 1; j++) {
            test_features[c] = test_sequences[i].substr(j, this->g);
            test_groups[c] = i;
            c++;
        }
    }
    if (n_test_feat != c) {
        printf("Something is wrong...\n");
    } else {
        printf("Done with test batch sequences...\n");
    }

    BatchFeature *features = (BatchFeature *) malloc(sizeof(BatchFeature));
    (*features).test_features = test_features;
    (*features).test_groups = test_groups;
    (*features).n_test_feat = n_test_feat;
    (*features).train_features = train_features;
    (*features).train_groups = train_groups;
    (*features).n_train_feat = n_train_feat;

    kernel_params params;
    params.g = this->g;
    params.k = this->k;
    params.m = this->m;
    params.n_str_train = Xtrain.size();
    params.n_str_test = Xbatch.size();
    params.total_str = Xtrain.size() + Xbatch.size();
    params.n_str_pairs = Xtrain.size() * Xbatch.size();
    params.batch_features = features;
    params.dict_size = 0;
    params.num_threads = this->num_threads;
    params.num_mutex = this->num_mutex;
    params.quiet = this->quiet;
    params.approx = this->approx;
    params.delta = this->delta;
    params.max_iters = this->max_iters;
    params.skip_variance = this->skip_variance;

    this->total_str = params.total_str;
    this->n_str_train = params.n_str_train;
    this->n_str_test = params.n_str_test;

    KernelFunction* kernel_function = new KernelFunction(&params);
    double *K = kernel_function->compute_test_kernel();
    free(features);

    this->K = K;
    this->stdevs = kernel_function->stdevs;
    this->nfeat = n_train_feat + n_test_feat;

}

vector<vector<double> > FastSK::get_train_kernel() {
    double *K = this->K;
    int n_str_train = this->n_str_train;
    vector<vector<double> > train_K(n_str_train, vector<double>(n_str_train, 0));
    for (int i = 0; i < n_str_train; i++) {
        for (int j = 0; j < n_str_train; j++) {
            train_K[i][j] = tri_access(K, i, j);
        }
    }
    return train_K;
}

vector<vector<double> > FastSK::get_test_kernel() {
    double *K = this->K;
    int n_str_train = this->n_str_train;
    int n_str_test = this->n_str_test;
    int total_str = this->n_str_train + this->n_str_test;

    vector<vector<double> > test_K(n_str_test, vector<double>(n_str_train, 0));

    for (int i = n_str_train; i < total_str; i++){
        for (int j = 0; j < n_str_train; j++){
            test_K[i - n_str_train][j]  = tri_access(K, i, j);
        }
    }

    return test_K;
}

vector<double> FastSK::get_stdevs() {
    return this->stdevs;
}

void FastSK::save_kernel(string kernel_file) {
    double *K = this->K;
    int total_str = this->n_str_train + this->n_str_test;
    if (!kernel_file.empty()) {
        printf("Writing kernel to %s...\n", kernel_file.c_str());
        FILE *kernelfile = fopen(kernel_file.c_str(), "w");
        for (int i = 0; i < total_str; ++i) {
            for (int j = 0; j < total_str; ++j) {
                fprintf(kernelfile, "%d:%e ", j + 1, tri_access(K,i,j));
            }
            fprintf(kernelfile, "\n");
        }
        fclose(kernelfile);
    }
}

void FastSK::fit(double C, double nu, double eps, const string kernel_type) {
    // if ((this->kernel_type == LINEAR || this->kernel_type == RBF) && test_file.empty()) {
    //     printf("A test file must be provided for kernel type '%s'\n", this->kernel_type_name.c_str());
    //     exit(1);
    // }

    this->C = C;
    this->nu = nu;
    this->eps = eps;
    if (kernel_type == "linear") {
        this->kernel_type = LINEAR;
        this->kernel_type_name = "linear";
    } else if (kernel_type == "fastsk") {
        this->kernel_type = FASTSK;
        this->kernel_type_name = "fastsk";
    } else if (kernel_type == "rbf"){
        this->kernel_type = RBF;
        this->kernel_type_name = "rbf";
    } else {
        printf("Error: kernel must be: 'linear', 'fastsk', or 'rbf'\n");
        exit(1);
    }

    int g = this->g;
    int m = this->m;
    bool quiet = false;
    int n_str_train = this->n_str_train;
    int n_str_test = this->n_str_test;
    int nfeat = this->nfeat;

    struct svm_parameter* svm_param = Malloc(svm_parameter, 1);
    svm_param->svm_type = this->svm_type;
    svm_param->kernel_type = this->kernel_type;
    svm_param->nu = this->nu;
    svm_param->gamma = 1 /(double) this->nfeat;
    svm_param->cache_size = this->cache_size;
    svm_param->C = this->C;
    svm_param->nr_weight = this->nr_weight;
    svm_param->weight_label = this->weight_label;
    svm_param->weight = this->weight;
    svm_param->shrinking = this->h;
    svm_param->probability = this->probability;
    svm_param->eps = this->eps;
    svm_param->degree = 0;

    svm_problem *prob;
    struct svm_model *model;

    prob = this->create_svm_problem(this->K, this->train_labels, svm_param);
    model = this->train_model(this->K, this->train_labels, svm_param);

    this->model = model;
}

svm_model* FastSK::train_model(double *K, int *labels, svm_parameter *svm_param) {
    int n_str_train = this->n_str_train;
    struct svm_problem* prob = Malloc(svm_problem, 1);
    const char* error_msg;

    svm_node** x;
    svm_node* x_space;
    prob->l = n_str_train;
    prob->y = Malloc(double, prob->l);
    x = Malloc(svm_node*, prob->l);
    if (svm_param->kernel_type == FASTSK) {
        x_space = Malloc(struct svm_node, (n_str_train + 1) * n_str_train);
        int totalind = 0;
        for (int i = 0; i < n_str_train; i++) {
            x[i] = &x_space[totalind];
            for (int j = 0; j < n_str_train; j++) {
                x_space[j + i * (n_str_train + 1)].index = j + 1;
                x_space[j + i * (n_str_train + 1)].value = tri_access(K, i, j);
            }
            totalind += n_str_train;
            x_space[totalind].index = -1;
            totalind++;
            prob->y[i] = labels[i];
        }
        //this->x_space = x_space;
    } else if (svm_param->kernel_type == LINEAR || svm_param->kernel_type == RBF) {
        x_space = Malloc(struct svm_node, (n_str_train + 1) * n_str_train);
        int totalind = 0;
        for (int i = 0; i < n_str_train; i++) {
            x[i] = &x_space[totalind];
            for (int j = 0; j < n_str_train; j++) {
                x_space[j + i * (n_str_train + 1)].index = j + 1;
                x_space[j + i * (n_str_train + 1)].value = tri_access(K, i, j);
            }
            totalind += n_str_train;
            x_space[totalind].index = -1;
            totalind++;
            prob->y[i] = labels[i];
        }
        //this->x_space = x_space;
    }

    prob->x = x;

    // if quiet mode, set libsvm's print function to null
    if (this->quiet) {
        svm_set_print_string_function(&print_null);
    }

    error_msg = svm_check_parameter(prob, svm_param);

    if (error_msg) {
        std::cerr << "ERROR: " << error_msg << std::endl;
        exit(1);
    }

    // train that ish
    struct svm_model* model;
    model = svm_train(prob, svm_param);

    free(svm_param);

    return model;
}

svm_problem* FastSK::create_svm_problem(double* K, int* labels, svm_parameter* svm_param) {
    int n_str_train = this->n_str_train;
    struct svm_problem* prob = Malloc(svm_problem, 1);
    const char* error_msg;
    svm_node** x;
    svm_node* x_space;

    prob->l = n_str_train;
    prob->y = Malloc(double, prob->l);
    x = Malloc(svm_node*, prob->l);

    if (svm_param->kernel_type == FASTSK) {
        x_space = Malloc(struct svm_node, (n_str_train + 1) * n_str_train);
        int totalind = 0;
        for (int i = 0; i < n_str_train; i++) {
            x[i] = &x_space[totalind];
            for (int j = 0; j < n_str_train; j++) {
                x_space[j + i * (n_str_train + 1)].index = j + 1;
                x_space[j + i * (n_str_train + 1)].value = tri_access(K, i, j);
            }
            totalind += n_str_train;
            x_space[totalind].index = -1;
            totalind++;
            prob->y[i] = labels[i];
        }
        //this->x_space = x_space;
    } else if (svm_param->kernel_type == LINEAR || svm_param->kernel_type == RBF) {
        x_space = Malloc(struct svm_node, (n_str_train + 1) * n_str_train);
        int totalind = 0;
        for (int i = 0; i < n_str_train; i++) {
            x[i] = &x_space[totalind];
            // seems like tri_access on K is causing the segfault
            for (int j = 0; j < n_str_train; j++) {
                x_space[j + i * (n_str_train + 1)].index = j + 1;
                x_space[j + i * (n_str_train + 1)].value = tri_access(K, i, j);
            }
            totalind += n_str_train;
            x_space[totalind].index = -1;
            totalind++;
            prob->y[i] = labels[i];
        }
        //this->x_space = x_space;
    }

    prob->x = x;

    if (this->quiet) {
        svm_set_print_string_function(&print_null);
    }

    error_msg = svm_check_parameter(prob, svm_param);

    if (error_msg) {
        std::cerr << "ERROR: " << error_msg << std::endl;
        exit(1);
    }

    return prob;
}

double FastSK::score(const string metric, const string outfile="auc_file.txt") {
    if (metric != "accuracy" && metric != "auc") {
        throw std::invalid_argument("metric argument must be 'accuracy' or 'auc'");
    }
    int n_str = this->total_str;
    int n_str_train = this->n_str_train;
    int n_str_test = this->n_str_test;
    printf("Predicting labels for %d sequences...\n", n_str_test);
    double *test_K = construct_test_kernel(n_str_train, n_str_test, this->K);
    int *test_labels = this->test_labels;
    printf("Test kernel constructed...\n");

    int num_sv = this->model->nSV[0] + this->model->nSV[1];
    printf("num_sv = %d\n", num_sv);
    struct svm_node *x = Malloc(struct svm_node, n_str_train + 1);
    int correct = 0;
    // aggregators for finding num of pos and neg samples for auc
    int pagg = 0, nagg = 0;
    double* neg = Malloc(double, n_str_test);
    double* pos = Malloc(double, n_str_test);

    int fp = 0, fn = 0; //counters for false postives and negatives
    int tp = 0, tn = 0; //counters for true postives and negatives
    int labelind = 0;
    for (int i =0; i < 2; i++){
        if (this->model->label[i] == 1)
            labelind = i;
    }

    FILE *auc_file;
    auc_file = fopen(outfile.c_str(), "w+");

    int svcount = 0;
    for (int i = 0; i < n_str_test; i++) {
        if (this->kernel_type == FASTSK) {
            for (int j = 0; j < n_str_train; j++){
                x[j].index = j + 1;
                x[j].value = 0;
            }
            svcount = 0;
            for (int j = 0; j < n_str_train; j++){
                if (j == this->model->sv_indices[svcount] - 1){
                    x[j].value = test_K[i * num_sv + svcount];
                    svcount++;
                }
            }
            x[n_str_train].index = -1;
        } else if (this->kernel_type == LINEAR || this->kernel_type == RBF) {
            for (int j = 0; j < n_str_train; j++){
                x[j].index = j + 1;
                x[j].value = test_K[i * n_str_train + j];
            }
            x[n_str_train].index = -1;
        }

        // probs = [prob_pos, prob_neg], not [prob_neg, prob_pos]
        double probs[2];
        double guess = svm_predict_probability(this->model, x, probs);
        fprintf(auc_file, "%d,%f\n", test_labels[i], probs[0]);

        if (test_labels[i] > 0) {
            pos[pagg] = probs[labelind];
            pagg += 1;
            if (guess < 0) {
                fn++;
            } else {
                tp++;
            }
        } else {
            neg[nagg] = probs[labelind];
            nagg += 1;
            if (guess > 0) {
                fp++;
            } else {
                tn++;
            }
        }

        //printf("guess = %f and test_labels[%d] = %d\n", guess, i, test_labels[i]);
        if ((guess < 0.0 && test_labels[i] < 0) || (guess > 0.0 && test_labels[i] > 0)) {
            correct++;
        }
    }

    fclose(auc_file);

    if (pagg == 0 && metric == "auc") {
        printf("No positive examples were in the test set. AUROC is undefined in this case.\n");
    }

    double tpr = tp / (double) pagg;
    double tnr = tn / (double) nagg;
    double fnr = fn / (double) pagg;
    double fpr = fp / (double) nagg;
    double auc = calculate_auc(pos, neg, pagg, nagg);
    double acc = 100 * correct / (double)  n_str_test;
    if (!this->quiet) {
        printf("Num sequences: %d\n", nagg + pagg);
        printf("Num positive: %d, Num negative: %d\n", pagg, nagg);
        printf("TPR: %f\n", tpr);
        printf("TNR: %f\n", tnr);
        printf("FNR: %f\n", fnr);
        printf("FPR: %f\n", fpr);
    }
    printf("\nAccuracy: %f\n", acc);
    printf("AUROC: %f\n", auc);

    if (metric == "auc") {
        return auc;
    }

    return acc;
}

double FastSK::predict(const string metric) {
    int n_str_train = this->n_str_train;
    int n_str_test = this->n_str_test;
    printf("Predicting labels for %d sequences...\n", n_str_test);
    int *test_labels = this->test_labels;

    int num_sv = this->model->nSV[0] + this->model->nSV[1];
    printf("num_sv = %d\n", num_sv);
    struct svm_node *x = Malloc(struct svm_node, n_str_train + 1);
    int correct = 0;
    // aggregators for finding num of pos and neg samples for auc
    int pagg = 0, nagg = 0;
    double* neg = Malloc(double, n_str_test);
    double* pos = Malloc(double, n_str_test);

    int fp = 0, fn = 0; //counters for false postives and negatives
    int tp = 0, tn = 0; //counters for true postives and negatives
    int labelind = 0;
    for (int i =0; i < 2; i++){
        if (this->model->label[i] == 1)
            labelind = i;
    }

    FILE *auc_file;
    auc_file = fopen("auc_pred_file.txt", "w+");

    int svcount = 0;
    for (int i = 0; i < n_str_test; i++) {
        if (this->kernel_type == FASTSK) {
            for (int j = 0; j < n_str_train; j++){
                x[j].index = j + 1;
                x[j].value = 0;
            }
            svcount = 0;
            for (int j = 0; j < n_str_train; j++) {
                x[j].value = this->K[i * n_str_train + j];
                svcount++;
            }

            x[n_str_train].index = -1;
        } else if (this->kernel_type == LINEAR || this->kernel_type == RBF) {
            for (int j = 0; j < n_str_train; j++){
                x[j].index = j + 1;
                x[j].value = this->K[i * n_str_train + j];
            }
            x[n_str_train].index = -1;
        }

        // probs = [prob_pos, prob_neg], not [prob_neg, prob_pos]
        double probs[2];
        double guess = svm_predict_probability(this->model, x, probs);
        fprintf(auc_file, "%d,%f\n", test_labels[i], probs[0]);

        if (test_labels[i] > 0) {
            pos[pagg] = probs[labelind];
            pagg += 1;
            if (guess < 0) {
                fn++;
            } else {
                tp++;
            }
        } else {
            neg[nagg] = probs[labelind];
            nagg += 1;
            if (guess > 0) {
                fp++;
            } else {
                tn++;
            }
        }

        //printf("guess = %f and test_labels[%d] = %d\n", guess, i, test_labels[i]);
        if ((guess < 0.0 && test_labels[i] < 0) || (guess > 0.0 && test_labels[i] > 0)) {
            correct++;
        }
    }

    fclose(auc_file);

    if (pagg == 0 && metric == "auc") {
        printf("No positive examples were in the test set. AUROC is undefined in this case.\n");
    }

    double tpr = tp / (double) pagg;
    double tnr = tn / (double) nagg;
    double fnr = fn / (double) pagg;
    double fpr = fp / (double) nagg;
    double auc = calculate_auc(pos, neg, pagg, nagg);
    double acc = 100 * correct / (double)  n_str_test;
    if (!this->quiet) {
        printf("Num sequences: %d\n", nagg + pagg);
        printf("Num positive: %d, Num negative: %d\n", pagg, nagg);
        printf("TPR: %f\n", tpr);
        printf("TNR: %f\n", tnr);
        printf("FNR: %f\n", fnr);
        printf("FPR: %f\n", fpr);
    }
    printf("\nAccuracy: %f\n", acc);
    printf("AUROC: %f\n", auc);

    free(this->K);

    if (metric == "auc") {
        return auc;
    }

    return acc;
}

