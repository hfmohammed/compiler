#pragma once

#include "parser.hpp"
#include <map>
#include <cassert>
#include <algorithm>

class Generator {
    public:
        inline Generator(NodeProg prog)
            : m_prog(std::move(prog))
        {

        }

        inline void gen_scope(const NodeScope* scope) {
            begin_scope();
            
            for (const NodeStmt* stmt : scope->stmts) {
                gen_stmt(stmt);
            }

            end_scope();
        }

        inline void gen_term(const NodeTerm* term) {
            struct TermVisitor {
                Generator& gen;

                void operator()(const NodeTermIntLit* term_int_lit) {
                    gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                    gen.push("rax");
                }

                void operator()(const NodeTermIdent* term_ident) {
                    auto it = std::find_if(
                            gen.m_vars.cbegin(),
                            gen.m_vars.cend(),
                            [&](const Var& var) { return var.name == term_ident->ident.value.value(); 
                        });

                    if (it == gen.m_vars.cend()) {
                        std::cerr << "Undeclared identifier: " << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    gen.push("QWORD [rsp + " + std::to_string((gen.m_stack_size - (*it).stack_loc - 1) * 8) + "]"); 
                }

                void operator()(const NodeTermParen* term_paren) {
                    gen.gen_expr(term_paren->expr);
                }
            };

            TermVisitor visitor({.gen = *this});
            std::visit(visitor, term->var);
        }

        inline void gen_bin_expr(const NodeBinExpr* bin_expr) {
            struct BinExprVisitor {
                Generator& gen;

                void operator()(const NodeBinExprAdd* add) const {
                    gen.gen_expr(add->rhs);
                    gen.gen_expr(add->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.m_output << "    add rax, rbx\n";
                    gen.push("rax");
                }

                void operator()(const NodeBinExprSub* sub) const {
                    gen.gen_expr(sub->rhs);
                    gen.gen_expr(sub->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.m_output << "    sub rax, rbx\n";
                    gen.push("rax");
                }

                void operator()(const NodeBinExprMul* multi) const {
                    gen.gen_expr(multi->rhs);
                    gen.gen_expr(multi->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.m_output << "    mul rbx\n";
                    gen.push("rax");
                }

                void operator()(const NodeBinExprDiv* div) const {
                    gen.gen_expr(div->rhs);
                    gen.gen_expr(div->lhs);
                    gen.pop("rax");
                    gen.pop("rbx");
                    gen.m_output << "    div rbx\n";
                    gen.push("rax");
                }

            };

            BinExprVisitor visitor({ .gen = *this });
            std::visit(visitor, bin_expr->var);
        }

        inline void gen_expr(const NodeExpr* expr) {
            struct ExprVisitor {
                Generator& gen;

                void operator()(const NodeTerm* term) const {
                    gen.gen_term(term);
                }

                void operator()(const NodeBinExpr* bin_expr) {
                    gen.gen_bin_expr(bin_expr);
                }
            };

            ExprVisitor expr_visitor({ .gen = *this });
            std::visit(expr_visitor, expr->var);
        }

        inline void gen_if_pred(const NodeIfPred* pred, const std::string& end_label) {
            struct PredVisitor {
                Generator& gen;
                const std::string& end_label;

                void operator()(const NodeIfPredElif* elif) const {
                    gen.gen_expr(elif->expr);
                    gen.pop("rax");
                    std::string label = gen.create_label();

                    gen.m_output << "    test rax, rax\n";
                    gen.m_output << "    jz " << label << "\n";
                    
                    gen.gen_scope(elif->scope);
                    gen.m_output << "    jmp " << end_label << "\n";

                    if (elif->pred.has_value()) {
                        gen.m_output << label << ":\n";
                        gen.gen_if_pred(elif->pred.value(), end_label);
                    }
                    
                }

                void operator()(const NodeIfPredElse* _else) const {
                    gen.gen_scope(_else->scope);

                }
            };

            PredVisitor visitor{ .gen = *this, .end_label = end_label };
            std::visit(visitor, pred->var);
        }

        inline void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor {
                Generator& gen;

                void operator()(const NodeStmtExit* stmt_exit) {
                    gen.gen_expr(stmt_exit->expr);

                    gen.m_output << "    mov rax, 60\n";
                    gen.pop("rdi");
                    gen.m_output << "    syscall\n";
                }

                void operator()(const NodeStmtLet* stmt_let) {
                    auto it = std::find_if(
                        gen.m_vars.cbegin(),
                        gen.m_vars.cend(),
                        [&](const Var& var) { 
                            return var.name == stmt_let->ident.value.value(); 
                        }
                    );

                    if (it != gen.m_vars.end()) {
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    Generator::Var var;
                    var.name = stmt_let->ident.value.value();
                    var.stack_loc = gen.m_stack_size;
                    gen.m_vars.push_back(var);
                    gen.gen_expr(stmt_let->expr);
                }

                void operator()(const NodeStmtAssign* stmt_assign) const {
                    const auto it = std::ranges::find_if(gen.m_vars.begin(), gen.m_vars.end(), [stmt_assign](const Var& var) {
                        return var.name == stmt_assign->ident.value.value();
                    });

                    if (it == gen.m_vars.end()) {
                        std::cerr << "Undeclared identifier " << stmt_assign->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    gen.gen_expr(stmt_assign->expr);
                    gen.pop("rax");
                    gen.m_output << "    mov [rsp +" << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
                }

                void operator()(const NodeScope* stmt_scope) {
                    gen.gen_scope(stmt_scope);
                }

                void operator()(const NodeStmtIf* stmt_if) {
                    gen.gen_expr(stmt_if->expr);
                    gen.pop("rax");
                    const std::string label = gen.create_label();

                    gen.m_output << "    test rax, rax\n";
                    gen.m_output << "    jz " << label << "\n";
                    
                    gen.gen_scope(stmt_if->scope);

                    std::optional<std::string> end_label;
                    if (stmt_if->pred.has_value()) {
                        end_label = gen.create_label();
                        gen.m_output << "    jmp " << end_label.value() << "\n";
                    }
                    gen.m_output << label << ":\n";

                    if (stmt_if->pred.has_value()) {
                        gen.gen_if_pred(stmt_if->pred.value(), end_label.value());
                        gen.m_output << end_label.value() << ":\n";
                    }
                    gen.m_output << "    ; if\n";
                }
            };

            StmtVisitor visitor{ .gen = *this };
            std::visit(visitor, stmt->var);
        }

        [[nodiscard]] inline std::string gen_prog() {
            m_output << "global _start\n_start:\n";
            
            for (const NodeStmt stmt : m_prog.stmts) {
                gen_stmt(&stmt);
            }

            m_output << "    mov rax, 60\n";
            m_output << "    mov rdi, 0\n";
            m_output << "    syscall\n";
            return m_output.str();
        }
    private:
        struct Var {
            std::string name;
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

        void begin_scope() {
            m_scopes.push_back(m_vars.size());
        }

        void end_scope() {
            size_t pop_count = m_vars.size() - m_scopes.back();
            m_output << "    sub rsp, " << pop_count * 8 << "\n";
            m_stack_size -= pop_count;
            
            for (int i = 0; i < pop_count; ++i) {
                m_vars.pop_back();
            }

            m_scopes.pop_back();
        }

        std::string create_label() {
            std::stringstream ss;
            ss << "label" << m_label_count++;
            return ss.str();
        }

        const NodeProg m_prog;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        std::vector<Var> m_vars {};
        std::vector<size_t> m_scopes {};
        size_t m_label_count = 0;
};
