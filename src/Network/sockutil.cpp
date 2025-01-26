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

static onceToken g_token([]() {
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

    bool getDomainIP(const char *host, sockaddr_storage &storage, int ai_family = AF_INET,
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

static int bind_sock(int fd, const char* ifr_ip, uint16_t port, int family) {
    switch (family)
    {
    case AF_INET:  return bind_sock4(fd, ifr_ip, port);
    case AF_INET6: return bind_sock6(fd, ifr_ip, port);
    default:    return -1;
    }
}


} // FFZKit