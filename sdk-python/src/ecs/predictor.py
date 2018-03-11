# coding=utf-8
import pandas as pd
import numpy as np
from get_sets import get_sets

def predict_vm(ecs_lines, input_lines):
    # Do your work from here#
    
    # 存放训练集，测试集，验证集的起止数据
    set_dates =  ["2015-01-01", "2015-05-30"]
    
    # 所需要预测的虚拟机类型
    target_types = ["flavor1", "flavor2", "flavor3", "flavor4", "flavor5",
                  "flavor6", "flavor7", "flavor8", "flavor9", "flavor10",
                  "flavor11", "flavor12", "flavor13", "flavor14", "flavor15"]
    
    # 该函数用于处理原始数据，分割得到训练集，测试集，验证集
    data = get_sets(ecs_lines, set_dates, target_types)
    print data['flavor1']
    
    result = []
    if ecs_lines is None:
        print 'ecs information is none'
        return result
    if input_lines is None:
        print 'input file information is none'
        return result

    return result
