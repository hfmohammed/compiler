#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <typeinfo>

#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"

struct NodeExpression;
struct NodeStatement;
struct NodeType;
struct NodeIdentifierToken;
struct NodeIdentifier;

struct NodeOperator {
    Token* _token;
};

struct NodeTupleIdentifier {
    std::vector<NodeIdentifier*> _identifiers;
};

struct NodeIdentifierToken {
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

struct NodeList {
    std::vector<NodeExpression*> _items;
};

struct NodeTypeVector {
    NodeType* _type;
};

struct NodeQualifier {
    Token* _token;
};

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

struct NodeTuple {
    std::vector<NodeExpression*> _expressions;
};

struct NodeArrayIndex {
    NodeIdentifier* _identifier;
    NodeExpression* _expression;
};

struct NodeIdentifier {
    std::variant<NodeIdentifierToken*, NodeTupleIdentifier*, NodeArrayIndex*, NodeFunctionCall*> _identifier;
    NodeIdentifier* _access_token;
};

struct NodeAssign {
    NodeExpression* _lhs;
    NodeOperator* _operator;
    NodeExpression* _rhs;
};

struct NodeExpression {
    std::variant<NodeExpressionBinary*, NodeInteger*, NodeString*, NodeBoolean*, NodeExpressionUnary*, NodeCharacter*, NodeGenerator*, NodeFunctionCall*, NodeTuple*, NodeIdentifier*, NodeList*, NodeStatement*, NodeAssign*> _expression;
};

struct NodeFunctionDeclerationArgument {
    NodeQualifier* _qualifier;
    NodeType* _type;
    NodeIdentifier* _identifier;
};

struct NodeStruct {
    NodeIdentifier* _type;
    std::vector<NodeFunctionDeclerationArgument*> _arguments;
};

struct NodeTypeTuple {
    std::vector<NodeType*> _types;
};

struct NodeTypeArray {
    NodeType* _type;
    NodeInteger* _index;
};

struct NodeType {
    std::variant<Token*, NodeTypeTuple*, NodeTypeVector*, NodeTypeArray*> _type;
};

struct NodeTypealias {
    NodeType* _original;
    Token* _new;
};

struct NodeDecleration {
    NodeQualifier* _qualifier;
    std::variant<NodeStruct*, Token*, NodeTypeTuple*, NodeType*> _type;
    std::variant<NodeIdentifierToken*, NodeTupleIdentifier*, NodeIdentifier*> _identifier;
    NodeExpression* _expression;
};

struct NodeCall {
    NodeFunctionCall* _function_call;
};

struct NodeFunctionDecleration {
    NodeIdentifier* _identifier;
    std::vector<NodeFunctionDeclerationArgument*> _arguments;
    NodeType* _return_type;
    NodeStatement* _statement;
    NodeExpression* _expression;
    bool* is_procedure;
};

struct NodeProgramElement {
    std::variant<NodeStatement*, NodeFunctionDecleration*, NodeTypealias*> _element;
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
    std::variant<NodeDecleration*, NodeBlock*, NodeControl*, NodeStatementToken*, NodeReturn*, NodeStream*, NodeLoop*, NodeCall*, NodeAssign*> _statement;
};

class Parser {
private:
    NodeProgram* m_program;
    std::vector<Token> m_tokens;
    std::vector<std::string> m_types;
    std::vector<Token>::iterator m_tokens_pointer;
public:
    Parser(std::vector<Token>& tokens) {
        m_program = new NodeProgram();
        m_tokens = tokens;
        m_tokens_pointer = m_tokens.begin();
    }

    void printStruct(NodeStruct* _struct, int indent) {
        printDebug("printing struct");

        std::string output_prefix = getDebugPrefix(indent);

        printIdentifier(_struct->_type, indent);

        printDebug("printing arguments");
        printFunctionDeclerationArguments(_struct->_arguments, indent, false);
    }

    void printTuple(NodeTuple* node_tuple, int indent) {
        std::string output_prefix = getDebugPrefix(indent);
        printDebug(output_prefix + "[tuple] (");
        for (auto expression : node_tuple->_expressions) {
            printExpression(expression, indent + 1);
        }
        printDebug(output_prefix + "),");
    }

    void printExpression(NodeExpression* expression, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        if (std::holds_alternative<NodeExpressionBinary*>(expression->_expression)) {

            NodeExpressionBinary* node_binary = std::get<NodeExpressionBinary*>(expression->_expression);
            if (node_binary->_operator) {
                printDebug(output_prefix + node_binary->_operator->_token->getStrValue() + " (");
                printExpression(node_binary->_lhs, indent + 1);
                printExpression(node_binary->_rhs, indent + 1);
                printDebug(output_prefix + ")");
            }

        } else if (std::holds_alternative<NodeTuple*>(expression->_expression)) {
            NodeTuple* node_tuple = std::get<NodeTuple*>(expression->_expression);
            printTuple(node_tuple, indent);

        } else if (std::holds_alternative<NodeInteger*>(expression->_expression)) {
            NodeInteger* node_integer = std::get<NodeInteger*>(expression->_expression);
            printDebug(output_prefix + std::to_string(node_integer->_value));

        } else if (std::holds_alternative<NodeCharacter*>(expression->_expression)) {
            NodeCharacter* node_character = std::get<NodeCharacter*>(expression->_expression);
            printDebug(output_prefix + node_character->_value);

        } else if (std::holds_alternative<NodeString*>(expression->_expression)) {
            NodeString* node_string = std::get<NodeString*>(expression->_expression);
            printDebug(output_prefix + node_string->_value);

        } else if (std::holds_alternative<NodeIdentifier*>(expression->_expression)) {
            NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(expression->_expression);
            printIdentifier(node_identifier, indent + 1);

        } else if (std::holds_alternative<NodeBoolean*>(expression->_expression)) {
            NodeBoolean* node_boolean = std::get<NodeBoolean*>(expression->_expression);
            printDebug(output_prefix + node_boolean->_token->getStrValue());

        } else if (std::holds_alternative<NodeGenerator*>(expression->_expression)) {
            NodeGenerator* node_generator = std::get<NodeGenerator*>(expression->_expression);
            printDebug(output_prefix + node_generator->_token->getStrValue());

        } else if (std::holds_alternative<NodeFunctionCall*>(expression->_expression)) {
            NodeFunctionCall* node_function_call = std::get<NodeFunctionCall*>(expression->_expression);
            printDebug(output_prefix + "[function call]");
            printIdentifier(node_function_call->_identifier, indent + 1);
            printDebug(output_prefix + "(");

            for (std::vector<NodeFunctionCallArgument*>::iterator i = node_function_call->_arguments.begin(); i < node_function_call->_arguments.end(); i++) {
                printExpression((*i)->_expression, indent + 1);
            }
            printDebug(output_prefix + ")");

        } else if (std::holds_alternative<NodeExpressionUnary*>(expression->_expression)) {
            NodeExpressionUnary* node_expression_unary = std::get<NodeExpressionUnary*>(expression->_expression);
            printDebug(output_prefix + node_expression_unary->_operator->_token->getStrValue() + " (");
            printExpression(node_expression_unary->_expression, indent + 1);
            printDebug(output_prefix + ")");

        } else if (std::holds_alternative<NodeList*>(expression->_expression)) {
            NodeList* node_list = std::get<NodeList*>(expression->_expression);
            printList(node_list, indent + 1);

        } else if (std::holds_alternative<NodeAssign*>(expression->_expression)) {
            std::string output_prefix = getDebugPrefix(indent);

            NodeAssign* node_assign = std::get<NodeAssign*>(expression->_expression);
            if (node_assign->_operator) {
                printDebug(output_prefix + node_assign->_operator->_token->getStrValue() + " (");
                printExpression(node_assign->_lhs, indent + 1);
                printExpression(node_assign->_rhs, indent + 1);
                printDebug(output_prefix + ")");
            }

        } else {
            printError("expression doesn't hold the required alternative: " + std::string(typeid(expression->_expression).name()));
        }
    }

    void printList(NodeList* node_list, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(output_prefix + "[");
        for (auto element : node_list->_items) {
            printExpression(element, indent + 1);
        }
        printDebug(output_prefix + "]");
    }

    void printDecleration(NodeDecleration* decleration, int indent) {
        std::string output_prefix = getDebugPrefix(indent);
        
        // print qualifier
        if (decleration->_qualifier) {
            printDebug(output_prefix + decleration->_qualifier->_token->getStrValue());
        } else {
            printDebug(output_prefix + "[No qualifier]");
        }

        // print type
        if (std::holds_alternative<Token*>(decleration->_type))  {
            Token* token = std::get<Token*>(decleration->_type);
            if (token != nullptr) {
                printDebug(output_prefix + token->getStrValue());
            }

        } else if (std::holds_alternative<NodeTypeTuple*>(decleration->_type))  {
            NodeTypeTuple* node_type_tuple = std::get<NodeTypeTuple*>(decleration->_type);
            if (node_type_tuple != nullptr) {
                printTupleType(node_type_tuple, indent);
            }

        } else if (std::holds_alternative<NodeType*>(decleration->_type))  {
            NodeType* node_type = std::get<NodeType*>(decleration->_type);
            if (node_type != nullptr) {
                printType(node_type, indent);
            }

        } else {
            printDebug(output_prefix + "[No type]");
        }

        printDebug("Passes");
        if (std::holds_alternative<NodeStruct*>(decleration->_type)) {
            printDebug("Passes struct");
            
            NodeStruct* node_struct = std::get<NodeStruct*>(decleration->_type);
            if (node_struct != nullptr) {
                printDebug(output_prefix + "[struct Type]");
                printStruct(node_struct, indent);
            }
        }

        // print identifier
        else if (std::holds_alternative<NodeIdentifier*>(decleration->_identifier)) {
            printDebug("Passes identifier");

            NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(decleration->_identifier);
            printIdentifier(node_identifier, indent);
        
        } else {
            printError("Invalid identifier");
        }

        if (decleration->_expression != NULL) {
            printDebug(output_prefix + "[Expression]");
            printExpression(decleration->_expression, indent + 1);
        } else {
            printDebug(output_prefix + "[No expression]");
        }
    }

    void printVectorType(NodeTypeVector* node_type_vector, int indent) {
        std::string output_prefix = getDebugPrefix(indent);
        printType(node_type_vector->_type, indent);
    }

    void printType(NodeType* node_type, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        if (std::holds_alternative<Token*>(node_type->_type)) {
            Token* token = std::get<Token*>(node_type->_type);
            printDebug(output_prefix + token->getStrValue());

        } else if (std::holds_alternative<NodeTypeTuple*>(node_type->_type)) {
            NodeTypeTuple* node_type_tuple = std::get<NodeTypeTuple*>(node_type->_type);
            printTupleType(node_type_tuple, indent + 1);

        } else if (std::holds_alternative<NodeTypeVector*>(node_type->_type)) {
            NodeTypeVector* node_type_vector = std::get<NodeTypeVector*>(node_type->_type);
            printDebug(output_prefix + "[vector identifier]");
            printVectorType(node_type_vector, indent + 1);

        } else if (std::holds_alternative<NodeTypeArray*>(node_type->_type)) {
            NodeTypeArray* node_type_array = std::get<NodeTypeArray*>(node_type->_type);
            printArrayType(node_type_array, indent);

        } else {
            printError("invalid type");
        }
    }

    void printInteger(NodeInteger* node_integer, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(output_prefix + std::to_string(node_integer->_value));
    }

    std::string getDebugPrefix(int indent) {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent) {
            output << "    ";
        }

        return output.str();
    }

    void printArrayType(NodeTypeArray* node_type_array, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        printType(node_type_array->_type, indent);
        if (node_type_array->_index) {
            printDebug(output_prefix + "[");
            printInteger(node_type_array->_index, indent + 1);
            printDebug(output_prefix + "]");
        } else {
            printDebug(output_prefix + "[*]");
        }
    }

    void printTupleType(NodeTypeTuple* node_type_tuple, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(output_prefix + "[tuple type]");

        for (NodeType* type : node_type_tuple->_types) {
            printType(type, indent + 1);
        }
    }

    void printIdentifierToken(NodeIdentifierToken* identifier, int indent, std::string prefix="") {
        std::string output_prefix = getDebugPrefix(indent);

        printToken(identifier->_token, indent);
    }

    void printIdentifier(NodeIdentifier* identifier, int indent) {
        std::string output_prefix = getDebugPrefix(indent);
        printDebug("printing identifier");

        if (identifier->_access_token) {
            printDebug(output_prefix + ". (");
            printIdentifier(identifier->_access_token, indent + 1);
            printDebug(output_prefix + ")");
        }

        if (std::holds_alternative<NodeIdentifierToken*>(identifier->_identifier)) {
            printDebug("printing NodeIdentifierToken");
            NodeIdentifierToken* node_identifier_token = std::get<NodeIdentifierToken*>(identifier->_identifier);
            printIdentifierToken(node_identifier_token, indent);
        }

        else if (std::holds_alternative<NodeTupleIdentifier*>(identifier->_identifier)) {
            printDebug("printing NodeTupleIdentifier");

            NodeTupleIdentifier* node_identifier_tuple = std::get<NodeTupleIdentifier*>(identifier->_identifier);
            for (auto identifier : node_identifier_tuple->_identifiers) {
                printIdentifier(identifier, indent);
            }

        } else if (std::holds_alternative<NodeArrayIndex*>(identifier->_identifier)) {
            printDebug("found NodeArrayIndex");
            NodeArrayIndex* node_array_index = std::get<NodeArrayIndex*>(identifier->_identifier);
            printIdentifier(node_array_index->_identifier, indent);

            printDebug(output_prefix + "[");
            printExpression(node_array_index->_expression, indent + 1);
            printDebug(output_prefix + "]");

        } else if (std::holds_alternative<NodeFunctionCall*>(identifier->_identifier)) {
            printDebug("found NodeFunctionCall");
            NodeFunctionCall* node_function_call = std::get<NodeFunctionCall*>(identifier->_identifier);
            printIdentifier(node_function_call->_identifier, indent + 1);
    
            printDebug(output_prefix + "(");        
            printCallArguments(node_function_call, indent + 1);
            printDebug(output_prefix + ")");

            
        } else {
            printError("couldnt find identifier");
        }


    }

    void printFunctionDecleration(NodeFunctionDecleration* function_decleration, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(output_prefix + "[arguments]");
        printFunctionDeclerationArguments(function_decleration->_arguments, indent, *(function_decleration->is_procedure));

        if (*(function_decleration->is_procedure) == false) {
            printDebug(output_prefix + "[function Decleration]");
            printIdentifier(function_decleration->_identifier, indent);

            output_prefix += "    ";
            printDebug(output_prefix + "[returns]");
            printType(function_decleration->_return_type, indent + 1);


        } else if (*(function_decleration->is_procedure) == true) {
            printDebug(output_prefix + "[procedure Decleration]");
            printIdentifier(function_decleration->_identifier, indent + 1);

        } else {
            printError("Invalid function or procedure decleration");
        }

        if (function_decleration->_expression) {
            printExpression(function_decleration->_expression, indent + 1);
        } else if (function_decleration->_statement) {
            printStatement(function_decleration->_statement, indent + 1);
        }
    }

    void printFunctionDeclerationArguments(std::vector<NodeFunctionDeclerationArgument*> arguments, int indent, bool is_procedure) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(std::to_string(arguments.size()));
        for (auto argument : arguments) {
            if (is_procedure == true) {
                printQualifier(argument->_qualifier, indent + 1);
            }

            printType(argument->_type, indent + 1);
            printIdentifier(argument->_identifier, indent + 2);
        }

    }

    void printQualifier(NodeQualifier* node_qualifier, int indent) {
        std::string output_prefix = getDebugPrefix(indent);
        printDebug(output_prefix + node_qualifier->_token->getStrValue());
    }

    void printElement(NodeProgramElement* program_element, int indent) {
        if (std::holds_alternative<NodeStatement*>(program_element->_element)) {
            NodeStatement* statement = std::get<NodeStatement*>(program_element->_element);
            printStatement(statement, indent);

        } else if (std::holds_alternative<NodeFunctionDecleration*>(program_element->_element)) {
            NodeFunctionDecleration* function_decleration = std::get<NodeFunctionDecleration*>(program_element->_element);
            printFunctionDecleration(function_decleration, indent);

        } else if (std::holds_alternative<NodeTypealias*>(program_element->_element)) {
            NodeTypealias* node_typealias = std::get<NodeTypealias*>(program_element->_element);
            printTypealias(node_typealias, indent);
        }
    }

    void printCallArguments(NodeFunctionCall* node_function_call, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        for (auto argument : node_function_call->_arguments) {
            printExpression(argument->_expression, indent + 1);
        }
    }

    void printStatement(NodeStatement* statement, int indent) {
        std::string output_prefix = getDebugPrefix(indent);

        if (std::holds_alternative<NodeDecleration*>(statement->_statement)) {
            printDebug(output_prefix + "[Decleration]");
            printDecleration(std::get<NodeDecleration*>(statement->_statement), indent + 1);

        } else if (std::holds_alternative<NodeBlock*>(statement->_statement)) {
            output_prefix += "[block]";
            printDebug(output_prefix);
            NodeBlock* node_block = std::get<NodeBlock*>(statement->_statement);
            for (auto element : node_block->_elements) {
                printElement(element, indent + 1);
            }

        } else if (std::holds_alternative<NodeControl*>(statement->_statement)) {
            NodeControl* node_control = std::get<NodeControl*>(statement->_statement);
            if (node_control->_token->getTokenType() == TokenType::_if) {
                printDebug(output_prefix + "[if]");
            
            } else if (node_control->_token->getTokenType() == TokenType::_else_if) {
                printDebug(output_prefix + "[else if]");

            } else if (node_control->_token->getTokenType() == TokenType::_else) {
                printDebug(output_prefix + "[else]");
            }
            
            if (node_control->_token->getTokenType() == TokenType::_if || node_control->_token->getTokenType() == TokenType::_else_if) {
                printExpression(node_control->_expression, indent);
            }

            printStatement(node_control->_statement, indent + 1);

        } else if (std::holds_alternative<NodeStatementToken*>(statement->_statement)) {
            NodeStatementToken* node_statement_token = std::get<NodeStatementToken*>(statement->_statement);

            if (node_statement_token->_token->getTokenType() == TokenType::_continue) {
                printDebug(output_prefix + "[continue]");

            } else if (node_statement_token->_token->getTokenType() == TokenType::_break) {
                printDebug(output_prefix + "[break]");
            }

        } else if (std::holds_alternative<NodeStream*>(statement->_statement)) {
            NodeStream* node_stream = std::get<NodeStream*>(statement->_statement);
            printDebug(output_prefix + "[stream]");
            output_prefix += "    ";
            printDebug(output_prefix + node_stream->_operator->getStrValue() + " " + node_stream->_destination->getStrValue() + " (");
            printExpression(node_stream->_expression, indent + 2);
            printDebug(output_prefix + ")");
        
        } else if (std::holds_alternative<NodeLoop*>(statement->_statement)) {
            NodeLoop* node_loop = std::get<NodeLoop*>(statement->_statement);
            printDebug(output_prefix + "[loop]");
            if (node_loop->_expression) {
                if (*node_loop->_predicated) {
                    printDebug(output_prefix + "[predicated while]");
                    printExpression(node_loop->_expression, indent + 1);
                    printStatement(node_loop->_statement, indent + 1);

                } else {
                    printStatement(node_loop->_statement, indent + 1);
                    printDebug(output_prefix + "[post-predicated while]");
                    printExpression(node_loop->_expression, indent + 1);
                }
            } else {
                printStatement(node_loop->_statement, indent + 1);
            }

        } else if (std::holds_alternative<NodeReturn*>(statement->_statement)) {
            NodeReturn* node_return = std::get<NodeReturn*>(statement->_statement);
            printDebug(output_prefix + "[return]");
            if (node_return->_expression) {
                printExpression(node_return->_expression, indent + 1);
            }

        } else if (std::holds_alternative<NodeCall*>(statement->_statement)) {
            NodeCall* node_call = std::get<NodeCall*>(statement->_statement);
            printDebug(output_prefix + "[call]");
            printIdentifier(node_call->_function_call->_identifier, indent + 1);

        } else if (std::holds_alternative<NodeAssign*>(statement->_statement)) {
            NodeAssign* node_assign = std::get<NodeAssign*>(statement->_statement);
            printExpression(new NodeExpression{._expression = node_assign}, indent);

        } else {
            printDebug("1");
            printError("Unexpected statement encountered during print" + std::string(typeid(statement->_statement).name()));
        }
    }

    void printToken(Token* token, int indent = 0) {
        std::string output_prefix = getDebugPrefix(indent);
        printDebug(output_prefix + token->getStrValue());
    }

    void printTypealias(NodeTypealias* node_typealias, int indent = 0) {
        std::string output_prefix = getDebugPrefix(indent);

        printDebug(output_prefix + "[typealias]");
        printDebug(output_prefix + "[original]");
        printType(node_typealias->_original, indent + 1);
        printDebug(output_prefix + "[new] " + node_typealias->_new->getStrValue());
    }

    void printProgram(NodeProgram* program){
        int indent = 0;

        printDebug(std::string("printing program ") + std::to_string((program->_elements).size()));

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

    bool isNumber(std::string content) { // not used
        bool decimal = false;
        for (std::string::iterator s = content.begin(); s < content.end(); s++) {
            if (*s == '.' && !decimal) decimal = true;
            else if (!std::isdigit(*s)) return false;
        }
        return true;
    }

    bool isInteger(Token* token) {
        printDebug("isInteger: `" + token->getStrValue() + "`");

        for (char s : token->getStrValue()) {
            printDebug("isInteger: " + std::string(1, s));
            if (!std::isdigit(s)) return false;
        }
        return true;
    }

    bool isType(Token* token) {
        TokenType token_type = token->getTokenType();
        printDebug(token->getStrValue());

        if (std::find(m_types.begin(), m_types.end(), token->getStrValue()) != m_types.end()) {
            return true;
        }

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
                printDebug("found type token");
                return true;
            default:
                printDebug("not a type token");
                return false;
        }
    }

    bool isOperator(Token* token) {
        printDebug(token->getStrValue());
        switch (token->getTokenType())
        {
            case TokenType::_period:
            case TokenType::_open_square:
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
            case TokenType::_in:
            case TokenType::_vert_line:
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

    NodeInteger* parseInteger(int raise_error = 0) {
        NodeInteger* node_integer = new NodeInteger();
        if (isInteger(&*m_tokens_pointer)) {
            node_integer->_token = &*m_tokens_pointer;
            node_integer->_value = std::stoi(m_tokens_pointer->getStrValue());
            m_tokens_pointer++;

        } else if (raise_error) {
            printError("Expected integer", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        return node_integer;
    }

    NodeExpression* parseTuple(NodeExpression* first_expression) {
        NodeExpression* node_expression = new NodeExpression();
        NodeTuple* node_tuple = new NodeTuple();
        node_tuple->_expressions.push_back(first_expression);

        NodeExpression* temp_expression = new NodeExpression();
        while (temp_expression = parseExpression()) {
            node_tuple->_expressions.push_back(temp_expression);

            if (_isTokenType(TokenType::_comma)) {
                m_tokens_pointer++;
            }
        }

        node_expression->_expression = node_tuple;
        return node_expression;
    }

    NodeExpression* parseExpression(int min_precedence = 0) {
        printDebug("parsing expression with precedence: " + std::to_string(min_precedence));
        printDebug("Token type: " + std::to_string(_isTokenType(TokenType::_number) ? 1 : 0));
        NodeExpression* lhs = new NodeExpression();
        
        if (_isTokenType(TokenType::_open_paren) || _isTokenType(TokenType::_identifier)) {
            if (_isTokenType(TokenType::_identifier)) {
                printDebug("parsing expression identifier");
                lhs = new NodeExpression();
                lhs->_expression = parseIdentifier();
                printDebug("Added identifier");

                if (_isTokenType(TokenType::_comma)) {
                    m_tokens_pointer++;
                    lhs = parseTuple(lhs);
                }
            } else {
                m_tokens_pointer++;
                lhs = parseExpression(0);

                if (_isTokenType(TokenType::_comma)) {
                    m_tokens_pointer++;
                    lhs = parseTuple(lhs);
                }

                if (_isTokenType(TokenType::_close_paren)) {
                    m_tokens_pointer++;
                }
            }
        
        } else if (_isTokenType(TokenType::_not) || _isTokenType(TokenType::_unary_plus) || _isTokenType(TokenType::_unary_minus)) {
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

        } else if (_isTokenType(TokenType::_char_lit)) {
            NodeCharacter* _character = new NodeCharacter{._token=&(*m_tokens_pointer), ._value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _character;
            m_tokens_pointer++;
            printDebug("Added character");

        } else if (_isTokenType(TokenType::_text)) {
            NodeString* _string = new NodeString{._token=&(*m_tokens_pointer), ._value=m_tokens_pointer->getStrValue()};
            lhs->_expression = _string;
            m_tokens_pointer++;
            printDebug("Added string");
        
        } else if (_isTokenType(TokenType::_number)) {
            NodeInteger* _integer = new NodeInteger{._token=&(*m_tokens_pointer), ._value=std::stoi(m_tokens_pointer->getStrValue())};
            lhs->_expression = _integer;
            m_tokens_pointer++;
            printDebug("Added integer");

        } else if (_isTokenType(TokenType::_true) || _isTokenType(TokenType::_false)) {
            NodeBoolean* _boolean = new NodeBoolean{
                ._token=&(*m_tokens_pointer), 
                ._value=(_isTokenType(TokenType::_true))
            };

            lhs->_expression = _boolean;
            m_tokens_pointer++;
            printDebug("Added boolean");

        } else if (_isTokenType(TokenType::_generator)) {
            NodeGenerator* _generator = new NodeGenerator{._token = &(*m_tokens_pointer)};
            lhs->_expression = _generator;
            m_tokens_pointer++;
            printDebug("Added generator");

        } else if (_isTokenType(TokenType::_open_square)) {
            m_tokens_pointer++;
            NodeList* node_list = parseList();
            lhs->_expression = node_list;
            parseToken(TokenType::_close_square);

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

    NodeList* parseList() {
        printDebug("parsing list");
        NodeList* node_list = new NodeList();
        NodeExpression* node_expression = new NodeExpression();
        while (node_expression = parseExpression()) {
            node_list->_items.push_back(node_expression);
            
            if (_isTokenType(TokenType::_comma)) {
                m_tokens_pointer++;
            }
        }

        return node_list;
    }

    int parseSemi() {
        if (_isTokenType(TokenType::_semi)) {
            m_tokens_pointer++;
            printOk("completed a statement");
            return 1;

        } else {
            printError("Expected `;` but got " + m_tokens_pointer->getStrValue(), m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            return 0;

        }
    }

    NodeTypeTuple* parseTypeTuple(int raise_error = 0) {
        NodeTypeTuple* node_type_tuple = new NodeTypeTuple();
        if (_isTokenType(TokenType::_open_paren)) {
            m_tokens_pointer++;

            while (NodeType* node_type = parseType()) {
                node_type_tuple->_types.push_back(node_type);
                if (_isTokenType(TokenType::_comma)) {
                    m_tokens_pointer++;
                }
            }

            if (!_isTokenType(TokenType::_close_paren)) {
                if (raise_error) {
                    printError("Expected `)`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                } else {
                    printDebug("returning null");
                    node_type_tuple = nullptr;
                }
            } else {
                m_tokens_pointer++;
            }

        } else if (raise_error) {
            printError("Expected in parse tuple `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        return node_type_tuple;
    }

    void expect(TokenType token_type, int next=0) {
        if (!_isTokenType(token_type, next)) {
            printError("Unexpected token_type");
        }
    }

    Token* parseToken(TokenType token_type) {
        expect(token_type);
        Token* token = &*m_tokens_pointer++;
        return token;
    }

    NodeDecleration* parseDecleration() {
        printDebug("parseDecleration");

        NodeDecleration* decleration = new NodeDecleration();
        bool is_decleration = false;
        bool is_struct_decleration = false;

        // parse qualifier
        printDebug("checking for qualifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (isQualifier(&(*m_tokens_pointer))) {
            printOk("found qualifier");
            decleration->_qualifier = parseQualifer();
            printDebug(decleration->_qualifier->_token->getStrValue());
            is_decleration = true;
        }

        // parse type
        printDebug("checking for type at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        if (_isTokenType(TokenType::_struct)) {
            NodeStruct* node_struct = new NodeStruct();
            printOk("found struct");
            m_tokens_pointer++;

            NodeIdentifier* type = parseIdentifier(false, -1);
            node_struct->_type = type;

            if (!std::holds_alternative<NodeIdentifierToken*>(type->_identifier)) {
                printError("Struct identifier doesnt hold token");
            }

            NodeIdentifierToken* token = std::get<NodeIdentifierToken*>(type->_identifier);
            m_types.push_back(token->_token->getStrValue());
            printOk("found type for struct");

            node_struct->_arguments = parseFunctionDeclerationArguments(new bool(false));
            printOk("found parseFunctionDeclerationArguments");
            is_struct_decleration = true;
            decleration->_type = node_struct;

        } else if ((_isTokenType(TokenType::_identifier) && _isTokenType(TokenType::_identifier, 1)) || isType(&*m_tokens_pointer)) {
            m_types.push_back(m_tokens_pointer->getStrValue());
            decleration->_type = parseType(1);
            printOk("found type");
            is_decleration = true;
        }

        printDebug("checking for identifier at " + std::to_string(m_tokens_pointer - m_tokens.begin()));
        printDebug(m_tokens_pointer->getStrValue());
        if (_isTokenType(TokenType::_identifier)) {
            is_decleration = true;
            decleration->_identifier = parseIdentifier(true, 0);
            
            printOk("parsed NodeIdentifier");

            if (std::holds_alternative<NodeIdentifier*>(decleration->_identifier) && _isTokenType(TokenType::_comma)) {
                printDebug("holds NodeIdentifier");

                NodeIdentifier* node_identifier = std::get<NodeIdentifier*>(decleration->_identifier);
                
                NodeTupleIdentifier* node_tuple_identifier = new NodeTupleIdentifier();
                node_tuple_identifier->_identifiers.push_back(node_identifier);

                while (_isTokenType(TokenType::_comma)) {
                    m_tokens_pointer++;
                    node_tuple_identifier->_identifiers.push_back(parseIdentifier(true, 0));
                } 

                if (node_tuple_identifier->_identifiers.size() >= 2) {
                    decleration->_identifier = new NodeIdentifier{._identifier = node_tuple_identifier};
                    printDebug("tuple");
                }
            } 
            else {
                printDebug("Single");
                printDebug(m_tokens_pointer->getStrValue());
            }
                
            if (_isTokenType(TokenType::_assign)) {
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
        
        } else if (is_struct_decleration) {
            is_decleration = true;
        }
        
        if (is_decleration) {
            parseSemi();
            return decleration;
        }

        return nullptr;
    }

    NodeBlock* parseBlock() {
        NodeBlock* node_block = new NodeBlock();
        if (_isTokenType(TokenType::_open_curly)) {
            m_tokens_pointer++;

            printDebug("Opening a block");
            
            while (!_isTokenType(TokenType::_close_curly)) {
                node_block->_elements.push_back(parseElement());
            }
            
            m_tokens_pointer++;
        }

        return node_block;
    }

    NodeAssign* parseAssign() {
        NodeAssign* assign = new NodeAssign();
        auto save = m_tokens_pointer;

        if (_isTokenType(TokenType::_identifier)) {
            NodeExpression* left = parseExpression();

            if (_isTokenType(TokenType::_assign)) {
                printDebug("parsing assign");

                assign->_lhs = left;

                assign->_operator = new NodeOperator();
                assign->_operator->_token = &*m_tokens_pointer;
                m_tokens_pointer++;
                
                NodeExpression* right = parseExpression();
                assign->_rhs = right;
                return assign;

            }
        }
        
        m_tokens_pointer = save;
        return nullptr;
    }

    NodeStatement* parseStatement() {
        NodeStatement* statement = new NodeStatement();

        if (_isTokenType(TokenType::_else) || _isTokenType(TokenType::_if)) {

            printDebug(std::string("parsing ") + (_isTokenType(TokenType::_if) ? "if" : "else if") + std::string(" statement"));
            
            NodeControl* node_control = new NodeControl();
            if (_isTokenType(TokenType::_if) || (_isTokenType(TokenType::_else) && !_isTokenType(TokenType::_if, 1))) {
                node_control->_token = &(*m_tokens_pointer);
                m_tokens_pointer++;

            } else {
                node_control->_token = new Token(TokenType::_else_if, "`else if`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                m_tokens_pointer += 2;
            }

            if (node_control->_token->getTokenType() == TokenType::_else_if || node_control->_token->getTokenType() == TokenType::_if) {
                if (_isTokenType(TokenType::_open_paren)) {
                    m_tokens_pointer++;

                    NodeExpression* node_expression = parseExpression();
                    if (node_expression == nullptr) {
                        printError("Expected expression for conditional statements");
                    }

                    if (_isTokenType(TokenType::_close_paren)) {
                        m_tokens_pointer++;
                    } else {
                        printError("Expected `)` after conditional statements");
                    }

                    node_control->_expression = node_expression;
                    printOk("found expression");
                } else {
                    printError("Expected `(` after conditional statements");
                }

            }
            

            NodeStatement* node_statement = parseStatement();
            if (node_statement == NULL) {
                printError("Expected body for if statement");
            }
            node_control->_statement = node_statement;
            printOk("found statement");
            statement->_statement = node_control;
        }

        else if (_isTokenType(TokenType::_loop)) {
            NodeLoop* node_loop = new NodeLoop();
            node_loop->_predicated = new bool(0);
            printDebug("parsing loop");
            m_tokens_pointer++;
            
            if (_isTokenType(TokenType::_while)) {
                printDebug("found while");
                node_loop->_predicated = new bool(1);
                m_tokens_pointer++;

                if (_isTokenType(TokenType::_open_paren)) {
                    m_tokens_pointer++;
                    node_loop->_expression = parseExpression();
                    if (!_isTokenType(TokenType::_close_paren)) {
                        printError("Expected in while `)`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    } else {
                        m_tokens_pointer++;
                        printDebug("closed expression");
                    }
                } else {
                    printError("Expected in whilw `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                }
            }

            printDebug("parsing statement 123");
            node_loop->_statement = parseStatement();
            printDebug("parsed statement 123");

            if (_isTokenType(TokenType::_while)) {
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

        else if (_isTokenType(TokenType::_open_curly)) {
            statement->_statement = parseBlock();
            printOk("parsed a block");
        }

        else if (_isTokenType(TokenType::_break) || _isTokenType(TokenType::_continue)) {
            NodeStatementToken* node_statement_token = new NodeStatementToken();
            node_statement_token->_token = &(*m_tokens_pointer);

            statement->_statement = node_statement_token;
            m_tokens_pointer++;
            parseSemi();
        }

        else if (_isTokenType(TokenType::_return)) {
            
            NodeReturn* node_return = new NodeReturn();
            node_return->_token = &(*m_tokens_pointer);
            m_tokens_pointer++;
            if (m_tokens_pointer->getTokenType() != TokenType::_semi) {
                node_return->_expression = parseExpression();
            }
            parseSemi();

            statement->_statement = node_return;
        }

        else if (_isTokenType(TokenType::_call)) {
            printDebug("parsing `call`");
            
            NodeCall* node_call = new NodeCall();
            m_tokens_pointer++;

            if (_isTokenType(TokenType::_identifier)) {
                printDebug("found identifier afer `call`");

                node_call->_function_call = new NodeFunctionCall();
                node_call->_function_call->_identifier = parseIdentifier(true, 0);
                if (!std::holds_alternative<NodeFunctionCall*>(node_call->_function_call->_identifier->_identifier)) {
                    printError("doesnt hold NodeFunctionCall");
                }

                parseSemi();
                statement->_statement = node_call;
            } else {
                printError("Expected identifier after `call`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }

        }

        else {
            auto temp = m_tokens_pointer;

            NodeExpression* expression = parseExpression();
            if (expression != nullptr && (_isTokenType(TokenType::_stream_input) || _isTokenType(TokenType::_stream_output))) {

                if (NodeStream* node_stream = parseStream(expression, 1)) {
                    node_stream->_expression = expression;
                    parseSemi();

                    statement->_statement = node_stream;
                    return statement;
                }
            } 
            
            m_tokens_pointer = temp;
            if (_isTokenType(TokenType::_identifier)) {
                m_tokens_pointer = temp;
                NodeAssign* node_assign = parseAssign();
                if (node_assign != nullptr) {
                    parseSemi();
                    statement->_statement = node_assign;
                }
            }

            else {
                m_tokens_pointer = temp;
                NodeDecleration* declaration = parseDecleration();

                if (declaration) {
                    printOk("Decleration parsed");
                    statement->_statement = declaration;
                }

                else {
                    m_tokens_pointer = temp;
                    printError("Invalid statement", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                }
            }
    

        }
        
        printDebug("Returning statement");
        return statement;
    }

    /*
        returns 1 if buffer meets the criteria to be an identifier otherwise 0
    */
    bool _isTokenType(TokenType token_type, int next = 0) {
        return (m_tokens_pointer + next)->getTokenType() == token_type;
    }

    NodeStream* parseStream(NodeExpression* expression, int raise_error=0) {
        NodeStream* node_stream = new NodeStream();

        if (
            (_isTokenType(TokenType::_stream_output) && _isTokenType(TokenType::_std_output, 1)) || 
            (_isTokenType(TokenType::_stream_input) && _isTokenType(TokenType::_std_input, 1) && std::holds_alternative<NodeIdentifier*>(expression->_expression))
        ) {

            node_stream->_operator = &(*m_tokens_pointer);
            node_stream->_destination = &(*(m_tokens_pointer + 1));
            m_tokens_pointer += 2;

            printOk(std::string("Found an ") + std::string(_isTokenType(TokenType::_stream_output) ? "in" : "out") + std::string("put stream statement"));
            
        } else if (raise_error) {
            printError("Found a stream statement but couldnt figure out which type");
        
        } else {
            return nullptr;
        }
        
        return node_stream;
    } 

    NodeIdentifier* parseIdentifier(bool raise_error=0, int main = 0) {
        printDebug("parseIdentifier function");

        if (_isTokenType(TokenType::_identifier)) {
            printDebug("found identifier");

            NodeIdentifier* node_identifier = new NodeIdentifier();
            NodeIdentifierToken* node_identifier_token = new NodeIdentifierToken();
            node_identifier_token->_token = &(*m_tokens_pointer);
            node_identifier->_identifier = node_identifier_token;
            m_tokens_pointer++;

            while (main != -1 && _isTokenType(TokenType::_period)) {
                m_tokens_pointer++;

                NodeIdentifier* new_identifier = new NodeIdentifier();

                new_identifier->_identifier = node_identifier_token;
                new_identifier->_access_token = parseIdentifier();

                node_identifier = new_identifier;
            }

            
            if (main == 0) {
                while (_isTokenType(TokenType::_open_square) || _isTokenType(TokenType::_open_paren)) {
                    if (_isTokenType(TokenType::_open_square)) {

                        printDebug("found _open_square");
                        m_tokens_pointer++;
                        NodeArrayIndex* node_array_index = new NodeArrayIndex();
                        node_array_index->_identifier = node_identifier;
                        node_array_index->_expression = parseExpression(1);
                        
                        parseToken(TokenType::_close_square);
                        node_identifier = new NodeIdentifier {._identifier = node_array_index};
                    }
                    
                    else if (_isTokenType(TokenType::_open_paren)) {
                        printDebug("found _open_paren");

                        std::vector<NodeFunctionCallArgument*> function_call_arguments = parseFunctionCallArguments();
                        NodeFunctionCall* function_call = new NodeFunctionCall();
                        function_call->_arguments = function_call_arguments;
                        function_call->_identifier = node_identifier;
                        
                        node_identifier = new NodeIdentifier {._identifier = function_call};
                        printDebug("Added function call with arguments: " + std::to_string(function_call_arguments.size()));

                    } 
                }

            }
            

            printDebug("returning from parseIdentifier");
            return node_identifier;
        }

        if (raise_error) {
            printError("Expected identifier", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        return nullptr;
    }

    NodeQualifier* parseQualifer(int raise_error=0) {
        if (isQualifier(&*m_tokens_pointer)) {
            return new NodeQualifier{._token=&*m_tokens_pointer++};

        } else if (raise_error) {
            printError("Expected qualifier");

        }

        return nullptr;
    }

    NodeType* parseType(int raise_error=0) {
        printDebug("parsing type..a.");
        if (isType(&*m_tokens_pointer)) {
            NodeType* node_type = new NodeType{._type=&*m_tokens_pointer};
            m_tokens_pointer++;

            while (_isTokenType(TokenType::_open_square)) {
                m_tokens_pointer++;
                NodeTypeArray* node_type_array = new NodeTypeArray();
                if (!_isTokenType(TokenType::_asterisk)) {
                    node_type_array->_index = parseInteger(1);
                } else {
                    m_tokens_pointer++;
                }

                if (!_isTokenType(TokenType::_close_square)) {
                    printError("Expected `]`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                } else {
                    m_tokens_pointer++;
                }

                node_type_array->_type = node_type;
                
                node_type = new NodeType{._type = node_type_array};
            }

            if (_isTokenType(TokenType::_vector, -1)) {
                
                parseToken(TokenType::_less_than);
                NodeTypeVector* node_type_vector = new NodeTypeVector();
                node_type_vector->_type = parseType();
                parseToken(TokenType::_greater_than);

                return new NodeType{._type = node_type_vector};
            }

            else if (_isTokenType(TokenType::_tuple, -1)) {
                if (isQualifier(&*(m_tokens_pointer - 1))) {
                    printError("tuple cant be declared with a qualifier");
                }

                printOk("found tuple");
                NodeTypeTuple* node_type_tuple = parseTypeTuple();
                return new NodeType{._type = node_type_tuple};
            }

            return node_type;
        
        } else if (raise_error) {
            printError("Expected type");
        }

        return nullptr;
    }

    NodeFunctionDeclerationArgument* parseFunctionDeclerationArgument(bool* is_procedure) {
        printDebug("parsing DeclerationArgument " + std::to_string(*is_procedure));
        
        if (!*is_procedure || isQualifier(&(*m_tokens_pointer))) {
            NodeFunctionDeclerationArgument* node_argument = new NodeFunctionDeclerationArgument();

            if (*is_procedure) {
                node_argument->_qualifier = parseQualifer(1);
            }

            if (isType(&(*m_tokens_pointer))) {
                
                node_argument->_type = parseType();
                printOk("parsed type");

                if (m_tokens_pointer < m_tokens.end() && _isTokenType(TokenType::_identifier)) {
                    node_argument->_identifier = parseIdentifier(false, -1);

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
    
    std::vector<NodeFunctionDeclerationArgument*> parseFunctionDeclerationArguments(bool* is_procedure) {
        std::vector<NodeFunctionDeclerationArgument*> node_arguments;
        if (m_tokens_pointer->getTokenType() != TokenType::_open_paren) {
            printError("Expected parseFunctionDeclerationArguments `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }
        m_tokens_pointer++;
        printDebug("found `(`");

        if (_isTokenType(TokenType::_close_paren)) {
            printDebug("found `)`");
            m_tokens_pointer++;
            return node_arguments;
        }

        printDebug("parsing argument");
        while (true) {
            NodeFunctionDeclerationArgument* node_argument = new NodeFunctionDeclerationArgument();
            node_argument = parseFunctionDeclerationArgument(is_procedure);
            if (!node_argument) break;

            node_arguments.push_back(node_argument);
            printDebug("parsed argument");
            
            if (_isTokenType(TokenType::_comma)) {
                printDebug("found comma");
                m_tokens_pointer++;

            } else if (_isTokenType(TokenType::_close_paren)) {
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
            printDebug("parsed call argument. next token: " + m_tokens_pointer->getStrValue());
            return call_argument;
        }

        return nullptr;
    }

    std::vector<NodeFunctionCallArgument*> parseFunctionCallArguments() {
        std::vector<NodeFunctionCallArgument*> call_arguments;
        if (m_tokens_pointer->getTokenType() != TokenType::_open_paren) {
            printError("Expected parseFunctionCallArguments `(`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
        }

        m_tokens_pointer++;
        printDebug("found `(`");

        if (_isTokenType(TokenType::_close_paren)) {
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
            
            if (_isTokenType(TokenType::_comma)) {
                printDebug("found comma");
                m_tokens_pointer++;

            } else if (_isTokenType(TokenType::_close_paren)) {
                printDebug("found `)`");
                m_tokens_pointer++;
                return call_arguments;
                
            } else {
                printError("Expected `)` or `,` but got" + m_tokens_pointer->getStrValue(), m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
            }
        }

        printError("shouldnt reach here when parsing call arguments");
        return call_arguments;
    }


    NodeFunctionDecleration* parseFunctionOrProcedure() {
        NodeFunctionDecleration* node_function = new NodeFunctionDecleration();
        bool* is_procedure = nullptr;

        if (_isTokenType(TokenType::_function) || _isTokenType(TokenType::_procedure)) {
            printDebug(std::to_string(_isTokenType(TokenType::_procedure)));

            if (_isTokenType(TokenType::_function)) {
                printDebug("parsing function");
                is_procedure = new bool(false);
            } else {
                printDebug("parsing procedure");
                is_procedure = new bool(true);
                printDebug("set is_procedure to false");
            }

            m_tokens_pointer++;

            printDebug("parsing identifier");
            node_function->_identifier = parseIdentifier(true, -1);
            if (node_function->_identifier) {
                printDebug("parsed identifier");

                printDebug("parsing arguments");
                node_function->_arguments = parseFunctionDeclerationArguments(is_procedure);
                printDebug("parsed arguments: " + std::to_string((node_function->_arguments).size()));

                if (m_tokens_pointer->getTokenType() != TokenType::_returns) {
                    if (is_procedure && *is_procedure == false) {
                        printError("Expected `returns`", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }

                } else {
                    m_tokens_pointer++;

                    if (m_tokens_pointer < m_tokens.end() && isType(&(*m_tokens_pointer))) {
                        node_function->_return_type = parseType();

                    } else {
                        printError("Expected type", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }
                }


                if (m_tokens_pointer < m_tokens.end() && _isTokenType(TokenType::_assign)) {
                    m_tokens_pointer++;
                    printDebug("parsing expression");

                    if (node_function->_expression = parseExpression()) {
                        printDebug("parsed expression");
                        parseSemi();
                    } else {
                        printError("Expected expression", m_tokens_pointer->getLine(), m_tokens_pointer->getChar());
                    }

                } else if (m_tokens_pointer < m_tokens.end() && _isTokenType(TokenType::_open_curly)) {
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

        node_function->is_procedure = new bool(*is_procedure);
        return node_function;
    }

    NodeProgramElement* parseElement() {
        NodeProgramElement* node_program_element = new NodeProgramElement();
        if (_isTokenType(TokenType::_function) || _isTokenType(TokenType::_procedure)) {
            printDebug("parsing function or procedure...");
            NodeFunctionDecleration* function_decleration = parseFunctionOrProcedure();
            if (!function_decleration) return nullptr;

            node_program_element->_element = function_decleration;
        }        
                
        else {
            printDebug("parsing statement in parseElement...");
            NodeStatement* statement = parseStatement();
            if (!statement) return nullptr;

            node_program_element->_element = statement;
        }
        return node_program_element;
    }

    NodeProgram* parseProgram() {
        printDebug("parsing program...");
        
        while (m_tokens_pointer < m_tokens.end()) {
            while (_isTokenType(TokenType::_typealias)) {
                parseToken(TokenType::_typealias);
                NodeTypealias* node_typealias = new NodeTypealias();
                node_typealias->_original = parseType(1);
                node_typealias->_new = parseToken(TokenType::_identifier);
                m_types.push_back(node_typealias->_new->getStrValue());
                parseSemi();

                NodeProgramElement* node_program_element = new NodeProgramElement{._element=node_typealias};
                m_program->_elements.push_back(node_program_element);
            }

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
