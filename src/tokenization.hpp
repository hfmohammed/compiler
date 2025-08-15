#pragma once
#include <string>
#include <cassert>

enum class TokenType {
    _exit,
    int_lit,
    _string,
    semi,
    open_paren,
    closed_paren,
    ident,
    let,
    eq,
    plus,
    star,
    minus,
    fslash,
    open_curly,
    closed_curly,
    _if,
    elif,
    _else,
};

inline std::string to_string(const TokenType type) {
    switch (type) {
    case TokenType::_exit:
        return "`exit`";

    case TokenType::int_lit:
        return "int literal";

    case TokenType::semi:
        return "`;`";

    case TokenType::open_paren:
        return "`(`";

    case TokenType::closed_paren:
        return "`)`";

    case TokenType::ident:
        return "identifier";

    case TokenType::let:
        return "`let`";

    case TokenType::eq:
        return "`=`";

    case TokenType::plus:
        return "`+`";

    case TokenType::star:
        return "`*`";

    case TokenType::minus:
        return "`-`";

    case TokenType::fslash:
        return "`/`";

    case TokenType::open_curly:
        return "`{`";

    case TokenType::closed_curly:
        return "`}`";

    case TokenType::_if:
        return "`if`";

    case TokenType::elif:
        return "`elif`";

    case TokenType::_else:
        return "`else`";

    }

    assert(false);
}

std::optional<int> bin_prec(TokenType type) {
    switch (type) {
    case TokenType::plus:
    case TokenType::minus:
        return 0;

    case TokenType::star:
    case TokenType::fslash:
        return 1;

    default:
        return {};
    }
}

struct Token {
    TokenType type;
    int line;
    std::optional<std::string> value;

};

class Tokenizer {
    public:
        inline Tokenizer(std::string& src)
            : m_src(std::move(src))
        {
            
        }


        inline std::vector<Token> tokenize() {
            std::vector<Token> tokens;
            std::string buffer;
            int line_count = 1;

            while (peek().has_value()) {
                if (std::isalpha(peek().value())) {
                    buffer.push_back(consume());

                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buffer.push_back(consume());
                    }

                    if (buffer == "exit") {
                        tokens.push_back(Token{ .type = TokenType::_exit, .line = line_count });
                        buffer.clear();

                    } else if (buffer == "let") {
                        tokens.push_back(Token{ .type = TokenType::let, .line = line_count });
                        buffer.clear();

                    } else if (buffer == "if") {
                        tokens.push_back(Token{ .type = TokenType::_if, .line = line_count });
                        buffer.clear();

                    } else if (buffer == "elif") {
                        tokens.push_back(Token{ .type = TokenType::elif, .line = line_count });
                        buffer.clear();
                    
                    } else if (buffer == "else") {
                        tokens.push_back(Token{ .type = TokenType::_else, .line = line_count });
                        buffer.clear();

                    } else {
                        tokens.push_back(Token{ .type = TokenType::ident, .line = line_count, .value = buffer });
                        buffer.clear();

                    }

                } else if (peek().has_value() && std::isdigit(peek().value()) || (peek().has_value() && peek().value() == '-' && peek(1).has_value() && std::isdigit(peek(1).value()))) {
                    if (peek().has_value() && peek().value() == '-') {
                        buffer.push_back('-');
                        consume();
                    }

                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buffer.push_back(consume());
                    }

                    int _value = stoi(buffer);
                    tokens.push_back(Token{ .type = TokenType::int_lit, .line = line_count, .value = buffer });
                    buffer.clear();

                } else if (peek().has_value() && peek().value() == '"') {
                    consume();
                    while (peek().has_value() && peek().value() != '"') {
                        buffer.push_back(consume());
                    }
                    consume();

                    tokens.push_back(Token{ .type = TokenType::_string, .line = line_count, .value = buffer });
                    buffer.clear();

                } else if (peek().has_value() && peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                    consume();
                    consume();
                    while (peek().has_value() && peek().value() != '\n') {
                        consume();
                    }

                } else if (peek().has_value() && peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                    while (peek().has_value()) {
                        if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                            break;
                        }
                        
                        std::cout << "consume" << std::endl;
                        std::cout << consume() << std::endl;
                    }

                    if (peek().has_value()) {
                        consume();
                    }

                    if (peek().has_value()) {
                        consume();
                    }

                } else if (peek().has_value() && peek().value() == '(') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::open_paren, .line = line_count });
                    
                } else if (peek().has_value() && peek().value() == ')') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::closed_paren, .line = line_count });
                    
                } else if (peek().has_value() && peek().value() == '=') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::eq, .line = line_count });

                } else if (peek().has_value() && peek().value() == '+') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::plus, .line = line_count });

                } else if (peek().has_value() && peek().value() == '-') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::minus, .line = line_count });

                } else if (peek().has_value() && peek().value() == '*') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::star, .line = line_count });

                } else if (peek().has_value() && peek().value() == '/') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::fslash, .line = line_count });

                } else if (peek().has_value() && peek().value() == ';') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::semi, .line = line_count });

                } else if (peek().has_value() && peek().value() == '{') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::open_curly, .line = line_count });

                } else if (peek().has_value() && peek().value() == '}') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::closed_curly, .line = line_count });

                } else if (peek().has_value() && peek().value() == '\n') {
                    line_count++;
                    consume();

                } else if (peek().has_value() && std::isspace(peek().value())) {
                    consume();

                } else {
                    std::cerr << "Invalid token" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            std::cout << tokens.size() << std::endl;
            return tokens;
        }



    private:
        [[nodiscard]] inline std::optional<char> peek(int ahead = 0) const {
            if (m_pos + ahead < m_src.length()) {
                return m_src.at(m_pos + ahead);
            } else {
                return {};
            }
        }

        inline char consume() {
            return m_src.at(m_pos++);
        }

        const std::string m_src;
        size_t m_pos = 0;
};
