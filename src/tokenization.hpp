#pragma once
#include <string>
#include <cassert>
#include <vector>

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

    // custom
    _plus,
    _minus,
    _asterisk,
    _div,
    _assign,
    _qualifier,
    _type,
    _identifier,
    _expression,
    _semi,
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
                std::cerr << "Expected `*/ at " << line << ":" << _char << std::endl;
                exit(1);
            }

        }

        else if (content == "and") {
            return Token(TokenType::_and, "`and`", line, _char);
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

        // custom
        else if (content == ";") {
            return Token(TokenType::_semi, "`;`", line, _char);
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
            return Token(TokenType::_plus, "`+`", line, _char);
        }

        else if (content == "-") {
            return Token(TokenType::_minus, "`-`", line, _char);
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

        else if (is_number(content)) {
            return Token(TokenType::_number, content, line, _char);
        }

        else if (is_identifier(content)) {
            return Token(TokenType::_identifier, content, line, _char);
        }

        std::cerr << "Unexpected token at " << line << ":" << _char << std::endl;
        exit(EXIT_FAILURE);
        return Token(TokenType::_text, "`text`: \"" + content + "\"", line, _char);
    }


    /*
        returns 1 if buffer meets the criteria to be an identifier otherwise 0
    */
    bool is_identifier(std::string content) {
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

    int is_number(std::string content) {
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
            if (std::isspace(*it) && buffer.empty()) {
                _char++;
               continue;  // skip if its space and buffer is empty
            }

            // handle token when space is encountered
            else if (std::isspace(*it)) {
                m_tokens.push_back(getToken(buffer, line, _char));
                buffer = "";

            }
            
            // handle escape/new line character
            else if (*it == '\n') {
                line++;
                _char = 0;
                if (buffer != "") {
                    m_tokens.push_back(getToken(buffer, line, _char));
                }
                buffer = "";
            }

            // handle single unique characters that may not have a space before them
            else if (*it == ';' || *it == '{' || *it == '}' || *it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '"' || *it == '"' || *it == '\'' || *it == '=' || *it == '.' || *it == ',' || *it == '<' || *it == '>' || *it == '.' || *it == ',' || *it == '+' || *it == '-' || *it == '*' || *it == '/' || *it == '|' || *it == '%' || *it == '&') {


                if (buffer == "") {

                    if ((it + 1 < m_content.end()) && (
                            (*it == '/' && *(it + 1) == '/') ||     // handle `//`
                            (*it == '/' && *(it + 1) == '*') ||     // handle `/*`
                            (*it == '>' && *(it + 1) == '=') ||     // handle `>=`
                            (*it == '<' && *(it + 1) == '=') ||     // handle `<=`
                            (*it == '<' && *(it + 1) == '<') ||     // handle `<<`
                            (*it == '>' && *(it + 1) == '>') ||     // handle `>>`
                            (*it == '=' && *(it + 1) == '=')        // handle `==`
                    )) {
                        // handle inline comments
                        if (*it == '/' && *(it + 1) == '/') {
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

                            while (*it != '*' && it + 1 < m_content.end() && *(it + 1) != '/') {
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
                        buffer = "";

                        it++;
                        _char++;
                        while (*it != '"') {
                            buffer += *it;

                            if (*it == '\n') line++;

                            it++;
                            _char++;
                        }

                        m_tokens.push_back(Token(TokenType::_string, "`string`: \"" + buffer + "\"", line, _char));

                        buffer = (*it);
                        token = getToken(buffer, line, _char);
                        m_tokens.push_back(token);
                        buffer = "";
                    }

                    // skip comments
                    else if (token.getTokenType() != TokenType::_comment) m_tokens.push_back(token);

                    buffer = "";

                } 
                
                // handle 
                else {
                    m_tokens.push_back(getToken(buffer, line, _char));
                    buffer = "";
                    it--;
                    _char--;
                }
            }

            else {
                buffer += (*it);
            }

            _char++;
        }

        m_tokens.push_back(getToken(buffer, line, _char));
        buffer = "";
        return m_tokens;
    }

    void print_tokens() {
        std::cout << "tokens array size " << m_tokens.size() << std::endl;
        for (auto it = m_tokens.begin(); it < m_tokens.end(); it++) {
            std::cout << (*it).getStrValue() << std::endl;
        }
        return;
    }
};
