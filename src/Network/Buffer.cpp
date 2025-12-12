#include "Buffer.h"
#include "Util/onceToken.h"

namespace FFZKit {

StatisticImp(Buffer)
StatisticImp(BufferRaw)
    
BufferRaw::Ptr BufferRaw::create(size_t size) {
#if 1
    static ResourcePool<BufferRaw> packet_pool;
    static OnceToken token([]() {
        packet_pool.setSize(1024);
    });

    auto ret = packet_pool.obtain2();
    ret->setCapacity(size);
    ret->setSize(0);
    return ret;
#else
    return std::make_shared<BufferRaw>(size);
#endif
}

} // namespace FFZKit
