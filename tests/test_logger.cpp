
#include <iostream>
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

    InfoL << "测试std::cout风格打印：";
    //ostream支持的数据类型都支持,可以通过友元的方式打印自定义类型数据  [AUTO-TRANSLATED:c857af94]
    // All data types supported by ostream are supported, and custom type data can be printed through friend methods
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


}