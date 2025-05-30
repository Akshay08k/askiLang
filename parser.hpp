#pragma once
#include <variant>
#include <vector>
#include <algorithm>
#include "./tokenization.hpp"
#include "./arena.hpp"

struct NodeTermIntLit
{
    Token int_lit;
};

struct NodeTermIdent
{
    Token ident;
};
struct NodeExpr;

struct NodeTermParen
{
    NodeExpr *expr;
};

struct NodeBinExprAdd
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprMulti
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprSub
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprDiv
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExpr
{
    std::variant<NodeBinExprAdd *, NodeBinExprMulti *, NodeBinExprSub *, NodeBinExprDiv *> var{};
};

struct NodeTerm
{
    std::variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var{};
};
struct NodeExpr
{
    std::variant<NodeTerm *, NodeBinExpr *> var{};
};
struct NodeStmtExit
{
    NodeExpr *expr;
};

struct NodeStmtLet
{
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmt;
struct NodeIfPred;
struct NodeScope
{
    std::vector<NodeStmt *> stmts;
};

struct NodeIfPredElif {
    NodeExpr *expr{};
    NodeScope *scope{};
    std::optional<NodeIfPred*> pred{};

};

struct NodeIfPredElse {
    NodeScope *scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*,NodeIfPredElse*> var;
};
struct NodeStmtIf
{
    NodeExpr *expr;
    NodeScope *scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *> var{};
};

struct NodeProg
{
    std::vector<NodeStmt *> stmts;
};

class Parser
{
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
          m_allocator(1024 * 1024 * 4)
    {
    }

    std::optional<NodeTerm *> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit))
        {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;
            return term;
        }

        if (auto ident = try_consume(TokenType::ident))
        {
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;
            return term;
        }
        if (auto open_paren = try_consume(TokenType::open_paran))
        {
            auto expr = parse_expr();
            if (!expr.has_value())
            {
                std::cerr << "Expected Expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paran, "Expected closing parenthesis");
            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;
            return term;
        }

        return {};
    }

    // Parsing according to precedence in order
    // Basically it treats expression in 3 parts
    // Lhs, Operator, Rhs
    // BinExpr.png
    std::optional<NodeExpr *> parse_expr(int min_precedence = 0)
    {
        // if we dont have lhs than just
        // return null
        std::optional<NodeTerm *> term_lhs = parse_term();
        if (!term_lhs.has_value())
        {
            return {};
        }

        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();

        while (true)
        {
            std::optional<Token> curr_token = peek();
            std::optional<int> prec;
            if (curr_token.has_value())
            {
                prec = binExpr_prec(curr_token->type);
                if (!prec.has_value() || prec < min_precedence)
                {
                    break;
                }
            }
            else
            {
                break;
            }
            const Token op = consume();
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);
            if (!expr_rhs.has_value())
            {
                std::cerr << "Unable to parse expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator.alloc<NodeBinExpr>();
            const auto expr_lhs2 = m_allocator.alloc<NodeExpr>();

            // expr_lhs->var = term_lhs.value();

            if (op.type == TokenType::plus)
            {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            }
            else if (op.type == TokenType::star)
            {
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            }
            else if (op.type == TokenType::minus)
            {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            }
            else if (op.type == TokenType::fslash)
            {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            }
            else
            {
                std::cerr << "Invalid operator" << std::endl;
                exit(EXIT_FAILURE);
            }
            expr_lhs->var = expr;
        }
        return expr_lhs;
    }
    std::optional<NodeScope *> parse_scope()
    {
        if (!try_consume(TokenType::open_curly).has_value())
        {
            return {};
        }

        auto scope = m_allocator.alloc<NodeScope>();
        while (auto stmt = parse_stmt())
        {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly, "Expected `}`");
        return scope;
    }

    std::optional<NodeIfPred*> parse_if_pred()
    {
        if (try_consume(TokenType::elif)) {
            try_consume(TokenType::open_paran, "Expected `(`");

            const auto elif = m_allocator.alloc<NodeIfPredElif>();

            if (const auto expr = parse_expr()) {
                elif->expr = expr.value();
            }
            else {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paran, "Expected `)`");
            if (const auto scope = parse_scope()) {
                elif->scope = scope.value();
            }
            else {
                std::cerr << "Expected scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            elif->pred = parse_if_pred();
            auto pred = m_allocator.emplace<NodeIfPred>(elif);
            return pred;
        }
        if (try_consume(TokenType::else_)) {
            auto else_ = m_allocator.alloc<NodeIfPredElse>();
            if (const auto scope = parse_scope()) {
                else_->scope = scope.value();
            }
            else {
                std::cerr << "Expected scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto pred = m_allocator.emplace<NodeIfPred>(else_);
            return pred;
        }
        return {};
    }


    std::optional<NodeStmt *>
    parse_stmt()
    {
        if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paran)
        {
            consume();
            consume();
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
            if (auto node_expr = parse_expr())
            {
                // error type checking
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paran, "Expected `)`");
            try_consume(TokenType::semi, "Expected ';'");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        else if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() && peek(1).value().type == TokenType::ident && peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume();
            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (auto expr = parse_expr())
            {
                stmt_let->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi, "Expected ';'");
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
        else if (peek().has_value() && peek().value().type == TokenType::open_curly)
        {
            if (auto scope = parse_scope())
            {
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }
            else
            {
                std::cerr << "Invalid Scope" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (auto if_ = try_consume(TokenType::if_))
        {
            try_consume(TokenType::open_paran, "Expected `(`");
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();
            if (auto expr = parse_expr())
            {
                stmt_if->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid Expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paran, "Expected `)`");
            if (auto scope = parse_scope())
            {
                stmt_if->scope = scope.value();
            }
            else
            {
                std::cerr << "Invalid Scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            stmt_if->pred =  parse_if_pred();
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_if;
            return stmt;
        }
        else
        {
            return {};
        }
    }

     std::optional<NodeProg> parseProg()
    {
        NodeProg prog;
        while (peek().has_value())
        {
            if (auto stmt = parse_stmt())
            {
                prog.stmts.push_back(stmt.value());
            }
            else
            {
                std::cerr << "Invalid statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }

private:
    [[nodiscard]] std::optional<Token> peek(const int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }
        else
        {
            return m_tokens.at(m_index + offset);
        }
    }
    Token consume()
    {
        return m_tokens.at(m_index++);
    }

    std::optional<Token> try_consume(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            return {};
        }
    }

    Token try_consume(TokenType type, const std::string &err_msg)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};