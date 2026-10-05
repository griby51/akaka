#include "NetConfig.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

NetConfig& netConfig(){
    static NetConfig config;
    return config;
}

static uint16_t parsePort(const char* text, uint16_t fallback){
    long value = strtol(text, nullptr, 10);
    if(value <= 0 || value > 65535){
        printf("[net] invalid port : %s, using %u\n", text, fallback);
        return fallback;
    }

    return (uint16_t)value;
}

void parseNetArgs(int argc, char* args[]){
    NetConfig& config = netConfig();

    for(int i = 1; i < argc; i++){
        if(strcmp(args[i], "--host") == 0){
            config.mode = NetConfig::Mode::Host;

            if(i + 1 < argc && args[i + 1][0] != '-'){
                config.port = parsePort(args[i + 1], config.port);
                i++;
            }

            printf("[net] host mode, port %u\n", config.port);
        }else if(strcmp(args[i], "--join") == 0){
            if(i + 1 >= argc){
                printf("[net] --join needs an address\n");
                continue;
            }

            config.mode = NetConfig::Mode::Client;
            config.address = args[i + 1];
            i++;

            if(i + 1 < argc && args[i + 1][0] != '-'){
                config.port = parsePort(args[i + 1], config.port);
                i++;
            }

            printf("[net] client mode, %s:%u\n", config.address.c_str(), config.port);
        }else{
            printf("[net] unknown argument : %s\n", args[i]);
        }
    }
}
