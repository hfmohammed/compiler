#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <typeinfo>

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
    NodeProgram* m_program;
    std::vector<Token> m_tokens;
    std::vector<Token>::iterator m_tokens_pointer;
public:
    Parser(std::vector<Token>& tokens) {
        m_program = new NodeProgram();
        m_tokens = tokens;
        m_tokens_pointer = m_tokens.begin();
    }

    void printExpression(NodeExpression* expression, int indent) {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }

        if (std::holds_alternative<NodeExpressionBinary*>(expression->_expression)) {
            // printDebug(output.str() + "[NodeExpressionBinary]");

            NodeExpressionBinary* node_binary = std::get<NodeExpressionBinary*>(expression->_expression);
            if (node_binary->_op) {
                // printDebug("Found operator");
                printDebug(output.str() + node_binary->_op->token.getStrValue() + " (");
                // printDebug(typeid(node_binary->_lhs).name());
                // if (std::holds_alternative<NodeExpressionBinary*>(node_binary->_lhs->_expression))
                printExpression(node_binary->_lhs, indent + 1);
                printExpression(node_binary->_rhs, indent + 1);
                printDebug(output.str() + ")");
            }

        } else if (std::holds_alternative<NodeInteger*>(expression->_expression)) {
            // printDebug("else if (std::holds_alternative<NodeInteger*>(expression->_expression))");
            NodeInteger* node_integer = std::get<NodeInteger*>(expression->_expression);
            printDebug(output.str() + std::to_string(node_integer->value));

            
        } else if (std::holds_alternative<NodeString*>(expression->_expression)) {
            printDebug("else if (std::holds_alternative<NodeString*>(expression->_expression))");
            
        } else if (std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
            printDebug("else if (std::holds_alternative<NodeIdentifier*>(expression->_expression))");
            
        } else {

            printDebug("else");
        }
    }

    void printDecleration(NodeDecleration* decleration, int indent) {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }
        
        printDebug(output.str() + decleration->_qualifier->getStrValue());
        printDebug(output.str() + decleration->_type->getStrValue());
        printDebug(output.str() + decleration->_identifier->getStrValue());
        if (decleration->_expression != NULL) {
            printDebug(output.str() + "[Expression]");
            printExpression(decleration->_expression, indent);
        } else {
            printDebug(output.str() + "[No expression]");
        }
    }

    void printStatement(NodeStatement* statement, int indent) {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }

        if (std::holds_alternative<NodeDecleration*>(statement->_statement)) {
            output << "[Decleration]";
            printDebug(output.str());
            printDecleration(std::get<NodeDecleration*>(statement->_statement), indent + 1);

        } else if (std::holds_alternative<NodeBlock*>(statement->_statement)) {
            output << "[block]";
            printDebug(output.str());
            printError("Not implemented");
        }
    }


    void printProgram(NodeProgram* program){
        printDebug(std::string("printing program") + std::to_string((program->_statements).size()));

        int indent = 0;
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }

        for (std::vector<NodeStatement*>::iterator i = (program->_statements).begin(); i < (program->_statements).end(); i++) {
            output << "[statement " << i - program->_statements.begin() << "]";
            printDebug(output.str());
            printStatement(*i, indent + 1);
            output.str("");
        }
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
        // checks if token is an operator
        return token.getTokenType() == TokenType::_period || 
            token.getTokenType() == TokenType::_dbl_period || 
            token.getTokenType() == TokenType::_plus || 
            token.getTokenType() == TokenType::_minus || 
            token.getTokenType() == TokenType::_not || 
            token.getTokenType() == TokenType::_hat || 
            token.getTokenType() == TokenType::_asterisk || 
            token.getTokenType() == TokenType::_fwd_slash || 
            token.getTokenType() == TokenType::_mod || 
            token.getTokenType() == TokenType::_dbl_asterisk || 
            token.getTokenType() == TokenType::_by || 
            token.getTokenType() == TokenType::_greater_than || 
            token.getTokenType() == TokenType::_less_than || 
            token.getTokenType() == TokenType::_greater_than_equal || 
            token.getTokenType() == TokenType::_less_than_equal || 
            token.getTokenType() == TokenType::_check_equal || 
            token.getTokenType() == TokenType::_not_eq || 
            token.getTokenType() == TokenType::_and || 
            token.getTokenType() == TokenType::_or || 
            token.getTokenType() == TokenType::_xor || 
            token.getTokenType() == TokenType::_dbl_vertical_line;
    }

    void parseMember() {
        return;
    }

    NodeExpression* parseExpression() {
        printDebug("parsing expression...");
        NodeExpression* expression = new NodeExpression();
        if (m_tokens_pointer->getTokenType() == TokenType::_string) {
            NodeString* _string = new NodeString{.token=*m_tokens_pointer, .value=m_tokens_pointer->getStrValue()};
            expression->_expression = _string;
            printDebug("Added string");
        }

        else if ((m_tokens_pointer)->getTokenType() == TokenType::_number) {
            NodeInteger* _integer = new NodeInteger{.token=*m_tokens_pointer, .value=std::stoi(m_tokens_pointer->getStrValue())};
            expression->_expression = _integer;
            printDebug("Added integer");
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            NodeIdentifier* _identifier = new NodeIdentifier{.token=*m_tokens_pointer};
            expression->_expression = _identifier;
            printDebug("Added identifier");
        } else {
            printError("Unexpected token in expression: " + std::to_string(static_cast<int>(m_tokens_pointer->getTokenType())));
        }
                
        m_tokens_pointer++;

        if (m_tokens_pointer->getTokenType() != TokenType::_semi) {

            if (is_operator(*m_tokens_pointer)) {
                NodeExpressionBinary* binary_expression = new NodeExpressionBinary();
                
                binary_expression->_lhs = expression;
                binary_expression->_op = new NodeOperator{.token = *m_tokens_pointer};
                m_tokens_pointer++;
                
                printDebug("parsing expression");
                binary_expression->_rhs = parseExpression();

                expression = new NodeExpression();
                expression->_expression = binary_expression;
                return expression;
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
                    m_tokens_pointer++;
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
        NodeStatement* statement;

        while (m_tokens_pointer < m_tokens.end()) {
            printDebug("parsing statement...");
            statement = parseStatement();
            if (!statement) break;
            m_program->_statements.push_back(statement);
        }

        return m_program;
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
        printProgram(m_program);
        return;
    }
};
