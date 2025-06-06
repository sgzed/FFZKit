//
// Created by FFZero on 2025-01-18.
//

#include <fcntl.h>
#include <assert.h>
#include <cstdio>
#include <unordered_map>

#include "sockutil.h"
#include "Util/logger.h"
#include "Util/uv_errno.h"
#include "Util/onceToken.h"

using namespace std;

namespace FFZKit {

#if defined(_WIN32)

static OnceToken g_token([]() {
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
}, []() {
    WSACleanup();
});

int ioctl(int fd, long cmd, u_long *ptr) {
    return ioctlsocket(fd, cmd, ptr);
}
int close(int fd) {
    return closesocket(fd);
}

#endif // defined(_WIN32)


static inline string my_inet_ntop(int af, const void* addr) {
    string ret;
    ret.resize(128);
    if(!inet_ntop(af, addr, (char *)ret.data(), ret.size())) {
        ret.clear();
    } else {
        ret.resize(strlen(ret.data()));
    }
    return ret;
}

static inline bool support_ipv6_l() {
    auto fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if(fd == -1) {
        return false;
    }
    close(fd);
    return true;
}

bool SockUtil::support_ipv6() {
    static auto flag = support_ipv6_l();
    return flag;
}

string SockUtil::inet_ntoa(const struct in_addr &addr) {
    return my_inet_ntop(AF_INET, &addr);
}

std::string SockUtil::inet_ntoa(const struct in6_addr &addr) {
    return my_inet_ntop(AF_INET6, &addr);
}

std::string SockUtil::inet_ntoa(const struct sockaddr *addr) {
    switch (addr->sa_family) {
        case AF_INET: return SockUtil::inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);
        case AF_INET6: {
            if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)addr)->sin6_addr)) {
                struct in_addr addr4;
                memcpy(&addr4, 12 + (char *)&(((struct sockaddr_in6 *)addr)->sin6_addr), 4);
                return SockUtil::inet_ntoa(addr4);
            }
            return SockUtil::inet_ntoa(((struct sockaddr_in6 *)addr)->sin6_addr);
        }
        default: return "";
    }
}

uint16_t SockUtil::inet_port(const struct sockaddr *addr) {
    switch (addr->sa_family) {
        case AF_INET: return ntohs(((struct sockaddr_in *)addr)->sin_port);
        case AF_INET6: return ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
        default: return 0;
    }
}

int SockUtil::setCloseWait(int fd, int second) {
    linger m_sLinger;
    //在调用closesocket()时还有数据未发送完，允许等待 
    //若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
    m_sLinger.l_onoff = (second > 0);
    // 设定等待时间为x秒
    m_sLinger.l_linger = second;  
    int ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&m_sLinger, sizeof(linger));
    if(-1 == ret) {
#ifndef _WIN32
        WarnL << "setsockopt SO_LINGER failed";
#endif
    }
    return ret;
}

int SockUtil::setNoDelay(int fd, bool on) {
    int opt = on ? 1: 0;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        WarnL << "setsockopt TCP_NODELAY failed";
    }
    return ret;
}

int SockUtil::setReuseable(int fd, bool on, bool reuse_port) {
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        WarnL << "setsockopt SO_REUSEADDR failed";
        return ret;
    }

#if defined(SO_REUSEPORT)
    if (reuse_port) {
        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
        if (ret == -1) {
            TraceL << "setsockopt SO_REUSEPORT failed";
        }
    }
#endif
    return ret;
}

int SockUtil::setKeepAlive(int fd, bool on, int interval, int idle, int times) {
    // Enable/disable the keep-alive option
    int opt = on ? 1 : 0;
    int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        WarnL << "setsockopt SO_KEEPALIVE failed";
    }
#if !defined(_WIN32)
    #if !defined(SOL_TCP) && defined(IPPROTO_TCP)
    #define SOL_TCP IPPROTO_TCP
    #endif

    #if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
    #define TCP_KEEPIDLE TCP_KEEPALIVE
    #endif

     // Set the keep-alive parameters
    if (on && interval > 0 && ret != -1) {
        ret = setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (char *) &idle, static_cast<socklen_t>(sizeof(idle)));
        if (ret == -1) {
            WarnL << "setsockopt TCP_KEEPIDLE failed";
        }
        ret = setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (char *) &interval, static_cast<socklen_t>(sizeof(interval)));
        if (ret == -1) {
            WarnL << "setsockopt TCP_KEEPINTVL failed";
        }
        ret = setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (char *) &times, static_cast<socklen_t>(sizeof(times)));
        if (ret == -1) {
            WarnL << "setsockopt TCP_KEEPCNT failed";
        }
    }
#endif
    return ret;
}

int SockUtil::setCloExec(int fd, bool on) {
#if !defined(_WIN32)
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        WarnL << "fcntl F_GETFD failed";
        return -1;
    }
    if (on) {
        flags |= FD_CLOEXEC;
    } else {
        int cloexec = FD_CLOEXEC;
        flags &= ~cloexec;
    }
    int ret = fcntl(fd, F_SETFD, flags);
    if (ret == -1) {
        WarnL << "fcntl F_SETFD failed";
        return -1;
    }
    return ret;
#else
    return -1;
#endif
}

int SockUtil::setNoSigpipe(int fd) {
#if defined(SO_NOSIGPIPE)
    int set = 1;
    auto ret = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (char *) &set, sizeof(int));
    if (ret == -1) {
        WarnL << "setsockopt SO_NOSIGPIPE failed";
    }
    return ret;
#else
    return -1;
#endif
}

int SockUtil::setNoBlocked(int fd, bool noblock) {
#if defined(_WIN32)
    unsigned long ul = noblock;
#else
    int ul = noblock;
#endif //defined(_WIN32)
    int ret = ioctl(fd, FIONBIO, &ul); //设置为非阻塞模式
    if (ret == -1) {
        WarnL << "ioctl FIONBIO failed";
    }

    return ret;
}

int SockUtil::setRecvBuf(int fd, int size) {
    if (size <= 0) {
        // use the system default value
        return 0;
    }
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
    if (ret == -1) {
        WarnL << "setsockopt SO_RCVBUF failed";
    }
    return ret;
}

int SockUtil::setSendBuf(int fd, int size) {
    if (size <= 0) {
        return 0;
    }
    int ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        WarnL << "setsockopt SO_SNDBUF failed";
    }
    return ret;
}

bool SockUtil::is_ipv4(const char *host) {
    struct in_addr addr;
    return 1 == inet_pton(AF_INET, host, &addr);
}

bool SockUtil::is_ipv6(const char *host) {
    struct in6_addr addr;
    return 1 == inet_pton(AF_INET6, host, &addr);
}

socklen_t SockUtil::get_sock_len(const struct sockaddr *addr) {
    switch (addr->sa_family) {
        case AF_INET : return sizeof(sockaddr_in);
        case AF_INET6 : return sizeof(sockaddr_in6);
        default: assert(0); return 0;
    }
}

struct sockaddr_storage SockUtil::make_sockaddr(const char *host, uint16_t port) {
    struct sockaddr_storage storage;
    bzero(&storage, sizeof(storage));

    struct in_addr addr;
    struct in6_addr addr6;

    if(1 == inet_pton(AF_INET, host, &addr)) {
        // host是ipv4 
        reinterpret_cast<struct sockaddr_in &>(storage).sin_addr = addr;
        reinterpret_cast<struct sockaddr_in &>(storage).sin_family = AF_INET;
        reinterpret_cast<struct sockaddr_in &>(storage).sin_port = htons(port);
        return storage;
    }
    if (1 == inet_pton(AF_INET6, host, &addr6)) {
        // host是ipv6 
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_addr = addr6;
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_family = AF_INET6;
        reinterpret_cast<struct sockaddr_in6 &>(storage).sin6_port = htons(port);
        return storage;
    }
    throw std::invalid_argument(string("Not ip address: ") + host);
}

class DnsCache {
public:
    static DnsCache& Instance() {
        static DnsCache s_instance;
        return s_instance;
    }

    bool getDomainIP(const char *host, struct sockaddr_storage &storage, int ai_family = AF_INET,
                         int ai_socktype = SOCK_STREAM, int ai_protocol = IPPROTO_TCP, int expire_sec = 60) {

        try {
            storage = SockUtil::make_sockaddr(host, 0);
            return true;
        } catch(...) {
            InfoL << "Not ip address: " << host << ", try to resolve it";
            auto item = getCacheDomainIP(host, expire_sec);
            if(!item) {
                item = getSystemDomainIP(host);
                if(item) {
                    setCacheDomainIP(host, item);
                }
            }

            if(item) {
                auto addr = getPreferredAddress(item.get(), ai_family, ai_socktype, ai_protocol);
                memcpy(&storage, addr->ai_addr, addr->ai_addrlen);
            }
            return (bool)item;
        }
    }

private:
    class DnsItem {
    public:
        std::shared_ptr<struct addrinfo> addr_info;
        time_t create_time;
    };

    std::shared_ptr<struct addrinfo> getCacheDomainIP(const char* host, int expireSec) {
        lock_guard<mutex> lock(mtx_);
        auto it = dns_cache_.find(host);
        if(it == dns_cache_.end()) {
            return nullptr;
        }

        if(it->second.create_time + expireSec < time(nullptr)) {
            // 缓存过期
            dns_cache_.erase(it);
            return nullptr;
        }   
        return it->second.addr_info;
    }

    void setCacheDomainIP(const char* host, std::shared_ptr<struct addrinfo> addr_info) {
        lock_guard<mutex> lock(mtx_);
        DnsItem item; 
        item.addr_info = addr_info;
        item.create_time = time(nullptr);
        dns_cache_[host] = std::move(item);
    }

    std::shared_ptr<struct addrinfo> getSystemDomainIP(const char *host) {
        struct addrinfo* answer = nullptr;
        //阻塞式dns解析，可能被打断
        int ret = -1;
        do {
            ret = getaddrinfo(host, nullptr, nullptr, &answer);
        }while(ret == -1 && get_uv_error(true) == UV_EINTR);

        if(!answer) {
            WarnL << "getaddrinfo failed: " << host;
            return nullptr;
        }
        return std::shared_ptr<struct addrinfo>(answer, freeaddrinfo);
    }

    struct addrinfo* getPreferredAddress(struct addrinfo* answer, int ai_family, int ai_socktype, int ai_protocol) {
        auto ptr = answer;
        while(ptr) {
            if(ptr->ai_family == ai_family && ptr->ai_socktype == ai_socktype && ptr->ai_protocol == ai_protocol) {
                return ptr;
            }
            ptr = ptr->ai_next;
        }
        return answer;
    }

private:
    mutex mtx_;
    unordered_map<string, DnsItem> dns_cache_;
};

bool SockUtil::getDomainIP(const char *host, uint16_t port, struct sockaddr_storage &addr, int ai_family,
                            int ai_socktype, int ai_protocol, int expire_sec) {

    bool flag = DnsCache::Instance().getDomainIP(host, addr, ai_family, ai_socktype, ai_protocol, expire_sec);     
    if(flag) {
        switch (addr.ss_family ) {
            case AF_INET: 
                ((sockaddr_in*) &addr)->sin_port = htons(port);
                break;
            case AF_INET6:
                ((sockaddr_in6*) &addr)->sin6_port = htons(port);
                break;
            default: 
                WarnL << "unknown family: " << addr.ss_family;
                break;
        }
    }                       
    return flag;
}

static int set_ipv6_only(int fd, bool flag) {
    int opt = flag; 
    int ret = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)opt, sizeof opt);
    if(ret == -1) {
        WarnL << "setsockopt IPV6_V6ONLY failed: " << get_uv_error(true);
    }
    return ret;
}

static int bind_sock6(int fd, const char* ifr_ip, uint16_t port) {
    set_ipv6_only(fd, false);
    struct sockaddr_in6 addr;
    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    // 不是有效的ipv6地址
    if(1 != inet_pton(AF_INET6, ifr_ip, &addr.sin6_addr)) {
        // 也不是ipv4通配地址
        if(strcmp(ifr_ip, "0.0.0.0")) {
            WarnL << "inet_pton to ipv6 address failed: " << ifr_ip;
        }
        addr.sin6_addr = IN6ADDR_ANY_INIT;
    }
    
    if(::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        WarnL << "Bind socket failed: " << get_uv_errmsg(true);
        return -1;
    }
    return 0;
}

static int bind_sock4(int fd, const char* ifr_ip, uint16_t port) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // 不是有效的ipv4地址
    if(1 != inet_pton(AF_INET, ifr_ip, &addr.sin_addr)) {
        // 也不是ipv6通配地址
        if(strcmp(ifr_ip, "::")) {
            WarnL << "inet_pton to ipv4 address failed: " << ifr_ip;
        }

        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if(::bind(fd, (struct sockaddr*)&addr, sizeof addr) == -1) {
        WarnL << "Bind socket failed: " << get_uv_errmsg(true);
        return -1;
    }
    return 0;
}

static int bind_sock(int fd, const char* ifr_ip, uint16_t port, int family) {
    switch (family) {
    case AF_INET:  return bind_sock4(fd, ifr_ip, port);
    case AF_INET6: return bind_sock6(fd, ifr_ip, port);
    default:    return -1;
    }
}

int SockUtil::connect(const char *host, uint16_t port, bool async, const char *local_ip, uint16_t local_port) {
    struct sockaddr_storage addr;
    //优先使用ipv4地址
     if (!getDomainIP(host, port, addr, AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        return -1;
    }

    int sockfd = (int) socket(addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        WarnL << "Create socket failed: " << host;
        return -1;
    }

    setReuseable(sockfd);
    setNoSigpipe(sockfd);
    setNoBlocked(sockfd, async);
    setNoDelay(sockfd);
    setSendBuf(sockfd);
    setRecvBuf(sockfd);
    setCloseWait(sockfd);
    setCloExec(sockfd);

    if (bind_sock(sockfd, local_ip, local_port, addr.ss_family) == -1) {
        close(sockfd);
        return -1;
    }

    if (::connect(sockfd, (sockaddr *) &addr, get_sock_len((sockaddr *)&addr)) == 0) {
        //同步连接成功
        return sockfd;
    }

    if (async && get_uv_error(true) == UV_EAGAIN) {
        //异步连接成功
        return sockfd;
    }
    WarnL << "Connect socket to " << host << " " << port << " failed: " << get_uv_errmsg(true);
    close(sockfd);
    return -1;
}


int SockUtil::listen(const uint16_t port, const char *local_ip, int back_log) {
    int fd = -1;
    int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    if ((fd = (int)::socket(family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        WarnL << "Create socket failed: " << get_uv_errmsg(true);
        return -1;
    }

    setReuseable(fd, true, false);
    setNoBlocked(fd);
    setCloExec(fd);

    if (bind_sock(fd, local_ip, port, family) == -1) {
        close(fd);
        return -1;
    }

    //开始监听
    if (::listen(fd, back_log) == -1) {
        WarnL << "Listen socket failed: " << get_uv_errmsg(true);
        close(fd);
        return -1;
    }

    return fd;
}


int SockUtil::getSockError(int fd) {
    int opt;
    socklen_t optLen = static_cast<socklen_t>(sizeof(opt));

    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&opt, &optLen) < 0) {
        return get_uv_error(true);
    } else {
        return uv_translate_posix_error(opt);
    }
}

using getsockname_type = decltype(getsockname);

static bool get_socket_addr(int fd, struct sockaddr_storage &addr, getsockname_type func) {
    socklen_t addr_len = static_cast<socklen_t>(sizeof(addr));
    if (-1 == func(fd, (struct sockaddr *)&addr, &addr_len)) {
        return false;
    }
    return true;
}

bool SockUtil::get_sock_local_addr(int fd, struct sockaddr_storage& addr) {
    return get_socket_addr(fd, addr, getsockname); 
}

bool SockUtil::get_sock_peer_addr(int fd, struct sockaddr_storage& addr) {
    return get_socket_addr(fd, addr, getpeername);
}

static string get_socket_ip(int fd, getsockname_type func) {
    struct sockaddr_storage addr;
    if (!get_socket_addr(fd, addr, func)) {
        return "";
    }
    return SockUtil::inet_ntoa((struct sockaddr *)&addr);
}

static uint16_t get_socket_port(int fd, getsockname_type func) {
    struct sockaddr_storage addr;
    if (!get_socket_addr(fd, addr, func)) {
        return 0;
    }
    return SockUtil::inet_port((struct sockaddr *)&addr);
}

string SockUtil::get_local_ip(int fd) {
    return get_socket_ip(fd, getsockname);
}

string SockUtil::get_peer_ip(int fd) {
    return get_socket_ip(fd, getpeername);
}

uint16_t SockUtil::get_local_port(int fd) {
    return get_socket_port(fd, getsockname);
}

uint16_t SockUtil::get_peer_port(int fd) {
    return get_socket_port(fd, getpeername);
}

#if defined(_WIN32)

template <typename FUN>
void for_each_netAdapter_win32(FUN &&fun) {
    ULONG nSize = 0;
    PIP_ADAPTER_INFO adapterList = nullptr;

    // 首次调用获取实际所需缓冲区大小
    DWORD nRet = GetAdaptersInfo(nullptr, &nSize);
    if (nRet != ERROR_BUFFER_OVERFLOW) {
        // 处理错误（如日志输出）
        return;
    }

    adapterList = reinterpret_cast<PIP_ADAPTER_INFO>(new char[nSize]);
    nRet = GetAdaptersInfo(adapterList, &nSize);
    if (nRet != ERROR_SUCCESS) {
        delete[] adapterList;
        return;
    }

    PIP_ADAPTER_INFO adapterPtr = adapterList;
    while (adapterPtr) {
        if (fun(adapterPtr)) {
            break;
        }
        adapterPtr = adapterPtr->Next;
    }

    delete[] adapterList;
}

#endif // defined(_WIN32)

#if !defined(_WIN32) && !defined(__APPLE__)
template <typename FUN>
void for_each_netAdapter_posix(FUN &&fun) { // type: struct ifreq *
    struct ifconf ifconf;
    char buf[1024 * 10];
    // 初始化ifconf
    ifconf.ifc_len = sizeof(buf);
    ifconf.ifc_buf = buf;
    int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        WarnL << "Create socket failed: " << get_uv_errmsg(true);
        return;
    }
    if (-1 == ioctl(sockfd, SIOCGIFCONF, &ifconf)) { // 获取所有接口信息
        WarnL << "ioctl SIOCGIFCONF failed: " << get_uv_errmsg(true);
        close(sockfd);
        return;
    }
    close(sockfd);
    // 接下来一个一个的获取IP地址 
    struct ifreq *adapter = (struct ifreq *)buf;
    for (int i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; --i, ++adapter) {
        if (fun(adapter)) {
            break;
        }
    }
}
#endif //! defined(_WIN32) && !defined(__APPLE__)

// 检查给定的 IP 地址是否为私有地址
bool check_ip(string &address, const string &ip) {
    if (ip != "127.0.0.1" && ip != "0.0.0.0") {
        /*获取一个有效IP
         */
        address = ip;
        uint32_t addressInNetworkOrder = htonl(inet_addr(ip.data()));
        if (/*(addressInNetworkOrder >= 0x0A000000 && addressInNetworkOrder < 0x0E000000) ||*/
            (addressInNetworkOrder >= 0xAC100000 && addressInNetworkOrder < 0xAC200000) ||
            (addressInNetworkOrder >= 0xC0A80000 && addressInNetworkOrder < 0xC0A90000)) {
            // A类私有IP地址： 
            // 10.0.0.0～10.255.255.255
            // B类私有IP地址： 
            // 172.16.0.0～172.31.255.255  
            // C类私有IP地址： 
            // 192.168.0.0～192.168.255.255
            // 如果是私有地址 说明在nat内部

            /* 优先采用局域网地址，该地址很可能是wifi地址
             * 一般来说,无线路由器分配的地址段是BC类私有ip地址
             * 而A类地址多用于蜂窝移动网络
             */
            return true;
        }
    }
    return false;
}

string SockUtil::get_local_ip() {
#if defined(_WIN32)
    string address = "127.0.0.1";
    for_each_netAdapter_win32([&](PIP_ADAPTER_INFO adapter) {
        IP_ADDR_STRING *ipAddr = &(adapter->IpAddressList);
        while (ipAddr) {
            string ip = ipAddr->IpAddress.String;
            if (strstr(adapter->AdapterName, "docker")) {
                return false;
            }
            if(check_ip(address,ip)){
                return true;
            }
            ipAddr = ipAddr->Next;
        }
        return false; 
    });
    return address;
#else
    string address = "127.0.0.1";
    for_each_netAdapter_posix([&](struct ifreq *adapter) {
        string ip = SockUtil::inet_ntoa(&(adapter->ifr_addr));
        if (strstr(adapter->ifr_name, "docker")) {
            return false;
        }
        return check_ip(address,ip); 
    });
    return address;
#endif
}

vector<map<string, string>> SockUtil::getInterfaceList() {
    vector<map<string, string>> ret;

#if defined(_WIN32)
    for_each_netAdapter_win32([&](PIP_ADAPTER_INFO adapter) {
        IP_ADDR_STRING *ipAddr = &(adapter->IpAddressList);
        while (ipAddr) {
            map<string,string> obj;
            obj["ip"] = ipAddr->IpAddress.String;
            obj["name"] = adapter->AdapterName;
            ret.emplace_back(std::move(obj));
            ipAddr = ipAddr->Next;
        }
        return false; 
    });
#else
    for_each_netAdapter_posix([&](struct ifreq *adapter) {
        map<string,string> obj;
        obj["ip"] = SockUtil::inet_ntoa(&(adapter->ifr_addr));
        obj["name"] = adapter->ifr_name;
        ret.emplace_back(std::move(obj));
        return false; 
    });
#endif
    return ret;
}

int SockUtil::bindUdpSock(const uint16_t port, const char *local_ip, bool enable_reuse) {
    int fd = -1;
    int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    if ((fd = (int)socket(family, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        WarnL << "Create socket failed: " << get_uv_errmsg(true);
        return -1;
    }
    if (enable_reuse) {
        setReuseable(fd);
    }
    setNoSigpipe(fd);
    setNoBlocked(fd);
    setSendBuf(fd);
    setRecvBuf(fd);
    setCloseWait(fd);
    setCloExec(fd);

    if (bind_sock(fd, local_ip, port, family) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

int SockUtil::dissolveUdpSock(int fd) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (-1 == getsockname(fd, (struct sockaddr *)&addr, &addr_len)) {
        return -1;
    }
    addr.ss_family = AF_UNSPEC;
    if (-1 == ::connect(fd, (struct sockaddr *)&addr, addr_len) && get_uv_error() != UV_EAFNOSUPPORT) {
        // mac/ios时返回EAFNOSUPPORT错误
        WarnL << "Connect socket AF_UNSPEC failed: " << get_uv_errmsg(true);
        return -1;
    }
    return 0;
}

string SockUtil::get_ifr_ip(const char *if_name) {
#if defined(_WIN32)
    string ret;
    for_each_netAdapter_win32([&](PIP_ADAPTER_INFO adapter) {
        IP_ADDR_STRING *ipAddr = &(adapter->IpAddressList);
        while (ipAddr){
            if (strcmp(if_name,adapter->AdapterName) == 0){
                //ip匹配到了 
                ret.assign(ipAddr->IpAddress.String);
                return true;
            }
            ipAddr = ipAddr->Next;
        }
        return false; 
    });
    return ret;
#else
    string ret;
    for_each_netAdapter_posix([&](struct ifreq *adapter) {
        if(strcmp(adapter->ifr_name,if_name) == 0) {
            ret = SockUtil::inet_ntoa(&(adapter->ifr_addr));
            return true;
        }
        return false; 
    });
    return ret;
#endif
}

string SockUtil::get_ifr_name(const char *local_ip) {
#if defined(_WIN32)
    string ret = "en0";
    for_each_netAdapter_win32([&](PIP_ADAPTER_INFO adapter) {
        IP_ADDR_STRING *ipAddr = &(adapter->IpAddressList);
        while (ipAddr){
            if (strcmp(local_ip,ipAddr->IpAddress.String) == 0){
                //ip匹配到了
                ret.assign(adapter->AdapterName);
                return true;
            }
            ipAddr = ipAddr->Next;
        }
        return false; });
    return ret;
#else
    string ret = "en0";
    for_each_netAdapter_posix([&](struct ifreq *adapter) {
        string ip = SockUtil::inet_ntoa(&(adapter->ifr_addr));
        if(ip == local_ip) {
            ret = adapter->ifr_name;
            return true;
        }
        return false; });
    return ret;
#endif
}

} // FFZKit