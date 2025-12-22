#ifndef POLLER_PIPE_H
#define POLLER_PIPE_H

#include <functional>

#include "PipeWrap.h"
#include "EventPoller.h"

namespace FFZKit {

class Pipe {
public:
    using onRead = std::function<void(int size, const char *buf)>;

    Pipe(const onRead& cb = nullptr, const EventPoller::Ptr &poller = nullptr);
    ~Pipe();

    void send(const void *send, int size);

private:
    std::shared_ptr<PipeWrap> pipe_;
    EventPoller::Ptr poller_;
};

}

#endif // POLLER_PIPE_H