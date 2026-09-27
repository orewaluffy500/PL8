#pragma once


#include <charconv>
#include <string>
namespace pl8 {
    constexpr int PROGRAM_SIZE = 16384;
    constexpr int REGISTER_COUNT = 129;

    namespace inst {
        constexpr int NOP           = 0x00;
        constexpr int HLT           = 0x01;
        constexpr int SYSCALL       = 0x02;

        constexpr int JMP           = 0x20;
        constexpr int CALL          = 0x21;
        constexpr int RET           = 0x22;
        constexpr int JZ            = 0x23;
        constexpr int JNZ           = 0x24;
        constexpr int CMP           = 0x25;
        constexpr int JL            = 0x26;
        constexpr int JG            = 0x27;
        constexpr int JLE           = 0x28;
        constexpr int JGE           = 0x29;
        constexpr int JE            = 0x2A;
        constexpr int JNE           = 0x2B;

        constexpr int ALLC          = 0x30;
        constexpr int WR            = 0x33;
        constexpr int RD            = 0x34;
        constexpr int SETA          = 0x35;

        constexpr int SETR          = 0x40;
        constexpr int INCR          = 0x41;
        constexpr int DECR          = 0x42;
        constexpr int MOV           = 0x43;

        constexpr int PREP          = 0x50;
        constexpr int ADD           = 0x51;
        constexpr int SUB           = 0x52;
        constexpr int MUL           = 0x53;
        constexpr int DIV           = 0x54;
        constexpr int POW           = 0x55;
    }

    namespace std_syscall {
        constexpr int SOUP          = 0x100;
        constexpr int PINT          = 0x101;
        constexpr int PCHAR         = 0x102;
        constexpr int PSTR          = 0x103;

        constexpr int RINT          = 0x200;
        constexpr int RCHAR         = 0x201;
        constexpr int RSTR          = 0x202;

        constexpr int SLEN          = 0x104;
    }

    namespace value_mode {
        constexpr int REGIST        = 0xAAA;
        constexpr int CONSTANT      = 0xBBB;
        constexpr int ACCUM         = 0xCCC;
    }


    inline int parse_int(const std::string& value, int& out){
        auto [ptr, ec] = std::from_chars(
            value.data(),
            value.data() + value.size(),
            out
        );

        if (ec != std::errc{} || ptr != value.data() + value.size()) return 0;
        return 1;
    }
}