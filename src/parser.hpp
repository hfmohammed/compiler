#pragma once
#include "tokenization.hpp"

namespace node {
    struct NodeExpr {
        Token int_lit;
    };
    struct NodeExit {
        NodeExpr expr;
    };
}

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens))
        {

        }

        inline std::optional<node::NodeExpr> parse_expr() {
            if (peek().has_value() && peek().value().type == TokenType::int_lit) {
                return node::NodeExpr{ .int_lit = consume() };
            } else {
                return {};
            }
        }

        std::optional<node::NodeExit> parse() {
            std::optional<node::NodeExit> exit_node;
            while (peek().has_value()) {
                if (peek().value().type == TokenType::_exit) {
                    consume();
                    if (auto node_expr = parse_expr()) {
                        exit_node = node::NodeExit { .expr = node_expr.value() };
                    } else {
                        std::cerr << "Invalid expression" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (peek().has_value() && peek().value().type == TokenType::semi) {
                        consume();
                    } else {
                        std::cerr << "Invalid expression" << std::endl;
                        exit(EXIT_FAILURE);

                    }
                } else {
                    return {};
                }
            }
            return exit_node;
        }

    private:
        [[nodiscard]] inline std::optional<Token> peek(int ahead = 0) const {
            if (m_pos + ahead < m_tokens.size()) {
                return m_tokens.at(m_pos + ahead);
            } else {
                return {};
            }
        }

        inline Token consume() {
            return m_tokens.at(m_pos++);
        }
        
        const std::vector<Token> m_tokens;
        size_t m_pos = 0;
};