#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generator.hpp"

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

    Parser parser(tokens);
    NodeProgram* program = parser.parse();

    Generator generator(program);
    generator.generate();

    return EXIT_SUCCESS;
}
