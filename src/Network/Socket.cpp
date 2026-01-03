#include "Socket.h"

using namespace std; 

namespace FFZKit {


std::ostream &operator<<(std::ostream &ost, const SockException &err) {
    ost << err.getErrCode() << "(" << err.what() << ")";
    return ost;
}

} // FFZKit