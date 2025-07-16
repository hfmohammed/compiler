#pragma once
#include <variant>

#include "tokenization.hpp"
#include "./arena.hpp"

/*
    NodeTermIntLit => 23
    NodeTermIdent => variable
    NodeExpr => integer or variable
    NodeStmtExit => exit(x) or exit(2)
    NodeStmtLet => let x = y or let x = 2
    NodeStmt => exit statement or let statement
    NodeProg => list of statements
*/

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMul {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    NodeBinExprAdd* add;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit*,NodeStmtLet*> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens))
            , m_allocator(1024*1024*4)
        {

        }

        std::optional<NodeTerm*> parse_term() {
            if (auto int_lit = try_consume(TokenType::int_lit)) {
                auto node_term_int_lit = m_allocator.alloc<NodeTermIntLit>();
                node_term_int_lit->int_lit = int_lit.value();

                auto term = m_allocator.alloc<NodeTerm>();
                term->var = node_term_int_lit;

                return term;

            } else if (auto ident = try_consume(TokenType::ident)) {
                auto node_term_ident = m_allocator.alloc<NodeTermIdent>();
                node_term_ident->ident = ident.value();

                auto term = m_allocator.alloc<NodeTerm>();
                term->var = node_term_ident;

                return term;
            } else {
                return {};
            }
        }

        inline std::optional<NodeExpr*> parse_expr() {
            if (auto term = parse_term()) {
                if (try_consume(TokenType::plus).has_value()) {
                    auto bin_expr = m_allocator.alloc<NodeBinExpr>();    
                    auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
                    auto lhs_expr = m_allocator.alloc<NodeExpr>();

                    lhs_expr->var = term.value();
                    bin_expr_add->lhs = lhs_expr;

                    if (auto rhs = parse_expr()) {
                        bin_expr_add->rhs = rhs.value();
                        bin_expr->add = bin_expr_add;

                        auto expr = m_allocator.alloc<NodeExpr>();
                        expr->var = bin_expr;
                        return expr;

                    } else {
                        std::cerr << "Expected expression" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                } else {
                    auto expr = m_allocator.alloc<NodeExpr>();
                    expr->var = term.value();
                    return expr;
                }
            } else {
                return {};
            }
        }


        inline std::optional<NodeStmt*> parse_stmt() {

            if (peek().value().type == TokenType::_exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {

                consume();
                consume();

                auto node_stmt_exit = m_allocator.alloc<NodeStmtExit>();
                if (auto node_expr = parse_expr()) {
                    node_stmt_exit->expr = node_expr.value();
                } else {
                    std::cerr << "Invalid expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                try_consume(TokenType::closed_paren, "Expected a closing parenthesis ')'");
                try_consume(TokenType::semi, "Expected a semi colon ';'");

                auto node_stmt = m_allocator.alloc<NodeStmt>();
                node_stmt->var = node_stmt_exit;
                return node_stmt;

            } else if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() && peek(1).value().type == TokenType::ident && peek(2).has_value() && peek(2).value().type == TokenType::eq) {
                consume();

                auto node_stmt_let = m_allocator.alloc<NodeStmtLet>();
                node_stmt_let->ident = consume();

                consume();
                if (auto expr = parse_expr()) {
                    node_stmt_let->expr = expr.value();
                } else {
                    std::cerr << "expected an expression after ('=')" << std::endl;
                    exit(EXIT_FAILURE);
                }

                try_consume(TokenType::semi, "Expected a semi colon ';'");
                
                auto node_stmt = m_allocator.alloc<NodeStmt>();
                node_stmt->var = node_stmt_let;
                return node_stmt;

            } else {
                std::cout << std::to_string(static_cast<int>(peek().value().type)) << std::endl;
                return {};

            }
        }

        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(*(stmt.value()));

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

        inline Token try_consume(TokenType type, const std::string& err_msg) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                std::cerr << err_msg << std::endl;
                exit(EXIT_FAILURE);
            }

        }

        inline std::optional<Token> try_consume(TokenType type) {
            if (peek().has_value() && peek().value().type == type) {
                return consume();
            } else {
                return {};
            }

        }


        inline Token consume() {
            return m_tokens.at(m_pos++);
        }
        
        const std::vector<Token> m_tokens;
        size_t m_pos = 0;
        ArenaAllocator m_allocator;
};