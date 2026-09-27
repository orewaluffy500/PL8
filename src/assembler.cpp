#include "pl8assembler.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc, char** argv){
    // read file

    std::string filename;
    std::cout << "file (make sure its UTF-8) ? ";
    std::getline(std::cin, filename);

    std::ifstream file(filename);
    if (!file.is_open()){
        std::cerr << "file doesn't exist!" << std::endl;
        return 3;
    }

    std::string contents(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );

    file.close();

    // tokenize

    pl8::assembler::Lexer lexer(contents, filename);
    auto tokens = lexer.make_tokens();

    // compile

    pl8::assembler::Compiler compiler(tokens);
    compiler.compile();

    // write to output file

    std::ofstream output(filename + ".pa");
    if (!output.is_open()){
        std::cerr << "unable to create output file for unknown reasons" << std::endl;
        return 3;
    }

    for (int i = 0; i < compiler.pc; i++){
        output << compiler.program[i] << " ";
    }

    output.flush();
    output.close();
    return 0;
}