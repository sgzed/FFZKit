
#ifndef FFZKIT_TIMETICKER_H_
#define FFZKIT_TIMETICKER_H_

#include "logger.h"

namespace FFZkit {

class Ticker {
public:
    /**
        * 此对象可以用于代码执行时间统计，以可以用于一般计时
        * @param min_ms 开启码执行时间统计时，如果代码执行耗时超过该参数，则打印警告日志
        * @param ctx 日志上下文捕获，用于捕获当前日志代码所在位置
        * @param print_log 是否打印代码执行时间
        */
    Ticker(uint64_t min_ms = 0,
        LogContextCapture ctx = LogContextCapture(Logger::Instance(), LWarn, __FILE__, "", __LINE__),
        bool print_log = false): _ctx(std::move(ctx)) {
        if (!print_log) {
            _ctx.clear();
        }
        _created = _begin = getCurrentMillisecond();
        _min_ms = min_ms;
    }

    ~Ticker() {
        uint64_t tm = createdTime();
        if (tm > _min_ms) {
            _ctx << "take time: " << tm << "ms" << ", thread may be overloaded";
        }
        else {
            _ctx.clear();
        }
    }

    /**
     * 获取上次resetTime后至今的时间，单位毫秒
     */
    uint64_t elapsedTime() const {
        return getCurrentMillisecond() - _begin;
    }

    /**
     * 获取从创建至今的时间，单位毫秒
     */
    uint64_t createdTime() const {
        return getCurrentMillisecond() - _created;
    }

    void resetTime() {
        _begin = getCurrentMillisecond();
    }

private:
    uint64_t _min_ms; 
    uint64_t _created;
    uint64_t _begin;
    LogContextCapture _ctx;
};


#if !defined(NDEBUG)
#define TimeTicker() Ticker __ticker(5, WarnL, true)
#define TimeTicker1(tm) Ticker __ticker1(tm, WarnL,true)
#define TimeTicker2(tm, log) Ticker __ticker2(tm, log, true)
#else 
#define TimeTicker()
#define TimeTicker1(tm)
#define TimeTicker2(tm,log)
#endif 

} //namesapce FFZKit 


#endif // FFZKIT_TIMETICKER_H_