#pragma once
#include "pl8const.hpp"
#include "pl8fault.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <format>

namespace pl8 {
    enum class ArithOper {
        ADD, SUB, MUL, DIV, POW, NOP
    };

    struct ArithOperInfo {
        ArithOper type = ArithOper::NOP;
        int operand1 = 0;
        int operand2 = 0;

        void feed(int op1, int op2){
            operand1 = op1;
            operand2 = op2;
        }

        int evaluate(){
            if (type == ArithOper::ADD) return operand1 + operand2;
            else if (type == ArithOper::SUB) return operand1 - operand2;
            else if (type == ArithOper::MUL) return operand1 * operand2;
            else if (type == ArithOper::DIV) return operand1 / operand2;
            else if (type == ArithOper::POW) return std::pow(operand1, operand2);

            return 0;
        }
    };


    struct CompInfo {
        bool greater = false;
        bool less = false;
        bool equal = false;
    };


    struct Allocation {
        std::vector<int> data;
        int size;

        Allocation(int size): size(size) {
            data.resize(size);
            fill(0);
        }
        
        Allocation(){
            data.resize(1);
            fill(0);
        }

        void fill(int value){
            std::fill(data.begin(), data.end(), value);
        }

        int get(int index){
            if (index < 0 || index >= size){
                fault(std::format("out-of-bounds index '{}'", index));
            }

            return data[index];
        }
    };


    struct VirtualMachine;
    using Syscall = std::function<void(VirtualMachine&)>;

    struct VirtualMachine {
        int pc = 0;
        int addr_counter = 0;
        int accum = 0;

        ArithOperInfo ao_info;
        CompInfo comp_info;

        int program_file[PROGRAM_SIZE]{};
        int register_file[REGISTER_COUNT]{};

        std::unordered_map<int, Allocation> memory;
        std::unordered_map<int, Syscall> syscall_map;

        std::vector<int> call_stack;

        // helpers
        
        int get_next(){
            if (pc + 1 >= PROGRAM_SIZE){
                fault("expected instruction", pc);
            }
            return program_file[pc++];
        }

        int get_next_value(){
            int mode = get_next();
            
            if (mode == value_mode::REGIST){
                return get_register(get_next());
            } 
            else if (mode == value_mode::ACCUM){
                return loaded();
            }
            else {
                return get_next();
            }
        }

        int loaded(){ return accum; }
        void load(int value){ accum = value; }
        
        int get_register(int index){
            if (index < 0 || index >= REGISTER_COUNT){
                fault(std::format("invalid register index '{}'", index), pc);
            }

            return register_file[index];
        }

        void set_register(int index, int value){
            get_register(index);
            register_file[index] = value;
        }

        Allocation& get_alloc(int addr){
            if (!memory.contains(addr)){
                fault(std::format("unknown address '{}'!", addr), pc);
            }

            return memory[addr];
        }

        void allocate(int size){
            memory[addr_counter++] = Allocation(size);
        }

        // exec-related

        bool cycle(){
            int instruction = get_next();

            switch (instruction){
            case inst::HLT: {
                return false;
            }

            // LOADING
            case inst::LD: {
                int value = get_next();
                load(value);
                break;
            }
            case inst::LDR: {
                int regs = get_next();
                load(get_register(regs));
                break;
            }

            // REGISTERS
            case inst::SETR: {
                set_register(get_next(), loaded());
                break;
            }
            case inst::MOV: {
                int regist = get_next();
                int value = get_next_value();
                set_register(regist, value);
                break;
            }
            case inst::INCR: {
                int regs = get_next();
                get_register(regs);
                register_file[regs]++;
                break;
            }
            case inst::DECR: {
                int regs = get_next();
                get_register(regs);
                register_file[regs]--;
                break;
            }

            // ALLOCATIONS
            case inst::ALLC: {
                int size = get_next_value();
                load(addr_counter);
                allocate(size);
                break;
            }
            case inst::RD: {
                int addr = loaded();
                int index = get_next_value();
                auto& alloc = get_alloc(addr);

                load(alloc.get(index));
                break;
            }
            case inst::WR: {
                int addr = loaded();
                int index = get_next_value();
                int value = get_next_value();
                auto& alloc = get_alloc(addr);

                alloc.get(index);
                alloc.data[index] = value;
                break;
            }
            case inst::SETA: {
                int addr = loaded();
                int value = get_next_value();
                auto& alloc = get_alloc(addr);

                alloc.fill(value);
                break;
            }

            // SYSCALL
            case inst::SYSCALL: {
                int syscall = get_next();
                if (syscall_map.contains(syscall)){
                    syscall_map[syscall](*this);
                } else {
                    fault(std::format("unexpected syscall '{}'", syscall), pc);
                }
                break;
            }

            // JUMP
            case inst::JMP: {
                int where = get_next();
                jump(where);
                break;
            }

            case inst::CALL: {
                int where = get_next();
                call_stack.push_back(pc);
                jump(where);
                break;
            }

            case inst::RET: {
                ret();
                break;
            }

            case inst::JZ: {
                int where = get_next();
                if (loaded() == 0) jump(where);
                break;
            }

            case inst::JNZ: {
                int where = get_next();
                if (loaded() != 0) jump(where);
                break;
            }

            case inst::CMP: {
                int value1 = get_next_value();
                int value2 = get_next_value();

                comp_info.greater = value1 > value2;
                comp_info.less = value1 < value2;
                comp_info.equal = value1 == value2;
                break;
            }
            
            case inst::JL: {
                int where = get_next();
                if (comp_info.less) jump(where);
                break;
            }

            case inst::JG: {
                int where = get_next();
                if (comp_info.greater) jump(where);
                break;
            }

            case inst::JLE: {
                int where = get_next();
                if (comp_info.less || comp_info.equal) jump(where);
                break;
            }

            case inst::JGE: {
                int where = get_next();
                if (comp_info.greater || comp_info.equal) jump(where);
                break;
            }

            case inst::JE: {
                int where = get_next();
                if (comp_info.equal) jump(where);
                break;
            }

            case inst::JNE: {
                int where = get_next();
                if (!comp_info.equal) jump(where);
                break;
            }


            // Arithmetic operations

            case inst::ADD: {
                ao_info.type = ArithOper::ADD;
                ao_info.feed(get_next_value(), get_next_value());
                load(ao_info.evaluate());
                break;
            }

            case inst::SUB: {
                ao_info.type = ArithOper::SUB;
                ao_info.feed(get_next_value(), get_next_value());
                load(ao_info.evaluate());
                break;
            }

            case inst::MUL: {
                ao_info.type = ArithOper::MUL;
                ao_info.feed(get_next_value(), get_next_value());
                load(ao_info.evaluate());
                break;
            }

            case inst::DIV: {
                ao_info.type = ArithOper::DIV;
                ao_info.feed(get_next_value(), get_next_value());
                load(ao_info.evaluate());
                break;
            }

            case inst::POW: {
                ao_info.type = ArithOper::POW;
                ao_info.feed(get_next_value(), get_next_value());
                load(ao_info.evaluate());
                break;
            }
            }

            return true;
        }

        void jump(int to){
            if (to < 0 || to >= PROGRAM_SIZE){
                fault(std::format("invalid jump '{}', out of bounds.", to), pc);
            }

            pc = to;
        }

        void ret(){
            if (call_stack.empty()) return;
            auto top = call_stack.back();
            pc = top;
            call_stack.pop_back();
        }

        void run(){
            while (cycle()){}
        }

        // user-related

        void put_program(std::vector<int> program){
            std::copy_n(program.begin(), std::min(program.size(), (std::size_t) PROGRAM_SIZE), std::begin(program_file));
        }

        void set_syscall(int key, Syscall handler){
            syscall_map[key] = handler;
        }

        void load_std_syscalls(){
            set_syscall(std_syscall::SOUP, [](auto& vm){
                std::printf("soup\n");
            });

            set_syscall(std_syscall::PINT, [](auto& vm){
                std::printf("%d", vm.loaded());
            });

            set_syscall(std_syscall::PCHAR, [](auto& vm){
                std::printf("%c", vm.loaded());
            });

            set_syscall(std_syscall::PSTR, [](auto& vm){
                int addr = vm.loaded();
                auto& alloc = vm.get_alloc(addr);
                int index = 0;
                while (index < alloc.size && alloc.get(index) != 0){
                    std::printf("%c", alloc.get(index));
                    index++;
                }
            });

            set_syscall(std_syscall::RINT, [](auto& vm){
                int x;
                std::scanf("%d", &x);
                std::fflush(stdin);

                vm.load(x);
            });

            set_syscall(std_syscall::RCHAR, [](auto& vm){
                char x;
                std::scanf("%c", &x);
                std::fflush(stdin);

                vm.load(x);
            });

            set_syscall(std_syscall::RSTR, [](auto& vm){
                std::string x;
                std::getline(std::cin, x);

                auto& alloc = vm.get_alloc(vm.loaded());
                
                std::copy_n(x.begin(), std::min(x.size(), alloc.data.size()), alloc.data.begin());
            });

            set_syscall(std_syscall::SLEN, [](auto& vm){
                Allocation& alloc = vm.get_alloc(vm.loaded());

                int counter = 0;
                while (counter < alloc.size){
                    if (alloc.data[counter] == 0) break;
                    counter++;
                }

                vm.load(counter);
            });
        }
    };
}