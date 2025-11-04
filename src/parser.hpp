#pragma once
#include "./tokenization.hpp"
#include <optional>
#include <variant>
#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"


struct NodeOperator {
    Token token;
};

struct NodeIdentifier {
    Token token;
};

struct NodeInteger {
    Token token;
    int value;
};

struct NodeString {
    Token token;
    std::string value;
};

struct NodeExpression;

struct NodeExpressionBinary {
    NodeExpression* _lhs;
    NodeOperator* _op;
    NodeExpression* _rhs;
};

struct NodeExpression {
    std::variant<NodeExpressionBinary*, NodeInteger*, NodeString*, NodeIdentifier*> _expression;
};

struct NodeDecleration {
    Token* _qualifier;
    Token* _type;
    Token* _identifier;
    NodeExpression* _expression;
};


struct NodeStatement;

struct NodeBlock {
    std::vector<NodeStatement*> _statements;
};

struct NodeProgram {
    std::vector<NodeStatement*> _statements;
};

struct NodeStatement {
    std::variant<NodeDecleration*, NodeBlock*> _statement;
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
        throw std::runtime_error(std::string(RED) + error_msg + " at " + std::to_string(line) + ":" + std::to_string(_char) + "\033[0m");
    }

    void printDebug(std::string msg) {
        std::cout << std::string(CYAN) + "[debug] " + msg + "\033[0m" << std::endl;
    }

    void printOk(std::string msg) {
        std::cout << std::string(GREEN) + "[ok] " + msg + "\033[0m" << std::endl;
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

    bool is_operator(Token token) {
        if (token.getTokenType() == TokenType::_period || token.getTokenType() == TokenType::_dbl_period || token.getTokenType() == TokenType::_plus || token.getTokenType() == TokenType::_minus || token.getTokenType() == TokenType::_not || token.getTokenType() == TokenType::_hat || token.getTokenType() == TokenType::_asterisk || token.getTokenType() == TokenType::_fwd_slash || token.getTokenType() == TokenType::_mod || token.getTokenType() == TokenType::_dbl_asterisk || token.getTokenType() == TokenType::_by || token.getTokenType() == TokenType::_greater_than || token.getTokenType() == TokenType::_less_than || token.getTokenType() == TokenType::_greater_than_equal || token.getTokenType() == TokenType::_less_than_equal || token.getTokenType() == TokenType::_check_equal || token.getTokenType() == TokenType::_not_eq || token.getTokenType() == TokenType::_and || token.getTokenType() == TokenType::_or || token.getTokenType() == TokenType::_xor || token.getTokenType() == TokenType::_dbl_vertical_line) {
            return true;
        } else {
            return false;
        }
    }

    void parseMember() {
        return;
    }

    NodeExpression* parseExpression() {
        printDebug("parsing expression...");
        NodeExpression* expression;
        if (m_tokens_pointer->getTokenType() == TokenType::_string) {
            NodeString _string = NodeString{.token=*m_tokens_pointer, .value=m_tokens_pointer->getStrValue()};
            expression->_expression = &_string;
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_integer) {
            NodeInteger _integer = NodeInteger{.token=*m_tokens_pointer, .value=std::stoi(m_tokens_pointer->getStrValue())};
            expression->_expression = &_integer;
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            NodeIdentifier _identifier = NodeIdentifier{.token=*m_tokens_pointer};
            expression->_expression = &_identifier;
        }
                
        m_tokens_pointer++;

        if (m_tokens_pointer->getTokenType() != TokenType::_semi) {

            if (is_operator(*m_tokens_pointer)) {
                NodeExpressionBinary* binary_expression = new NodeExpressionBinary();
                binary_expression->_lhs = expression;
                binary_expression->_op = new NodeOperator{.token = *m_tokens_pointer};
                m_tokens_pointer++;

                binary_expression->_rhs = parseExpression();
            }

            else if (m_tokens_pointer->getTokenType() == TokenType::_open_square) {
                // handle indexing
            }

            else {
                printError("Expected an operator", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            } 
        }
        
        return expression;
    }

    NodeDecleration* parseDecleration() {
        NodeDecleration* decleration = new NodeDecleration();
        bool is_decleration = false;

        printDebug("checking for qualifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (isQualifier(*m_tokens_pointer)) {
            printOk("found qualifier");
            decleration->_qualifier = &(*m_tokens_pointer);
            printDebug((decleration->_qualifier)->getStrValue());
            m_tokens_pointer++;
            is_decleration = true;
        }

        printDebug("checking for type at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (isType(*m_tokens_pointer)) {
            printOk("found type");

            is_decleration = true;
            decleration->_type = &(*m_tokens_pointer);
            m_tokens_pointer++;

            if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
                printOk("found identifier");

                decleration->_identifier = &(*m_tokens_pointer);
                m_tokens_pointer++;

                if (m_tokens_pointer->getTokenType() == TokenType::_assign) {
                    printOk("found assign");

                    int temp_line = m_tokens_pointer->getLine();
                    int temp_char = m_tokens_pointer->getChar();
                    m_tokens_pointer++;

                    printDebug("need to parse expression...");
                    if ( !(decleration->_expression = parseExpression()) ) {
                        printError("Expected <expression>", temp_line, temp_char);
                        return decleration;
                    }
                }


                if ((m_tokens_pointer)->getTokenType() == TokenType::_semi) {
                    printOk("completed a statement");
                    return decleration;

                } else {
                    printError("Expected `;`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    return decleration;

                }
            }

            printError("expected <identifier>", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return decleration;
        }


        if (is_decleration) {
            printError("expected <type>", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return decleration;
        } else {
            return decleration;
        }
    }


    NodeStatement* parseStatement() {
        NodeStatement* statement = new NodeStatement();
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
            NodeDecleration* declaration = parseDecleration();
            printOk("Decleration parsed");
            if (declaration) {
                statement->_statement = declaration;
            }
        }

        printDebug("Returning statement");
        return statement;
    }

    NodeProgram* parseProgram() {
        NodeProgram* program;
        NodeStatement* statement;

        while (true) {
            if (m_tokens_pointer >= m_tokens.end()) break;
            printDebug("parsing statement...");
            statement = parseStatement();
            break;
            if (!statement) break;
            program->_statements.push_back(statement);
        }

        return program;
    }

    void printTokens() {
        std::cout << "tokens array size " << m_tokens.size() << std::endl;
        for (auto it = m_tokens.begin(); it < m_tokens.end(); it++) {
            std::cout << "::" << it->getStrValue() << std::endl;
        }
        return;
    }

    void parse() {
        printDebug("================= Parser ===============");
        printDebug(std::to_string(m_tokens.size()));
        printTokens();
        parseProgram();
        printOk("Program parsed");
        return;
    }
};
