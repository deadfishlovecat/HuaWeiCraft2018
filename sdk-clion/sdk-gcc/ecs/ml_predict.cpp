//
// Created by caocongcong on 18-3-31.
//

#include <cstring>
#include "ml_predict.h"
#include "SVR.h"

std::map<int, int> predict_by_knn (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
        std::map<std::vector<double>, double> train_data_need = timeseries_to_supervised(ecs_data, split_windows, mv_flag);
        std::vector<double> frist_predict_data = get_frist_predict_data(ecs_data, split_windows, mv_flag);
//        result[t.first] = 1;
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = knn_regresion_brust(train_data_need,frist_predict_data, 4);
            frist_predict_data.erase(frist_predict_data.begin());
            frist_predict_data.push_back(tmp_predict);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
            //result[t.first] = predict_ecs_data[0]*need_predict_day*2;
            result[t.first] = (int) ecs_sum;
    }

    return result;
}

std::map<int, int> predict_by_knn_method2 (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
        std::vector<std::vector<double>> train_serial = timeseries_to_supervised_data(ecs_data, split_windows, mv_flag);
        std::vector<double> target_serial = timeseries_to_supervised_target(ecs_data, split_windows, mv_flag);
        std::vector<std::vector<double>> train = get_vector_train_method2(train_serial,need_predict_day);
        std::vector<double> target = get_vector_target_method2(target_serial,need_predict_day);
        std::vector<std::vector<double>> test = get_vector_test_method2(train_serial,need_predict_day);
        std::map<std::vector<double>, double> train_data_use;
        for(int i=0; i<train.size(); i++)
        {
            train_data_use[train[i]] = target[i];
        }
        //printf("获取数据成功\n");
        //依次是树的数量，每课树的特征，树的最大深度，每个叶节点的最大样本数，最小的下降不纯度
        //printf("训练成功\n");
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = knn_regresion_brust(train_data_use, test[i],4);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
        result[t.first] = (int)ecs_sum;
        //result[t.first] = (int)(predict_ecs_data[need_predict_day-1]*need_predict_day);
        //result[t.first] = get_bigger_mean(predict_ecs_data, need_predict_day/2)*need_predict_day;
    }

    return result;
}

std::map<int, int> predict_by_cart (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        printf("训练第%d种服务器：\n",t.first);
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
        std::vector<double> frist_predict_data = get_frist_predict_data(ecs_data, split_windows, mv_flag);
        std::vector<std::vector<double>> train = timeseries_to_supervised_data(ecs_data, split_windows, mv_flag);
        std::vector<double> target = timeseries_to_supervised_target(ecs_data, split_windows, mv_flag);
        decision_tree df(6,2,0.0);
        df.train(train,target);
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = df.predict(frist_predict_data);
            frist_predict_data.erase(frist_predict_data.begin());
            frist_predict_data.push_back(tmp_predict);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
        result[t.first] = (int)(predict_ecs_data[need_predict_day-1]*need_predict_day);
    }

    return result;
}

std::map<int, int> predict_by_randomForest (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        //printf("训练第%d种服务器：\n",t.first);
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
//        std::map<std::vector<double>, double> train_data_need = timeseries_to_supervised(ecs_data, split_windows, mv_flag);
//        std::vector<std::vector<double>> train = get_vector_train(train_data_need);
//        std::vector<double> target = get_vector_target(train_data_need);
        std::vector<std::vector<double>> train = timeseries_to_supervised_data(ecs_data, split_windows, mv_flag);
        std::vector<double> target = timeseries_to_supervised_target(ecs_data, split_windows, mv_flag);
        std::vector<double> frist_predict_data = get_frist_predict_data(ecs_data, split_windows, mv_flag);
        //依次是树的数量，每课树的特征，树的最大深度，每个叶节点的最大样本数，最小的下降不纯度
        //50,4,7,1,1.0 72.6
        RandomForest rf(50,4,7,1,1.0, sqrt(train.size()));
        //RandomForest rf(100,7,5,3,1.0, sqrt(train.size()));
        rf.train(train,target);
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = rf.predict(frist_predict_data);
            frist_predict_data.erase(frist_predict_data.begin());
            frist_predict_data.push_back(tmp_predict);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
        result[t.first] = (int)(predict_ecs_data[need_predict_day-1]*need_predict_day);
        //result[t.first] = get_bigger_mean(predict_ecs_data, need_predict_day/2)*need_predict_day;
    }

    return result;
}

std::map<int, int> predict_by_randomForest_method2 (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        //printf("训练第%d种服务器：\n",t.first);
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
        std::vector<std::vector<double>> train_serial = timeseries_to_supervised_data(ecs_data, split_windows, mv_flag);
        std::vector<double> target_serial = timeseries_to_supervised_target(ecs_data, split_windows, mv_flag);
        std::vector<std::vector<double>> train = get_vector_train_method2(train_serial,need_predict_day);
        std::vector<double> target = get_vector_target_method2(target_serial,need_predict_day);
        std::vector<std::vector<double>> test = get_vector_test_method2(train_serial,need_predict_day);
        //printf("获取数据成功\n");
        //依次是树的数量，每课树的特征，树的最大深度，每个叶节点的最大样本数，最小的下降不纯度
        RandomForest rf(50,3,5,2,1.0,train.size()-train.size()/4);
        rf.train(train,target);
        //printf("训练成功\n");
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = rf.predict(test[i]);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
        result[t.first] = (int)ecs_sum;
        //result[t.first] = (int)(predict_ecs_data[need_predict_day-1]*need_predict_day);
        //result[t.first] = get_bigger_mean(predict_ecs_data, need_predict_day/2)*need_predict_day;
    }

    return result;
}


//使用线性回归进行预测

std::map<int, int> predict_by_LR (std::map<int, Vm> vm_info, std::map<int, std::vector<double>> train_data, int need_predict_day)
{
    std::map<int,int>result;
    for (auto &t: vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        //printf("训练第%d种服务器：\n",t.first);
        bool mv_flag = true;
        int split_windows = get_split_window(ecs_data);
        std::vector<std::vector<double>> train = timeseries_to_supervised_data(ecs_data, split_windows, mv_flag);
        std::vector<double> target = timeseries_to_supervised_target(ecs_data, split_windows, mv_flag);
        std::vector<double> frist_predict_data = get_frist_predict_data(ecs_data, split_windows, mv_flag);
        LR lr(train, target);
        lr.train();
        double ecs_sum = 0.0;
        std::vector <double> predict_ecs_data;
        for(int i=0; i < need_predict_day; i++)
        {
            double tmp_predict = lr.predict(frist_predict_data);
            if(tmp_predict<0.0)
            {
                tmp_predict = 0.0;
            }
            frist_predict_data.erase(frist_predict_data.begin());
            frist_predict_data.push_back(tmp_predict);
            predict_ecs_data.push_back(tmp_predict);
            ecs_sum += tmp_predict;
        }
        result[t.first] = std::round(ecs_sum);
    }

    return result;
}


int get_split_window(std::vector<double> data)
{
    if (unchangale)
    {
        return 10;
    }
    else
    {
        int tmp;
        tmp = int(round(12 * pow((data.size() / 100.0), 1.0/4)));
        return tmp;
    }
}


int get_bigger_mean(std::vector<double> data, int num)
{
    std::sort(data.begin(),data.end());
    double sum = 0.0;
    for(int i=0; i< num; i++)
    {
        sum+=data[i];
    }
    return (int)sum/num;
}


svm_parameter init_svm_parameter()
{
    svm_parameter param;

    // default values

    param.nu = 0.5;
    param.C = 0.13;
    param.eps = 1e-3;
    param.shrinking = 1;


    return param;
}



/**
 * 使用svm进行预测
 * @param train_data
 * @return
 */

std::map<int, int> predict_by_svm (std::map<int, std::vector<double>> train_data){

    std::map<int,int> result;

    for (auto &t: BasicInfo::vm_info) {
        std::vector<double> ecs_data = train_data[t.first];
        printf("训练第%d种服务器：\n",t.first);
        /* 1. 准备训练集合*/
        bool mv_flag = true;

        int split_windows = 10;
        std::map<std::vector<double>, double> train_data_need = timeseries_to_supervised(ecs_data, split_windows, mv_flag);
        std::vector<std::vector<double>> train_x = get_vector_train(train_data_need);
        std::vector<double> train_y = get_vector_target(train_data_need);

        /* 2. 初始化问题*/
//        svm_problem prob = init_svm_problem(train_x, train_y);     // 打包训练样本
        svm_parameter param = init_svm_parameter();   // 初始化训练参数

        param.C = 0.13;

        /* 3. 训练模型 */
//        svm_model model = svm_train(prob, param);
        SVR svr(train_x, train_y, param);
        svr.train();

        /* 4. 获取所需要的特征 */
        std::vector<double> frist_predict_data = get_frist_predict_data(ecs_data, split_windows, mv_flag);

        /* 5. 开始预测 */
        std::vector<double> predict_ecs_data;
        for(int i=0; i < BasicInfo::need_predict_cnt; i++)
        {
            double tmp_predict = svr.predict(frist_predict_data);

            /* 6. 构造新的预测所需特征 */
            frist_predict_data.erase(frist_predict_data.begin());
            frist_predict_data.push_back(tmp_predict);

            /* 7. 存储预测结果 */
            predict_ecs_data.push_back(tmp_predict);
        }

        double ecs_sum = std::accumulate(predict_ecs_data.begin(), predict_ecs_data.end(), 0.0);
        result[t.first] = round(std::max(0.0, ecs_sum));
//        result[t.first] = (int)(predict_ecs_data[BasicInfo::need_predict_day-1]*BasicInfo::need_predict_day);
    }

    return result;

};

