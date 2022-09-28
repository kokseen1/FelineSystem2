#include <iostream>

#include <parser.hpp>

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
void Lexer::advance()
{
    if (current_token != Token::Eof)
    {
        current_token = get_token();
    }
}

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
            return result == add_expr();
        }
        double res = add_expr();
        std::cout << "SETVAR #" << last_var_name << "="
                  << res << std::endl;
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

    p_lexer = NULL;
    return result;
}