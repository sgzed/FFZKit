//
// Created by FFZero on 2025-01-18.
//

#ifndef FFZKIT_EVENTPOLLER_H
#define FFZKIT_EVENTPOLLER_H

#include <memory>

#include "Util/logger.h"
#include "Thread/TaskExecutor.h"
#include "Thread/ThreadPool.h"

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
    using PollEventCB = std::function<void(int event)>;
    using DelayTask = TaskCancelableImp<uint64_t(void)>;

    typedef enum {
        Event_Read = 1 << 0, //读事件
        Event_Write = 1 << 1, //写事件
        Event_Error = 1 << 2, //错误事件
        Event_LT = 1 << 3,//水平触发
    } Poll_Event;

    ~EventPoller();

    /**
     * 添加事件监听
     * @param fd 监听的文件描述符
     * @param event 事件类型，例如 Event_Read | Event_Write
     * @param cb 事件回调functional
     * @return -1:失败，0:成功
     */
    int addEvent(int fd, int event, PollEventCB cb);


private:
    /**
     * 本对象只允许在EventPollerPool中构造
     */
    EventPoller(std::string name);

private:
    class ExitException : public std::exception {};

private:
    //标记loop线程是否退出
    bool exit_flag_;

    // Thread name
    std::string name_;

    //当前线程下，所有socket共享的读缓存
    //std::weak_ptr<SocketRecvBuffer> _shared_buffer[2];
};

} // FFZKit

#endif //FFZKIT_EVENTPOLLER_H
