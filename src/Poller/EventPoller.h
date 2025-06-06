//
// Created by FFZero on 2025-01-18.
//

#ifndef FFZKIT_EVENTPOLLER_H
#define FFZKIT_EVENTPOLLER_H

#include <memory>

#include "Util/logger.h"
#include "Thread/TaskExecutor.h"

#if defined(__linux__) || defined(__linux)
#define HAS_EPOLL
#endif //__linux__

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define HAS_KQUEUE
#endif // __APPLE__


namespace FFZKit {

class EventPoller : public TaskExecutor, public std::enable_shared_from_this<EventPoller>  {
public:
    using Ptr = std::shared_ptr<EventPoller>;

    typedef enum {
        Event_Read = 1 << 0, //读事件
        Event_Write = 1 << 1, //写事件
        Event_Error = 1 << 2, //错误事件
        Event_LT = 1 << 3,//水平触发
    } Poll_Event;

    ~EventPoller();

      /**
     * 获取EventPollerPool单例中的第一个EventPoller实例，
     * 保留该接口是为了兼容老代码
     * @return 单例
     */
    static EventPoller &Instance();


private:
    /**
     * 本对象只允许在EventPollerPool中构造
     */
    EventPoller(std::string name);

private:
    class ExitException : public std::exception {};

private:
    //标记loop线程是否退出
    bool _exit_flag;

// Thread name
    std::string _name;
    
};

} // FFZKit

#endif //FFZKIT_EVENTPOLLER_H
