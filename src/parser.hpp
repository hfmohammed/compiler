#pragma once
#include "./tokenization.hpp"

struct Expression {
    
};

struct Statement {

};

class Parser {
private:
    std::vector<Token> m_tokens;
public:
    Parser(std::vector<Token> tokens) {
        m_tokens = tokens;
    }

    void parse() {
        std::cout << "parser not implemented" << std::endl;
        return;
    }
};