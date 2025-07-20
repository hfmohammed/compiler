#pragma once

#include "parser.hpp"
#include <unordered_map>
#include <cassert>

class Generator {
    public:
        inline Generator(NodeProg prog)
            : m_prog(std::move(prog))
        {

        }

        inline void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator* gen;

                void operator()(const NodeTermIntLit* term_int_lit) {
                    gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen->push("rax");
                }

                void operator()(const NodeTermIdent* term_ident) {
                    if (gen->m_vars.find(term_ident->ident.value.value()) == gen->m_vars.end()) {
                        std::cerr << "Undeclared identifier: " << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    const auto& var = gen->m_vars.at(term_ident->ident.value.value());
                    gen->push("QWORD [rsp + " + std::to_string((gen->m_stack_size - var.stack_loc - 1) * 8) + "]\n"); 
                }

                void operator()(const NodeTermParen* term_paren) {
                    gen->gen_expr(term_paren->expr);
                }
            };

            TermVisitor visitor({.gen = this});
            std::visit(visitor, term->var);
        }

        inline void gen_bin_expr(const NodeBinExpr* bin_expr) {
            struct BinExprVisitor {
                Generator* gen;

                void operator()(const NodeBinExprAdd* add) const {
                    gen->gen_expr(add->rhs);
                    gen->gen_expr(add->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->m_output << "    add rax, rbx\n";
                    gen->push("rax");
                }

                void operator()(const NodeBinExprSub* sub) const {
                    gen->gen_expr(sub->rhs);
                    gen->gen_expr(sub->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->m_output << "    sub rax, rbx\n";
                    gen->push("rax");
                }

                void operator()(const NodeBinExprMul* multi) const {
                    gen->gen_expr(multi->rhs);
                    gen->gen_expr(multi->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->m_output << "    mul rbx\n";
                    gen->push("rax");
                }

                void operator()(const NodeBinExprDiv* div) const {
                    gen->gen_expr(div->rhs);
                    gen->gen_expr(div->lhs);
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->m_output << "    div rbx\n";
                    gen->push("rax");
                }

            };

            BinExprVisitor visitor({ .gen = this });
            std::visit(visitor, bin_expr->var);
        }

        inline void gen_expr(const NodeExpr* expr) {
            struct ExprVisitor {
                Generator* gen;

                void operator()(const NodeTerm* term) const {
                    gen->gen_term(term);
                }

                void operator()(const NodeBinExpr* bin_expr) {
                    gen->gen_bin_expr(bin_expr);
                }
            };

            ExprVisitor expr_visitor({ .gen = this });
            std::visit(expr_visitor, expr->var);
        }

        inline void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor {
                Generator* gen;

                void operator()(const NodeStmtExit* stmt_exit) {
                    gen->gen_expr(stmt_exit->expr);

                    gen->m_output << "    mov rax, 60\n";
                    gen->pop("rdi");
                    gen->m_output << "    syscall\n";
                }

                void operator()(const NodeStmtLet* stmt_let) {
                    if (gen->m_vars.find(stmt_let->ident.value.value()) != gen->m_vars.end()) {
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    gen->m_vars.insert({stmt_let->ident.value.value(), Var{ .stack_loc = gen->m_stack_size }});
                    gen->gen_expr(stmt_let->expr);
                }
            };

            StmtVisitor visitor{ .gen = this };
            std::visit(visitor, stmt->var);
        }

        [[nodiscard]] inline std::string gen_prog() {
            m_output << "global _start\n_start:\n";
            
            for (const NodeStmt stmt : m_prog.stmts) {
                gen_stmt(&stmt);
            }


            m_output << "    mov rax, 60\n";
            m_output << "    mov rdi, 0\n";
            m_output << "    syscall";
            return m_output.str();
        }
    private:
        struct Var {
            size_t stack_loc;
        };

        void push(const std::string& reg) {
            m_output << "    push " << reg << "\n";
            m_stack_size++;
        }

        void pop(const std::string& reg) {
            m_output << "    pop " << reg << "\n";
            m_stack_size--;
        }

        const NodeProg m_prog;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::unordered_map<std::string, Var> m_vars {};

};