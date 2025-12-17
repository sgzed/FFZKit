#ifndef SRC_POLLER_SELECTWRAP_H_
#define SRC_POLLER_SELECTWRAP_H_

#include "Util/util.h"

namespace FFZKit {

class FdSet {
public:
    FdSet();
    ~FdSet();
    void fdZero();
    void fdSet(int fd);
    void fdClr(int fd);
    bool isSet(int fd);
    void *_ptr;
};

int fz_select(int cnt, FdSet *read, FdSet *write, FdSet *err, struct timeval *tv);

} // namespace FFZKit

#endif //SRC_POLLER_SELECTWRAP_H_