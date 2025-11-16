#pragma once
#include "./tokenization.hpp"
#include "./parser.hpp"

class Generator {
    private:
        NodeProgram* m_program;
    public:

    Generator(std::vector<Token>& tokens, NodeProgram* program) {
        printDebug("================= Generator ===============");
        m_program = program;

        Parser Parser(tokens);
        Parser.printProgram(m_program);
    }

    void printError(std::string error_msg) {
        throw std::runtime_error(std::string(RED) + error_msg + "\033[0m");
    }

    void printError(std::string error_msg, int line, int _char) {
        throw std::runtime_error(std::string(RED) + error_msg + " at " + std::to_string(line) + ":" + std::to_string(_char) + "\033[0m");
    }

    void printDebug(std::string msg) {
        std::cout << std::string(CYAN) + "[debug] " + msg + "\033[0m" << std::endl;
    }

    void printOk(std::string msg) {
        std::cout << std::string(GREEN) + "[ok] " + msg + "\033[0m" << std::endl;
    }
};