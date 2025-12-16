#pragma once
#include "./tokenization.hpp"
#include "./parser.hpp"
#include <algorithm>

#define DEBUG 1

struct LoopContext {
    int loop_id;
};

class Generator
{
private:
    NodeProgram *m_program;
    std::stringstream m_output_stream;
    std::vector<std::unordered_map<std::string, int>> m_scopes;

    int m_local_size = 0;
    int m_temp_size = 0;
    int m_max_temp_size = 0;
    int m_frame_size = 0;
    bool m_count_only = false;
    int m_current_scope = -1;
    int m_label_count = 0;
    std::vector<LoopContext> m_loop_stack;

public:
    Generator(NodeProgram *program)
    {
        printDebug("================= Generator ===============");
        m_program = program;
    }

    void push_scope()
    {
        printDebug("pushing scope::");
        m_scopes.emplace_back();
        m_current_scope = static_cast<int>(m_scopes.size()) - 1;
    }

    void pop_scope()
    {
        printDebug("popping scope::");
        if (!m_scopes.empty())
        {
            printDebug(std::to_string(m_scopes.size()));
            m_scopes.pop_back();
            printDebug(std::to_string(m_scopes.size()));
            m_current_scope = static_cast<int>(m_scopes.size()) - 1;
        }
        else
        {
            m_current_scope = -1;
        }
    }

    void generate()
    {
        printDebug("cp2");
        generateProgram(m_program);

        // output the assembly code to a file
        {
            std::ofstream ofile("output.s");
            ofile << m_output_stream.str();
        }

        printDebug("================= Generation complete ===============");
    }

    // output the assembly code to std output and to output.s
    void emit(const std::string &s, std::string comment = "", int indent = 0)
    {
        if (m_count_only)
            return;

        if (s.size())
        {
            std::cout << getDebugPrefix(indent) << s;
            m_output_stream << getDebugPrefix(indent) << s;

            if (comment.size())
            {
                std::cout << "            // " << comment;
                m_output_stream << "            // " << comment;
            }

            std::cout << std::endl;
            m_output_stream << std::endl;
        }
        else
        {
            std::cout << std::endl;
            m_output_stream << std::endl;
        }
    }

    void store_var(std::string _register, int offset, int indent = 0)
    {
        if (m_count_only)
            return;

        emit("str " + _register + ", [x29, #" + std::to_string(-offset) + "]", "store x0 at fp + " + std::to_string(-offset) + " into " + _register, indent);
    }

    void push_temp(std::string _register, int indent = 0)
    {
        m_temp_size += 8;
        m_max_temp_size = std::max(m_max_temp_size, m_temp_size);
        int offset = -(m_local_size + m_temp_size);

        if (m_count_only)
            return;

        emit("");
        emit("str " + _register + ", [x29, #" + std::to_string(offset) + "]", "store x0 at fp + " + std::to_string(offset) + " into " + _register, indent);
    }

    static int align16(int n)
    {
        return (n + 15) & ~15;
    }

    // peak by loading into _register from stack
    void peak(std::string _register, int offset, int indent = 0)
    {
        emit("");
        emit("ldr " + _register + ", [x29, #" + std::to_string(-offset) + "]", "peak from fp + " + std::to_string(-offset) + " into " + _register, indent);
    }

    void pop_temp(std::string _register, int indent = 0)
    {
        if (m_count_only)
        {
            m_temp_size -= 8;
            return;
        }

        peak(_register, m_local_size + m_temp_size, indent);
        m_temp_size -= 8;
    }

    std::string getDebugPrefix(int indent)
    {
        int tab_count = 0;
        std::stringstream output;
        while (tab_count++ < indent)
        {
            output << "    ";
        }

        return output.str();
    }

    void generateExpression(NodeExpression *expression, int indent)
    {
        if (!expression)
        {
            printDebug("null expression encountered in generateExpression");
        }

        // generate integer literal
        if (std::holds_alternative<NodeInteger *>(expression->_expression))
        {

            NodeInteger *node_integer = std::get<NodeInteger *>(expression->_expression);
            if (!node_integer)
                printError("Null NodeInteger");
            emit("");
            emit("mov x0, #" + std::to_string(node_integer->_value), "store the integer in x0", indent);

            return;
        }

        // generate a unary expression
        else if (std::holds_alternative<NodeExpressionUnary *>(expression->_expression))
        {
            NodeExpressionUnary *node_expression_unary = std::get<NodeExpressionUnary *>(expression->_expression);
            generateExpression(node_expression_unary->_expression, indent);

            switch (node_expression_unary->_operator->_token->getTokenType())
            {
            case TokenType::_unary_minus:
                printDebug("found unary minus");
                emit("neg x0, x0", "store in x0 negation of x0", indent);
                break;

            case TokenType::_not:
                printDebug("found unary not");
                emit("");
                emit("cmp x0, #0", "set flags: Z=1 if x0 == x0", indent);
                emit("cset x0, eq", "x0 = (x0 == 0) ? 1 : 0", indent);
                break;

            case TokenType::_unary_plus:
                printDebug("found unary plus");
                break;

            default:
                printError("invalid unary operator");
            }
        }

        // generate a binary expression
        else if (std::holds_alternative<NodeExpressionBinary *>(expression->_expression))
        {
            NodeExpressionBinary *node_expression_binary = std::get<NodeExpressionBinary *>(expression->_expression);
            if (!node_expression_binary)
                printError("Null NodeExpressionBinary");

            // generate and store lhs
            generateExpression(node_expression_binary->_lhs, indent);
            push_temp("x0", indent);

            // generate and store rhs
            generateExpression(node_expression_binary->_rhs, indent);
            pop_temp("x1", indent);

            // arithmetic
            NodeOperator *node_operator = node_expression_binary->_operator;
            if (!node_operator)
                printError("Null NodeOperator");

            switch (node_operator->_token->getTokenType())
            {
            case TokenType::_greater_than_equal:
                emit("cmp x1, x0", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, ge", "set x0 to the result", indent);
                // printError("found >=");
                break;

            case TokenType::_greater_than:
                emit("cmp x1, x0", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, gt", "set x0 to the result", indent);
                // printError("found >");
                break;

            case TokenType::_less_than_equal:
                emit("cmp x1, x0", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, le", "set x0 to the result", indent);
                // printError("found <=");
                break;

            case TokenType::_less_than:
                emit("cmp x1, x0", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, lt", "set x0 to the result", indent);
                // printError("found <");
                break;

            case TokenType::_check_equal:
                emit("cmp x0, x1", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, eq", "set x0 to the result", indent);
                // printError("found ==");
                break;

            case TokenType::_asterisk:
                emit("mul x0, x1, x0", "x0 = x1 * x0", indent);
                break;

            case TokenType::_fwd_slash:
                emit("");
                emit("div x0, x1, x0", "x0 = x1 / x0", indent);
                break;

            case TokenType::_binary_plus:
                emit("");
                emit("add x0, x1, x0", "x0 = x1 + x0", indent);
                break;

            case TokenType::_binary_minus:
                emit("");
                emit("sub x0, x1, x0", "x0 = x1 - x0", indent);
                break;

            case TokenType::_or:
                emit("");
                emit("cmp x0, #0", "compare if x0 is not 0 and set a flag", indent);
                emit("cset x0, ne", "set x0 to the result", indent);

                emit("cmp x1, #0", "compare x1 with 0 and set a flag", indent);
                emit("cset x1, ne", "set x1 to the result", indent);

                emit("orr x0, x1, x0", "x0 = x1 | x0", indent);
                break;

            case TokenType::_xor:
                emit("");
                emit("cmp x0, #0", "compare if x0 is 0 and set a flag", indent);
                emit("cset x0, eq", "set x0 to the result", indent);

                emit("cmp x1, #0", "compare x1 with 0 and set a flag", indent);
                emit("cset x1, eq", "set x1 to the result", indent);

                emit("eor x0, x0, x1", "", indent);
                break;

            case TokenType::_and:
                emit("");
                emit("cmp x0, #0", "compare if x0 is not 0 and set a flag", indent);
                emit("cset x0, ne", "set x0 to the result", indent);

                emit("cmp x1, #0", "compare x1 with 0 and set a flag", indent);
                emit("cset x1, ne", "set x1 to the result", indent);

                emit("and x0, x0, x1", "", indent);
                break;

            default:
                printError("Invalid binary expression");
            }
        }

        // generate assign expression
        else if (std::holds_alternative<NodeAssign *>(expression->_expression))
        {
            NodeAssign *node_assign = std::get<NodeAssign *>(expression->_expression);
            generateExpression(node_assign->_rhs, indent);
            printError("found node assign");
        }

        // generate identifier
        else if (std::holds_alternative<NodeIdentifier *>(expression->_expression))
        {
            NodeIdentifier *node_identifier = std::get<NodeIdentifier *>(expression->_expression);

            NodeIdentifierToken *node_identifier_token = std::get<NodeIdentifierToken *>(node_identifier->_identifier);
            printDebug("stack size::" + std::to_string(m_local_size));

            std::string identifier_name = node_identifier_token->_token->getStrValue();
            int offset = lookup(identifier_name);
            peak("x0", offset, indent);

            printDebug("offset from top::" + std::to_string(offset));

            return;
        }

        else
        {
            printError("Invalid expression variant");
        }
        return;
    }

    int lookup(const std::string &name)
    {
        for (int i = m_scopes.size() - 1; i >= 0; --i)
        {
            auto it = m_scopes[i].find(name);
            if (it != m_scopes[i].end())
                return it->second;
        }
        printError("use of undeclared variable '" + name + "'");
        return -1;
    }

    void generateDecleration(NodeDecleration *decleration, int indent)
    {
        // print qualifier
        if (decleration->_qualifier)
        {
            printDebug("[has qualifier]");
        }
        else
        {
            printDebug("[No qualifier]");
        }

        // print type
        if (std::holds_alternative<Token *>(decleration->_type))
        {
            Token *token = std::get<Token *>(decleration->_type);
            if (token != nullptr)
            {
                printDebug("Generating type token");
                // printDebug(token->getStrValue());
            }
        }
        else if (std::holds_alternative<NodeTypeTuple *>(decleration->_type))
        {
            NodeTypeTuple *node_type_tuple = std::get<NodeTypeTuple *>(decleration->_type);
            if (node_type_tuple != nullptr)
            {
                printDebug("Generating node_type_tuple");
                // printTupleType(node_type_tuple, indent);
            }
        }
        else if (std::holds_alternative<NodeType *>(decleration->_type))
        {
            NodeType *node_type = std::get<NodeType *>(decleration->_type);
            if (node_type != nullptr)
            {
                printDebug("Generating type");
                // printType(node_type, indent);
            }
        }
        else
        {
            printDebug("[No type]");
        }

        if (std::holds_alternative<NodeStruct *>(decleration->_type) && std::get<NodeStruct *>(decleration->_type))
        {
            printDebug("Passes struct");

            NodeStruct *node_struct = std::get<NodeStruct *>(decleration->_type);
            if (node_struct != nullptr)
            {
                printDebug("generating struct");
                // printStruct(node_struct, indent);
            }
        }

        // print identifier
        else if (decleration->_identifier)
        {

            printDebug("generating identifier");
            NodeIdentifier *node_identifier = decleration->_identifier;
            NodeIdentifierToken *node_identifier_token = std::get<NodeIdentifierToken *>(node_identifier->_identifier);
            std::string identifier_name = node_identifier_token->_token->getStrValue();
            printDebug(std::to_string(m_scopes.size()));
            int offset;

            // redecleration check
            if (m_scopes.size() > m_current_scope && m_scopes[m_current_scope].contains(identifier_name))
            {
                printError("redecleration of variable not allowed: " + identifier_name);
            }
            else
            {
                printDebug("cs1::" + std::to_string(m_current_scope) + "::" + std::to_string(m_local_size));
                m_local_size += 8;
                
                printDebug("cs1::" + std::to_string(m_current_scope) + "::" + std::to_string(m_local_size));
                m_scopes[m_current_scope][identifier_name] = m_local_size;
                printDebug(node_identifier_token->_token->getStrValue() + "::" + std::to_string(m_local_size));
            }
            
            if (decleration->_expression != NULL)
            {
                printDebug("generating expression");
                generateExpression(decleration->_expression, indent);
                emit("// expression generated");
                if (!m_count_only) {
                    offset = lookup(identifier_name);
                    printDebug("a::" + std::to_string(offset));
                    store_var("x0", offset, indent);
                }
            }
            else
            {
                printDebug("[No expression]");
            }
        }
        else
        {
            printError("Invalid identifier");
        }

    }

    int genLabel() {
        return m_label_count++;
    }

    void generateStatement(NodeStatement *statement, int indent)
    {

        if (std::holds_alternative<NodeDecleration *>(statement->_statement))
        {
            printDebug("Generating NodeDecleration");
            generateDecleration(std::get<NodeDecleration *>(statement->_statement), indent);
        }

        else if (std::holds_alternative<NodeBlock *>(statement->_statement))
        {
            printDebug("Generating NodeBlock");
            NodeBlock *node_block = std::get<NodeBlock *>(statement->_statement);

            push_scope();
            for (NodeProgramElement *element : node_block->_elements)
            {
                generateElement(element, indent + 1);
            }
            pop_scope();
        }

        else if (std::holds_alternative<NodeControl *>(statement->_statement))
        {
            printDebug("Generating NodeControl");
            NodeControl *node_control = std::get<NodeControl *>(statement->_statement);
            int id = genLabel();

            auto L = [&](const std::string& name) {
                return ".L" + name + "_" + std::to_string(id);
            };

            // generate if
            generateExpression(node_control->_if.first, indent);
            emit("");
            emit("cmp x0, #0", "compare if x0 is 0 and set a flag", indent);
            emit("b.eq " + L("next"), "branch to else if res is 0", indent);


            generateStatement(node_control->_if.second, indent);
            emit("");
            emit("b " + L("end"), "branch to else after completing if", indent);
            emit(L("next") + ":", "", indent);

            // generate else if
            int else_if_count = 0;
            for (int i = 0; i < node_control->_else_if.size(); i++) {
                auto elif = node_control->_else_if[i];

                generateExpression(elif.first, indent);
                emit("");
                emit("cmp x0, #0", "compare if x0 is 0 and set a flag", indent);
                emit("b.eq .Lelifnext_" + std::to_string(id) + "_" + std::to_string(i), "branch to else if res is 0", indent);

                generateStatement(elif.second, indent);
                emit("");
                emit("b " + L("end"), "branch to else after completing else if", indent);
                emit(".Lelifnext_" + std::to_string(id) + "_" + std::to_string(i) + ":", "", indent);
            }
            
            // generate else
            if (node_control->_statement_else) {
                emit("");
                generateStatement(node_control->_statement_else, indent);
                emit("");
            }

            emit(L("end") + ":", "", indent);
            emit("// incremented branch count");
        }
        else if (std::holds_alternative<NodeStatementToken *>(statement->_statement))
        {
            printDebug("Generating NodeStatementToken");
            NodeStatementToken *node_statment_token = std::get<NodeStatementToken*>(statement->_statement);

            switch (node_statment_token->_token->getTokenType()) {
                
                case TokenType::_break:
                    if (!m_loop_stack.size()) {
                        printError("`break` can only be used inside the body of a loop");
                    }

                    emit("b EndLoop_" + std::to_string(m_loop_stack.back().loop_id), "", indent);
                    break;

                case TokenType::_continue:
                    if (!m_loop_stack.size()) {
                        printError("`continue` can only be used inside the body of a loop");
                    }

                    emit("b LoopCondition_" + std::to_string(m_loop_stack.back().loop_id), "", indent);
                    break;

                default:
                    printError("Invalid token statement:" + node_statment_token->_token->getStrValue());
                    break;
            }
        }
        else if (std::holds_alternative<NodeStream *>(statement->_statement))
        {
            printDebug("Generating NodeStream");
            // todo
        }
        else if (std::holds_alternative<NodeLoop *>(statement->_statement))
        {
            printDebug("Generating NodeLoop");
            NodeLoop* node_loop = std::get<NodeLoop *>(statement->_statement);
            int loop_id = 0;
            if (!m_count_only) loop_id = genLabel();

            emit("BeginLoop_" + std::to_string(loop_id) + ":", "", 1);
            m_loop_stack.push_back({ loop_id });

            if (*node_loop->_predicated) {
                printDebug("predicated");
                if (!m_count_only)
                    emit("LoopCondition_" + std::to_string(loop_id) + ":", "", indent);
    
                generateExpression(node_loop->_expression, indent);
                emit("cmp x0, #0", "", indent);
                
                if (!m_count_only)
                    emit("b.eq EndLoop_" + std::to_string(loop_id), "", indent);

                generateStatement(node_loop->_statement, indent);

                if (!m_count_only)
                    emit("b BeginLoop_" + std::to_string(loop_id), "", indent);
                
            } else if (node_loop->_expression) {
                printDebug("postpredicated");
                generateStatement(node_loop->_statement, indent);
                
                if (!m_count_only)
                    emit("LoopCondition_" + std::to_string(loop_id) + ":", "", indent);
    
                generateExpression(node_loop->_expression, indent);
                emit("cmp x0, #0", "", indent);
    
                if (!m_count_only)
                    emit("b.ne BeginLoop_" + std::to_string(loop_id), "", indent);


            } else {
                printDebug("infinite");
                generateStatement(node_loop->_statement, indent);

                if (!m_count_only)
                    emit("b BeginLoop_" + std::to_string(loop_id), "", indent);
            }

            emit("EndLoop_" + std::to_string(loop_id) + ":", "", 1);
            m_loop_stack.pop_back();
        }

        else if (std::holds_alternative<NodeReturn *>(statement->_statement))
        {
            printDebug("Generating NodeReturn");
            NodeReturn *node_return = std::get<NodeReturn *>(statement->_statement);
            generateExpression(node_return->_expression, indent);

            emit("");
            emit("mov sp, x29", "restore sp from fp", indent);
            emit("ldp x29, x30, [sp], 16", "restore x29 and x30 from sp", indent);
            emit("ret", "end of function", indent);
        }
        else if (std::holds_alternative<NodeCall *>(statement->_statement))
        {
            printDebug("Generating NodeCall");
            // todo
        }
        else if (std::holds_alternative<NodeAssign *>(statement->_statement))
        {
            printDebug("Generating NodeAssign");
            NodeAssign *node_assign = std::get<NodeAssign *>(statement->_statement);
            generateExpression(node_assign->_rhs, indent);

            if (!m_count_only)
            {
                NodeIdentifier *lhs_identifier = std::get<NodeIdentifier *>(node_assign->_lhs->_expression);

                NodeIdentifierToken *lhs_identifier_token = std::get<NodeIdentifierToken *>(lhs_identifier->_identifier);

                emit("str x0, [x29, #" + std::to_string(-lookup(lhs_identifier_token->_token->getStrValue())) + "]", "store the new value", indent);
            }
        }
        else
        {
            printDebug("1");
            printError("Unexpected statement encountered during print" + std::string(typeid(statement->_statement).name()));
        }
    }

    void generateElement(NodeProgramElement *element, int indent)
    {
        if (std::holds_alternative<NodeStatement *>(element->_element))
        {
            NodeStatement *statement = std::get<NodeStatement *>(element->_element);
            printDebug("Generating statement");
            generateStatement(statement, indent);
        }
        else if (std::holds_alternative<NodeFunctionDecleration *>(element->_element))
        {
            NodeFunctionDecleration *function_decleration = std::get<NodeFunctionDecleration *>(element->_element);
            printDebug("Generating functionDecleration");
            // printFunctionDecleration(function_decleration, indent);
        }
        else if (std::holds_alternative<NodeTypealias *>(element->_element))
        {
            NodeTypealias *node_typealias = std::get<NodeTypealias *>(element->_element);
            printError("cant hold NodeTypealias");
            // printTypealias(node_typealias, indent);
        }
    }

    void resetFrameTracking()
    {
        m_scopes.clear();
        m_scopes.emplace_back(); // ensure there is always a base scope for declarations
        m_current_scope = 0;
        m_local_size = 0;
        m_temp_size = 0;
        m_max_temp_size = 0;
    }

    void generateProgram(NodeProgram *program)
    {
        printDebug("cp1");

        Parser Parser;
        // Parser.printProgram(program);
        int indent = 1;

        // Pass 1: count only
        printDebug("cp7");
        resetFrameTracking();
        printDebug("cp8");

        m_count_only = true;
        for (auto element : program->_elements)
        {
            printDebug("cp5");
            generateElement(element, indent);
            printDebug("cp6");
        }

        // Pass 2: code generation
        m_frame_size = align16(m_local_size + m_max_temp_size);

        resetFrameTracking();
        m_count_only = false;

        printDebug(std::string("generating program ") + std::to_string((program->_elements).size()));

        printDebug("cp4");
        emit(".global _main");
        printDebug("scope_size::" + std::to_string(m_current_scope));
        push_scope();
        printDebug("scope_size::" + std::to_string(m_current_scope));

        emit("_main:");
        emit("stp x29, x30, [sp, -16]!", "store pair fp & lr", indent);
        emit("mov x29, sp", "store sp", indent);
        emit("sub sp, sp, #" + std::to_string(m_frame_size), "allocate " + std::to_string(m_frame_size) + "-byte stack frame", indent);

        for (auto element : program->_elements)
        {
            generateElement(element, indent);
        }

        printDebug("scope_size::" + std::to_string(m_current_scope));
        pop_scope();
        printDebug("scope_size::" + std::to_string(m_current_scope));
    }

    void printError(std::string error_msg)
    {
        throw std::runtime_error(std::string(RED) + error_msg + "\033[0m");
    }

    void printError(std::string error_msg, int line, int _char)
    {
        throw std::runtime_error(std::string(RED) + error_msg + " at " + std::to_string(line) + ":" + std::to_string(_char) + "\033[0m");
    }

    void printDebug(std::string msg)
    {
#if DEBUG
        std::cout << std::string(CYAN) + "[debug] " + msg + "\033[0m" << std::endl;
#endif
    }

    void printOk(std::string msg)
    {
        std::cout << std::string(GREEN) + "[ok] " + msg + "\033[0m" << std::endl;
    }
};
