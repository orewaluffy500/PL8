#include "pl8const.hpp"
#include "pl8cpu.hpp"
#include <vector>

using namespace pl8::inst;
using namespace pl8::std_syscall;
using namespace pl8::value_mode;

int main(int argc, char **argv) {
    std::vector<int> program = {
        ALLC, CONSTANT, 16,
        SYSCALL, PINT,
        HLT
    };

    pl8::VirtualMachine vm;

    vm.put_program(program);
    vm.load_std_syscalls();

    vm.run();
    return 0;
}
