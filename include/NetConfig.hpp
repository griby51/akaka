#pragma once

#include <cstdint>
#include <string>

struct NetConfig{
    enum class Mode { Offline, Host, Client };

    Mode mode = Mode::Offline;
    std::string address = "127.0.0.1";
    uint16_t port = 7777;
};

NetConfig& netConfig();
void parseNetArgs(int argc, char* args[]);
