#pragma once
#include "tokenization.hpp"
#include <variant>

/*
    NodeExprIntLit => 23
    NodeExprIdent => variable
    NodeExpr => integer or variable
    NodeStmtExit => exit(x) or exit(2)
    NodeStmtLet => let x = y or let x = 2
    NodeStmt => exit statement or let statement
    NodeProg => list of statements
*/

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStmtExit {
    NodeExpr expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit,NodeStmtLet> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens))
        {

        }

        inline std::optional<NodeExpr> parse_expr() {
            if (peek().has_value() && peek().value().type == TokenType::int_lit) {
                return NodeExpr{ .var = NodeExprIntLit{ .int_lit = consume() } };

            } else if (peek().has_value() && peek().value().type == TokenType::identifier) {
                return NodeExpr{ .var = NodeExprIdent{ .ident = consume() } };

            } else {
                return {};
            }
        }

        inline std::optional<NodeStmt> parse_stmt() {

            if (peek().value().type == TokenType::_exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {

                consume();
                consume();

                NodeStmtExit stmt_exit;

                if (auto node_expr = parse_expr()) {
                    stmt_exit = NodeStmtExit{ .expr = node_expr.value() };
                } else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (peek().has_value() && peek().value().type == TokenType::closed_paren) {
                    consume();
                } else {
                    std::cerr << "Expected a closing parenthesis (')')" << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (peek().has_value() && peek().value().type == TokenType::semi) {
                    consume();
                } else {
                    std::cerr << "Expected semi-colon (';')" << std::endl;
                    exit(EXIT_FAILURE);

                }

                return NodeStmt{ .var = stmt_exit };

            } else if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() && peek(1).value().type == TokenType::identifier && peek(2).has_value() && peek(2).value().type == TokenType::eq) {
                consume();
                NodeStmtLet stmt_let = NodeStmtLet{ .ident = consume() };

                consume();
                if (auto expr = parse_expr()) {
                    stmt_let.expr = expr.value();
                } else {
                    std::cerr << "expected an expression after ('=')" << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (peek().has_value() && peek().value().type == TokenType::semi) {
                    consume();
                } else {
                    std::cerr << "Expected semi-colon (';')" << std::endl;
                    exit(EXIT_FAILURE);

                }

                return NodeStmt{ .var = stmt_let };

            } else {
                return {};
            }
        }

        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(stmt.value());

                } else {
                    std::cerr << "Invalid statement" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            return prog;
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