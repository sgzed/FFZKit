#include <atomic>
#include <csignal>

#include "Util/logger.h"
#include "Util/onceToken.h"
#include "Util/TimeTicker.h"
#include "Thread/ThreadPool.h"

using namespace std;
using namespace FFZKit;

int main() {
    signal(SIGINT,[](int ){
        exit(0);
    });

    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    
    atomic_llong count(0);
    ThreadPool pool(1, ThreadPool::PRIORITY_HIGHEST, false);

    Ticker ticker;
    for(int i=0; i < 1000 * 10000; ++i) {
        pool.async([&]() {
            if(++count >= 1000*10000){ 
                InfoL << "all task done, cost : " << ticker.elapsedTime() << " ms";
            }
        });
    }

    InfoL << "all task posted, queue cost : " << ticker.elapsedTime() << " ms";
    ticker.resetTime();
    pool.start();

    uint64_t lastCount =0, nowCount = 1;
    while(true) {
        this_thread::sleep_for(chrono::seconds(1));
        nowCount = count.load();
        InfoL << "execute " << (nowCount - lastCount) << " tasks in last second";
        if(nowCount - lastCount == 0){
            break;
        }
        lastCount = nowCount;
    }

    return 0; 
}