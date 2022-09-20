#include <asmodean.h>

typedef struct
{
    byte Byte;
    byte Type;
    char StringStart; // Null terminated; Arbitrary length
} StringTable;

typedef struct
{
    uint32 Offset;
} StringOffsetTable;

typedef struct
{
    uint32 OffsetNext;
    uint32 Index;
} InputOffsetTable;

typedef struct
{
    uint32 ScriptLength;
    uint32 InputCount;
    uint32 StringOffsetTableOffset;
    uint32 StringTableOffset;
} ScriptDataHeader;

typedef struct
{
    char FileSignature[8];
    uint32 CompressedSize;
    uint32 DecompressedSize;
} CSTHeader;
