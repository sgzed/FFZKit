#include <iostream>
#include <csignal>

#include "Util/logger.h"
#include "Util/util.h"
#include "Thread/semaphore.h"
#include "Poller/Pipe.h"

using namespace std;
using namespace FFZKit;

int main() {
    // Set up logging
    Logger::Instance().add(std::make_shared<ConsoleChannel>());

     // Get the parent process's PID
    auto parentPid = getpid();
    InfoL << "parent pid:" << parentPid << endl;

    Pipe pipe([](int size, const char *buf) {
        InfoL << getpid() << " received data: " << buf;
    });

    auto pid = fork();
    if (pid == 0) {
        // child process 
        int i = 10; 
        while(i--) {
            sleep(1);
            string msg = StrPrinter << "message " << i << " from child process " << getpid();
            DebugL << "subprocess sending: " << msg << endl;
            pipe.send(msg.data(), msg.size());
        }
        DebugL << "child process exit" << endl;
        exit(0);
    } else {
        static semaphore sem;
        signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
        sem.wait();
        InfoL << "parent process exit" << endl;
    }

    return 0;
}

