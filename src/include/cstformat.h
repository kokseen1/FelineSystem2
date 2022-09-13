#include <asmodean.h>

typedef struct
{
    byte Byte;
    byte Type;
    char StringStart;
} StringTable;

typedef struct
{
    uint32 Offset;
} StringOffsetTable;

typedef struct
{
    uint32 OffsetNext;
    uint32 Index;
    byte next;
} InputOffsetTable;

typedef struct
{
    uint32 ScriptLength;
    uint32 InputCount;
    uint32 StringOffsetTableOffset;
    uint32 StringTableOffset;
    byte TablesStart;
} ScriptDataHeader;

typedef struct
{
    char FileSignature[8];
    uint32 CompressedSize;
    uint32 DecompressedSize;
    ScriptDataHeader ScriptDataHeader;
} CSTHeader;
