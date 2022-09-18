#include <vector>
#include <string>
#include <cstformat.h>

class ScriptParser
{
private:
    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void loadFromFile(char *);

    void loadFromBuf(byte *);

    void handleCommand(std::string);

    static void onLoad(void *, void *, int);
    static void onError(void *);

public:
    void parseNext();
    void setScript(char *);
};