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

    void loadFromFile(char *);

    void loadFromBuf(byte *);

    void handleCommand(std::string);

    static void onLoad(void *, void *, int);
    static void onError(void *);

public:
    ScriptParser(MusicPlayer*);

    void parseNext();
    void setScript(char *);
};