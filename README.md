# 2018华为软件精英挑战赛初赛解决方案 
Solution for 2018 Huawei Software Craft Preliminary Contest

## 前言

## 算法思路

### 高分思路

这里介绍两个思路，第一个思路是正经的做最高的分的思路, 第二个思路是不正经的做最高的分的思路

* 纯预测方法纯分配下，我们的实验效果是AR模型坠吼(但这并不是我们最高分代码所使用的模型)，思路如下:
  * 先滑动平均(阶数6)
  * AR预测结果
  * 背包分配
  * 遗传算法(初始种群的个体使用的是背包的结果)
  * 补充算法: 分配结果中如果服务器(箱子)还有剩余空间, 那么则修改预测结果, 新增虚拟机, 然后接着装入服务器(先找mem最大的放入)
  * 这一趟跑完, 测试阶段有80.37分，比赛阶段有233.186分
* 我们的最高分代码(不正经的思路)。思路如下：
  * 先滑动平均(阶数6)
  * SVM预测结果 **乘以** 1.1, 是的, 你没看错, 就是这么粗暴的**乘以**1.1
  * 背包分配
  * 遗传算法
  * 补充算法: 同上
  * 这一趟跑完, 测试阶段有85.982分, 比赛阶段有244.172分
  
### 其他算法思路

这里介绍其他几种我们实现的，但是效果不怎么出色的算法。

预测尝试了各种机器学习的算法, 但是并不怎么理想，从最后结果看，其实还是线性模型最好。

分配其实既不是一维装箱问题, 也不是二维装箱问题, 有一个更专有的名词形容它, 叫做向量装箱问题(Vector Bin Packing)。这里我们找到了一篇微软的论文,
其中的思路是FFD的变种，其实就是将二个维度资源约束进行加权得到一维资源约束，然后再利用FFD的一种变种进行处理。我们实现了论文中的算法, 结果还不如背
包裹。然后还尝试了退火，效果也不怎么样。

* 预测:
  * 随机森林
  * Knn
  * 决策树
* 分配
  * FFD的各种变种
  * 退火

除了以上思路之外，我们预测方法还尝试了随机森林、Knn、线性回归、决策树等方案, 分配方法参考了大量向量装箱问题()

## 版本说明

v1.0.0 是初赛最后的提交版本, 里面包含了开赛以来截至到初赛结束编写的所有方法的代码
v1.1.0 是预测只保留了AR模型、svm模型, 分配只保留了背包、分组遗传的纯净版


## 目录结构

这里只对重要的文件进行了说明

```
.
├── data                  % 数据文件, 包括输入文件, 输出文件, 测试数据集均存放于此
├── reference             % 参考文献
├── sdk-clion             % c++ 项目代码, 比赛所提交代码基本在此
│   ├── CMakeLists.txt    % 自己编写的CMakeLists.txt, 使得可以在CLion下进行方便的开发
│   └── sdk-gcc
│       ├── bin
│       │   └── ecs       % build.sh脚本生成的二进制文件
│       ├── build         % 构建目录, build.sh脚本构建中间文件存放于此
│           ├── ...
│       ├── build.sh
│       ├── ecs
│       │   ├── type_def.cpp                 % 一些基本的定义
│       │   └── type_def.h
│       │   ├── BasicInfo.cpp                
│       │   ├── BasicInfo.h                  % 基本信息 - 存放输入文件中的基本信息
│       │   ├── math_utils.cpp               % 数学工具 - 各种数据处理工具, 比如矩阵LU算法求逆等等
│       │   ├── math_utils.h
│       │   ├── Random.cpp                   % 数学工具 - 随机数相关的类, 这里单独提出来做了一个随机数类
│       │   ├── Random.h
│       │   ├── get_data.cpp                 % 数据处理 - 对序列数据根据时间切分
│       │   ├── get_data.h
│       │   ├── data_format_change.cpp       % 数据处理 - 接口之间的数据转换
│       │   ├── data_format_change.h         
│       │   ├── data_preprocess.cpp          % 数据处理 - 机器学习算法所需要的一些数据格式的转换
│       │   ├── data_preprocess.h
│       │   ├── date_utils.cpp               % 数据处理 - 日期处理
│       │   ├── date_utils.h
│       │   ├── ma.cpp                       % 数据处理 - 滑动平均
│       │   ├── ma.h
│       │   ├── AR.cpp                       % 预测 - AR类, 参考sklearn中的AR(不是arima，这里只有ar)库编写
│       │   ├── AR.h                       
│       │   ├── ar_variant.cpp               % 预测 - 基于AR模型的预测方法
│       │   ├── ar_variant.h 
│       │   ├── SVR.cpp                      % 预测 - svr预测，这里只实现了线性核
│       │   ├── SVR.h
│       │   ├── ml_predict.cpp               % 预测 - 基于机器学习的预测方法
│       │   ├── ml_predict.h
│       │   ├── ff.cpp                       % 分配 - first_fit算法
│       │   ├── ff.h                         
│       │   ├── FFD.cpp                      % 分配 - first_fit_decrease算法
│       │   ├── FFD.h
│       │   ├── packing.cpp                  % 分配 - 背包
│       │   ├── packing.h         
│       │   ├── Bin.cpp                      % 分配 - 箱子类
│       │   ├── Bin.h
│       │   ├── Chromo.cpp                   % 分配 - 遗传算法中的染色体类
│       │   ├── Chromo.h 
│       │   ├── GGA.cpp                      % 分配 - 分组遗传算法(Group Genetic Algorithm)
│       │   ├── GGA.h
│       │   ├── CMakeLists.txt               % 官方提供 - 官方提供的CMakeLists.txt
│       │   ├── ecs.cpp                      % 官方提供 - 文件处理: 文件的读入, 文件的写入
│       │   ├── io.cpp                       % 官方提供
│       │   ├── lib                          % 官方提供
│       │   │   ├── lib_io.h
│       │   │   └── lib_time.h
│       │   ├── predict.cpp                  % 官方提供 - 主函数
│       │   ├── predict.h
│       ├── ecs.tar.gz                       % 结果 - 要提交的压缩包
│       ├── README.md                        % 官方提供 - 对sdk-gcc的readme
│       └── readme.txt                       % 官方提供 - 对sdk-gcc的readme

└── sdk-python            % python 项目代码, 用于本地实验所用
```
基本主要的代码是`sdk-clion`, sdk-clion的文件说明如上, sdk-clion下一级目录是sdk-gcc即官方提供的开发包, 由于不方便, 因此我又对其进行了一层包装,
使其可以在CLion下进行开发, 并编写了对应的CMakeLists.txt。可执行文件的参数中CLion中进行配置，因此开发调试非常方便，并且也没有更改sdk-gcc中官方
不允许修改的文件。

## 补充说明

1. sdk-clion是CLion项目根目录，导入后重新检出sdk-CLion目录下的CMakeLists.txt即可编译调试。