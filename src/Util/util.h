#ifndef FFZKIT_UTIL_H
#define FFZKIT_UTIL_H

#include <cstring>
#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include "function_traits.h"

#if defined(_WIN32)
#undef FD_SETSIZE
//修改默认64为1024路
#define FD_SETSIZE 1024
#include <winsock2.h>
#pragma comment (lib,"WS2_32")
#else
#include <unistd.h>
#include <sys/time.h>
#endif // defined(_WIN32)

#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}

namespace FFZKit {

class noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;
private:
    noncopyable(const noncopyable &that) = delete;
    noncopyable(noncopyable &&that) = delete;
    noncopyable &operator=(const noncopyable &that) = delete;
    noncopyable &operator=(noncopyable &&that) = delete;
};

#ifndef CLASS_FUNC_TRAITS
#define CLASS_FUNC_TRAITS(func_name) \
template<typename T, typename ... ARGS> \
constexpr bool Has_##func_name(decltype(&T::on##func_name)) { \
    using RET = typename function_traits<decltype(&T::on##func_name)>::return_type; \
    using FuncType = RET (T::*)(ARGS...);   \
    return std::is_same<decltype(&T::on##func_name), FuncType>::value; \
} \
\
template<class T, typename ... ARGS> \
constexpr bool Has_##func_name(...) { \
    return false; \
} \
\
template<typename T, typename ... ARGS> \
static void InvokeFunc_##func_name(typename std::enable_if<!Has_##func_name<T, ARGS...>(nullptr), T>::type &obj, ARGS ...args) {} \
\
template<typename T, typename ... ARGS>\
static typename function_traits<decltype(&T::on##func_name)>::return_type InvokeFunc_##func_name(typename std::enable_if<Has_##func_name<T, ARGS...>(nullptr), T>::type &obj, ARGS ...args) {\
    return obj.on##func_name(std::forward<ARGS>(args)...);\
}
#endif //CLASS_FUNC_TRAITS

#ifndef CLASS_FUNC_INVOKE
#define CLASS_FUNC_INVOKE(T, obj, func_name, ...) InvokeFunc_##func_name<T>(obj, ##__VA_ARGS__)
#endif //CLASS_FUNC_INVOKE

CLASS_FUNC_TRAITS(Destory)
CLASS_FUNC_TRAITS(Create)

/**
 * 对象安全的构建和析构,构建后执行onCreate函数,析构前执行onDestory函数
 * 在函数onCreate和onDestory中可以执行构造或析构中不能调用的方法，比如说shared_from_this或者虚函数
 * @warning onDestory函数确保参数个数为0；否则会被忽略调用
 */

class Creator {
public:
     /**
     * 创建对象，用空参数执行onCreate和onDestory函数
     * @param args 对象构造函数参数列表
     * @return args对象的智能指针
     */
    template<typename C, typename ...ArgsType>
    static std::shared_ptr<C> create(ArgsType &&...args) {
        std::shared_ptr<C> ret(new C(std::forward<ArgsType>(args)...), [](C *ptr) {
             try {
                CLASS_FUNC_INVOKE(C, *ptr, Destory);
            } catch (std::exception &ex){
                onDestoryException(typeid(C), ex);
            }
            delete ptr;
        });
        CLASS_FUNC_INVOKE(C, *ret, Create);
        return ret;
    }

    template<typename C, typename ...ArgsType>
    static std::shared_ptr<C> create2(ArgsType &&...args) {
        std::shared_ptr<C> ret(new C(), [](C *ptr) {
            try {
                CLASS_FUNC_INVOKE(C, *ptr, Destory);
            } catch (std::exception &ex){
                onDestoryException(typeid(C), ex);
            }
            delete ptr;
        });
        CLASS_FUNC_INVOKE(C, *ret, Create, std::forward<ArgsType>(args)...);
        return ret;
    }

private:
    static void onDestoryException(const std::type_info &info, const std::exception &ex);

private:
    Creator() = default;
    ~Creator() = default;
};


std::string exePath(bool isExe = true);
std::string exeDir(bool isExe = true);
std::string exeName(bool isExe = true);

//字符串是否以xx开头
bool start_with(const std::string &str, const std::string &substr);
//字符串是否以xx结尾
bool end_with(const std::string &str, const std::string &substr);

std::vector<std::string> split(const std::string& s, const char *delim);

//去除前后的空格、回车符、制表符...
std::string& trim(std::string &s,const std::string &chars=" \r\n\t");
std::string trim(std::string &&s,const std::string &chars=" \r\n\t");

// string转小写
std::string &strToLower(std::string &str);
std::string strToLower(std::string &&str);
// string转大写
std::string &strToUpper(std::string &str);
std::string strToUpper(std::string &&str);


#ifndef bzero
#define bzero(ptr,size)  memset((ptr),0,(size));
#endif //bzero

#if defined(ANDROID)
template <typename T>
std::string to_string(T value) {
    std::ostringstream os;
    os << std::forward<T>(value);
    return os.str();
}
#endif//ANDROID

#if defined(_WIN32)
int gettimeofday(struct timeval* tp, void* tzp);
void usleep(int micro_seconds);
void sleep(int second);
int vasprintf(char** strp, const char* fmt, va_list ap);
int asprintf(char** strp, const char* fmt, ...);
const char* strcasestr(const char* big, const char* little);

#if !defined(strcasecmp)
#define strcasecmp _stricmp
#endif

#if !defined(strncasecmp)
#define strncasecmp _strnicmp
#endif

#ifndef ssize_t
#ifdef _WIN64
#define ssize_t int64_t
#else
#define ssize_t int32_t
#endif
#endif
#endif //WIN32


/**
 * 获取时间差, 返回值单位为秒
 */
long getGMTOff();

/**
 * 获取1970年至今的毫秒数
 * @param system_time 是否为系统时间(系统时间可以回退),否则为程序启动时间(不可回退)
 */
uint64_t getCurrentMillisecond(bool system_time = false);

uint64_t getCurrentMicrosecond(bool system_time = false);

std::string getTimeStr(const char *fmt,time_t time = 0);

/**
 * 根据unix时间戳获取本地时间
 * @param sec unix时间戳
 * @return tm结构体
 */
struct tm getLocalTime(time_t sec);

void setThreadName(const char *name);

std::string getThreadName();

/**
 * 根据typeid(class).name()获取类名
 */
std::string demangle(const char *mangled);

/**
 * 获取环境变量内容，以'$'开头
 */
std::string getEnv(const std::string &key);


} // namespace FFZKit


#endif