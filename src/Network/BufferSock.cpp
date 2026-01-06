#include <assert.h>

#include "BufferSock.h"
#include "Util/logger.h"

namespace FFZKit {  

/////////////////////////////////////// BufferSock ///////////////////////////////////////

BufferSock::BufferSock(Buffer::Ptr buffer, struct sockaddr *addr, int addr_len) {
    if (addr) {
        addr_len_ = addr_len ? addr_len : SockUtil::get_sock_len(addr);
        memcpy(&addr_, addr, addr_len_);
    }
    assert(buffer);
    buffer_ = std::move(buffer);
}

char *BufferSock::data() const {
    return buffer_->data();
}

size_t BufferSock::size() const {
    return buffer_->size(); 
}

const struct sockaddr *BufferSock::sockaddr() const {
    return (struct sockaddr *)&addr_;
}

socklen_t BufferSock::socklen() const {
    return addr_len_;
}


} // namespace FFZKit