//
// Created by ruiy on 18-4-20.
//

#ifndef SDK_CLION_SA_H
#define SDK_CLION_SA_H
#include "Bin.h"

class SA {
    std::vector<Bin> initial_bins;
    double calc_cost(const std::vector<Bin> &bins);
    std::vector<std::vector<Bin>> one_way_transfer(const std::vector<Bin> &bins);
    double calc_t0();
    double t0;
    double alpha;
    double beta;
public:
    SA(std::vector<Bin> initial_bins, double alpha, double beta);
    void start();
};


#endif //SDK_CLION_SA_H
