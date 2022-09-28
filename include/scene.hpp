#include <cstformat.h>

#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <regex>
#include <map>

#include <utils.hpp>
#include <music.hpp>
#include <image.hpp>

#define SCRIPT_PATH "scene/"
#define SCRIPT_EXT ".cst"
#define SCRIPT_SIGNATURE "CatScene"
#define SCRIPT_START "op_cont2"

#define DIALOGUE_ENABLE

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
    std::istringstream iss;

    Token get_token();

public:
    Lexer(const std::string);
    Token get_current_token() { return current_token; };
    std::string get_curr_buffer() { return buffer; };
    void advance();
};

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
    double unary_expr();
    double primary();
};

class SceneManager
{
private:
    MusicPlayer *musicPlayer;
    ImageManager *imageManager;
    Parser parser;

    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void handleCommand(std::string);

    std::vector<std::string> getArgsFromMatch(std::smatch);

    // Read a script from a memory buffer
    template <typename T>
    void loadFromBuf(byte *buf, size_t sz, T userdata)
    {
        CSTHeader *scriptHeader = reinterpret_cast<CSTHeader *>(buf);

        // Verify signature
        if (strncmp(scriptHeader->FileSignature, SCRIPT_SIGNATURE, sizeof(scriptHeader->FileSignature)) != 0)
        {
            printf("Invalid CST file signature!\n");
            return;
        }

        // Get pointer to start of raw data
        auto *scriptDataRaw = reinterpret_cast<byte *>(scriptHeader + 1);

        if (scriptHeader->CompressedSize == 0)
        {
            // Uncompressed script
            currScriptData = std::vector<byte>(scriptDataRaw, scriptDataRaw + scriptHeader->DecompressedSize);
        }
        else
        {
            auto scriptDataDecompressed = Utils::zlibUncompress(scriptHeader->DecompressedSize, scriptDataRaw, scriptHeader->CompressedSize);
            if (scriptDataDecompressed.empty())
            {
                printf("Script uncompress error\n");
                return;
            }
            currScriptData = scriptDataDecompressed;
        }

        // Init script data

        ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(currScriptData.data());
        byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

        // Locate offset table and string table
        stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
        stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

        // Calculate entry count
        stringEntryCount = (stringTableBase - reinterpret_cast<byte *>(stringOffsetTable)) / sizeof(StringOffsetTable);
        currStringEntry = 0;
    }

public:
    SceneManager(MusicPlayer *, ImageManager *);

    void parseNext();

    void setScript(const std::string);
};