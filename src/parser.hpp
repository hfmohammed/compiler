#pragma once
#include "./tokenization.hpp"
#include <optional>


struct NodeQualifier {
    Token* token;
};

struct NodeType {
    Token* token;
};

struct Operator {
    Token* token;
};

struct NodeExpression {
    std::optional<NodeExpression*> _lhs;
    std::optional<NodeExpression*> _rhs;
    std::optional<NodeExpression*> _op;
};

struct NodeDecleration {
    std::optional<Token> _qualifier;
    Token _type;
    Token _identifier;
    std::optional<NodeExpression> _expression;
};

struct NodeStatement {
    std::optional<NodeDecleration> _decleration;
};

struct NodeBlock {
    std::vector<NodeStatement*> _statements;
};

struct NodeProgram {
    std::vector<NodeStatement> _statements;
};

class Parser {
private:
    std::vector<Token> m_tokens;
    std::vector<Token>::iterator m_tokens_pointer;
public:
    Parser(std::vector<Token>& tokens) {
        m_tokens = tokens;
        m_tokens_pointer = m_tokens.begin();
    }

    void printError(std::string error_msg, int line, int _char) {
        throw std::runtime_error(error_msg + " at " + std::to_string(line) + ":" + std::to_string(_char));
    }

    void printDebug(std::string error_msg) {
        std::cout << error_msg << std::endl;
    }


    bool isQualifier(Token& token) {
        TokenType token_type = token.getTokenType();

        printDebug(token.getStrValue());

        return token_type == TokenType::_const || token_type == TokenType::_var;
    }


    bool isType(Token& token) {
        TokenType token_type = token.getTokenType();
        
        if (token_type == TokenType::_boolean || token_type == TokenType::_character || token_type == TokenType::_integer || token_type == TokenType::_real ||
            token_type == TokenType::_tuple || token_type == TokenType::_struct || token_type == TokenType::_vector || token_type == TokenType::_string
        ) {
            return true;
        }

        return false;
    }

    NodeExpression parseExpression() {
        return NodeExpression{};
    }

    std::optional<NodeDecleration> parseDecleration() {
        NodeDecleration decleration;
        bool is_decleration = false;

        printDebug("checking for qualifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (isQualifier(*m_tokens_pointer)) {
            printDebug("found qualifier");
            decleration._qualifier = *m_tokens_pointer;
            m_tokens_pointer++;
            is_decleration = true;
        }

        if (isType(*m_tokens_pointer)) {
            printDebug("found type");

            is_decleration = true;
            decleration._type = *m_tokens_pointer;

            if ((++m_tokens_pointer)->getTokenType() == TokenType::_identifier) {
                printDebug("found identifier");

                decleration._identifier = *m_tokens_pointer;

                if ((++m_tokens_pointer)->getTokenType() == TokenType::_assign) {
                    printDebug("found assign");

                    int temp_line = m_tokens_pointer->getLine();
                    int temp_char = m_tokens_pointer->getChar();
                    m_tokens_pointer++;

                    printDebug("parse expression");
                    if ( !(decleration._expression = parseExpression()) ) {
                        printError("Expected <expression>", temp_line, temp_char);
                        return std::nullopt;
                    }
                }


                if ((++m_tokens_pointer)->getTokenType() == TokenType::_semi) {
                    printDebug("completed a statement");
                    return decleration;
                } else {
                    printError("Expected `;`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    return std::nullopt;
                }
            }

            printError("expected <identifier>", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return std::nullopt;
        }


        if (is_decleration) {
            printError("expected <type>", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return std::nullopt;
        } else {
            return std::nullopt;
        }
    }


    std::optional<NodeStatement> parseStatement() {
        NodeStatement statement;
        if (m_tokens_pointer->getTokenType() == TokenType::_minus && (m_tokens_pointer + 1)->getTokenType() == TokenType::_greater_than) {} // parse stream input
        else if (m_tokens_pointer->getTokenType() == TokenType::_less_than && (m_tokens_pointer + 1)->getTokenType() == TokenType::_minus) {} // parse stream output
        else if (m_tokens_pointer->getTokenType() == TokenType::_else && (m_tokens_pointer + 1)->getTokenType() == TokenType::_if) {} // parse else if
        else if (m_tokens_pointer->getTokenType() == TokenType::_if) {} // parseIf()
        else if (m_tokens_pointer->getTokenType() == TokenType::_else) {} // parseElse()
        else if (m_tokens_pointer->getTokenType() == TokenType::_loop) {} // parseLoop()
        else if (m_tokens_pointer->getTokenType() == TokenType::_open_curly) {} // parseBlock()
        else if (m_tokens_pointer->getTokenType() == TokenType::_break) {} // parseBreak()
        else if (m_tokens_pointer->getTokenType() == TokenType::_continue) {} // parseContinue()
        else if (m_tokens_pointer->getTokenType() == TokenType::_return) {} // parseReturn()
        else {
            printDebug("parsing decleration... " + std::to_string(m_tokens_pointer - m_tokens.begin()));
            std::optional<NodeDecleration> declaration = parseDecleration();
            if (declaration.has_value()) {
                statement._decleration = *declaration;
            }
        }

        return statement;
    }

    NodeProgram parseProgram() {
        NodeProgram program;
        std::optional<NodeStatement> statement;

        while (true) {
            if (m_tokens_pointer >= m_tokens.end()) break;
            printDebug("parsing statement...");
            statement = parseStatement();
            break;
            if (!statement) break;
            program._statements.push_back(*statement);
        }

        return program;
    }

    void printTokens() {
        std::cout << "tokens array size " << m_tokens.size() << std::endl;
        for (auto it = m_tokens.begin(); it < m_tokens.end(); it++) {
            std::cout << "::" << (*it).getStrValue() << std::endl;
        }
        return;
    }

    void parse() {
        printDebug("================= Parser ===============");
        printDebug(std::to_string(m_tokens.size()));
        printTokens();
        parseProgram();
        return;
    }
};
