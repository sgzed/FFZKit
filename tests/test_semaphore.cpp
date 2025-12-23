#include <csignal>
#include <atomic>

#include "Util/logger.h"
#include "Util/util.h"
#include "Util/TimeTicker.h"
#include "Thread/semaphore.h"   

#include "Thread/ThreadGroup.h"

using namespace std;
using namespace FFZKit;

#define MAX_TASK_SIZE (1000 * 10000)

semaphore g_sem; 
atomic_llong g_produced(0);
atomic_llong g_consumed(0);


void onProduce() {
    while (true) {
        ++g_produced;
        g_sem.post();
        if (g_produced >= MAX_TASK_SIZE) {
            break;
        }
    }
}

void onConsume() {
    while (true) {
        g_sem.wait();
        long long expected = g_consumed.load();
        bool success = false;
        while (expected < MAX_TASK_SIZE) {
            if (g_consumed.compare_exchange_weak(expected, expected + 1)) {
                success = true;
                break;
            }
        }
        if (!success) {
            break;
        }
        
        if (g_consumed > MAX_TASK_SIZE) {
            ErrorL << g_consumed << " > " << g_produced;
        }
    }
}   

int main() {
    Logger::Instance().add(std::make_shared<ConsoleChannel>());

    Ticker ticker;
    ThreadGroup thread_producer;

    auto cpus = thread::hardware_concurrency();
    for (unsigned int i = 0; i < cpus; ++i) {
        thread_producer.create_thread([]() {
            onProduce();
        });
    }

    ThreadGroup thread_consumer;
    for (unsigned int i = 0; i < 4; ++i) {
        thread_consumer.create_thread([]() {
            onConsume();
        });
    }

    thread_producer.join_all();
    DebugL << "all producer done! cost: " << ticker.elapsedTime() << 
            " ms, produced: " << g_produced << " consumed: " << g_consumed;

    g_sem.post(4);
    thread_consumer.join_all();

    int i = 5;
    while(--i){
        DebugL << "Program exit countdown:" << i << ", number of consumed tasks:" << g_consumed;
        sleep(1);
    }

    WarnL << "Program exit, total consumed tasks:" << g_consumed;
    return 0; 
}