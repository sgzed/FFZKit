#ifndef NETWORK_BUFFERSOCK_H
#define NETWORK_BUFFERSOCK_H

#include "sockutil.h"
#include "Buffer.h"

namespace FFZKit {

class BufferSock : public Buffer {
public:
    using Ptr = std::shared_ptr<BufferSock>;

    BufferSock(Buffer::Ptr ptr, struct sockaddr *addr = nullptr, int addr_len = 0);
    ~BufferSock() override = default;

    char* data() const override;
    size_t size() const override;

    const struct sockaddr* sockaddr() const; 

    socklen_t socklen() const;

private:
    int addr_len_ = 0;
    struct sockaddr_storage addr_;
    Buffer::Ptr buffer_;
};

} // namespace FFZKit


#endif // NETWORK_BUFFERSOCK_H