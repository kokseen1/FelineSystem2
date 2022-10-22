#include <parser.hpp>

#include <iostream>

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
        // Multi character tokens
        // First check for first character match
        // Then check next character, otherwise call unget and return first token
    case '|':
        if (iss.get() == '|')
            return Token::Or;
        iss.unget();
        return Token::Bor;
    case '&':
        if (iss.get() == '&')
            return Token::And;
        iss.unget();
        return Token::Band;
    case '!':
        if (iss.get() == '=')
            return Token::Neq;
        iss.unget();
        // No other token that starts with !
        throw std::runtime_error(std::string("Invalid token: ") + c);
    case '=':
        if (iss.get() == '=')
            return Token::Eq;
        iss.unget();
        return Token::Assign;
    case '+':
        if (iss.get() == '+')
            return Token::Incr;
        iss.unget();
        return Token::Plus;
    case '-':
        if (iss.get() == '-')
            return Token::Decr;
        iss.unget();
        return Token::Minus;
    case '<':
        if (iss.get() == '<')
            return Token::Shl;
        iss.unget();
        return Token::Lt;
    case '>':
        if (iss.get() == '>')
            return Token::Shr;
        iss.unget();
        return Token::Gt;

        // Single character tokens
    case '#':
    case '*':
    case '/':
    case '(':
    case ')':
        buffer = c;
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
        // std::cout << "curr_token: " << static_cast<int>(current_token) << std::endl;
    }
}

void Parser::set_lexer_buffer(std::string &buffer)
{
    p_lexer->buffer = buffer;
}

double Parser::primary()
{
    std::string buffer;
    double result;

    switch (p_lexer->get_current_token())
    {
    case Token::Id:
        p_lexer->advance();

        // Evaluate variable name
        last_var_name = std::to_string(primary());
        set_lexer_buffer(last_var_name);

        // Store initial result, assume post increment/decrement
        result = symbol_table[last_var_name];
        for (;;)
        {
            switch (p_lexer->get_current_token())
            {
            case (Token::Decr):
                // std::cout << "DECREMENT" << std::endl;
                p_lexer->advance();
                symbol_table[last_var_name]--;
                break;
            case (Token::Incr):
                // std::cout << "INCREMENT" << std::endl;
                p_lexer->advance();
                symbol_table[last_var_name]++;
                break;
            default:
                return result;
            }
        }
    case Token::Number:
        buffer = p_lexer->get_curr_buffer();
        // std::cout << "NUM " << buffer << std::endl;
        p_lexer->advance();
        return std::stoi(buffer);
    case Token::Lp:
        p_lexer->advance();
        result = add_expr();
        if (p_lexer->get_current_token() != Token::Rp)
            throw std::runtime_error("No closing parentheses!");
        p_lexer->advance();
        return result;

    default:
        auto token = static_cast<char>(p_lexer->get_current_token());
        if (token == -1)
            throw std::runtime_error("Unexpected EOF!");
        throw std::runtime_error(std::string("Invalid primary operand! Token: ") + token);
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
    double operand;

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
            operand = unary_expr();
            if (operand == 0)
                throw(std::runtime_error("Division by 0!"));
            result /= operand;
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

double Parser::bitshift_expr()
{
    int result = add_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Shl:
            p_lexer->advance();
            result <<= static_cast<int>(add_expr());
            break;
        case Token::Shr:
            p_lexer->advance();
            result >>= static_cast<int>(add_expr());
            break;
        default:
            return result;
        }
    }
}

double Parser::inequality_expr()
{
    auto result = bitshift_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Gt:
            p_lexer->advance();
            result = result > bitshift_expr();
            break;
        case Token::Lt:
            p_lexer->advance();
            result = result < bitshift_expr();
            break;
        default:
            return result;
        }
    }
}

double Parser::equality_expr()
{
    auto result = inequality_expr();
    // auto tmp = result;
    // int boolean = -1;

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Eq:
            p_lexer->advance();
            result = result == inequality_expr(); // C-style

            // Python-style operator chaining (ANDed)
            // boolean &= result == (tmp = inequality_expr());
            // result = tmp;
            break;
        case Token::Neq:
            p_lexer->advance();
            result = result != inequality_expr();
            break;
        default:
            return result;
            // return boolean == -1 ? result : boolean;
        }
    }
}

double Parser::band_expr()
{
    int result = equality_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Band:
            p_lexer->advance();
            result &= static_cast<int>(equality_expr());
            break;
        default:
            return result;
        }
    }
}

double Parser::bor_expr()
{
    int result = band_expr();

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Bor:
            p_lexer->advance();
            result |= static_cast<int>(band_expr());
            break;
        default:
            return result;
        }
    }
}

double Parser::and_expr()
{
    int result = bor_expr();
    int result_rhs;

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::And:
            p_lexer->advance();
            // Evaluate separately to ensure function is called instead of short-circuiting
            result_rhs = bor_expr();
            result = result && result_rhs;
            break;
        default:
            return result;
        }
    }
}

double Parser::or_expr()
{
    int result = and_expr();
    int result_rhs;

    for (;;)
    {
        switch (p_lexer->get_current_token())
        {
        case Token::Or:
            p_lexer->advance();
            // Evaluate separately to ensure function is called instead of short-circuiting
            result_rhs = and_expr();
            result = result || result_rhs;
            break;
        default:
            return result;
        }
    }
}

double Parser::assign_expr()
{
    Token t = p_lexer->get_current_token();
    auto result = or_expr();

    if (t == Token::Id && p_lexer->get_current_token() == Token::Assign)
    {
        // Verify valid LHS (Not working for dynamic variable names)
        auto buffer = p_lexer->get_curr_buffer();
        if (buffer != last_var_name)
            throw(std::runtime_error("Invalid '" + buffer + "' on LHS of assignment!"));

        // Evaluate RHS
        p_lexer->advance();
        result = or_expr();
        // std::cout << "SETVAR #" << last_var_name << "=" << result << std::endl;
        symbol_table[last_var_name] = result;
    }

    if (p_lexer->get_current_token() != Token::Eof)
    {
        throw std::runtime_error(std::string("Unexpected before EOF: ") + static_cast<char>(p_lexer->get_current_token()));
    }

    return result;
}

// Evaluate a single string
double Parser::parse(const std::string &s)
{
    // Initialize the lexer with the string
    Lexer lexer = Lexer{s};
    p_lexer = &lexer;

    // Begin with expression of least precedence
    double result = assign_expr();

    p_lexer = NULL;
    return result;
}
