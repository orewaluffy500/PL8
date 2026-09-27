#include "pl8cpu.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
int main(){
    std::string filename;
    std::cout << "app file? ";
    std::getline(std::cin, filename);

    std::ifstream file(filename);
    if (!file.is_open()){
        std::cout << "unable to open file: " << filename << std::endl;
        return 3;
    }

    std::string part;
    std::vector<int> program;
    while (file >> part){
        int out = 0;
        if (pl8::parse_int(part, out) == 0){
            std::cerr << "warning: invalid instruction: " << part << "\n";
        }

        program.push_back(out);
    }

    pl8::VirtualMachine vm;
    vm.load_std_syscalls();
    vm.put_program(program);
    vm.run();
    return 0;
}