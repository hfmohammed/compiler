#pragma once
#include <string>
#include <cassert>
#include <vector>


#define CYAN    "\033[36m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"


enum class TokenType {
    _and,
    _as,
    _boolean,
    _break,
    _by,
    _call,
    _character,
    _columns,
    _const,
    _continue,
    _else,
    _false,
    _format,
    _function,
    _if,
    _in,
    _integer,
    _length,
    _loop,
    _not,
    _or,
    _procedure,
    _real,
    _return,
    _returns,
    _reverse,
    _rows,
    _std_input,
    _std_output,
    _stream_state,
    _string,
    _struct,
    _true,
    _tuple,
    _typealias,
    _var,
    _vector,
    _while,
    _xor,

    // custom
    _else_if,
    _binary_plus,
    _unary_plus,
    _binary_minus,
    _unary_minus,
    _stream_input,
    _stream_output,
    _asterisk,
    _assign,
    _qualifier,
    _type,
    _identifier,
    _expression,
    _semi,
    _hat,
    _text,
    _open_curly,
    _close_curly,
    _open_paren,
    _close_paren,
    _open_square,
    _close_square,
    _check_equal,
    _less_than,
    _less_than_equal,
    _greater_than,
    _greater_than_equal,
    _dbl_quote,
    _sgl_quote,
    _fwd_slash,
    _period,
    _comma,
    _vert_line,
    _mod,
    _ampersand,
    _comment,
    _number,
    _int_lit,
    _char_lit,
    _dbl_period,
    _dbl_vertical_line,
    _not_eq,
    _dbl_asterisk,
    _generator,
};

class Token {
private:
    TokenType m_type;
    std::string m_str_value;
    int m_line;
    int m_char;

public:
    Token() {}

    Token(TokenType type, std::string str_value, int line, int _char) {
        m_type = type;
        m_str_value = str_value;
        m_line = line;
        m_char = _char;
    }

    int getLine() {
        return m_line;
    }

    int getChar() {
        return m_char;
    }

    std::vector<int> getPos() {
        return std::vector<int>{getLine(), getChar()};
    }

    TokenType getTokenType() {
        return m_type;
    }

    std::string getStrValue() {
        return m_str_value;
    }

};

class Tokenizer {
private:
    std::vector<Token> m_tokens;
    std::string m_content;

public:
    explicit Tokenizer(std::string content) {
        m_content = content;
    }

    void printDebug(std::string msg) {
        std::cout << std::string(CYAN) + "[debug] " + msg + "\033[0m" << std::endl;
    }

    void printOk(std::string msg) {
        std::cout << std::string(GREEN) + "[ok] " + msg + "\033[0m" << std::endl;
    }

    void printError(std::string error_msg) {
        throw std::runtime_error(std::string(RED) + error_msg + "\033[0m");
    }

    void printError(std::string error_msg, int line, int _char) {
        throw std::runtime_error(std::string(RED) + error_msg + " at " + std::to_string(line) + ":" + std::to_string(_char) + "\033[0m");
    }

    bool isOperator(Token token) {
        // checks if token is an operator
        return token.getTokenType() == TokenType::_period || 
            token.getTokenType() == TokenType::_dbl_period || 
            token.getTokenType() == TokenType::_binary_plus || 
            token.getTokenType() == TokenType::_unary_plus || 
            token.getTokenType() == TokenType::_binary_minus || 
            token.getTokenType() == TokenType::_unary_minus || 
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

    Token getToken(std::string content, int line, int _char) {
        // adjust the marker to point before the token
        _char -= content.length();

        if (content.starts_with("//")) {
            return Token(TokenType::_comment, content, line, _char);
        }

        else if (content.starts_with("/*")) {
            if (content.ends_with("*/"))
                return Token(TokenType::_comment, content, line, _char);
            else {
                printError("Expected `*/`", line, _char);
                exit(1);
            }

        }

        else if (content == "and") {
            return Token(TokenType::_and, "`and`", line, _char);
        }

        else if (content == "->") {
            return Token(TokenType::_stream_output, "`->`", line, _char);
        }

        else if (content == "<-") {
            return Token(TokenType::_stream_input, "`<-`", line, _char);
        }

        else if (content == "as") {
            return Token(TokenType::_as, "`as`", line, _char);
        }

        else if (content == "boolean") {
            return Token(TokenType::_boolean, "`boolean`", line, _char);
        }

        else if (content == "break") {
            return Token(TokenType::_break, "`break`", line, _char);
        }

        else if (content == "by") {
            return Token(TokenType::_by, "`by`", line, _char);
        }

        else if (content == "call") {
            return Token(TokenType::_call, "`call`", line, _char);
        }

        else if (content == "character") {
            return Token(TokenType::_character, "`character`", line, _char);
        }

        else if (content == "columns") {
            return Token(TokenType::_columns, "`columns`", line, _char);
        }

        else if (content == "const") {
            return Token(TokenType::_const, "`const`", line, _char);
        }

        else if (content == "continue") {
            return Token(TokenType::_continue, "`continue`", line, _char);
        }

        else if (content == "else") {
            return Token(TokenType::_else, "`else`", line, _char);
        }

        else if (content == "false") {
            return Token(TokenType::_false, "`false`", line, _char);
        }

        else if (content == "format") {
            return Token(TokenType::_format, "`format`", line, _char);
        }

        else if (content == "function") {
            return Token(TokenType::_function, "`function`", line, _char);
        }

        else if (content == "if") {
            return Token(TokenType::_if, "`if`", line, _char);
        }

        else if (content == "else if") {
            return Token(TokenType::_else_if, "`else if`", line, _char);
        }

        else if (content == "in") {
            return Token(TokenType::_in, "`in`", line, _char);
        }

        else if (content == "integer") {
            return Token(TokenType::_integer, "`integer`", line, _char);
        }

        else if (content == "length") {
            return Token(TokenType::_length, "`length`", line, _char);
        }

        else if (content == "loop") {
            return Token(TokenType::_loop, "`loop`", line, _char);
        }

        else if (content == "not") {
            return Token(TokenType::_not, "`not`", line, _char);
        }

        else if (content == "or") {
            return Token(TokenType::_or, "`or`", line, _char);
        }

        else if (content == "procedure") {
            return Token(TokenType::_procedure, "`procedure`", line, _char);
        }

        else if (content == "real") {
            return Token(TokenType::_real, "`real`", line, _char);
        }

        else if (content == "return") {
            return Token(TokenType::_return, "`return`", line, _char);
        }

        else if (content == "returns") {
            return Token(TokenType::_returns, "`returns`", line, _char);
        }

        else if (content == "reverse") {
            return Token(TokenType::_reverse, "`reverse`", line, _char);
        }

        else if (content == "rows") {
            return Token(TokenType::_rows, "`rows`", line, _char);
        }

        else if (content == "std_input") {
            return Token(TokenType::_std_input, "`std_input`", line, _char);
        }

        else if (content == "std_output") {
            return Token(TokenType::_std_output, "`std_output`", line, _char);
        }

        else if (content == "stream_state") {
            return Token(TokenType::_stream_state, "`stream_state`", line, _char);
        }

        else if (content == "string") {
            return Token(TokenType::_string, "`string`", line, _char);
        }

        else if (content == "struct") {
            return Token(TokenType::_struct, "`struct`", line, _char);
        }

        else if (content == "true") {
            return Token(TokenType::_true, "`true`", line, _char);
        }

        else if (content == "tuple") {
            return Token(TokenType::_tuple, "`tuple`", line, _char);
        }

        else if (content == "typealias") {
            return Token(TokenType::_typealias, "`typealias`", line, _char);
        }

        else if (content == "var") {
            return Token(TokenType::_var, "`var`", line, _char);
        }

        else if (content == "vector") {
            return Token(TokenType::_vector, "`vector`", line, _char);
        }

        else if (content == "while") {
            return Token(TokenType::_while, "`while`", line, _char);
        }

        else if (content == "xor") {
            return Token(TokenType::_xor, "`xor`", line, _char);
        }

        // custom
        else if (content == ";") {
            return Token(TokenType::_semi, "`;`", line, _char);
        }

        else if (content == "^") {
            return Token(TokenType::_hat, "`^`", line, _char);
        }

        else if (content == "{") {
            return Token(TokenType::_open_curly, "`{`", line, _char);
        }

        else if (content == "}") {
            return Token(TokenType::_close_curly, "`}`", line, _char);
        }

        else if (content == "(") {
            return Token(TokenType::_open_paren, "`(`", line, _char);
        }

        else if (content == ")") {
            return Token(TokenType::_close_paren, "`)`", line, _char);
        }

        else if (content == "[") {
            return Token(TokenType::_open_square, "`[`", line, _char);
        }

        else if (content == "]") {
            return Token(TokenType::_close_square, "`]`", line, _char);
        }

        else if (content == "=") {
            return Token(TokenType::_assign, "`=`", line, _char);
        }

        else if (content == ">") {
            return Token(TokenType::_greater_than, "`>`", line, _char);
        }

        else if (content == "<") {
            return Token(TokenType::_less_than, "`<`", line, _char);
        }

        else if (content == "'") {
            return Token(TokenType::_sgl_quote, "`'`", line, _char);
        }

        else if (content == "\"") {
            return Token(TokenType::_dbl_quote, "`\"`", line, _char);
        }

        else if (content == ",") {
            return Token(TokenType::_comma, "`,`", line, _char);
        }

        else if (content == ".") {
            return Token(TokenType::_period, "`.`", line, _char);
        }

        else if (content == "+") {
            if (m_tokens.size() == 0 || isOperator(m_tokens.back()) || 
                m_tokens.back().getTokenType() == TokenType::_open_curly || m_tokens.back().getTokenType() == TokenType::_assign || m_tokens.back().getTokenType() == TokenType::_open_paren || m_tokens.back().getTokenType() == TokenType::_open_square 
            ) {
                return Token(TokenType::_unary_plus, "`+`", line, _char);
            }
            return Token(TokenType::_binary_plus, "`+`", line, _char);
        }

        else if (content == "-") {
            if (m_tokens.size() == 0 || isOperator(m_tokens.back()) || 
                m_tokens.back().getTokenType() == TokenType::_open_curly || m_tokens.back().getTokenType() == TokenType::_assign || m_tokens.back().getTokenType() == TokenType::_open_paren || m_tokens.back().getTokenType() == TokenType::_open_square 
            ) return Token(TokenType::_unary_minus, "`-`", line, _char);

            return Token(TokenType::_binary_minus, "`-`", line, _char);
        }

        else if (content == "*") {
            return Token(TokenType::_asterisk, "`*`", line, _char);
        }

        else if (content == "/") {
            return Token(TokenType::_fwd_slash, "`/`", line, _char);
        }

        else if (content == "|") {
            return Token(TokenType::_vert_line, "`|`", line, _char);
        }

        else if (content == "%") {
            return Token(TokenType::_mod, "`%`", line, _char);
        }

        else if (content == "&") {
            return Token(TokenType::_ampersand, "`&`", line, _char);
        }

        // double chars
        else if (content == "==") {
            return Token(TokenType::_check_equal, "`==`", line, _char);
        }

        else if (content == ">=") {
            return Token(TokenType::_greater_than_equal, "`>=`", line, _char);
        }

        else if (content == "<=") {
            return Token(TokenType::_less_than_equal, "`<=`", line, _char);
        }

        else if (content == "..") {
            return Token(TokenType::_dbl_period, "`..`", line, _char);
        }

        else if (content == "||") {
            return Token(TokenType::_dbl_vertical_line, "`||`", line, _char);
        }

        else if (content == "!=") {
            return Token(TokenType::_not_eq, "`!=`", line, _char);
        }

        else if (content == "**") {
            return Token(TokenType::_dbl_asterisk, "`**`", line, _char);
        }

        
        else if (isNumber(content)) {
            printDebug("Found number");
            return Token(TokenType::_number, content, line, _char);
        }
        
        else if (isInt(content)) {
            return Token(TokenType::_int_lit, content, line, _char);
        }

        else if (isIdentifier(content)) {
            return Token(TokenType::_identifier, content, line, _char);
        
        } else if (isGenerator(content)) {
            return Token(TokenType::_generator, content, line, _char);
        }

        printError("Unexpected token", line, _char);
        exit(EXIT_FAILURE);
        return Token(TokenType::_text, "`text`: \"" + content + "\"", line, _char);
    }


    /*
        returns 1 if buffer meets the criteria to be an identifier otherwise 0
    */
    bool isIdentifier(std::string content) {
        printDebug("checking if identifier: " + content);

        if (std::isalpha(*content.begin()) || *content.begin() == '_') {
            for (std::string::iterator s = content.begin() + 1; s < content.end(); s++) {
                if (!std::isalnum(*s) && *s != '_') {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    bool isInt(std::string content) {
        printDebug("checking is int");
        for (std::string::iterator s = content.begin(); s < content.end(); s++) {
            if (!std::isdigit(*s)) return false;
        }
        return true;
    }

    bool isGenerator(std::string content) {
        printDebug("checking is generator");
        bool found_1 = false;
        bool found_2 = false;
        for (std::string::iterator s = content.begin(); s < content.end(); s++) {
            if ((!std::isdigit(*s) && *s != '.') || (*s == '.' && found_1 && found_2)) return false;
            
            if (*s == '.' && found_1) found_2 = true;
            else if (*s == '.') found_1 = true;
            printDebug(std::string(1, *s) + " " + std::to_string(found_1) + " " + std::to_string(found_2));
        }
        return found_1 && found_2;
    }

    bool isNumber(std::string content) {
        bool decimal = false;
        for (std::string::iterator s = content.begin(); s < content.end(); s++) {
            if (*s == '.' && !decimal) decimal = true;
            else if (!std::isdigit(*s)) return false;
        }
        return true;
    }

    std::vector<Token> tokenize() {
        std::string buffer;
        int line = 0;
        int _char = 0;

        for (std::string::iterator it = m_content.begin(); it < m_content.end(); it++)
        {
            // printDebug(std::string(1, *it) + " at " + std::to_string(line) + ":" + std::to_string(_char));

            if (*it == EOF) {
                break;
            }

            else if (std::isspace(*it) && buffer.empty()) {
                if (*it == '\n') {
                    line++;
                    _char = 0;
                    if (!buffer.empty()) {
                        m_tokens.push_back(getToken(buffer, line, _char));
                    }
                    buffer = "";
                    continue;
                }
            }

            // handle token when space is encountered
            else if (std::isspace(*it)) {
                m_tokens.push_back(getToken(buffer, line, _char));
                buffer = "";

            }

            // handle single unique characters that may not have a space before them
            else if (*it == ';' || *it == '{' || *it == '}' || *it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '"' || *it == '"' || *it == '\'' || *it == '=' || *it == '.' || *it == ',' || *it == '<' || *it == '>' || *it == '.' || *it == ',' || *it == '+' || *it == '-' || *it == '*' || *it == '/' || *it == '|' || *it == '%' || *it == '&' || *it == '^') {


                if (buffer == "") {
                    // handle double characters
                    if ((it + 1 < m_content.end()) && (
                            (*it == '-' && *(it + 1) == '>') ||     // handle `->`
                            (*it == '<' && *(it + 1) == '-') ||     // handle `<-`
                            (*it == '/' && *(it + 1) == '/') ||     // handle `//`
                            (*it == '/' && *(it + 1) == '/') ||     // handle `//`
                            (*it == '/' && *(it + 1) == '*') ||     // handle `/*`
                            (*it == '>' && *(it + 1) == '=') ||     // handle `>=`
                            (*it == '<' && *(it + 1) == '=') ||     // handle `<=`
                            (*it == '<' && *(it + 1) == '<') ||     // handle `<<`
                            (*it == '>' && *(it + 1) == '>') ||     // handle `>>`
                            (*it == '.' && *(it + 1) == '.') ||     // handle `..`
                            (*it == '*' && *(it + 1) == '*') ||     // handle `**`
                            (*it == '!' && *(it + 1) == '=') ||     // handle `!=`
                            (*it == '|' && *(it + 1) == '|') ||     // handle `||`
                            (*it == '=' && *(it + 1) == '=')        // handle `==`
                    )) {
                        // handle inline comments
                        if (*it == '/' && it + 1 < m_content.end() && *(it + 1) == '/') {
                            while (*it != '\n') {
                                buffer += *it;
                                _char++;
                                it++;
                            }
                        } 

                        // handle multi-line comments
                        else if (*it == '/' && *(it + 1) == '*') {
                            buffer += "/*";
                            _char += 2;
                            it += 2;

                            while (*it != '*' || it + 1 < m_content.end() && *(it + 1) != '/') {
                                if (*it == '\n') line++;
                                else _char++;
                                buffer += *it;
                                _char++;
                                it++;
                            }

                            if (*it == '*' && it + 1 < m_content.end() && *(it + 1) == '/') {
                                buffer += *it;
                                _char++;
                                it++;
                                buffer += *it;
                            }
                        } 
                        
                        else {
                            if (*it == '\n') line++;
                            else _char++;

                            buffer += *it;
                            buffer += *(it + 1);
                            it++;
                        }
                    } 

                    else {
                        buffer = *it;
                    }
                    

                    Token token = getToken(buffer, line, _char);
                    
                    // handle string
                    if (token.getTokenType() == TokenType::_dbl_quote) {
                        printDebug("Handing string");
                        buffer = "";

                        it++;
                        _char++;
                        while (it < m_content.end() && *it != '"') {
                            printDebug(buffer);
                            if (*it == '\\' && it + 1 < m_content.end()) {
                                printDebug("Found an escape character");
                                printDebug(std::string(1, *(it + 1)));

                                if (*(it + 1) == '0') {
                                    buffer += '\0';

                                } else if (*(it + 1) == 'a') {
                                    buffer += '\a';

                                } else if (*(it + 1) == 'b') {
                                    buffer += '\b';

                                } else if (*(it + 1) == 't') {
                                    buffer += '\t';
                                
                                } else if (*(it + 1) == 'n') {
                                    buffer += '\n';

                                } else if (*(it + 1) == 'r') {
                                    buffer += '\r';
                                
                                } else if (*(it + 1) == '"') {
                                    buffer += "\"";

                                } else if (*(it + 1) == '\'') {
                                    buffer += "'";

                                } else if (*(it + 1) == '\\') {
                                    buffer += '\\';
                                }

                                it += 2;
                                _char += 2;

                            } else {
                                buffer += *it;
    
                                if (*it == '\n') line++;
    
                                it++;
                                _char++;
                            }
                        }

                        m_tokens.push_back(Token(TokenType::_text, "`string`: \"" + buffer + "\"", line, _char));
                        buffer = (*it);
                        printDebug(buffer);
                        token = getToken(buffer, line, _char);
                        if (token.getTokenType() != TokenType::_dbl_quote) {
                            printError("Expected `\"`", token.getLine(), token.getChar());
                        }

                        buffer = "";
                    }

                    else if (token.getTokenType() == TokenType::_sgl_quote) {
                        buffer = "";
                        it++;
                        _char++;

                        if (*it == '\\' && it + 1 < m_content.end()) {
                            printDebug("Found an escape character");
                            printDebug(std::string(1, *(it + 1)));

                            if (*(it + 1) == '0') {
                                buffer += '\0';

                            } else if (*(it + 1) == 'a') {
                                buffer += '\a';

                            } else if (*(it + 1) == 'b') {
                                buffer += '\b';

                            } else if (*(it + 1) == 't') {
                                buffer += '\t';
                            
                            } else if (*(it + 1) == 'n') {
                                buffer += '\n';

                            } else if (*(it + 1) == 'r') {
                                buffer += '\r';
                            
                            } else if (*(it + 1) == '"') {
                                buffer += "\"";

                            } else if (*(it + 1) == '\'') {
                                buffer += "'";

                            } else if (*(it + 1) == '\\') {
                                buffer += '\\';
                            }

                            
                        } else {
                            buffer += *it;
                        }
                        it++;
                        _char += 1;

                        if (token.getTokenType() != TokenType::_sgl_quote) {
                            printError("Exected `'`", line, _char);
                        } else {
                            m_tokens.push_back(Token(TokenType::_char_lit, "`character`: '" + buffer + "'", line, _char));
                        }
                    }

                    // skip comments
                    else if (token.getTokenType() != TokenType::_comment) m_tokens.push_back(token);
                    

                    buffer = "";

                } 
                
                // handle .
                else {
                    if (isNumber(buffer) && *it == '.') {
                        buffer += '.';
                        if (!(isNumber(buffer))) {
                            if (buffer.back() != '.' || (buffer.size() > 1 && buffer[buffer.size() - 2] != '.')) {
                                printError("Invalid number", line, _char);
                            }
                        }
                    } else {
                        m_tokens.push_back(getToken(buffer, line, _char));
                        buffer = "";
                        it--;
                        _char--;
                    }
                }
            }

            else {
                buffer += (*it);
            }

            _char++;
        }
        
        if (!buffer.empty()) {
            m_tokens.push_back(getToken(buffer, line, _char));
            buffer = "";
        }

        return m_tokens;
    }

    void print_tokens() {
        printDebug("tokens array size " + std::to_string(m_tokens.size()));
        for (auto it = m_tokens.begin(); it < m_tokens.end(); it++) {
            printDebug( "::" + (*it).getStrValue());
        }
        return;
    }
};
