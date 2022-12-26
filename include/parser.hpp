#include <sstream>
#include <string>
#include <map>

typedef std::map<std::string, double> SymbolTable;

enum class Token
{
    Eof = -1,
    Number = 0,

    // Descending order of precedence
    Prev = '@',
    Lp = '(',
    Rp = ')',
    Id = '#',
    Incr = '++',
    Decr = '--',

    Mul = '*',
    Div = '/',

    Plus = '+',
    Minus = '-',

    Shl = '<<',
    Shr = '>>',

    Lt = '<',
    Gt = '>',
    Lte = '<=',
    Gte = '>=',

    Eq = '==',
    Neq = '!=',

    Band = '&',

    Bor = '|',

    And = '&&',

    Or = '||',

    Assign = '=',
    PlusAssign = '+=',
    MinusAssign = '-=',
    MulAssign = '*=',
    DivAssign = '/=',
};

class Lexer;

class Parser
{
public:
    double parse(const std::string &, const int = 0);

    void set_lexer_buffer(std::string &);

    // Replace the existing symbol table with a new one
    void setSymbolTable(const SymbolTable &symbolTable) { symbol_table = symbolTable; }

    // Return a reference to the symbol table map object
    const SymbolTable &getSymbolTable() { return symbol_table; }

    void setVar(const std::string &, const double);

private:
    int prevValue = 0;
    std::string last_var_name; // TODO: Might have better alternative
    Lexer *p_lexer = NULL;
    SymbolTable symbol_table{};

    double primary();
    double unary_expr();
    double mul_expr();
    double add_expr();
    double bitshift_expr();
    double inequality_expr();
    double equality_expr();
    double band_expr();
    double bor_expr();
    double and_expr();
    double or_expr();
    double assign_expr();
};

class Lexer
{
    friend void Parser::set_lexer_buffer(std::string &);

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
