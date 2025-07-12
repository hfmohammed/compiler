#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include "./tokenization.hpp"

std::string tokens_to_asm(const std::vector<Token> tokens) {
    std::stringstream output_stream;
    output_stream << "global _start\n_start:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i).type == TokenType::_exit) {
            if ((i + 1) < tokens.size() && tokens.at(i + 1).type == TokenType::int_lit) {
                if ((i + 2) < tokens.size() && tokens.at(i + 2).type == TokenType::semi) {
                    output_stream << "    mov rax, 60" << std::endl;
                    output_stream << "    mov rdi, " << tokens.at(i + 1).value.value() << std::endl;
                    output_stream << "    syscall" << std::endl;
                }
            }
        }
    }

    std::string output_asm = output_stream.str();
    std::cout << output_asm;
    return output_asm;
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect number of parameters" << std::endl;
    }

    std::string content;
    {
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "File not found" << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str(); 
    }

    Tokenizer tokenizer(std::move(content));
    std::vector<Token> tokens = tokenizer.tokenize();
    std::string asm_code = tokens_to_asm(tokens);

    {
        std::ofstream ofile("test.asm");
        if (!ofile) {
            std::cerr << "File not found" << std::endl;
        }
        ofile << asm_code;
    }

    if (system("nasm -felf64 test.asm -o test.o") != 0)
        std::cerr << "NASM failed\n";
    if (system("ld -o test test.o") != 0)
        std::cerr << "Linker failed\n";

    return EXIT_SUCCESS;
}
