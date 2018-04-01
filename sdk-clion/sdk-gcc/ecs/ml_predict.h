//
// Created by caocongcong on 18-3-31.
//

#ifndef SDK_CLION_ML_PREDICT_H
#define SDK_CLION_ML_PREDICT_H

#include <map>
#include <vector>
#include "type_def.h"
#include "data_preprocess.h"
#include "KNN.h"

const  int split_windows = 6;

//使用线性回归进行预测
//std::map<int, int> predict_by_lr (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day);

//使用KNN进行预测
std::map<int, int> predict_by_knn (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day);

//使用随机森林进行预测
std::map<int, int> predict_by_randomForest (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day);

#endif //SDK_CLION_ML_PREDICT_H