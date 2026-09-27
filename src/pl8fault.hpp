#pragma once

#include <cstdio>
#include <string>
namespace pl8 {
    inline void fault(const std::string& message, int pc){
        std::printf("execution fault: %s (at %d)\n", message.c_str(), pc);
        std::exit(1);
    }

    inline void fault(const std::string& message){
        std::printf("execution fault: %s\n", message.c_str());
        std::exit(1);
    }
}