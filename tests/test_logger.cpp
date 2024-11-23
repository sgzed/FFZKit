
#include <iostream>
#include <thread>
#include <chrono>
#include "Util/logger.h"

using namespace std;
using namespace FFZKit;

class TestLog {
public:
    template<typename T>
    TestLog(const T &t){
        _ss << t;
    };
    ~TestLog(){};

    //通过此友元方法，可以打印自定义数据类型
    friend ostream& operator<<(ostream& out,const TestLog& obj){
        return out << obj._ss.str();
    }
private:
    stringstream _ss;
};

int main(){
    // Initialize the logging system
    Logger::Instance().add(std::make_shared<ConsoleChannel> ());
    Logger::Instance().add(std::make_shared<FileChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    InfoL << "test std::cout style print";
    TraceL << "object int"<< TestLog((int)1)  << endl;
    DebugL << "object short:"<<TestLog((short)2)  << endl;
    InfoL << "object float:" << TestLog((float)3.12345678)  << endl;
    WarnL << "object double:" << TestLog((double)4.12345678901234567)  << endl;
    ErrorL << "object void *:" << TestLog((void *)0x12345678) << endl;
    ErrorL << "object string:" << TestLog("test string") << endl;

    //这是ostream原生支持的数据类型  [AUTO-TRANSLATED:c431abc8]
    // These are the data types natively supported by ostream
    TraceL << "int"<< (int)1  << endl;
    DebugL << "short:"<< (short)2  << endl;
    InfoL << "float:" << (float)3.12345678  << endl;
    WarnL << "double:" << (double)4.12345678901234567  << endl;
    ErrorL << "void *:" << (void *)0x12345678 << endl;
    //根据RAII的原理，此处不需要输入 endl，也会在被函数栈pop时打印log
    ErrorL << "without endl!";

    PrintI("test printf style print:");
    PrintT("this is a %s test:%d", "printf trace", 124);
    PrintD("this is a %s test:%p", "printf debug", (void*)124);
    PrintI("this is a %s test:%c", "printf info", 'a');
    PrintW("this is a %s test:%X", "printf warn", 0x7F);
    PrintE("this is a %s test:%x", "printf err", 0xab);


    for (int i = 0; i < 2; ++i) {
        DebugL << "this is a repeat 2 times log";
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    for (int i = 0; i < 3; ++i) {
        DebugL << "this is a repeat 3 times log";
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    for (int i = 0; i < 100; ++i) {
        DebugL << "this is a repeat 100 log";
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    InfoL << "done!";
    return 0;

}