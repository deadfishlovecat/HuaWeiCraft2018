//
// Created by ruiy on 18-3-13.
//
#include <stdio.h>
#include "date_utils.h"

time_t convert(int year,int month,int day)
{
    tm info={0};
    info.tm_year=year-1900;
    info.tm_mon=month-1;
    info.tm_mday=day;
    return mktime(&info);
}

int get_days(const char *from, const char *to)
{
    int year,month,day;
    sscanf(from,"%d-%d-%d",&year,&month,&day);
    int fromSecond=(int)convert(year,month,day);
    sscanf(to,"%d-%d-%d",&year,&month,&day);
    int toSecond=(int)convert(year,month,day);
    return (toSecond-fromSecond)/24/3600;
}