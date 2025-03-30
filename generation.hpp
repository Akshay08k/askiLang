#pragma once
#include "./parser.hpp"
#include <unordered_map>
class Generator
{
public:
    inline explicit Generator(NodeProg prog)

        : m_prog(std::move(prog))
    {
    }

    void gen_expr(const NodeExpr &expr)
    {
        struct ExprVisitor
        {
            Generator *gen;

            void operator()(const NodeExprIntLit &expr_int_lit) const
            {
                gen->m_output << "    mov rax," << expr_int_lit.int_lit.value.value() << "\n";
                gen->push("rax");
            };
            void operator()(const NodeExprIdent &expr_ident) const
            {
                // first check if the variable is in the map
                if (!gen->m_vars.contains(expr_ident.ident.value.value()))
                {
                    std::cerr << "Identifier " << expr_ident.ident.value.value() << " does not exist" << std::endl;
                    exit(EXIT_FAILURE);
                }

                const auto &var = gen->m_vars.at(expr_ident.ident.value.value());
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
        };
        ExprVisitor visitor{.gen = this};
        std::visit(visitor, expr.var);
    }

    void gen_stmt(const NodeStmt &stmt)
    {
        struct StmtVisitor
        {
            Generator *gen;
            void operator()(const NodeStmtExit &stmt_exit) const
            {

                gen->gen_expr(stmt_exit.expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            };
            void operator()(const NodeStmtLet &stmt_let) const
            {
                if (gen->m_vars.contains(stmt_let.ident.value.value()))
                {
                    std::cerr << "Identifier " << stmt_let.ident.value.value() << " already exists" << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.insert({stmt_let.ident.value.value(), Var{.stack_loc = gen->m_stack_size}});
                gen->gen_expr(stmt_let.expr);
            }
        };

        StmtVisitor visitor{.gen = this};
        std::visit(visitor, stmt.var);
    }

    [[nodiscard]] std::string
    gen_prog()
    {

        m_output << "global _start\n_start:\n";

        for (const NodeStmt &stmt : m_prog.stmts)
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
    void push(const std::string &reg)
    {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    void pop(const std::string &reg)
    {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    // this is struct that holds the location of the variable
    // in future we will do add a types of this variable
    // so we can do type checking
    struct Var
    {
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::unordered_map<std::string, Var> m_vars{};
};