#include <iostream>
#include <cstdarg>
#include "logger.h"

using namespace std;

namespace FFZKit {

static const char *LOG_CONST_TABLE[] = {"TRA", "DBG", "INF", "WAR", "ERR"};

Logger *g_default_logger = nullptr;
Logger &getLogger() {
    if(!g_default_logger) {
        g_default_logger = &Logger::Instance();
    }
    return *g_default_logger;
}

///////////////////Logger///////////////////

    INSTANCE_IMP(Logger, exeName())

    Logger::Logger(const std::string& loggerName) {
        _logger_name = loggerName;
        _last_log = make_shared<LogContext>();
        _default_channel = make_shared<ConsoleChannel>("default", LTrace);
    }

    void Logger::add(const std::shared_ptr<LogChannel> & channel) {
        _channels[channel->name()] = std::move(channel);
    }

    void Logger::del(const std::string &name) {
        _channels.erase(name);
    }

    std::shared_ptr<LogChannel> Logger::get(const std::string &name) {
        auto it = _channels.find(name);
        return it != _channels.end() ? it->second : nullptr;
    }

    void Logger::setLevel(LogLevel level) {
        for(auto& chn : _channels) {
            chn.second->setLevel(level);
        }
    }

    void Logger::write(const LogContextPtr &ctx) {
        writeChannels(ctx);
    }

//返回毫秒
    static int64_t timevalDiff(struct timeval &a, struct timeval &b) {
        return (1000 * (b.tv_sec - a.tv_sec)) + ((b.tv_usec - a.tv_usec) / 1000);
    }

    void Logger::writeChannels(const LogContextPtr &ctx) {
        if (ctx->_line == _last_log->_line && ctx->_file == _last_log->_file && ctx->str() == _last_log->str() && ctx->_thread_name == _last_log->_thread_name) {
            //重复的日志每隔500ms打印一次，过滤频繁的重复日志
            ++_last_log->_repeat;
            if (timevalDiff(_last_log->_tv, ctx->_tv) > 500) {
                ctx->_repeat = _last_log->_repeat;
                writeChannels_l(ctx);
            }
            return;
        }
        if (_last_log->_repeat) {
            writeChannels_l(_last_log);
        }
        writeChannels_l(ctx);
    }

    void Logger::writeChannels_l(const LogContextPtr &ctx) {
        if (_channels.empty()) {
            _default_channel->write(*this, ctx);
        } else {
            for (auto &chn : _channels) {
                chn.second->write(*this, ctx);
            }
        }
        _last_log = ctx;
        _last_log->_repeat = 0;
    }



///////////////////LogContext///////////////////
LogContext::LogContext(LogLevel level, const char *file, const char *function, int line,
                       const char *module_name, const char *flag) :
                       _level(level), _file(file), _function(function), _line(line),
                        _module_name(module_name), _flag(flag) {

    gettimeofday(&_tv, nullptr);
    _thread_name = getThreadName();
}

const string &LogContext::str() {
    if(_got_content) {
        return _content;
    }
    _content = ostringstream::str();
    _got_content = true;
    return _content;
}

///////////////////LogContextCapture///////////////////

static string s_module_name = exeName(false);

LogContextCapture::LogContextCapture(Logger& logger, LogLevel level, const char *file, const char *function, int line,
    const char *flag) :_ctx(new LogContext(level, file, function, line, s_module_name.c_str(), flag)), _logger(logger) {

}

LogContextCapture::~LogContextCapture() {
    *this << endl;
}

LogContextCapture &LogContextCapture::operator<<(std::ostream &(*f)(std::ostream &)) {
    if(!_ctx) {
        return *this;
    }
    _logger.write(_ctx);
    _ctx.reset();
    return *this;
}

void LogContextCapture::clear() {
    _ctx.reset();
}

//////////////////LogChannel///////////////////

LogChannel::LogChannel(const string& name, LogLevel level):_name(name), _level(level) { }

LogChannel::~LogChannel() { }

const string &LogChannel::name() const {
    return _name;
}

void LogChannel::setLevel(LogLevel level) {
    _level = level;
}

string LogChannel::printTime(const timeval &tv) {
    auto tm = getLocalTime(tv.tv_sec);
    char buf[128];
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d.%03d",
             1900 + tm.tm_year,
             1 + tm.tm_mon,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             (int) (tv.tv_usec / 1000));
    return buf;
}

#ifdef _WIN32
#define printf_pid() GetCurrentProcessId()
#else
#define printf_pid() getpid()
#endif

void LogChannel::format(const Logger& logger, std::ostream& ost, const LogContextPtr& ctx, bool enable_detail) {
    if (!enable_detail && ctx->str().empty()) {
        // 没有任何信息打印
        return;
    }

    // print log time and level
#ifdef _WIN32
    ost << printTime(ctx->_tv) << " ";
#else
    ost << printTime(ctx->_tv) << " " << LOG_CONST_TABLE[ctx->_level] << " ";
#endif

    if (enable_detail) {
        // tag or process name
        ost << "[" << (!ctx->_flag.empty() ? ctx->_flag : logger.getName()) << "] ";
        // pid and thread_name
        ost << "[" << printf_pid() << "-" << ctx->_thread_name << "] ";
        // source file location
        ost << ctx->_file << ":" << ctx->_line << " " << ctx->_function << " | ";
    }

    // log content
    ost << ctx->str();
    if (ctx->_repeat > 1) {
        // log repeated
        ost << "\r\n    Last message repeated " << ctx->_repeat << " times";
    }

    // flush log and new line
    ost << endl;
}

///////////////////ConsoleChannel///////////////////

ConsoleChannel::ConsoleChannel(const string &name, LogLevel level) : LogChannel(name, level) {}

void ConsoleChannel::write(const Logger& logger, const LogContextPtr& ctx){
    if (_level > ctx->_level) {
        return;
    }

#if defined(OS_IPHONE)
        //ios禁用日志颜色
    format(logger, std::cout, ctx);
#elif defined(ANDROID)
        static android_LogPriority LogPriorityArr[10];
    static onceToken s_token([](){
        LogPriorityArr[LTrace] = ANDROID_LOG_VERBOSE;
        LogPriorityArr[LDebug] = ANDROID_LOG_DEBUG;
        LogPriorityArr[LInfo] = ANDROID_LOG_INFO;
        LogPriorityArr[LWarn] = ANDROID_LOG_WARN;
        LogPriorityArr[LError] = ANDROID_LOG_ERROR;
    });
    __android_log_print(LogPriorityArr[ctx->_level],"JNI","%s %s",ctx->_function.data(),ctx->str().data());
#else
        //linux/windows日志启用颜色并显示日志详情
        format(logger, std::cout, ctx);
#endif
    }

///////////////////////////////////////////////////////////////////////////////////////////////

void LoggerWrapper::printLog(Logger &logger, int level, const char *file, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    LogContextCapture info(logger, (LogLevel) level, file, function, line);
    char *str = nullptr;
    if (vasprintf(&str, fmt, ap) >= 0 && str) {
        info << str;
#ifdef ASAN_USE_DELETE
        delete [] str; // 开启asan后，用free会卡死
#else
        free(str);
#endif
    }
    va_end(ap);
}


} // namespace FFZKit