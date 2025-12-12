
#include <csignal>
#include <iostream> 
#include <random>

#include "Util/logger.h"
#include "Thread/ThreadGroup.h"
#include "Util/ResourcePool.h"

using namespace std;
using namespace FFZKit;

// Program exit flag
bool g_bExitFlag = false;

class string_imp : public string{
public:
    template<typename ...ArgTypes>
    string_imp(ArgTypes &&...args) : string(std::forward<ArgTypes>(args)...){
        DebugL << "construct string_imp:" << this << " " << *this;
    };
    ~string_imp(){
        WarnL << "destruct string_imp:" << this << " " << *this;
    }
};

void onRun(ResourcePool<string_imp> &pool,int threadNum){
    std::random_device rd;
    while(!g_bExitFlag){
        // Get an available object from the loop pool
        auto obj_ptr = pool.obtain();
        if(obj_ptr->empty()){
            // This object is brand new and unused
            InfoL << "backend thread: " << threadNum << ":" << "obtain a emptry object!";
        }else{
            // This object is looped for reuse
            InfoL << "backend thread " << threadNum << ":" << *obj_ptr;
        }
    
        // Mark this object as used by the current thread
        obj_ptr->assign(StrPrinter << "keeped by thread:" << threadNum );

        // Random sleep to disrupt the loop usage order
        usleep( 1000 * (rd()% 10));
        obj_ptr.reset();
        usleep( 1000 * (rd()% 1000));
    }
}

int main() {
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());


    ResourcePool<string_imp> pool;
    pool.setSize(50);

    // Get an object that will be held by the main thread
    auto reservedObj = pool.obtain();
    reservedObj->assign("This is a reserved object , and will never be used!");

    WarnL << "Main thread obtained reserved object:" << reservedObj.get() << " " << *reservedObj;
    ThreadGroup group;
    for(int i = 0 ;i < 4 ; ++i){
        group.create_thread([i,&pool](){
            onRun(pool,i);
        });
    }

    sleep(3);

    WarnL << "Main thread releasing reserved object:" << reservedObj.get() << " " << *reservedObj;
    
    auto &objref = *reservedObj;

    reservedObj.reset(); 

    sleep(3);

    WarnL << "reserved object released:" << &objref;

    {
        WarnL << "Test recycle quit function:";

        List<decltype(pool)::ValuePtr> objlist;
        for (int i = 0; i < 8; ++i) {
            reservedObj = pool.obtain();
            string str = StrPrinter << i << " " << (i % 2 == 0 ? "quit" : "recycle");
            reservedObj->assign(str);
            reservedObj.quit(i % 2 == 0);
            objlist.emplace_back(reservedObj);
        }
    }

    sleep(3);

    g_bExitFlag = true;
    group.join_all();

    return 0;
}