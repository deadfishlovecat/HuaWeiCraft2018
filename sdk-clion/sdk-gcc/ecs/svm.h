#ifndef _LIBSVM_H
#define _LIBSVM_H


#include <vector>

struct svm_node
{
    int index;
    double value;
};

struct svm_problem
{
    int l;
    std::vector<double> y;
//    struct svm_node **x;
    std::vector<std::vector<svm_node>> x;
};

enum { C_SVC, NU_SVC, ONE_CLASS, EPSILON_SVR, NU_SVR };	/* svm_type */
enum { LINEAR, POLY, RBF, SIGMOID, PRECOMPUTED }; /* kernel_type */

struct svm_parameter
{
    int svm_type;
    int kernel_type;
    int degree;	/* for poly */
    double gamma;	/* for poly/rbf/sigmoid */
    double coef0;	/* for poly/sigmoid */

    /* these are for training only */
    double cache_size; /* in MB */
    double eps;	/* stopping criteria */
    double C;	/* for C_SVC, EPSILON_SVR and NU_SVR */
    int nr_weight;		/* for C_SVC */
    int *weight_label;	/* for C_SVC */
    double* weight;		/* for C_SVC */
    double nu;	/* for NU_SVC, ONE_CLASS, and NU_SVR */
    double p;	/* for EPSILON_SVR */
    int shrinking;	/* use the shrinking heuristics */
    int probability; /* do probability estimates */
};

//
// svm_model
//
struct svm_model
{
    svm_parameter param;	/* parameter */
    int nr_class;		/* number of classes, = 2 in regression/one class svm */
    int l;			/* total #SV */
//    struct svm_node **SV;		/* SVs (SV[l]) */
    std::vector<std::vector<svm_node>> SV;		/* SVs (SV[l]) */
    std::vector<std::vector<double>> sv_coef;	/* coefficients for SVs in decision functions (sv_coef[k-1][l]) */
    std::vector<double> rho;		/* constants in decision functions (rho[k*(k-1)/2]) */
    std::vector<double> probA;		/* pariwise probability information */
    std::vector<double> probB;
    std::vector<int> sv_indices;        /* sv_indices[0,...,nSV-1] are values in [1,...,num_traning_data] to indicate SVs in the training set */

    /* for classification only */

    std::vector<int> label;		/* label of each class (label[k]) */
    std::vector<int> nSV;		/* number of SVs for each class (nSV[k]) */
    /* nSV[0] + nSV[1] + ... + nSV[k-1] = l */
    /* XXX */
    int free_sv;		/* 1 if svm_model is created by svm_load_model*/
    /* 0 if svm_model is created by svm_train */
};

svm_model svm_train(const svm_problem &prob, const svm_parameter &param);
void svm_cross_validation(svm_problem prob, svm_parameter param, int nr_fold, std::vector<double> target);


double svm_predict_values(const svm_model &model, const std::vector<svm_node> x, std::vector<double> &dec_values);
double svm_predict(const svm_model &model, const std::vector<svm_node> x);

#endif /* _LIBSVM_H */