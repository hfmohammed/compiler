#pragma once
#include <variant>
#include <cassert>
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

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMul {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMul*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
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
    std::variant<NodeStmtExit*,NodeStmtLet*,> var;
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
            } else if (auto open_paren = try_consume(TokenType::open_paren)) {
                auto expr = parse_expr();
                if (!expr.has_value()) {
                    std::cerr << "Expected expression" << std::endl;
                    exit(EXIT_FAILURE);
                }

                try_consume(TokenType::closed_paren, "Expected ')'");
                auto term_paren = m_allocator.alloc<NodeTermParen>();
                term_paren->expr = expr.value();

                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_paren;

                return term;
            } else {
                return {};
            }
        }

        inline std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
            std::optional<NodeTerm*> term_lhs = parse_term();
            if (!term_lhs.has_value()) {
                return {};
            }

            auto expr_lhs = m_allocator.alloc<NodeExpr>();
            expr_lhs->var = term_lhs.value();

            while (true) {
                std::optional<Token> curr_tok = peek();
                std::optional<int> prec;
                if (curr_tok.has_value()) {
                    prec = bin_prec(curr_tok->type);
                    if (!prec.has_value() || prec < min_prec) {
                        break;
                    }
                } else {
                    break;
                }

                Token op = consume();
                int next_min_prec = prec.value() + 1;
                auto expr_rhs = parse_expr(next_min_prec);

                if (!expr_rhs.has_value()) {
                    std::cerr << "Unable to parse expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                
                auto expr = m_allocator.alloc<NodeBinExpr>();
                auto expr_2 = m_allocator.alloc<NodeExpr>();

                if (op.type == TokenType::plus) {
                    auto add = m_allocator.alloc<NodeBinExprAdd>();
                    expr_2->var = expr_lhs->var;

                    add->lhs = expr_2;
                    add->rhs = expr_rhs.value();
                    expr->var = add;

                } else if (op.type == TokenType::minus) {
                    auto sub = m_allocator.alloc<NodeBinExprSub>();
                    expr_2->var = expr_lhs->var;

                    sub->lhs = expr_2;
                    sub->rhs = expr_rhs.value();
                    expr->var = sub;

                } else if (op.type == TokenType::star) {
                    auto multi = m_allocator.alloc<NodeBinExprMul>();
                    expr_2->var = expr_lhs->var;

                    multi->lhs = expr_2;
                    multi->rhs = expr_rhs.value();
                    expr->var = multi;

                } else if (op.type == TokenType::div) {
                    auto div = m_allocator.alloc<NodeBinExprDiv>();
                    expr_2->var = expr_lhs->var;

                    div->lhs = expr_2;
                    div->rhs = expr_rhs.value();
                    expr->var = div;

                } else {
                    assert(false);
                }
                expr_lhs->var = expr;
            }

            return expr_lhs;
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