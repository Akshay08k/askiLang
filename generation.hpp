#pragma once

#include "./parser.hpp"
#include <cassert>

#include <algorithm>
class Generator
{
public:
    inline explicit Generator(NodeProg prog)
        : m_prog(std::move(prog))
    {
    }

    void gen_term(const NodeTerm *term)
    {
        struct TermVisitor
        {
            Generator &gen;
            void operator()(const NodeTermIntLit *term_int_lit) const
            {
                gen.m_output << "    mov rax," << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            };
            void operator()(const NodeTermIdent *term_ident) const
            {
                auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var)
                    { return var.name == term_ident->ident.value.value(); });
                if (it == gen.m_vars.cend())
                {
                    std::cerr << "Identifier " << term_ident->ident.value.value() << " does not exist" << std::endl;
                    exit(EXIT_FAILURE);
                }

                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]\n";
                gen.push(offset.str());
            }
            void operator()(const NodeTermParen *term_paren) const
            {
                gen.gen_expr(term_paren->expr);
            }
        };

        TermVisitor visitor({.gen = *this});
        std::visit(visitor, term->var);
    }

    void gen_bin_expr(const NodeBinExpr *bin_expr)
    {
        struct BinExprVisitor
        {
            Generator &gen;
            void operator()(const NodeBinExprSub *bin_expr_sub) const
            {
                gen.gen_expr(bin_expr_sub->rhs);
                gen.gen_expr(bin_expr_sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    sub rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprDiv *bin_expr_div) const
            {
                gen.gen_expr(bin_expr_div->rhs);
                gen.gen_expr(bin_expr_div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    div rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprAdd *bin_expr_add) const
            {
                gen.gen_expr(bin_expr_add->rhs);
                gen.gen_expr(bin_expr_add->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprMulti *bin_expr_multi) const
            {
                gen.gen_expr(bin_expr_multi->rhs);
                gen.gen_expr(bin_expr_multi->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    mul rbx\n";
                gen.push("rax");
            }
        };
        BinExprVisitor visitor({.gen = *this});
        std::visit(visitor, bin_expr->var);
    }

    void gen_expr(const NodeExpr *expr)
    {
        struct ExprVisitor
        {
            Generator &gen;
            void operator()(const NodeTerm *term) const
            {
                gen.gen_term(term);
            }
            void operator()(const NodeBinExpr *bin_expr) const
            {
                gen.gen_bin_expr(bin_expr);
            }
        };
        ExprVisitor visitor{.gen = *this};
        std::visit(visitor, expr->var);
    }

    void gen_scope(const NodeScope *scope)
    {
        begin_scope();
        for (const NodeStmt *stmt : scope->stmts)
        {
            gen_stmt(stmt);
        }
        end_scope();
    }

    void gen_if_pred(const NodeIfPred* pred, const std::string& end_label)



    {


        struct PredVisitor {


            Generator& gen;


            const std::string& end_label;





            void operator()(const NodeIfPredElif* elif) const


            {


                gen.gen_expr(elif->expr);


                gen.pop("rax");


                const std::string label = gen.create_label();


                gen.m_output << "    test rax, rax\n";


                gen.m_output << "    jz " << label << "\n";


                gen.gen_scope(elif->scope);


                gen.m_output << "    jmp " << end_label << "\n";


                if (elif->pred.has_value()) {


                    gen.m_output << label << ":\n";


                    gen.gen_if_pred(elif->pred.value(), end_label);


                }


            }

            void operator()(const NodeIfPredElse* else_) const
            {
                gen.gen_scope(else_->scope);
            }
        };
        PredVisitor visitor{ .gen = *this, .end_label = end_label };
        std::visit(visitor, pred->var);
    }
    void gen_stmt(const NodeStmt *stmt)
    {
        struct StmtVisitor
        {
            Generator &gen;
            void operator()(const NodeStmtExit *stmt_exit) const
            {

                gen.gen_expr(stmt_exit->expr);
                gen.m_output << "    mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "    syscall\n";
            };
            void operator()(const NodeStmtLet *stmt_let) const
            {
                auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var)
                    { return var.name == stmt_let->ident.value.value(); });

                // if the variable is not in the vector(MAP)
                // than and only create it
                // otherwise exit with error
                if (it != gen.m_vars.cend())
                {
                    std::cerr << "Identifier " << stmt_let->ident.value.value() << " already exists" << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back({.name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size});
                gen.gen_expr(stmt_let->expr);
            }

            // scope statements
            void operator()(const NodeScope *scope) const
            {
                gen.gen_scope(scope);
            }

            void operator()(const NodeStmtIf *stmt_if) const
            {
                gen.gen_expr(stmt_if->expr);
                gen.pop("rax");
                std::string label = gen.create_label();
                gen.m_output << "    test rax,rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.gen_scope(stmt_if->scope);
                gen.m_output << label << ":\n";
                if (stmt_if->pred.has_value()) {
                    const std::string end_label = gen.create_label();
                    gen.gen_if_pred(stmt_if->pred.value(), end_label);
                    gen.m_output <<  end_label<< ":\n";
                }
            }
        };

        StmtVisitor visitor{.gen = *this};
        std::visit(visitor, stmt->var);
    }

    // Main Program generation template
    [[nodiscard]] std::string
    gen_prog()
    {

        m_output << "global _start\n_start:\n";

        for (const NodeStmt *stmt : m_prog.stmts)
        {
            gen_stmt(stmt);
        }

        // if our program does not have an exit statement than exit with zero
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";
        return m_output.str();
    }

private:
    // Pushing to the stack
    // and incrementing the stack size
    // taking the register name as an argument
    void push(const std::string &reg)
    {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    // Popping from the stack
    // and decrementing the stack size
    // taking the register name as an argument
    void pop(const std::string &reg)
    {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    void begin_scope()
    {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope()
    {
        size_t popCount = m_vars.size() - m_scopes.back();
        // Resetting the stack pointer
        m_output << "    add rsp, " << popCount * 8 << "\n";
        m_stack_size -= popCount;
        for (int i = 0; i < popCount; i++)
        {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    // label is used to create unique labels in assembly
    // it mainly used for if statements
    std::string create_label()
    {
        std::stringstream ss;
        ss << "label" << m_label_count++;
        return ss.str();
    }

    // this is struct that holds the location of the variable
    // in future we will do add a types of this variable
    // so we can do type checking
    struct Var
    {
        std::string name;
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    // vector(MAP) of variables
    std::vector<Var> m_vars{};
    // vector(STACK) of scopes
    std::vector<size_t> m_scopes{};
    int m_label_count = 0;
};