//
// Created by Administrator on 2024-11-23.
//

#include "Util/logger.h"
#include "Util/mini.h"

using namespace FFZKit;

class TestVariant {
public:
    TestVariant(int value) : value(value) {}

    int getValue() const { return value; }

private:
    int value;
};


int main() {
    Logger::Instance().add(std::make_shared<ConsoleChannel>());
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

    mINI ini;
    ini.emplace("a", "true");
    ini["b"] = "false";
    ini["c"] = "123";
    ini["d"] = "good";

    InfoL << ini["a"].as<bool>() << (bool) ini["a"];
    InfoL << ini["b"].as<bool>() << (bool) ini["b"];
    InfoL << ini["c"].as<int>() << (int) ini["c"];
    InfoL << (int)(ini["c"].as<uint8_t>()) << (int)((uint8_t) ini["c"]);

    InfoL << ini["d"].as<int>() << (int) ini["d"];

    return 0;
}