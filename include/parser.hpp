#include <sstream>
#include <string>
#include <map>

enum class Token
{
    Number,
    Id = '#',
    Assign = '=',
    Eq = '==',
    Lt = '<',
    Gt = '>',
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
    std::istringstream iss;

    Token get_next_token();

public:
    Lexer(const std::string);
    Token get_current_token() { return current_token; };
    std::string get_curr_buffer() { return buffer; };
    void advance();
};

class Parser
{
public:
    double parse(std::string &);

private:
    std::string last_var; // TODO: Might have better alternative
    Lexer *p_lexer = NULL;
    std::map<std::string, double> symbol_table{};

    double equality_expr();
    double assign_expr();
    double add_expr();
    double mul_expr();
    double unary_expr();
    double primary();
};