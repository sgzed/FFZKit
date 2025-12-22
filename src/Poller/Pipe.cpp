#include "Pipe.h"

#include "Network/sockutil.h"

using namespace std; 

namespace FFZKit {
    
Pipe::Pipe(const onRead& cb, const EventPoller::Ptr &poller) {
    poller_ = poller;
    if (!poller_) {
        poller_ = EventPollerPool::Instance().getPoller();
    }

    pipe_ = make_shared<PipeWrap>();
    auto pipe = pipe_;
    poller_->addEvent(pipe->readFD(), EventPoller::Event_Read, [pipe, cb](int event) {
#if defined(_WIN32)
        unsigned long nread = 1024;
#else
        int nread = 1024;
#endif //defined(_WIN32)
        //File I/O Number of bytes to READ
        ioctl(pipe->readFD(), FIONREAD, &nread);

#if defined(_WIN32)
        shared_ptr<char> buf(new char[nread + 1], [](char* ptr) {delete[] ptr; });
        buf.get()[nread] = '\0';
        nread = pipe->read(buf.get(), nread + 1);
        if (cb) {
            cb(nread, buf.get());
        }
#else

        char buf[nread + 1];
        buf[nread] = '\0';
        nread = pipe->read(buf, sizeof(buf));
        if (cb) {
            cb(nread, buf);
        }
#endif // _WIN32
    });
}
    
Pipe::~Pipe() {
    if(pipe_) {
        poller_->delEvent(pipe_->readFD());
    }
} 

void Pipe::send(const void *send, int size) {
    if (pipe_) {
        pipe_->write(send, size);
    }
}


} // namespace FFZKit
