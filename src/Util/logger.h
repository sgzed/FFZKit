#ifndef ZZKIT_LOGGER_H
#define ZZKIT_LOGGER_H

#include <memory>
#include <map>
#include <fstream>

#include "util.h"

namespace FFZKit {

class LogContext;
class LogChannel;
class Logger;

using LogContextPtr = std::shared_ptr<LogContext>;

Logger &getLogger();

typedef enum {
    LTrace = 0, LDebug, LInfo, LWarn, LError
} LogLevel;

class Logger : public std::enable_shared_from_this<Logger>, public noncopyable {
public:
    using Ptr = std::shared_ptr<Logger>;
    
    /* 
     * 获取日志实例
    */
    static Logger &Instance();

    ~Logger() = default;

    const std::string &getName() const {
        return _logger_name;
    }

    void add(const std::shared_ptr<LogChannel> & channel);

    void del(const std::string &name);

    std::shared_ptr<LogChannel> get(const std::string &name);

    void setLevel(LogLevel level);

    void write(const LogContextPtr &ctx);

protected:
    explicit Logger(const std::string& loggerName);

private:
    void writeChannels(const LogContextPtr &ctx);
    void writeChannels_l(const LogContextPtr &ctx);

private:
    std::string  _logger_name;
    LogContextPtr _last_log;
    std::shared_ptr<LogChannel> _default_channel;
    std::map<std::string, std::shared_ptr<LogChannel>> _channels;
};

///////////////////LogContext///////////////////
class LogContext : public std::ostringstream {
public:
    LogContext() = default;
    LogContext(LogLevel level, const char *file, const char *function, int line, const char *module_name,
               const char *flag);

    ~LogContext() override = default;

    const std::string &str();

    LogLevel  _level;
    int _line;
    int _repeat = 0;
    std::string _file;
    std::string _function;
    std::string _module_name;
    std::string _flag;
    std::string _thread_name;
    struct timeval _tv;

private:
    bool _got_content = false;
    std::string _content;
};

class LogContextCapture {
public:
    using Ptr = std::shared_ptr<LogContextCapture>;

    LogContextCapture(Logger& logger, LogLevel level, const char *file, const char *function, int line,
                      const char *flag = "");

    ~LogContextCapture();

    /**
   * 输入std::endl(回车符)立即输出日志
   * @param f std::endl(回车符)
   * @return 自身引用
   */
    LogContextCapture &operator<<(std::ostream &(*f)(std::ostream &));

    template<class T>
    LogContextCapture &operator<<(T &&data) {
        if(!_ctx) {
            return *this;
        }
        (*_ctx) << std::forward<T>(data);
        return *this;
    }

    void clear();

private:
    LogContextPtr _ctx;
    Logger &_logger;
};

///////////////////LogChannel///////////////////

class LogChannel : public  noncopyable {
public:
    LogChannel(const std::string& name, LogLevel level = LTrace);
    virtual ~LogChannel() ;

    virtual void write(const Logger& logger, const LogContextPtr& ctx) = 0;
    const std::string &name() const;
    void setLevel(LogLevel level);
    static std::string printTime(const timeval &tv);

protected:
    virtual void format(const Logger& logger, std::ostream& ost, const LogContextPtr& ctx, bool enable_detail = true);

protected:
    std::string _name;
    LogLevel _level;
};

class ConsoleChannel : public LogChannel {
public:
    ConsoleChannel(const std::string &name = "ConsoleChannel", LogLevel level = LTrace);
    ~ConsoleChannel() override = default;

    void write(const Logger& logger, const LogContextPtr& ctx) override;
};

class BaseLogFlagInterface {
protected:
    virtual ~BaseLogFlagInterface() {}
    // 获得日志标记Flag
    const char* getLogFlag(){
        return _log_flag;
    }
    void setLogFlag(const char *flag) { _log_flag = flag; }
private:
    const char *_log_flag = "";
};


//可重置默认值
extern Logger *g_defaultLogger;

//用法: DebugL << 1 << "+" << 2 << '=' << 3;  [AUTO-TRANSLATED:e6efe6cb]
//Usage: DebugL << 1 << "+" << 2 << '=' << 3;
#define WriteL(level) ::FFZKit::LogContextCapture(::FFZKit::getLogger(), level, __FILE__, __FUNCTION__, __LINE__)
#define TraceL WriteL(::FFZKit::LTrace)
#define DebugL WriteL(::FFZKit::LDebug)
#define InfoL WriteL(::FFZKit::LInfo)
#define WarnL WriteL(::FFZKit::LWarn)
#define ErrorL WriteL(::FFZKit::LError)

//只能在虚继承BaseLogFlagInterface的类中使用
#define WriteF(level) ::FFZKit::LogContextCapture(::FFZKit::getLogger(), level, __FILE__, __FUNCTION__, __LINE__, getLogFlag())
#define TraceF WriteF(::FFZKit::LTrace)
#define DebugF WriteF(::FFZKit::LDebug)
#define InfoF WriteF(::FFZKit::LInfo)
#define WarnF WriteF(::FFZKit::LWarn)
#define ErrorF WriteF(::FFZKit::LError)

//用法: PrintD("%d + %s = %c", 1 "2", 'c');
#define PrintLog(level, ...) ::FFZKit::LoggerWrapper::printLog(::FFZKit::getLogger(), level, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PrintT(...) PrintLog(::FFZKit::LTrace, ##__VA_ARGS__)
#define PrintD(...) PrintLog(::FFZKit::LDebug, ##__VA_ARGS__)
#define PrintI(...) PrintLog(::FFZKit::LInfo, ##__VA_ARGS__)
#define PrintW(...) PrintLog(::FFZKit::LWarn, ##__VA_ARGS__)
#define PrintE(...) PrintLog(::FFZKit::LError, ##__VA_ARGS__)

} // namespace FFZKit 


#endif