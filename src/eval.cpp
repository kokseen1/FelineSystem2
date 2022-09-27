#include <sstream>
#include <string>
#include <iostream>
#include <map>

enum class Token
{
    Number,
    Id = '#',
    Assign = '=',
    Plus = '+',
    Minus = '-',
    Mul = '*',
    Div = '/',
    Lp = '(',
    Rp = ')',
    Eof = -1,
};

class Lexer
{

private:
    Token current_token;
    std::string buffer;

    Token get_token();

public:
    std::istringstream iss;
    Lexer(const std::string);
    Token get_current_token() { return current_token; };
    std::string get_curr_buffer() { return buffer; };
    void advance();
};

void Lexer::advance()
{
    if (current_token != Token::Eof)
    {
        current_token = get_token();
    }
}

Lexer::Lexer(const std::string s) : iss{s}
{
    current_token = get_token();
}

Token Lexer::get_token()
{
    char c;

    // Attempt to get a char
    if (!iss.get(c))
        return Token::Eof;

    // Skip until non-whitespace or eof
    while (isspace(c))
    {
        // std::cout << "Space" << std::endl;
        if (!iss.get(c))
            return Token::Eof;
    }

    switch (c)
    {
    case '#':
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
        return Token(c);
    }

    // Tokens that utilize the buffer
    buffer.clear();

    if (isdigit(c))
    {
        while (isdigit(c))
        {
            buffer += c;
            if (!iss.get(c))
                return Token::Number;
        }
        // Put back c if loop exits as it is not a digit
        iss.putback(c);
        return Token::Number;
    }

    throw std::runtime_error("Invalid token");
}

class Parser
{
public:
    double operator()(std::string &);

private:
    std::string last_var_name; // TODO: Might have better alternative
    Lexer *p_lexer = NULL;
    std::map<std::string, double> symbol_table{};

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
    double result = add_expr();

    if (t == Token::Id && p_lexer->get_current_token() == Token::Assign)
    {
        p_lexer->advance();
        if (p_lexer->get_current_token() == Token::Assign)
        {
            p_lexer->advance();
            std::cout << "equality" << result << std::endl;
            return result == add_expr();
        }
        double res = add_expr();
        std::cout << "Var '" << last_var_name << "' = "
                  << " " << res << std::endl;
        return symbol_table[last_var_name] = res;
    }

    return result;
}

double Parser::primary()
{
    std::string text = p_lexer->get_curr_buffer();
    // std::cout << "Buf: " << text << std::endl;
    double arg;
    std::string var_name;

    switch (p_lexer->get_current_token())
    {
    case Token::Number:
        // std::cout << "NUM" << std::endl;
        p_lexer->advance();
        return std::stoi(text);
    case Token::Id:
        p_lexer->advance();
        last_var_name = var_name = std::to_string(static_cast<int>(primary()));
        return symbol_table[var_name];
        break;
    case Token::Lp:
        p_lexer->advance();
        arg = add_expr();
        if (p_lexer->get_current_token() != Token::Rp)
            throw std::runtime_error("No closing parentheses!");
        p_lexer->advance();
        return arg;

    default:
        std::cout << "TOKEN: " << static_cast<int>(p_lexer->get_current_token()) << std::endl;
        throw std::runtime_error("Invalid primary value! (likely EOF)");
    }
}

double Parser::unary_expr()
{
    switch (p_lexer->get_current_token())
    {
    case Token::Plus:
        p_lexer->advance();
        return +primary();
    case Token::Minus:
        p_lexer->advance();
        return -primary();
    default:
        return primary();
    }
}

double Parser::mul_expr()
{
    double result = unary_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case (Token::Mul):
            // std::cout << "MUL" << std::endl;
            p_lexer->advance();
            result *= unary_expr();
            break;
        case (Token::Div):
            // std::cout << "DIV" << std::endl;
            p_lexer->advance();
            result /= unary_expr();
            break;
        default:
            return result;
        }
    }
}

double Parser::add_expr()
{
    double result = mul_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Plus:
            p_lexer->advance();
            result += mul_expr();
            break;
        case Token::Minus:
            p_lexer->advance();
            result -= mul_expr();
            break;
        default:
            return result;
        }
    }
}

// Functor to evaluate a single string
double Parser::operator()(std::string &s)
{
    // Initialize the lexer with the string
    Lexer lexer = Lexer{s};
    p_lexer = &lexer;

    double result = assign_expr();
    std::cout << result << std::endl;

    p_lexer = NULL;
    return result;
}

int main()
{
    std::string d = "#(955+3)=576";
    std::string s = "#(955+3)";

    Parser parser;
    parser(d);
    parser(s);

    return 0;
}
