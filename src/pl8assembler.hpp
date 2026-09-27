#pragma once


#include "pl8const.hpp"
#include <cctype>
#include <format>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace pl8::assembler {

    struct Position {
        std::string_view file_name;
        int index = -1;
        int column = 0;
        int line = 0;

        Position(const std::string& file_name): file_name(file_name){}

        void advance(bool is_newline){
            index++;
            column++;
            if (is_newline){
                column = 0;
                line++;
            }
        }
    };


    inline void fault(const std::string& message, Position pos){
        std::printf("tokenization fault: %s (at line %d, column %d of file '%s')\n", message.c_str(), pos.line, pos.column, pos.file_name.data());
        std::exit(2);
    }

    inline void compilation_fault(const std::string& message, Position pos){
        std::printf("compilation fault: %s (at line %d, col %d of file '%s')\n", message.c_str(), pos.line, pos.column, pos.file_name.data());
        std::exit(4);
    }

    inline void compilation_fault(const std::string& message){
        std::printf("compilation fault: %s\n", message.c_str());
        std::exit(4);
    }


    using TokenType = unsigned char;

    constexpr TokenType TT_KEYWORD  = 0x1;
    constexpr TokenType TT_IDENT    = 0x2;
    constexpr TokenType TT_STRING   = 0x3;
    constexpr TokenType TT_INT      = 0x4;
    constexpr TokenType TT_NEWLINE  = 0x5;
    constexpr TokenType TT_END      = 0x6;
    constexpr TokenType TT_COMMA    = 0x7;
    constexpr TokenType TT_REGIST   = 0x8;
    constexpr TokenType TT_ACCUM    = 0x9;
    constexpr TokenType TT_MINUS    = 0xA;

    inline std::string IGNORED = " \t\v\r";
    inline std::string KEYWORDS[] = {
        "mov", "load", "syscall", "hlt", "nop",
        "label", "jmp", "call", "ret",
        "jz", "jnz", "jg", "jl", "je", "jge", "jle", "jne",
        "cmp", "add", "mul", "div", "sub", "pow",
        "decr", "incr",
        "allc", "setx", "getx", "fill", "str"
    };


    struct Token {
        TokenType type;
        Position pos;
        std::string value;

        Token(TokenType type, Position pos, std::string value = ""): type(type), pos(pos), value(value) {
        }
        Token(): type(0x0), value(""), pos(""){}

        std::string to_string(){
            return std::format("{:02X}:{}", type, value);
        }
    };

    struct Lexer {
        std::string_view source;
        Position pos;
        char current = 0;

        Lexer(const std::string& source, const std::string& file_name): source(source), pos(file_name) {
            advance();
        }

        void advance(){
            pos.advance(current == '\n');
            current = pos.index < source.size() ? source[pos.index] : 0;
        }

        std::vector<Token> make_tokens(){
            std::vector<Token> tokens;

            while (current != 0){
                if (std::find(IGNORED.begin(), IGNORED.end(), current) != IGNORED.end()){
                    advance();
                }

                else if (current == ';'){
                    while (current != 0 && current != '\n'){
                        advance();
                    }
                }

                else if (current == ','){
                    tokens.push_back(Token(TT_COMMA, pos));
                    advance();
                }

                else if (current == '$'){
                    tokens.push_back(Token(TT_ACCUM, pos));
                    advance();
                }

                else if (current == '-'){
                    tokens.push_back(Token(TT_MINUS, pos));
                    advance();
                }

                else if (current == '\''){
                    tokens.push_back(make_character());
                    advance();
                }

                else if (current == '\n'){
                    tokens.push_back(Token(TT_NEWLINE, pos));
                    advance();
                }

                else if (std::isdigit(current)){
                    tokens.push_back(make_int());
                }

                else if (std::isalpha(current) || current == '_'){
                    tokens.push_back(make_ident());
                }

                else if (current == '.'){
                    advance();
                    Token num = make_int();
                    tokens.push_back(Token(TT_REGIST, num.pos, num.value));
                }

                else if (current == '"'){
                    advance();
                    tokens.push_back(make_string());
                    advance();
                }
                else {
                    advance();
                }
            }

            tokens.push_back(Token(TT_END, pos));
            return tokens;
        }

        Token make_character(){
            char final = 0;
            Position start = pos;
            
            advance();
            
            if (current == '\\'){
                advance();
                switch (current){
                    case 'n':
                    final = '\n';
                    break;
                    
                    case 'a':
                    final = '\'';
                    break;

                    case 't':
                    final = '\t';
                    break;

                    case '\\':
                    final = '\\';
                    break;

                    default:
                    fault(std::format("invalid escape char: {}", current), pos);
                }
            } else {
                final = current;
            }

            advance();
            if (current != '\''){
                fault("expected apostrophe!", pos);
            }

            return Token(TT_INT, start, std::to_string((int) final));
        }

        Token make_ident(){
            std::string final = "";
            Position start = pos;

            while (current != 0 && (std::isalnum(current) || current == '_')){
                final += current;
                advance();
            }

            TokenType type = TT_IDENT;
            if (std::find(std::begin(KEYWORDS), std::end(KEYWORDS), final) != std::end(KEYWORDS)){
                type = TT_KEYWORD;
            }

            return Token(type, start, final);
        }

        Token make_int(){
            std::string final = "";
            Position start = pos;

            while (current != 0 && std::isdigit(current)){
                final += current;
                advance();
            }

            return Token(TT_INT, start, final);
        }

        Token make_string(){
            std::string final = "";
            Position start = pos;

            while (current != 0 && current != '"'){
                final += current;
                advance();
            }

            final = std::regex_replace(final, std::regex("\\q"), "\"");
            final = std::regex_replace(final, std::regex("\\n"), "\n");
            final = std::regex_replace(final, std::regex("\\t"), "\t");
            final = std::regex_replace(final, std::regex("\\\\"), "\\");

            return Token(TT_STRING, start, final);
        }
    };





    // COMPILER

    struct Compiler {
        int index = -1;
        int pc = 0;
        int program[PROGRAM_SIZE];
        std::vector<Token> tokens;
        Token current;

        std::unordered_map<std::string, int> label_definitions;
        std::unordered_map<int, std::string> label_references;

        Compiler(std::vector<Token> tokens): tokens(tokens), current(tokens.back()){
            advance();
        }

        void advance(){
            index++;
            current = index < tokens.size() ? tokens[index] : tokens.back();
        }

        void compile(){
            while (current.type != TT_END){
                compile_one();
            }

            // resolve label references
            for (auto [where, dest]: label_references){
                if (!label_definitions.contains(dest)){
                    compilation_fault("unable to resolve label: " + dest);
                }
                program[where] = label_definitions[dest];
            }
        }

        void compile_one(){
            Token token = current;
            if (token.type == TT_KEYWORD){
                advance();
                compile_keyword(token.value);
                return;
            }

            advance();
        }

        void compile_keyword(const std::string& keyword){
            if (keyword == "mov"){
                int regist = expect_register();
                expect_comma();

                push_instruction(inst::MOV);
                push_instruction(regist);
                expect_value_and_push();
            }

            else if (keyword == "incr"){
                int regist = expect_register();

                push_instruction(inst::INCR);
                push_instruction(regist);
            }

            else if (keyword == "decr"){
                int regist = expect_register();

                push_instruction(inst::DECR);
                push_instruction(regist);
            }

            else if (keyword == "syscall"){
                int syscall = expect_constant_int();

                push_instruction(inst::SYSCALL);
                push_instruction(syscall);
            }

            else if (keyword == "label"){
                std::string ident = expect_identifier();

                label_definitions[ident] = pc;
            }

            // jump stuff

            else if (keyword == "jmp"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JMP);
                schedule_jump(label_name);
            }

            else if (keyword == "jl"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JL);
                schedule_jump(label_name);
            }

            else if (keyword == "jg"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JG);
                schedule_jump(label_name);
            }

            else if (keyword == "jle"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JLE);
                schedule_jump(label_name);
            }

            else if (keyword == "jge"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JGE);
                schedule_jump(label_name);
            }

            else if (keyword == "je"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JE);
                schedule_jump(label_name);
            }

            else if (keyword == "jne"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JNE);
                schedule_jump(label_name);
            }

            else if (keyword == "jz"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JZ);
                schedule_jump(label_name);
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "jnz"){
                std::string label_name = expect_identifier();

                push_instruction(inst::JNZ);
                schedule_jump(label_name);
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "call"){
                std::string label_name = expect_identifier();

                push_instruction(inst::CALL);
                schedule_jump(label_name);
            }

            else if (keyword == "ret"){
                push_instruction(inst::RET);
            }

            // operations

            else if (keyword == "cmp"){
                push_instruction(inst::CMP);
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "add"){
                push_instruction(inst::ADD);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "sub"){
                push_instruction(inst::SUB);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "mul"){
                push_instruction(inst::MUL);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "div"){
                push_instruction(inst::DIV);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "pow"){
                push_instruction(inst::POW);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            // heap stuff
            else if (keyword == "allc"){
                push_instruction(inst::ALLC);
                int regist = expect_register();
                push_instruction(regist);
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "setx"){
                push_instruction(inst::WR);
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "getx"){
                push_instruction(inst::RD);
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
                int regist = expect_register();
                push_instruction(regist);
            }

            else if (keyword == "fill"){
                push_instruction(inst::SETA);
                expect_value_and_push();
                expect_comma();
                expect_value_and_push();
            }

            else if (keyword == "str"){
                auto [tokType, addr] = expect_value();
                expect_comma();
                std::string value = expect_string();

                int counter = 0;
                while (counter < value.size()){
                    push_instruction(inst::WR);
                    push_value(tokType, addr);
                    push_value(value_mode::CONSTANT, counter);
                    push_value(value_mode::CONSTANT, (unsigned char) value[counter]);
                    counter++;
                }
            }

            // halt & nop
            else if (keyword == "hlt"){
                push_instruction(inst::HLT);
            }

            else if (keyword == "nop"){
                push_instruction(inst::NOP);
            }
        }


        // helpers

        void push_instruction(int inst){
            program[pc++] = inst;
        }
        
        void push_value(int type, int value){
            program[pc++] = type;
            if (type != value_mode::ACCUM) program[pc++] = value;
        }

        void push_value(TokenType type, int value){
            if (type == TT_REGIST){
                push_value(value_mode::REGIST, value);
            } else {
                push_value(value_mode::CONSTANT, value);
            }
        }

        int expect_register(){
            if (current.type != TT_REGIST){
                compilation_fault("expected register.", current.pos);
            }

            std::string value = current.value;
            advance();
            return std::stoi(value);
        }

        int expect_constant_int(){
            if (current.type == TT_MINUS){
                advance();
                int value = expect_constant_int();

                return -value;
            }

            if (current.type != TT_INT){
                compilation_fault("expected int.", current.pos);
            }
            
            std::string value = current.value;
            advance();
            
            int result = 0;
            parse_int(value, result);
            return result;
        }

        void expect_comma(){
            if (current.type != TT_COMMA){
                compilation_fault("expected comma.", current.pos);
            }

            advance();
        }

        std::string expect_string(){
            if (current.type != TT_STRING){
                compilation_fault("expected string.", current.pos);
            }

            std::string value = current.value;
            advance();
            return value;
        }

        std::string expect_identifier(){
            if (current.type != TT_IDENT){
                compilation_fault("expected identifier.", current.pos);
            }
            
            std::string identifier = current.value;
            advance();
            return identifier;
        }

        std::tuple<TokenType, int> expect_value(bool with_accum = true){
            if (current.type == TT_MINUS){
                return { TT_INT, expect_constant_int() };
            }

            if (current.type != TT_INT && current.type != TT_REGIST){
                compilation_fault("expected int or register.", current.pos);
            }

            std::string value = current.value;
            TokenType type = current.type;
            advance();
            return { type, value != "" ? std::stoi(value) : 0 };
        }

        void expect_value_and_push(){
            auto [type, value] = expect_value();
            push_value(type, value);
        }

        void schedule_jump(std::string label_name){
            label_references[pc] = label_name;
            push_instruction(inst::NOP);
        }
    };
}