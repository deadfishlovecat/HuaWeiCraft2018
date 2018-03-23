//
// Created by ruiy on 18-3-16.
//

#ifndef SDK_GCC_AR_H
#define SDK_GCC_AR_H

#include <vector>
#include <cmath>
#include <string>
#include "type_def.h"
class AR {
private:

    std::vector<Double> get_auto_cov();
    std::vector<Double> get_auto_cor(std::vector<Double> auto_cov);

    std::vector<Double> ic_vals; // 如果是aic或者bic，那么这里就是对应的一串值
    std::vector<Double> auto_cov; // 自协方差
    std::vector<Double> auto_cor; // 自相关系数
    std::vector<Double> bias_cor; // 偏自相关系数
    std::vector<Double> data; // 所要预测的序列
    std::vector<Double> noise_var; // 白噪声方差, [0..len-1], len为数据长度
    int best_p; // 滞后阶数
    std::vector<Double> a; // 存放滞后p阶时的自回归系数, [0..p-1]
    std::vector<Double> res; // 存放预测结果,若预测接下来k天，则为[0..k-1]

public:
    AR(std::vector<Double>);
    void fit(std::string ic="none", int max_lag=-1);
    std::vector<Double> predict(int k, int diff_day=0);
    void print_model_info();

    Double get_bias();
    std::pair<std::vector<Double>, Double> least_squares(std::vector<double> data, int lag);
    std::pair<std::vector<std::vector<Double>>, std::vector<std::vector<Double>>> format_data(std::vector<double> data, int lag);

    inline int get_p () {return best_p;}
    inline std::vector<double> get_res () {return res;}
};


#endif //SDK_GCC_AR_H
