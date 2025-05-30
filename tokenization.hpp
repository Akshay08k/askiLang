#pragma once
#include <string>
#include <vector>
#include <optional>
enum class TokenType
{
    exit,
    int_lit,
    semi,
    open_paran,
    close_paran,
    ident,
    let,
    eq,
    plus,
    star,
    minus,
    fslash,
    open_curly,
    close_curly,
    if_,
    elif,
    else_
};

inline std::optional<int> binExpr_prec(const TokenType type)
{
    switch (type)
    {
    case TokenType::star:
    case TokenType::fslash:
        return 1;
    case TokenType::plus:
    case TokenType::minus:
        return 0;
    default:
        return {};
    }
}

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

class Tokenizer
{
public:
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    // Tokenize function which is used convert the code into tokenize and
    // Extract the tokens from it
    inline std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;
        std::string buf;
        while (peek().has_value())
        {
            // cant able to use switch statement here
            // because switch requires constant expression to be
            // evaluated
            if (std::isalpha(peek().value()))
            {
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value()))
                {
                    buf.push_back(consume());
                }
                if (buf == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear();
                }
                else if (buf == "let")
                {
                    tokens.push_back({.type = TokenType::let});
                    buf.clear();
                }
                else if (buf == "if")
                {
                    tokens.push_back({.type = TokenType::if_});
                    buf.clear();
                }
                else if (buf == "elif") {
                    tokens.push_back({.type = TokenType::elif});
                    buf.clear();
                }else if (buf == "else") {
                    tokens.push_back({.type = TokenType::else_});
                    buf.clear();
                }
                // ident can be any so don't want to apply if else
                else
                {
                    tokens.push_back({.type = TokenType::ident, .value = buf});
                    buf.clear();
                }
            }
            else if (std::isdigit(peek().value()))
            {
                buf.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value()))
                {
                    buf.push_back(consume());
                }
                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                consume();
                consume();
                while (peek().has_value() && peek(1).value() != '\n') {
                    consume();
                }
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                consume();
                consume();
                while (peek().has_value() ) {
                    if ( peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                        break;
                    }
                    consume();
                }
                if (peek().has_value()) {
                    consume();
                }
                if (peek().has_value()) {
                    consume();
                }
            }
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paran});
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paran});
            }
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
            }
            else if (std::isspace(peek().value()))
            {
                consume();
            }
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq});
            }
            else if (peek().value() == '+')
            {
                consume();
                tokens.push_back({.type = TokenType::plus});
            }
            else if (peek().value() == '*')
            {
                consume();
                tokens.push_back({.type = TokenType::star});
            }
            else if (peek().value() == '-')
            {
                consume();
                tokens.push_back({.type = TokenType::minus});
            }
            else if (peek().value() == '/')
            {
                consume();
                tokens.push_back({.type = TokenType::fslash});
            }
            else if (peek().value() == '{')
            {
                consume();
                tokens.push_back({.type = TokenType::open_curly});
            }
            else if (peek().value() == '}')
            {
                consume();
                tokens.push_back({.type = TokenType::close_curly});
            }
            else
            {
                std::cerr << "You messed up! Unexpected character: '" << peek().value() << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index = 0;
        return tokens;
    }

private:
    [[nodiscard]] inline std::optional<char> peek(const int offset = 0) const
    {
        if (m_index + offset >= m_src.length())
        {
            return {};
        }
        else
        {
            return m_src.at(m_index + offset);
        }
    }
    inline char consume()
    {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index = 0;
};