#pragma once
#include <string>

enum class TokenType {
    _exit,
    int_lit,
    semi,
    open_paren,
    closed_paren,
    ident,
    let,
    eq,
    plus,
    multi,
};

struct Token {
    TokenType type;
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

            while (peek().has_value()) {

                if (std::isalpha(peek().value())) {
                    buffer.push_back(consume());

                    while (peek().has_value() && std::isalnum(peek().value())) {
                        buffer.push_back(consume());
                    }

                    if (buffer == "exit") {
                        tokens.push_back(Token{ .type = TokenType::_exit });
                        buffer.clear();

                    } else if (buffer == "let") {
                        tokens.push_back(Token{ .type = TokenType::let });
                        buffer.clear();

                    } else {
                        tokens.push_back(Token{ .type = TokenType::ident, .value = buffer });
                        buffer.clear();

                    }

                } else if (peek().has_value() && std::isdigit(peek().value())) {
                    while (peek().has_value() && std::isdigit(peek().value())) {
                        buffer.push_back(consume());
                    }

                    int _value = stoi(buffer);
                    tokens.push_back(Token{ .type = TokenType::int_lit, .value = buffer });
                    buffer.clear();

                } else if (peek().has_value() && peek().value() == '(') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::open_paren });
                    
                } else if (peek().has_value() && peek().value() == ')') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::closed_paren });
                    
                } else if (peek().has_value() && peek().value() == '=') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::eq });

                } else if (peek().has_value() && peek().value() == '+') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::plus });

                } else if (peek().has_value() && peek().value() == '-') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::multi });

                } else if (peek().has_value() && peek().value() == ';') {
                    consume();
                    tokens.push_back(Token{ .type = TokenType::semi });

                } else if (peek().has_value() && std::isspace(peek().value())) {
                    consume();

                }

                else {
                    std::cerr << "Error" << std::endl;
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
