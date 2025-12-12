//
// Created by FFZero on 2025-01-18.
//

#include "EventPoller.h"

#if defined(HAS_EPOLL)
#include <sys/epoll.h>

#if !defined(EPOLLEXCLUSIVE)
#define EPOLLEXCLUSIVE 0  
#endif

#define EPOLL_SIZE 1024

#endif // HAS_EPOLL

using namespace std;

namespace FFZKit {

// EventPoller::EventPoller(string name) {
// #if defined(HAS_EPOLL) || defined(HAS_KQUEUE)
// //     _event_fd = create_event();
// //     if (_event_fd == -1) {
// //         throw runtime_error(StrPrinter << "Create event fd failed: " << get_uv_errmsg());
// //     }
// //     SockUtil::setCloExec(_event_fd);
//  #endif //HAS_EPOLL

// //     _name = std::move(name);
// //     _logger = Logger::Instance().shared_from_this();
// //     addEventPipe();
// }

} // FFZKit