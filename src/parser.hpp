#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <typeinfo>

#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"


struct NodeOperator {
    Token* _token;
};

struct NodeIdentifier {
    Token* _token;
};

struct NodeBoolean {
    Token* _token;
    bool _value;
};

struct NodeInteger {
    Token* _token;
    int _value;
};

struct NodeCharacter {
    Token* _token;
    std::string _value;
};

struct NodeString {
    Token* _token;
    std::string _value;
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
    Token* _token;
};

struct NodeFunctionCallArgument {
    NodeExpression* _expression;
};

struct NodeFunctionCall {
    NodeIdentifier* _identifier;
    std::vector<NodeFunctionCallArgument*> _arguments;
};

struct NodeExpression {
    std::variant<NodeExpressionBinary*, NodeInteger*, NodeString*, NodeIdentifier*, NodeBoolean*, NodeExpressionUnary*, NodeCharacter*, NodeGenerator*, NodeFunctionCall*> _expression;
};

struct NodeDecleration {
    Token* _qualifier;
    Token* _type;
    Token* _identifier;
    NodeExpression* _expression;
};



struct NodeFunctionDeclerationArgument {
    Token* _type;
    NodeIdentifier* _identifier;
};

struct NodeFunctionDecleration {
    NodeIdentifier* _identifier;
    std::vector<NodeFunctionDeclerationArgument*> _arguments;
    Token* _return_type;
    NodeStatement* _statement;
    NodeExpression* _expression;
    bool* is_function;
};

struct NodeProgramElement {
    std::variant<NodeStatement*, NodeFunctionDecleration*> _element;
};

struct NodeBlock {
    std::vector<NodeProgramElement*> _elements;
};

struct NodeProgram {
    std::vector<NodeProgramElement*> _elements;
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
                printDebug(output.str() + node_binary->_operator->_token->getStrValue() + " (");
                printExpression(node_binary->_lhs, indent + 1);
                printExpression(node_binary->_rhs, indent + 1);
                printDebug(output.str() + ")");
            }

        } else if (std::holds_alternative<NodeInteger*>(expression->_expression)) {
            NodeInteger* node_integer = std::get<NodeInteger*>(expression->_expression);
            printDebug(output.str() + std::to_string(node_integer->_value));

        } else if (std::holds_alternative<NodeCharacter*>(expression->_expression)) {
            NodeCharacter* node_character = std::get<NodeCharacter*>(expression->_expression);
            printDebug(output.str() + node_character->_value);

        } else if (std::holds_alternative<NodeString*>(expression->_expression)) {
            NodeString* node_string = std::get<NodeString*>(expression->_expression);
            printDebug(output.str() + node_string->_value);

        } else if (std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
            NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(expression->_expression);
            printDebug(output.str() + node_identifier->_token->getStrValue());

        } else if (std::holds_alternative<NodeBoolean*>(expression->_expression)) {
            NodeBoolean* node_boolean = std::get<NodeBoolean*>(expression->_expression);
            printDebug(output.str() + node_boolean->_token->getStrValue());

        } else if (std::holds_alternative<NodeGenerator*>(expression->_expression)) {
            NodeGenerator* node_generator = std::get<NodeGenerator*>(expression->_expression);
            printDebug(output.str() + node_generator->_token->getStrValue());

        } else if (std::holds_alternative<NodeFunctionCall*>(expression->_expression)) {
            NodeFunctionCall* node_function_call = std::get<NodeFunctionCall*>(expression->_expression);
            printDebug(output.str() + "[function call]");
            printDebug(output.str() + node_function_call->_identifier->_token->getStrValue() + "(");

            for (std::vector<NodeFunctionCallArgument*>::iterator i = node_function_call->_arguments.begin(); i < node_function_call->_arguments.end(); i++) {
                printExpression((*i)->_expression, indent + 1);
            }
            printDebug(output.str() + ")");

        } else if (std::holds_alternative<NodeExpressionUnary*>(expression->_expression)) {
            NodeExpressionUnary* node_expression_unary = std::get<NodeExpressionUnary*>(expression->_expression);
            printDebug(output.str() + node_expression_unary->_operator->_token->getStrValue() + " (");
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
            printDebug(output.str() + "[No quantifier]");
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

    void printFunctionDecleration(NodeFunctionDecleration* function_decleration, int indent) {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }
        if (*(function_decleration->is_function) == true) {
            printDebug(output.str() + "[function Decleration] `" + function_decleration->_identifier->_token->getStrValue() + "`");

            output << "    ";
            printDebug(output.str() + "[returns] " + function_decleration->_return_type->getStrValue());

        } else if (*(function_decleration->is_function) == false) {
            printDebug(output.str() + "[procedure Decleration] `" + function_decleration->_identifier->_token->getStrValue() + "`");

            output << "    ";
            
        } else {
            printError("Invalid function or procedure decleration");
        }

        printDebug(output.str() + "[arguments] `");
        for (std::vector<NodeFunctionDeclerationArgument*>::iterator i = function_decleration->_arguments.begin(); i < function_decleration->_arguments.end(); i++) {
            printDebug(output.str() + "    " + (*i)->_type->getStrValue() + ": " + (*i)->_identifier->_token->getStrValue());
        }

        if (function_decleration->_expression) {
            printExpression(function_decleration->_expression, indent + 1);
        } else if (function_decleration->_statement) {
            printStatement(function_decleration->_statement, indent + 1);
        }
    }

    void printElement(NodeProgramElement* program_element, int indent) {
        if (std::holds_alternative<NodeStatement*>(program_element->_element)) {
            NodeStatement* statement = std::get<NodeStatement*>(program_element->_element);
            printStatement(statement, indent);

        } else if (std::holds_alternative<NodeFunctionDecleration*>(program_element->_element)) {
            NodeFunctionDecleration* function_decleration = std::get<NodeFunctionDecleration*>(program_element->_element);
            printFunctionDecleration(function_decleration, indent);
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
            for (auto element : node_block->_elements) {
                printElement(element, indent + 1);
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
            printDebug("1");
            printError("Unexpected statement encountered during print" + std::string(typeid(statement->_statement).name()));
        }
    }


    void printProgram(NodeProgram* program){
        printDebug(std::string("printing program ") + std::to_string((program->_elements).size()));

        int indent = 0;
        for (std::vector<NodeProgramElement*>::iterator i = program->_elements.begin(); i < program->_elements.end(); i++) {
            printElement(*i, indent);
        }

        printOk("Program printed");
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

    bool isType(Token* token) {
        TokenType token_type = token->getTokenType();
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
            node_expression_unary->_operator = new NodeOperator{._token = &(*m_tokens_pointer)};
            printOk("parsed unary operator");
            m_tokens_pointer++;

            node_expression_unary->_expression = parseExpression(10);
            if (!node_expression_unary->_expression) {
                printError("Expected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
            printOk("parsed unary expression");
            lhs->_expression = node_expression_unary;

        } else if (m_tokens_pointer->getTokenType() == TokenType::_char_lit) {
            NodeCharacter* _character = new NodeCharacter{._token=&(*m_tokens_pointer), ._value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _character;
            m_tokens_pointer++;
            printDebug("Added character");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_text) {
            NodeString* _string = new NodeString{._token=&(*m_tokens_pointer), ._value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _string;
            m_tokens_pointer++;
            printDebug("Added string");
        
        } else if (m_tokens_pointer->getTokenType() == TokenType::_number) {
            NodeInteger* _integer = new NodeInteger{._token=&(*m_tokens_pointer), ._value=std::stoi(m_tokens_pointer->getStrValue())};
            lhs->_expression = _integer;
            m_tokens_pointer++;
            printDebug("Added integer");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_true || m_tokens_pointer->getTokenType() == TokenType::_false) {
            NodeBoolean* _boolean = new NodeBoolean{
                ._token=&(*m_tokens_pointer), 
                ._value=(m_tokens_pointer->getTokenType() == TokenType::_true)
            };

            lhs->_expression = _boolean;
            m_tokens_pointer++;
            printDebug("Added boolean");

        } else if (m_tokens_pointer->getTokenType() == TokenType::_identifier) {
            NodeIdentifier* identifier = new NodeIdentifier{._token = &(*m_tokens_pointer)};
            m_tokens_pointer++;

            if (m_tokens_pointer->getTokenType() == TokenType::_open_paren) {
                std::vector<NodeFunctionCallArgument*> function_call_arguments = parseFunctionCallArguments();
                NodeFunctionCall* function_call = new NodeFunctionCall();
                function_call->_arguments = function_call_arguments;
                function_call->_identifier = identifier;
                
                lhs->_expression = function_call;
                printDebug("Added function call with arguments: " + std::to_string(function_call_arguments.size()));

            } else {
                lhs->_expression = identifier;
                printDebug("Added identifier");
            }

        } else if (m_tokens_pointer->getTokenType() == TokenType::_generator) {
            NodeGenerator* _generator = new NodeGenerator{._token = &(*m_tokens_pointer)};
            lhs->_expression = _generator;
            m_tokens_pointer++;
            printDebug("Added generator");

        } else {
            printDebug("no expression");
            return nullptr;
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
                printError("Expected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
            bin->_lhs = lhs;
            bin->_operator = new NodeOperator{._token = op};
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
        if (isType(&(*m_tokens_pointer))) {
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
        }

        return nullptr;
    }

    NodeBlock* parseBlock() {
        NodeBlock* node_block = new NodeBlock();
        if (m_tokens_pointer->getTokenType() == TokenType::_open_curly) {
            m_tokens_pointer++;

            printDebug("Opening a block");
            
            while (m_tokens_pointer->getTokenType() != TokenType::_close_curly) {
                node_block->_elements.push_back(parseElement());
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

        else if (m_tokens_pointer->getTokenType() == TokenType::_loop) {
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
                    printError("Found a stream statement but couldnt figure out which type");
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

    /*
        returns 1 if buffer meets the criteria to be an identifier otherwise 0
    */
    bool isIdentifier() {
        return m_tokens_pointer->getTokenType() == TokenType::_identifier;
    }

    NodeIdentifier* parseIdentifier(bool raise_error=0) {
        if (isIdentifier()) {
            printDebug("found identifier");

            NodeIdentifier* node_identifier = new NodeIdentifier();
            node_identifier->_token = &(*m_tokens_pointer);
            m_tokens_pointer++;
            return node_identifier;
        }

        if (raise_error) {
            printError("Expected identifier", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        return nullptr;
    }

    NodeFunctionDeclerationArgument* parseFunctionDeclerationArgument(bool* is_function) {
        printDebug("parsing DeclerationArgument");

        if (*is_function || (m_tokens_pointer++)->getTokenType() == TokenType::_var) {
            if (isType(&(*m_tokens_pointer))) {
                printDebug("found type");

                NodeFunctionDeclerationArgument* node_argument = new NodeFunctionDeclerationArgument();

                node_argument->_type = &(*m_tokens_pointer);
                m_tokens_pointer++;

                if (m_tokens_pointer < m_tokens.end() && m_tokens_pointer->getTokenType() == TokenType::_identifier) {
                    NodeIdentifier* node_identifier = new NodeIdentifier();
                    node_identifier->_token = &(*m_tokens_pointer);

                    node_argument->_identifier = node_identifier;
                    m_tokens_pointer++;

                } else {
                    printError("Expected identifier", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                }

                printDebug("parsed argument");
                return node_argument;
            } else {
                printError("couldnt find type: " + m_tokens_pointer->getStrValue());
            }

        } else {
            printError("Failed to parse argument");
        }
        return nullptr;
    }
    
    std::vector<NodeFunctionDeclerationArgument*> parseFunctionDeclerationArguments(bool* is_function) {
        std::vector<NodeFunctionDeclerationArgument*> node_arguments;
        if (m_tokens_pointer->getTokenType() != TokenType::_open_paren) {
            printError("Expected `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }
        m_tokens_pointer++;
        printDebug("found `(`");

        if (m_tokens_pointer->getTokenType() == TokenType::_close_paren) {
            printDebug("found `)`");
            m_tokens_pointer++;
            return node_arguments;
        }

        printDebug("parsing argument");
        while (true) {
            NodeFunctionDeclerationArgument* node_argument = new NodeFunctionDeclerationArgument();
            node_argument = parseFunctionDeclerationArgument(is_function);
            if (!node_argument) break;

            node_arguments.push_back(node_argument);
            printDebug("parsed argument");
            
            if (m_tokens_pointer->getTokenType() == TokenType::_comma) {
                printDebug("found comma");
                m_tokens_pointer++;

            } else if (m_tokens_pointer->getTokenType() == TokenType::_close_paren) {
                printDebug("found `)`");
                m_tokens_pointer++;
                return node_arguments;
                
            } else {
                printError("Expected `)` or `,`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
        }

        printError("shouldnt reach here when parsing arguments");
        return node_arguments;
    }

    NodeFunctionCallArgument* parseFunctionCallArgument() {
        printDebug("parsing call argument");
        NodeFunctionCallArgument* call_argument = new NodeFunctionCallArgument();

        if (call_argument->_expression = parseExpression()) {
            printDebug("parsed call argument");
            return call_argument;
        }

        return nullptr;
    }

    std::vector<NodeFunctionCallArgument*> parseFunctionCallArguments() {
        std::vector<NodeFunctionCallArgument*> call_arguments;
        if (m_tokens_pointer->getTokenType() != TokenType::_open_paren) {
            printError("Expected `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        m_tokens_pointer++;
        printDebug("found `(`");

        if (m_tokens_pointer->getTokenType() == TokenType::_close_paren) {
            printDebug("found `)`");
            m_tokens_pointer++;
            return call_arguments;
        }

        printDebug("parsing call argument");
        while (true) {
            NodeFunctionCallArgument* node_argument = new NodeFunctionCallArgument();
            node_argument = parseFunctionCallArgument();
            if (!node_argument) break;

            call_arguments.push_back(node_argument);
            printDebug("parsed call argument");
            
            if (m_tokens_pointer->getTokenType() == TokenType::_comma) {
                printDebug("found comma");
                m_tokens_pointer++;

            } else if (m_tokens_pointer->getTokenType() == TokenType::_close_paren) {
                printDebug("found `)`");
                m_tokens_pointer++;
                return call_arguments;
                
            } else {
                printError("Expected `)` or `,`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
        }

        printError("shouldnt reach here when parsing call arguments");
        return call_arguments;
    }


    NodeFunctionDecleration* parseFunctionOrProcedure() {
        NodeFunctionDecleration* node_function = new NodeFunctionDecleration();
        bool* is_function = nullptr;

        if (m_tokens_pointer->getTokenType() == TokenType::_function || m_tokens_pointer->getTokenType() == TokenType::_procedure) {
            printDebug(std::to_string(m_tokens_pointer->getTokenType() == TokenType::_procedure));

            if (m_tokens_pointer->getTokenType() == TokenType::_function) {
                printDebug("parsing function");
                is_function = new bool(true);
            } else {
                printDebug("parsing procedure");
                is_function = new bool(false);
                printDebug("set is_function to false");
            }

            m_tokens_pointer++;

            printDebug("parsing identifier");
            node_function->_identifier = parseIdentifier(1);
            if (node_function->_identifier) {
                printDebug("parsed identifier");

                printDebug("parsing arguments");
                node_function->_arguments = parseFunctionDeclerationArguments(is_function);
                printDebug("parsed arguments: " + std::to_string((node_function->_arguments).size()));

                if (m_tokens_pointer->getTokenType() != TokenType::_returns) {
                    if (is_function && *is_function == true)
                    printError("Expected `returns`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());

                } else {
                    m_tokens_pointer++;

                    if (m_tokens_pointer < m_tokens.end() && isType(&(*m_tokens_pointer))) {
                        node_function->_return_type = &(*m_tokens_pointer);
                        m_tokens_pointer++;

                    } else {
                        printError("Expected type", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }
                }


                if (m_tokens_pointer < m_tokens.end() && m_tokens_pointer->getTokenType() == TokenType::_assign) {
                    m_tokens_pointer++;
                    printDebug("parsing expression");

                    if (node_function->_expression = parseExpression()) {
                        printDebug("parsed expression");
                        parseSemi();
                    } else {
                        printError("Expected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }

                } else if (m_tokens_pointer < m_tokens.end() && m_tokens_pointer->getTokenType() == TokenType::_open_curly) {
                    printDebug("parsed statement");

                    if (node_function->_statement = parseStatement()) {
                        printDebug("parsed statement");
                    } else {
                        printError("Expected statement", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }                    
                } else {
                    printError("Invalid function body", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                }

                printDebug("parsed function");
            }
        } else {
            printError("Shouldnt reach here");
        }

        node_function->is_function = new bool(*is_function);
        return node_function;
    }

    NodeProgramElement* parseElement() {
        NodeProgramElement* node_program_element = new NodeProgramElement();
        if (m_tokens_pointer->getTokenType() == TokenType::_function || m_tokens_pointer->getTokenType() == TokenType::_procedure) {
            printDebug("parsing function or procedure...");
            NodeFunctionDecleration* function_decleration = parseFunctionOrProcedure();
            if (!function_decleration) return nullptr;

            node_program_element->_element = function_decleration;

        } else {
            printDebug("parsing statement...");
            NodeStatement* statement = parseStatement();
            if (!statement) return nullptr;

            node_program_element->_element = statement;
        }
        return node_program_element;
    }

    NodeProgram* parseProgram() {
        printDebug("parsing program...");
        
        while (m_tokens_pointer < m_tokens.end()) {
            if (NodeProgramElement* node_program_element = parseElement()) {
                m_program->_elements.push_back(node_program_element);
            } else {
                printError("Invalid element");
            }
        }

        printOk("Program parsed");
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
        printProgram(m_program);
        return;
    }
};
