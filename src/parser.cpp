#include <iostream>

#include <parser.hpp>

Lexer::Lexer(const std::string s) : iss{s}
{
    current_token = get_next_token();
}

Token Lexer::get_next_token()
{
    char c = EOF;

    // Attempt to get a char
    c = iss.get();

    // Skip until non-whitespace or eof
    while (isspace(c))
        c = iss.get();

    if (c == EOF)
        return Token::Eof;

    switch (c)
    {
    case '=':
        c = iss.get();
        if (c == '=')
            return Token::Eq;
        iss.putback(c);
        return Token::Assign;
    case '<':
    case '>':
    case '#':
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
        return Token(c);
    }

    // Handle tokens that utilize the buffer
    buffer.clear();

    if (isdigit(c))
    {
        while (isdigit(c))
        {
            buffer += c;
            c = iss.get();
        }
        // Put back c if loop exits as it is not a digit
        iss.putback(c);
        return Token::Number;
    }

    throw std::runtime_error(std::string("Invalid token: ") + c);
}

void Lexer::advance()
{
    if (current_token != Token::Eof)
    {
        current_token = get_next_token();
    }
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
        int token = static_cast<int>(p_lexer->get_current_token());
        auto msg = token == -1 ? "Unexpected EOF" : std::to_string(token);
        throw std::runtime_error(std::string("Invalid primary value! ") + msg);
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

double Parser::equality_expr()
{
    auto result = add_expr();
    auto tmp = result;
    int boolean = -1;

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Eq:
            p_lexer->advance();
            boolean &= result == (tmp = add_expr());
            result = tmp;
            break;
        case Token::Gt:
            p_lexer->advance();
            boolean &= result > (tmp = add_expr());
            result = tmp;
            break;
        case Token::Lt:
            p_lexer->advance();
            boolean &= result < (tmp = add_expr());
            result = tmp;
            break;
        default:
            return boolean == -1 ? result : boolean;
        }
    }
}

double Parser::assign_expr()
{
    Token t = p_lexer->get_current_token();
    double result = equality_expr();

    if (t == Token::Id && p_lexer->get_current_token() == Token::Assign)
    {
        p_lexer->advance();
        double res = equality_expr();
        std::cout << "SETVAR #" << last_var_name << "="
                  << res << std::endl;
        return symbol_table[last_var_name] = res;
    }

    return result;
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