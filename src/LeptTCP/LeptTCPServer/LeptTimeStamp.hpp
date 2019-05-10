#ifndef _LEPTTIMESTAMP_HPP_
#define _LEPTTIMESTAMP_HPP_

//#include <windows.h>
#include<chrono>
using namespace std::chrono;

class LeptTime
{
public:
    //��ȡ��ǰʱ��� (����)
    static time_t getNowInMilliSec()
    {
        return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
    }
};

class LeptTimeStamp
{
public:
    LeptTimeStamp()
    {
        //QueryPerformanceFrequency(&_frequency);
        //QueryPerformanceCounter(&_startCount);
        update();
    }
    ~LeptTimeStamp(){}

    void update()
    {
        //QueryPerformanceCounter(&_startCount);
        _begin = high_resolution_clock::now();
    }
    /**
    *   ��ȡ��ǰ��
    */
    double getElapsedSecond()
    {
        return  getElapsedTimeInMicroSec() * 0.000001;
    }
    /**
    *   ��ȡ����
    */
    double getElapsedTimeInMilliSec()
    {
        return this->getElapsedTimeInMicroSec() * 0.001;
    }
    /**
    *   ��ȡ΢��
    */
    long long getElapsedTimeInMicroSec()
    {
        /*
        LARGE_INTEGER endCount;
        QueryPerformanceCounter(&endCount);
        double  startTimeInMicroSec =   _startCount.QuadPart * (1000000.0 / _frequency.QuadPart);
        double  endTimeInMicroSec   =   endCount.QuadPart * (1000000.0 / _frequency.QuadPart);
        return  endTimeInMicroSec - startTimeInMicroSec;
        */

        return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
    }
protected:
    //LARGE_INTEGER   _frequency;
    //LARGE_INTEGER   _startCount;
    time_point<high_resolution_clock> _begin;
};

#endif // !_LEPTTIMESTAMP_HPP_