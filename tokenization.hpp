#pragma once
#include <string>
#include <vector>
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
    star, // multiplication
    sub,
    div
};

std::optional<int> binExpr_prec(TokenType type)
{
    switch (type)
    {
    case TokenType::star:
    case TokenType::div:
        return 1;
    case TokenType::plus:
    case TokenType::sub:
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
                tokens.push_back({.type = TokenType::sub});
            }
            else if (peek().value() == '/')
            {
                consume();
                tokens.push_back({.type = TokenType::div});
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
    [[nodiscard]] inline std::optional<char> peek(int offset = 0) const
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