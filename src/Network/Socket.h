#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include <string>
#include <memory>
#include <exception>
#include <sstream>

#include "Util/SpeedStatistic.h"
#include "sockutil.h"

namespace FFZKit {

#if defined(MSG_NOSIGNAL)
#define FLAG_NOSIGNAL MSG_NOSIGNAL
#else
#define FLAG_NOSIGNAL 0
#endif //MSG_NOSIGNAL

#if defined(MSG_MORE)
#define FLAG_MORE MSG_MORE
#else
#define FLAG_MORE 0
#endif //MSG_MORE

#if defined(MSG_DONTWAIT)
#define FLAG_DONTWAIT MSG_DONTWAIT
#else
#define FLAG_DONTWAIT 0
#endif //MSG_DONTWAIT

//默认的socket flags:不触发SIGPIPE,非阻塞发送
#define SOCKET_DEFAULE_FLAGS (FLAG_NOSIGNAL | FLAG_DONTWAIT )

//发送超时时间，如果在规定时间内一直没有发送数据成功，那么将触发onErr事件
#define SEND_TIME_OUT_SEC 10

//错误类型枚举  [AUTO-TRANSLATED:c85ff6f6]
//Error type enumeration
typedef enum {
    Err_success = 0, //成功 success
    Err_eof, //eof
    Err_timeout, //超时 socket timeout
    Err_refused,//连接被拒绝 socket refused
    Err_reset,//连接被重置  socket reset
    Err_dns,//dns解析失败 dns resolve failed
    Err_shutdown,//主动关闭 socket shutdown
    Err_other = 0xFF,//其他错误 other error
} ErrCode;

//错误信息类
class SockException : public std::exception {
public:
    SockException(ErrCode err = Err_success, const std::string &what = "")
            : code_(err), msg_(what) {}     

    // 重置错误
    void reset(ErrCode code, const std::string &what) {
        code_ = code;
        msg_ = what;
    }

    ErrCode getErrCode() const {
        return code_;
    }

    // Error prompt
    const char *what() const noexcept override {
        return msg_.c_str();
    }

    operator bool() const {
        return code_ != Err_success;
    }

private:
    ErrCode code_;
    std::string msg_;
};

std::ostream &operator<<(std::ostream &ost, const SockException &err);

class SockNum {
public:
    using Ptr = std::shared_ptr<SockNum>;

    typedef enum {
        Sock_Invalid = -1,
        Sock_TCP = 0,
        Sock_UDP = 1,
        Sock_TCP_Server = 2
    } SockType;

    SockNum(int fd, SockType type) : 
        fd_(fd), type_(type) {}

    ~SockNum() {
        // 停止socket收发能力
        #if defined(_WIN32)
        ::shutdown(fd_, SD_BOTH);
        #else
        ::shutdown(fd_, SHUT_RDWR);
        #endif
        // 关闭socket句柄
        ::close(fd_);
    }

private:
    int fd_; 
    SockType type_;
}; 

}  // FFZKit

#endif // NETWORK_SOCKET_H