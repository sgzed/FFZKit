//
// Created by FFZero on 2025-01-18.
//

#ifndef FFZKIT_EVENTPOLLER_H
#define FFZKIT_EVENTPOLLER_H

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

#include "PipeWrap.h"
#include "Util/logger.h"
#include "Thread/TaskExecutor.h"
#include "Thread/ThreadPool.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define HAS_KQUEUE
#endif // __APPLE__

#if defined(HAS_EPOLL) || defined(HAS_KQUEUE)
#if defined(_WIN32)
using epoll_fd = void *;
constexpr epoll_fd INVALID_EVENT_FD = nullptr;
#else
using epoll_fd = int;
constexpr epoll_fd INVALID_EVENT_FD = -1;
#endif
#endif


namespace FFZKit {

class EventPoller : public TaskExecutor, public std::enable_shared_from_this<EventPoller>  {
public:
    using Ptr = std::shared_ptr<EventPoller>;
    using PollEventCB = std::function<void(int event)>;
    using DelayTask = TaskCancelableImp<uint64_t(void)>;
    using PollCompleteCB = std::function<void(bool success)>;

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

    /**
     * 删除事件监听
     * @param fd 监听的文件描述符
     * @param cb 删除成功回调functional
     * @return -1:失败，0:成功
     * Deletes an event listener
     * @param fd The file descriptor to stop listening to
     * @param cb The callback function for successful deletion
     * @return -1: failed, 0: success
     * [AUTO-TRANSLATED:be6fdf51]
     */
    int delEvent(int fd, PollCompleteCB cb = nullptr);

    int modifyEvent(int fd, int event, PollCompleteCB cb = nullptr);

    size_t fdCount() const;

     /**
     * 异步执行任务
     * @param task 任务
     * @param may_sync 如果调用该函数的线程就是本对象的轮询线程，那么may_sync为true时就是同步执行任务
     * @return 是否成功，一定会返回true
     */
    Task::Ptr async(TaskIn task, bool may_sync = true) override;

    Task::Ptr async_first(TaskIn task, bool may_sync = true) override;

    bool isCurrentThread();

     /**
     * 延时执行某个任务
     * @param delay_ms 延时毫秒数
     * @param task 任务，返回值为0时代表不再重复任务，否则为下次执行延时，如果任务中抛异常，那么默认不重复任务
     * @return 可取消的任务标签
     */
    DelayTask::Ptr doDelayTask(uint64_t delay_ms, std::function<uint64_t()> task);

     /**
     * 获取poller线程id
     */
    std::thread::id getThreadId() const;

    /**
     * 获取线程名
     */
    const std::string &getThreadName() const;

private:
    /**
     * 本对象只允许在EventPollerPool中构造
     */
    EventPoller(std::string name);

    /**
     * 执行事件轮询
     * @param blocked 是否用执行该接口的线程执行轮询
     * @param ref_self 是否记录本对象到thread local变量
     */
    void runLoop(bool blocked, bool ref_self);

     /**
     * 结束事件轮询
     * 需要指出的是，一旦结束就不能再次恢复轮询线程
     */
    void shutdown();

    int64_t flushDelayTask(uint64_t now_time);

    /**
     * 获取select或epoll休眠时间
     */
    int64_t getMinDelay();

    /**
     * 添加管道监听事件
     */
    void addEventPipe();

     /**
     * 内部管道事件，用于唤醒轮询线程用
     * Internal pipe event, used to wake up the polling thread
     * [AUTO-TRANSLATED:022754b9]
     */
    void onPipeEvent(bool flush = false);

    /**
     * 切换线程并执行任务
     * @param task
     * @param may_sync
     * @param first
     * @return The cancellable task itself, or nullptr if it has been executed synchronously
     */
    Task::Ptr async_I(TaskIn task, bool may_sync = true, bool first = false);

    class ExitException : public std::exception {};

private:
    //标记loop线程是否退出
    bool exit_flag_;

    // Thread name
    std::string name_;

    // 统计监听了多少个fd
    size_t fd_count_ = 0;

    // 执行事件循环的线程 
    std::thread *loop_thread_ = nullptr;

    semaphore sem_loop_start_;

    // 内部事件管道
    PipeWrap pipe_;

    // 从其他线程切换过来的任务 
    std::mutex mtx_task_;
    List<Task::Ptr> list_task_;

    // 保持日志可用
    Logger::Ptr logger_;

#if defined(HAS_EPOLL) || defined(HAS_KQUEUE)
    // epoll和kqueue相关
    epoll_fd event_fd_ = INVALID_EVENT_FD;
    std::unordered_map<int, std::shared_ptr<PollEventCB>> event_map_;
#else
    // select相关 
    struct Poll_Record {
        using Ptr = std::shared_ptr<Poll_Record>;
        int fd;
        int event;
        int attach;
        PollEventCB call_back;
    };
    std::unordered_map<int, Poll_Record::Ptr> event_map_;
#endif // HAS_EPOLL

    std::unordered_set<int> event_cache_expired_; // 缓存已经删除的fd，防止重复删除

    //当前线程下，所有socket共享的读缓存
    //std::weak_ptr<SocketRecvBuffer> shared_buffer_[2];

     // 定时器相关 
    std::multimap<uint64_t, DelayTask::Ptr> delay_task_map_;
};

} // FFZKit

#endif //FFZKIT_EVENTPOLLER_H
