
#include <cstformat.h>

class ScriptParser
{
public:
    void readFromFile(char *);

    void readFromBuf(void *);

    void parseScript(ScriptDataHeader *);
}