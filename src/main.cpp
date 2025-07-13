#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generation.hpp"


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

    Tokenizer tokenizer(content);
    std::vector<Token> tokens = tokenizer.tokenize();
    
    Parser parser(std::move(tokens));
    std::optional<node::NodeExit> tree = parser.parse();

    if (!tree.has_value()) {
        std::cerr << "No exit statement found" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(tree.value());

    {
        std::ofstream ofile("test.asm");
        if (!ofile) {
            std::cerr << "File not found" << std::endl;
        }
        ofile << generator.generate();
    }

    if (system("nasm -felf64 test.asm -o test.o") != 0)
        std::cerr << "NASM failed\n";
    if (system("ld -o test test.o") != 0)
        std::cerr << "Linker failed\n";

    return EXIT_SUCCESS;
}
