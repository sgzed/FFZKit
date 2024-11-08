#ifndef ZZKIT_LOGGER_H
#define ZZKIT_LOGGER_H

#include <memory>

#include "util.h"

namespace FFZKit {

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

protected:
    explicit Logger(const std::string& loggerName) {}
};

    
} // namespace FFZKit 


#endif