//
// Created by FFZero on 2025-01-18.
//

#include "EventPoller.h"

#include "SelectWrap.h"
#include "Util/util.h"
#include "Util/TimeTicker.h"
#include "Util/uv_errno.h"
#include "Network/sockutil.h"

#if defined(HAS_EPOLL)
#include <sys/epoll.h>

//Prevent epoll thundering ?
#if !defined(EPOLLEXCLUSIVE)
#define EPOLLEXCLUSIVE 0  
#endif

#define EPOLL_SIZE 1024

#define toEpoll(event)        (((event) & Event_Read)  ? EPOLLIN : 0) \
                            | (((event) & Event_Write) ? EPOLLOUT : 0) \
                            | (((event) & Event_Error) ? (EPOLLHUP | EPOLLERR) : 0) \
                            | (((event) & Event_LT)    ? 0 : EPOLLET)

                            
#define toPoller(epoll_event)     (((epoll_event) & (EPOLLIN | EPOLLRDNORM | EPOLLHUP)) ? Event_Read   : 0) \
                                | (((epoll_event) & (EPOLLOUT | EPOLLWRNORM)) ? Event_Write : 0) \
                                | (((epoll_event) & EPOLLHUP) ? Event_Error : 0) \
                                | (((epoll_event) & EPOLLERR) ? Event_Error : 0)                            

#define create_event() epoll_create(EPOLL_SIZE)
#if !defined(_WIN32)
#define close_event(fd) close(fd)
#else
#define close_event(fd) epoll_close(fd)
#endif

#endif // HAS_EPOLL


using namespace std;

namespace FFZKit {

static thread_local std::weak_ptr<EventPoller> s_current_poller;

void EventPoller::addEventPipe() {
    SockUtil::setNoBlocked(pipe_.readFD());
    // 对于唤醒机制来说，写入失败是可以接受的
    // 因为如果管道满了，说明管道里已经有数据了
    // 如果强制阻塞写，反而会拖慢任务提交者的速度
    SockUtil::setNoBlocked(pipe_.writeFD());

    // 添加内部管道事件 
    if (addEvent(pipe_.readFD(), EventPoller::Event_Read, [this](int event) { onPipeEvent(); }) == -1) {
        throw std::runtime_error("Add pipe fd to poller failed");
    }
}

EventPoller::EventPoller(string name) {
#if defined(HAS_EPOLL) || defined(HAS_KQUEUE)
    event_fd_ = create_event();
    if (event_fd_ == -1) {
        throw runtime_error(StrPrinter << "Create event fd failed: " << get_uv_errmsg());
    }
#if !defined(_WIN32)
    SockUtil::setCloExec(event_fd_);
#endif
 #endif //HAS_EPOLL

    name_ = std::move(name);
    logger_ = Logger::Instance().shared_from_this();
    addEventPipe();
}

void EventPoller::runLoop(bool blocked, bool ref_self) {
    if (blocked) {
        if(ref_self) {
            s_current_poller = shared_from_this();
        }
        sem_loop_start_.post();
        exit_flag_ = false;
        int64_t minDelay;

#if defined(HAS_EPOLL)
        struct epoll_event events[EPOLL_SIZE];
        while (!_exit_flag) {
            //minDelay = getMinDelay();
            startSleep(); // 用于统计当前线程负载情况
            int nfds = epoll_wait(event_fd_, events, EPOLL_SIZE, -1 /*minDelay*/);
            sleepWakeUp(); // 结束统计当前线程负载情况
            if (nfds < 0) {
                // Timed out or interrupted
                continue;
            }
            
            event_cache_expired_.clear();
            for(int i = 0; i < nfds; ++i) {
                struct epoll_event &ev = events[i];
                int fd = ev.data.fd;
                if (event_cache_expired_.count(fd)) {
                    // event cache refresh
                    continue;
                }

                auto it = event_map_.find(fd);
                if(it == event_map_.end()) {
                    // 该fd已经被删除
                    epoll_ctl(event_fd_, EPOLL_CTL_DEL, fd, nullptr);
                    continue;
                }
                auto cb = it->second;
                try {
                    (*cb)(toPoller(ev.events));
                } catch (std::exception &ex) {
                    ErrorL << "Exception occurred when do event task: " << ex.what();
                }
            }
        }
#elif defined(HAS_KQUEUE)

#else 
        int ret, max_fd;
        FdSet set_read, set_write, set_err;
        List<Poll_Record::Ptr> callback_list;
        struct timeval tv;

        while (!exit_flag_) {
            set_read.fdZero();
            set_write.fdZero();
            set_err.fdZero();
            max_fd = 0;

            for(auto &pr : event_map_) {
                int fd = pr.first;
                auto record = pr.second;
                if(record->event & Event_Read) {
                    set_read.fdSet(fd);
                }
                if(record->event & Event_Write) {
                    set_write.fdSet(fd);
                }
                if (pr.second->event & Event_Error) {
                    set_err.fdSet(pr.first); // 监听管道错误事件
                }
                if(fd > max_fd) {
                    max_fd = fd;
                }
            }

            startSleep();
            ret = fz_select(max_fd + 1, &set_read, &set_write, &set_err, nullptr);
            sleepWakeUp();

            if (ret < 0) {
                // Timed out or interrupted
                continue;
            }

            event_cache_expired_.clear();

            for(auto &pr : event_map_) {
                int fd = pr.first;
                auto record = pr.second;
                int event = 0;
                if(set_read.isSet(fd)) {
                    event |= Event_Read;
                }
                if(set_write.isSet(fd)) {
                    event |= Event_Write;
                }
                if(set_err.isSet(fd)) {
                    event |= Event_Error;
                }
                if(event) {
                    pr.second->attach = event;
                    callback_list.emplace_back(record);
                }
            }

            callback_list.for_each([&](const Poll_Record::Ptr &record) {
                if(event_cache_expired_.count(record->fd)) {
                    // event cache refresh
                    return;
                }

                try {
                    record->call_back(record->attach);
                } catch (std::exception &ex) {
                    ErrorL << "Exception occurred when do event task: " << ex.what();
                }
            });
            callback_list.clear();
        }
#endif // HAS_EPOLL
    } else {
        loop_thread_ = new std::thread(&EventPoller::runLoop, this, true, ref_self);
        sem_loop_start_.wait();
    }
}

bool EventPoller::isCurrentThread() {
    return !loop_thread_  || this_thread::get_id() == loop_thread_->get_id();
}

int EventPoller::addEvent(int fd, int event, PollEventCB cb) {
    TimeTicker();
    if (!cb) {
        WarnL << "PollEventCB is empty";
        return -1;
    }

    if (isCurrentThread()) {
#if defined(HAS_EPOLL)
        struct epoll_event ev = {0};
        ev.events = toEpoll(event) ;
        ev.data.fd = fd;
        int ret = epoll_ctl(event_fd_, EPOLL_CTL_ADD, fd, &ev);
        if (ret != -1) {
            event_map_.emplace(fd, std::make_shared<PollEventCB>(std::move(cb)));
        }
        fd_count_ = event_map_.size();
        return ret;
#else
#ifndef _WIN32
        // win32平台，socket套接字不等于文件描述符，所以可能不适用这个限制
        if (fd >= FD_SETSIZE) {
            WarnL << "select() can not watch fd bigger than " << FD_SETSIZE;
            return -1;
        }
#endif  //  _WIN32
        auto record = std::make_shared<Poll_Record>();
        record->fd = fd;
        record->event = event;
        record->call_back = std::move(cb);
        event_map_.emplace(fd, record);
        fd_count_ = event_map_.size();
        return 0;
#endif // HAS_EPOLL
    } 

    async([this, fd, event, cb]() mutable {
        addEvent(fd, event, std::move(cb));
    });
    return 0;
}

inline void EventPoller::onPipeEvent(bool flush) {
    char buf[1024];
    int err = 0;

    if (!flush) {
       for (;;) {
            if ((err = pipe_.read(buf, sizeof(buf))) > 0) {
                // 读到管道数据,继续读,直到读空为止
                continue;
            }
            if (err == 0 || get_uv_error(true) != UV_EAGAIN) {
                // 收到eof或非EAGAIN(无更多数据)错误,说明管道无效了,重新打开管道  
                ErrorL << "Invalid pipe fd of event poller, reopen it";
                delEvent(pipe_.readFD());
                pipe_.reOpen();
                addEventPipe();
            }
            break;
        }
    }

    decltype(list_task_) list_swap_;
    {
        lock_guard<mutex> lck(mtx_task_);
        list_swap_.swap(list_task_);
    }
    
    list_swap_.for_each([&](const Task::Ptr &task) {
        try {
            (*task)();
        } catch (ExitException &) {
            exit_flag_ = true;
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when do async task: " << ex.what();
        }
    });
}

} // FFZKit