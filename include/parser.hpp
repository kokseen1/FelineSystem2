#include <vector>
#include <string>

#include <cstformat.h>

#include <music.hpp>
#include <scene.hpp>

class ScriptParser
{
private:
    MusicPlayer *musicPlayer;
    SceneManager *sceneManager;

    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void loadFromBuf(byte *, size_t, int &);

    void handleCommand(std::string);

public:
    ScriptParser(MusicPlayer *, SceneManager *);

    void parseNext();

    void setScript(const std::string);
};