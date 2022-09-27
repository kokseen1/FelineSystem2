#include <sstream>
#include <string>
#include <iostream>
#include <map>

std::map<std::string, double> symbol_table;

enum class Token
{
    Id,
    Number,
    Assign,
    Plus,
    Minus,
    Mul,
    Div,
    Eof
};

class Lexer
{

public:
    Lexer(const std::string);
    Token get_current_token();

private:
    std::istringstream iss;
    Token current_token;

    Token get_token();
};

Lexer::Lexer(const std::string s) : iss{s}
{
    current_token = get_token();
};

Token Lexer::get_token()
{
    char c;

    while (isspace(c))
        iss.get(c);

    if (!iss.get(c))
        return Token::Eof;

    if (c == '#')
    {
        return Token::Id;
    }

    return Token::Plus;
}

Token Lexer::get_current_token()
{
    return current_token;
}

class Parser
{
public:
    double operator()(const std::string &);

private:
    Lexer *p_lexer = NULL;

    double assign_expr();
    double add_expr();
    double mul_expr();
    double pow_expr();
    double unary_expr();
    double primary();
};

double Parser::assign_expr()
{
    Token t = p_lexer->get_current_token();
    std::cout << static_cast<int>(t);
}

double Parser::operator()(const std::string &s)
{
    Lexer lexer = Lexer{s};
    p_lexer = &lexer;

    double result = assign_expr();

    p_lexer = NULL;
    return result;
}

int main()
{
    std::string s = "#1";

    Parser parser;
    parser(s);

    return 0;
}