#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <typeinfo>

#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"


struct NodeOperator {
    Token* token;
};

struct NodeIdentifier {
    Token* token;
};

struct NodeBoolean {
    Token* token;
    bool value;
};

struct NodeInteger {
    Token* token;
    int value;
};

struct NodeCharacter {
    Token* token;
    std::string value;
};

struct NodeString {
    Token* token;
    std::string value;
};

struct NodeExpression;
struct NodeStatement;

struct NodeExpressionUnary {
    NodeOperator* _operator;
    NodeExpression* _expression;
};

struct NodeExpressionBinary {
    NodeExpression* _lhs;
    NodeOperator* _operator;
    NodeExpression* _rhs;
};

struct NodeStatementToken {
    Token* _token;
};

struct NodeLoop {
    bool* _predicated;
    NodeExpression* _expression;
    NodeStatement* _statement;
};

struct NodeGenerator {
    Token* token;
};

struct NodeExpression {
    std::variant<NodeExpressionBinary*, NodeInteger*, NodeString*, NodeIdentifier*, NodeBoolean*, NodeExpressionUnary*, NodeCharacter*, NodeGenerator*> _expression;
};

struct NodeDecleration {
    Token* _qualifier;
    Token* _type;
    Token* _identifier;
    NodeExpression* _expression;
};


struct NodeBlock {
    std::vector<NodeStatement*> _statements;
};

struct NodeProgram {
    std::vector<NodeStatement*> _statements;
};

struct NodeControl {
    Token* _token;
    NodeExpression* _expression;
    NodeStatement* _statement;
};

struct NodeReturn {
    Token* _token;
    NodeExpression* _expression;
};

struct NodeStream {
    NodeExpression* _expression;
    Token* _operator;
    Token* _destination;
};

struct NodeStatement {
    std::variant<NodeDecleration*, NodeBlock*, NodeControl*, NodeStatementToken*, NodeReturn*, NodeStream*, NodeLoop*> _statement;
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
                printDebug(output.str() + node_binary->_operator->token->getStrValue() + " (");
                printExpression(node_binary->_lhs, indent + 1);
                printExpression(node_binary->_rhs, indent + 1);
                printDebug(output.str() + ")");
            }

        } else if (std::holds_alternative<NodeInteger*>(expression->_expression)) {
            NodeInteger* node_integer = std::get<NodeInteger*>(expression->_expression);
            printDebug(output.str() + std::to_string(node_integer->value));

        } else if (std::holds_alternative<NodeCharacter*>(expression->_expression)) {
            NodeCharacter* node_character = std::get<NodeCharacter*>(expression->_expression);
            printDebug(output.str() + node_character->value);

        } else if (std::holds_alternative<NodeString*>(expression->_expression)) {
            NodeString* node_string = std::get<NodeString*>(expression->_expression);
            printDebug(output.str() + node_string->value);

        } else if (std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
            NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(expression->_expression);
            printDebug(output.str() + node_identifier->token->getStrValue());

        } else if (std::holds_alternative<NodeBoolean*>(expression->_expression)) {
            NodeBoolean* node_boolean = std::get<NodeBoolean*>(expression->_expression);
            printDebug(output.str() + node_boolean->token->getStrValue());

        } else if (std::holds_alternative<NodeGenerator*>(expression->_expression)) {
            NodeGenerator* node_generator = std::get<NodeGenerator*>(expression->_expression);
            printDebug(output.str() + node_generator->token->getStrValue());

        } else if (std::holds_alternative<NodeExpressionUnary*>(expression->_expression)) {
            NodeExpressionUnary* node_expression_unary = std::get<NodeExpressionUnary*>(expression->_expression);
            printDebug(output.str() + node_expression_unary->_operator->token->getStrValue() + " (");
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
        
        if (decleration->_qualifier) {
            printDebug(output.str() + decleration->_qualifier->getStrValue());
        } else {
            printDebug(output.str() + "[No decleration]");
        }

        if (decleration->_type) {
            printDebug(output.str() + decleration->_type->getStrValue());
        } else {
            printDebug(output.str() + "[No type]");
        }

        printDebug(output.str() + decleration->_identifier->getStrValue());
        if (decleration->_expression != NULL) {
            printDebug(output.str() + "[Expression]");
            printExpression(decleration->_expression, indent + 1);
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
            NodeBlock* node_block = std::get<NodeBlock*>(statement->_statement);
            for (auto statement : node_block->_statements) {
                printStatement(statement, indent + 1);
            }

        } else if (std::holds_alternative<NodeControl*>(statement->_statement)) {
            NodeControl* node_control = std::get<NodeControl*>(statement->_statement);
            if (node_control->_token->getTokenType() == TokenType::_if) {
                printDebug(output.str() + "[if]");
            
            } else if (node_control->_token->getTokenType() == TokenType::_else_if) {
                printDebug(output.str() + "[else if]");

            } else if (node_control->_token->getTokenType() == TokenType::_else) {
                printDebug(output.str() + "[else]");
            }
            
            if (node_control->_token->getTokenType() == TokenType::_if || node_control->_token->getTokenType() == TokenType::_else_if) {
                printExpression(node_control->_expression, indent);
            }

            printStatement(node_control->_statement, indent + 1);

        } else if (std::holds_alternative<NodeStatementToken*>(statement->_statement)) {
            NodeStatementToken* node_statement_token = std::get<NodeStatementToken*>(statement->_statement);

            if (node_statement_token->_token->getTokenType() == TokenType::_continue) {
                printDebug(output.str() + "[continue]");

            } else if (node_statement_token->_token->getTokenType() == TokenType::_break) {
                printDebug(output.str() + "[break]");
            }

        } else if (std::holds_alternative<NodeStream*>(statement->_statement)) {
            NodeStream* node_stream = std::get<NodeStream*>(statement->_statement);
            printDebug(output.str() + "[stream]");
            output << "    ";
            printDebug(output.str() + node_stream->_operator->getStrValue() + " " + node_stream->_destination->getStrValue() + " (");
            printExpression(node_stream->_expression, indent + 2);
            printDebug(output.str() + ")");
        
        } else if (std::holds_alternative<NodeLoop*>(statement->_statement)) {
            NodeLoop* node_loop = std::get<NodeLoop*>(statement->_statement);
            printDebug(output.str() + "[loop]");
            if (node_loop->_expression) {
                if (*node_loop->_predicated) {
                    printDebug(output.str() + "[predicated while]");
                    printExpression(node_loop->_expression, indent + 1);
                    printStatement(node_loop->_statement, indent + 1);

                } else {
                    printStatement(node_loop->_statement, indent + 1);
                    printDebug(output.str() + "[post-predicated while]");
                    printExpression(node_loop->_expression, indent + 1);
                }
            } else {
                printStatement(node_loop->_statement, indent + 1);
            }

        } else if (std::holds_alternative<NodeReturn*>(statement->_statement)) {
            NodeReturn* node_return = std::get<NodeReturn*>(statement->_statement);
            printDebug(output.str() + "[return]");
            if (node_return->_expression) {
                printExpression(node_return->_expression, indent + 1);
            }

        } else {
            printError("Unexpected statement encountered during print" + std::string(typeid(statement->_statement).name()));
        }
    }


    void printProgram(NodeProgram* program){
        printDebug(std::string("printing program ") + std::to_string((program->_statements).size()));

        int indent = 0;
        for (std::vector<NodeStatement*>::iterator i = (program->_statements).begin(); i < (program->_statements).end(); i++) {
            printStatement(*i, indent);
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


    bool isQualifier(Token* token) {
        TokenType token_type = token->getTokenType();

        printDebug(token->getStrValue());

        return token_type == TokenType::_const || token_type == TokenType::_var;
    }

    bool isNumber(std::string content) {
        bool decimal = false;
        for (std::string::iterator s = content.begin(); s < content.end(); s++) {
            if (*s == '.' && !decimal) decimal = true;
            else if (!std::isdigit(*s)) return false;
        }
        return true;
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

    bool isOperator(Token* token) {
        switch (token->getTokenType())
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
        printDebug("Token type: " + std::to_string(m_tokens_pointer->getTokenType() == TokenType::_number ? 1 : 0));
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
            node_expression_unary->_operator = new NodeOperator{.token = &(*m_tokens_pointer)};
            printOk("parsed unary operator");
            m_tokens_pointer++;

            node_expression_unary->_expression = parseExpression(10);
            if (!node_expression_unary->_expression) {
                printError("Expected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
            printOk("parsed unary expression");
            lhs->_expression = node_expression_unary;

        } else if (m_tokens_pointer->getTokenType() == TokenType::_char_lit) {
            NodeCharacter* _character = new NodeCharacter{.token=&(*m_tokens_pointer), .value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _character;
            m_tokens_pointer++;
            printDebug("Added character");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_text) {
            NodeString* _string = new NodeString{.token=&(*m_tokens_pointer), .value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _string;
            m_tokens_pointer++;
            printDebug("Added string");
        
        } else if (m_tokens_pointer->getTokenType() == TokenType::_number) {
            NodeInteger* _integer = new NodeInteger{.token=&(*m_tokens_pointer), .value=std::stoi(m_tokens_pointer->getStrValue())};
            lhs->_expression = _integer;
            m_tokens_pointer++;
            printDebug("Added integer");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_true || m_tokens_pointer->getTokenType() == TokenType::_false) {
            NodeBoolean* _boolean = new NodeBoolean{
                .token=&(*m_tokens_pointer), 
                .value=(m_tokens_pointer->getTokenType() == TokenType::_true)
            };

            lhs->_expression = _boolean;
            m_tokens_pointer++;
            printDebug("Added boolean");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            NodeIdentifier* _identifier = new NodeIdentifier{.token = &(*m_tokens_pointer)};
            lhs->_expression = _identifier;
            m_tokens_pointer++;
            printDebug("Added identifier");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_generator) {
            NodeGenerator* _generator = new NodeGenerator{.token = &(*m_tokens_pointer)};
            lhs->_expression = _generator;
            m_tokens_pointer++;
            printDebug("Added generator");

        } else {
            printDebug("no expression");
            NodeExpression* null_expression = nullptr;
            return null_expression;
        }

        printOk("parsed lhs");

        while (m_tokens_pointer < m_tokens.end() && isOperator(&(*m_tokens_pointer))) {
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
            if (!lhs || !rhs) {
                printError("Exprected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
            bin->_lhs = lhs;
            bin->_operator = new NodeOperator{.token = op};
            bin->_rhs = rhs;

            lhs = new NodeExpression{._expression = bin};
        }

        printDebug("returning expression");
        return lhs;
    }

    int parseSemi() {
        if (m_tokens_pointer->getTokenType() == TokenType::_semi) {
            m_tokens_pointer++;
            printOk("completed a statement");
            return 1;

        } else {
            printError("Expected `;` but got " + m_tokens_pointer->getStrValue(), m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return 0;

        }
    }

    NodeDecleration* parseDecleration() {
        NodeDecleration* decleration = new NodeDecleration();
        bool is_decleration = false;

        printDebug("checking for qualifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (isQualifier(&(*m_tokens_pointer))) {
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
        }

        printDebug("checking for identifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            is_decleration = true;
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
            parseSemi();
            printOk("Decleration parsed");
        }


        if (is_decleration) {
            return decleration;
        } else {
            NodeDecleration* decleration = nullptr;
            return decleration;
        }
    }

    NodeBlock* parseBlock() {
        NodeBlock* node_block = new NodeBlock();
        if (m_tokens_pointer->getTokenType() == TokenType::_open_curly) {
            m_tokens_pointer++;

            printDebug("Opening a block");
            
            while (m_tokens_pointer->getTokenType() != TokenType::_close_curly) {
                node_block->_statements.push_back(parseStatement());
            }

            m_tokens_pointer++;
        }

        return node_block;
    }


    NodeStatement* parseStatement() {
        NodeStatement* statement = new NodeStatement();
        if (m_tokens_pointer->getTokenType() == TokenType::_else || m_tokens_pointer->getTokenType() == TokenType::_if) {
            printDebug(std::string("parsing ") + (m_tokens_pointer->getTokenType() == TokenType::_if ? "if" : "else if") + std::string(" statement"));
            
            NodeControl* node_control = new NodeControl();
            if (m_tokens_pointer->getTokenType() == TokenType::_if || (m_tokens_pointer->getTokenType() == TokenType::_else && (m_tokens_pointer + 1)->getTokenType() != TokenType::_if)) {
                node_control->_token = &(*m_tokens_pointer);
                m_tokens_pointer++;

            } else {
                node_control->_token = new Token(TokenType::_else_if, "`else if`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                m_tokens_pointer += 2;
            }

            if (node_control->_token->getTokenType() == TokenType::_else_if || node_control->_token->getTokenType() == TokenType::_if) {
                NodeExpression* node_expression = parseExpression();
                if (node_expression == NULL) {
                    printError("Expected expression for if statement");
                }
                node_control->_expression = node_expression;
                printOk("found expression");
            }
            

            NodeStatement* node_statement = parseStatement();
            if (node_statement == NULL) {
                printError("Expected body for if statement");
            }
            node_control->_statement = node_statement;
            printOk("found statement");
            statement->_statement = node_control;
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_loop) { // parseLoop()
            NodeLoop* node_loop = new NodeLoop();
            node_loop->_predicated = new bool(0);
            printDebug("parsing loop");
            m_tokens_pointer++;
            
            if (m_tokens_pointer->getTokenType() == TokenType::_while) {
                node_loop->_predicated = new bool(1);
                m_tokens_pointer++;

                node_loop->_expression = parseExpression();
            }

            node_loop->_statement = parseStatement();

            if (m_tokens_pointer->getTokenType() == TokenType::_while) {
                if (!*node_loop->_predicated) {
                    m_tokens_pointer++;

                    node_loop->_expression = parseExpression();
                    parseSemi();
                
                } else {
                    printError("Unexpected `while`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                }
            }

            statement->_statement = node_loop;
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_open_curly) {
            statement->_statement = parseBlock();
            printOk("parsed a block");
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_break || m_tokens_pointer->getTokenType() == TokenType::_continue) {
            NodeStatementToken* node_statement_token = new NodeStatementToken();
            node_statement_token->_token = &(*m_tokens_pointer);

            statement->_statement = node_statement_token;
            m_tokens_pointer++;
            parseSemi();
        }

        else if (m_tokens_pointer->getTokenType() == TokenType::_return) {
            
            NodeReturn* node_return = new NodeReturn();
            node_return->_token = &(*m_tokens_pointer);
            m_tokens_pointer++;
            if (m_tokens_pointer->getTokenType() != TokenType::_semi) {
                node_return->_expression = parseExpression();
            }
            parseSemi();

            statement->_statement = node_return;
        }

        else if (NodeDecleration* declaration = parseDecleration()) {
            printOk("Decleration parsed");
            if (declaration) {
                statement->_statement = declaration;
            }
        }

        else {
            auto temp = m_tokens_pointer;
            if (NodeExpression* expression = parseExpression()) {
                printDebug(m_tokens_pointer->getStrValue() + "::");
                if (m_tokens_pointer->getTokenType() == TokenType::_stream_output && (m_tokens_pointer + 1)->getTokenType() == TokenType::_std_output) {
                    NodeStream* node_stream = new NodeStream();
                    node_stream->_operator = &(*m_tokens_pointer);
                    node_stream->_destination = &(*(m_tokens_pointer + 1));
                    node_stream->_expression = expression;
                    m_tokens_pointer += 2;

                    parseSemi();
                    statement->_statement = node_stream;

                    printOk("Found an output stream statement");

                } else if (m_tokens_pointer->getTokenType() == TokenType::_stream_input && (m_tokens_pointer + 1)->getTokenType() == TokenType::_std_input && std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
                    NodeStream* node_stream = new NodeStream();
                    node_stream->_operator = &(*m_tokens_pointer);
                    node_stream->_destination = &(*(m_tokens_pointer + 1));
                    node_stream->_expression = expression;
                    m_tokens_pointer += 2;

                    parseSemi();
                    statement->_statement = node_stream;

                    printOk("Found an output stream statement");

                } else {
                    printOk("Found a stream statement");
                }
            } 

            else {
                m_tokens_pointer = temp;
                printError("Invalid statement", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
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
        printOk("Program printed");
        return;
    }
};
