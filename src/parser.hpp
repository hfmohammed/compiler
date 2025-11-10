#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <typeinfo>

#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"


struct NodeMemberAccess {

};

struct NodeOperator {
    Token token;
};

struct NodeIdentifier {
    Token* token;
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

struct NodeExpressionUnary {
    NodeOperator* _operator;
    NodeExpression* _expression;
};

struct NodeExpressionBinary {
    NodeExpression* _lhs;
    NodeOperator* _operator;
    NodeExpression* _rhs;
};

struct NodeExpression {
    std::variant<NodeExpressionBinary*, NodeInteger*, NodeString*, NodeIdentifier*, NodeExpressionUnary*> _expression;
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

            NodeExpressionBinary* node_binary = std::get<NodeExpressionBinary*>(expression->_expression);
            if (node_binary->_operator) {
                printDebug(output.str() + node_binary->_operator->token.getStrValue() + " (");
                printExpression(node_binary->_lhs, indent + 1);
                printExpression(node_binary->_rhs, indent + 1);
                printDebug(output.str() + ")");
            }

        } else if (std::holds_alternative<NodeInteger*>(expression->_expression)) {
            // printDebug("else if (std::holds_alternative<NodeInteger*>(expression->_expression))");
            NodeInteger* node_integer = std::get<NodeInteger*>(expression->_expression);
            printDebug(output.str() + std::to_string(node_integer->value));

        } else if (std::holds_alternative<NodeString*>(expression->_expression)) {
            NodeString* node_string = std::get<NodeString*>(expression->_expression);
            printDebug(output.str() + node_string->value);
            
        } else if (std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
            NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(expression->_expression);
            printDebug(output.str() + node_identifier->token->getStrValue());

        } else if (std::holds_alternative<NodeExpressionUnary*>(expression->_expression)) {
            NodeExpressionUnary* node_expression_unary = std::get<NodeExpressionUnary*>(expression->_expression);
            printDebug(output.str() + node_expression_unary->_operator->token.getStrValue() + " (");
            printExpression(node_expression_unary->_expression, indent + 1);
            printDebug(output.str() + ")");
            
        } else {
            printError("expression doesn't hold the required alternative: " + std::string(typeid(expression->_expression).name()));
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
        switch (token_type)
        {
            case TokenType::_boolean:
            case TokenType::_character:
            case TokenType::_integer:
            case TokenType::_real:
            case TokenType::_tuple:
            case TokenType::_struct:
            case TokenType::_vector:
            case TokenType::_string:
                return true;
            default:
                return false;
        }
    }

    bool is_operator(Token token) {
        switch (token.getTokenType())
        {
            case TokenType::_period:
            case TokenType::_dbl_period:
            case TokenType::_unary_plus:
            case TokenType::_binary_plus:
            case TokenType::_unary_minus:
            case TokenType::_binary_minus:
            case TokenType::_not:
            case TokenType::_hat:
            case TokenType::_asterisk:
            case TokenType::_fwd_slash:
            case TokenType::_mod:
            case TokenType::_dbl_asterisk:
            case TokenType::_by:
            case TokenType::_greater_than:
            case TokenType::_less_than:
            case TokenType::_greater_than_equal:
            case TokenType::_less_than_equal:
            case TokenType::_check_equal:
            case TokenType::_not_eq:
            case TokenType::_and:
            case TokenType::_or:
            case TokenType::_xor:
            case TokenType::_dbl_vertical_line:
                return true;
            default:
                return false;
        }
    }

    int isRightAssociative(Token* token) {
        TokenType token_type = token->getTokenType();

        if (token_type == TokenType::_unary_plus || token_type == TokenType::_unary_minus || token_type == TokenType::_not || token_type == TokenType::_hat || token_type == TokenType::_dbl_vertical_line) {
            return 1;
        }
        return 0;
    }

    int getOperatorPrec(Token* token) {
        TokenType token_type = token->getTokenType();
        printDebug("getting operator precedence");
        printDebug(token->getStrValue());

        if (token_type == TokenType::_period) {
            return 13;
        } else if (token_type == TokenType::_open_square) {
            return 12;
        } else if (token_type == TokenType::_dbl_period) {
            return 11;
        } else if (token_type == TokenType::_unary_plus || token_type == TokenType::_unary_minus || token_type == TokenType::_not) {
            return 10;
        } else if (token_type == TokenType::_hat) {
            return 9;
        } else if (token_type == TokenType::_asterisk || token_type == TokenType::_fwd_slash || token_type == TokenType::_mod || token_type == TokenType::_dbl_asterisk) {
            return 8;
        } else if (token_type == TokenType::_binary_plus || token_type == TokenType::_binary_minus) {
            return 7;
        } else if (token_type == TokenType::_by) {
            return 6;
        } else if (token_type == TokenType::_less_than || token_type == TokenType::_greater_than || token_type == TokenType::_less_than_equal || token_type == TokenType::_greater_than_equal) {
            return 5;
        } else if (token_type == TokenType::_check_equal || token_type == TokenType::_not_eq) {
            return 4;
        } else if (token_type == TokenType::_and) {
            return 3;
        } else if (token_type == TokenType::_or || token_type == TokenType::_xor) {
            return 2;
        } else if (token_type == TokenType::_dbl_vertical_line) {
            return 1;
        } else {
            return 0;
        }
    }

    NodeExpression* parseExpression(int min_precedence = 0) {
        printDebug("parsing expression with precedence: " + std::to_string(min_precedence));
        NodeExpression* lhs = new NodeExpression();
        
        if (m_tokens_pointer->getTokenType() == TokenType::_open_paren) {
            m_tokens_pointer++;
            lhs = parseExpression(0);

            if (m_tokens_pointer >= m_tokens.end() || m_tokens_pointer->getTokenType() != TokenType::_close_paren) {
                printError("Expected `)`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
            
            m_tokens_pointer++;
        
        } else if (m_tokens_pointer->getTokenType() == TokenType::_not || m_tokens_pointer->getTokenType() == TokenType::_unary_plus || m_tokens_pointer->getTokenType() == TokenType::_unary_minus) {
            printDebug("parsing unary operator");
            
            NodeExpressionUnary* node_expression_unary = new NodeExpressionUnary();
            node_expression_unary->_operator = new NodeOperator{.token = *m_tokens_pointer};
            printOk("parsed unary operator");
            m_tokens_pointer++;

            node_expression_unary->_expression = parseExpression(10);
            printOk("parsed unary expression");
            lhs->_expression = node_expression_unary;
        
        } else if (m_tokens_pointer->getTokenType() == TokenType::_string) {
            NodeString* _string = new NodeString{.token=*m_tokens_pointer, .value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _string;
            m_tokens_pointer++;
            printDebug("Added string");
        
        } else if (m_tokens_pointer->getTokenType() == TokenType::_number) {
            NodeInteger* _integer = new NodeInteger{.token=*m_tokens_pointer, .value=std::stoi(m_tokens_pointer->getStrValue())};
            lhs->_expression = _integer;
            m_tokens_pointer++;
            printDebug("Added integer");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            NodeIdentifier* _identifier = new NodeIdentifier{.token = &(*m_tokens_pointer)};
            lhs->_expression = _identifier;
            m_tokens_pointer++;
            printDebug("Added identifier");

        } else {
            printError("Unexpected token in expression: " + std::string(typeid(m_tokens_pointer->getTokenType()).name()));
        }

        printOk("parsed lhs");

        while (m_tokens_pointer < m_tokens.end() && is_operator(*m_tokens_pointer)) {
            printDebug("found an operator");

            Token* op = &(*m_tokens_pointer);
            int prec = getOperatorPrec(op);
            bool right_assoc = isRightAssociative(op);

            if (prec < min_precedence) {
                break;
            }

            m_tokens_pointer++;

            int next_min_prec = right_assoc ? prec : prec + 1;

            NodeExpression* rhs = parseExpression(next_min_prec);

            NodeExpressionBinary* bin = new NodeExpressionBinary();
            bin->_lhs = lhs;
            bin->_operator = new NodeOperator{.token = *op};
            bin->_rhs = rhs;

            lhs = new NodeExpression{._expression = bin};
        }

        printDebug("returning expression");
        return lhs;
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


                if (m_tokens_pointer->getTokenType() == TokenType::_semi) {
                    m_tokens_pointer++;
                    printOk("completed a statement");
                    return decleration;

                } else {
                    printError("Expected `;` but got " + m_tokens_pointer->getStrValue(), m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
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
        if (m_tokens_pointer->getTokenType() == TokenType::_binary_minus && (m_tokens_pointer + 1)->getTokenType() == TokenType::_greater_than) {} // parse stream input
        else if (m_tokens_pointer->getTokenType() == TokenType::_less_than && (m_tokens_pointer + 1)->getTokenType() == TokenType::_binary_minus) {} // parse stream output
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
