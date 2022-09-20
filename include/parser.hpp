#include <vector>
#include <string>
#include <cstformat.h>

#include <music.hpp>

class ScriptParser
{
private:
    MusicPlayer *musicPlayer;

    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void loadFromBuf(byte *, size_t);

    void handleCommand(std::string);

public:
    ScriptParser(MusicPlayer *);

    void parseNext();
    void setScript(const char *);
};