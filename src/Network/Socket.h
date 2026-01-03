#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include <string>
#include <memory>
#include <exception>
#include <sstream>

#include "Util/SpeedStatistic.h"
#include "sockutil.h"
#include "Poller/EventPoller.h"

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

    int rawFd() const {
        return fd_;
    }

    SockType type() const {
        return type_;
    }

private:
    int fd_; 
    SockType type_;
}; 

//socket文件描述符的包装
//在析构时自动移除监听并close套接字，防止描述符溢

class SockFD : public noncopyable {
public:
    using Ptr = std::shared_ptr<SockFD>;

     /**
     * 创建一个fd对象
     * @param num 文件描述符，int数字
     * @param poller 事件监听器
     * Create an fd object
     * @param num File descriptor, int number
     * @param poller Event listener
     */
    SockFD(SockNum::Ptr num, EventPoller::Ptr poller)
        : sock_num_(num), poller_(std::move(poller)) {}

     /**
     * 复制一个fd对象
     * @param that 源对象
     * @param poller 事件监听器
     * Copy an fd object
     * @param that Source object
     * @param poller Event listener
     */

    SockFD(const SockFD &that, EventPoller::Ptr poller) {
        sock_num_ = that.sock_num_;
        poller_ = std::move(poller);
        if(poller_ == that.poller_) {
            throw std::invalid_argument("Copy a SockFD with same poller");
        }
    }

    ~SockFD() {
        delEvent();
    }

     /**
     * 移除事件监听
     * Remove event listener
     */

    void delEvent() {
        if(poller_) {
            auto num = sock_num_;
            poller_->delEvent(num->rawFd(), [num](bool) {});
            poller_ = nullptr;
        }
    }

    int rawFd() const {
        return sock_num_->rawFd();
    } 

    SockNum::SockType type() const {
        return sock_num_->type();
    }

    const SockNum::Ptr &sockNum() const {
        return sock_num_;
    }

    const EventPoller::Ptr& getPoller() const {
        return poller_;
    }

private:
    SockNum::Ptr sock_num_;
    EventPoller::Ptr poller_;
};

class SockInfo {
public:
    SockInfo() = default;
    virtual ~SockInfo() = default;

    //获取本机ip  [AUTO-TRANSLATED:02d3901d]
    //Get local IP
    virtual std::string get_local_ip() = 0;
    //获取本机端口号  [AUTO-TRANSLATED:f883cf62]
    //Get local port number
    virtual uint16_t get_local_port() = 0;
    //获取对方ip  [AUTO-TRANSLATED:f042aa78]
    //Get peer IP
    virtual std::string get_peer_ip() = 0;
    //获取对方端口号  [AUTO-TRANSLATED:0d085eca]
    //Get the peer's port number
    virtual uint16_t get_peer_port() = 0;
    //获取标识符  [AUTO-TRANSLATED:e623608c]
    //Get the identifier
    virtual std::string getIdentifier() const { return ""; }
};

#define TraceP(ptr) TraceL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define DebugP(ptr) DebugL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define InfoP(ptr) InfoL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define WarnP(ptr) WarnL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "
#define ErrorP(ptr) ErrorL << ptr->getIdentifier() << "(" << ptr->get_peer_ip() << ":" << ptr->get_peer_port() << ") "


//异步IO Socket对象，包括tcp客户端、服务器和udp套接字
class Socket : public std::enable_shared_from_this<Socket>, public SockInfo {
public:
    using Ptr = std::shared_ptr<Socket>;

};


}  // FFZKit

#endif // NETWORK_SOCKET_H