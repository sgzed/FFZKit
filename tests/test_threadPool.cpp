#include <chrono>

#include "Util/logger.h"
#include "Util/onceToken.h"
#include "Util/TimeTicker.h"
#include "Thread/ThreadPool.h"

using namespace std;
using namespace FFZKit;

int main() {
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    auto cpus = std::thread::hardware_concurrency();
    ThreadPool pool(cpus, ThreadPool::PRIORITY_HIGHEST, true);

    auto task_second = 3;
    auto task_count = cpus * 4;

    semaphore sem;
    vector<int> vec;
    vec.resize(task_count);
    Ticker ticker;

    {
        auto token = make_shared<OnceToken>(nullptr, [&]() {
            sem.post();
        });

        for (int i = 0; i < task_count; ++i) {
            pool.async([i, task_second, &vec, token]() {
                setThreadName(("thread_pool " + to_string(i)).c_str());
                this_thread::sleep_for(chrono::seconds(task_second));
                InfoL << "task " << i << " done!";
                vec[i] = i;
            });
        }
    }

    sem.wait();
    InfoL << "all task done! cost: " << ticker.elapsedTime() << " ms";
    for (auto &i : vec) {
        InfoL << "task result: " << i;
    }
    return 0; 
}