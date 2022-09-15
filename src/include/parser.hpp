
#include <cstformat.h>

class ScriptParser
{
public:
    void readFromFile(char *);

    void readFromBuf(byte *);

private:
    void parseScriptData(byte *);
};