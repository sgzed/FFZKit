#include <csignal>
#include "Util/logger.h"
#include "Util/NoticeCenter.h"

using namespace std;
using namespace FFZKit;


//广播名称1
#define EVENT_NAME1 "EVENT_NAME1"
//广播名称2
#define EVENT_NAME2 "EVENT_NAME2"

//程序退出标记
bool g_bExitFlag = false;

int main() {
	//设置程序退出信号处理函数
	signal(SIGINT, [](int) {g_bExitFlag = true;});

	Logger::Instance().add(std::make_shared<ConsoleChannel>());

	//对事件NOTICE_NAME1新增一个监听
	NoticeCenter::Instance().addListener(0, EVENT_NAME1,

		[](int& a, const char*& b, double& c, string& d) {
			DebugL << "a:" << a << ",b:" << b << ",c:" << c << ",d:" << d;

			NoticeCenter::Instance().delListener(0, EVENT_NAME1);

			NoticeCenter::Instance().addListener(0, EVENT_NAME1,
				[](int& a, const char*& b, double& c, string& d) {
					InfoL << a << " " << b << " " << c << " " << d;
				});
	});

	//监听NOTICE_NAME2事件
	NoticeCenter::Instance().addListener(0, EVENT_NAME2,
		[](string& d, double& c, const char*& b, int& a) {
			DebugL << a << " " << b << " " << c << " " << d;
		
		NoticeCenter::Instance().delListener(0, EVENT_NAME2);
		NoticeCenter::Instance().addListener(0, EVENT_NAME2,
			[](string& d, double& c, const char*& b, int& a) {
				WarnL << a << " " << b << " " << c << " " << d;
			});
    });

	int a = 0;

	while (!g_bExitFlag) {
		const char* b = "b";
		double c = 3.14;
		string d("d");

		//每隔1秒广播一次事件，如果无法确定参数类型，可加强制转换
		NoticeCenter::Instance().emitEvent(EVENT_NAME1, ++a, (const char*)"b", c, d);
		NoticeCenter::Instance().emitEvent(EVENT_NAME2, d, c, b, a);
		sleep(1); // sleep 1 second
	}

	return 0;
}