# ![icon](https://i.imgur.com/fgr54yA.png) FelineSystem2

An open source from-scratch remake of the CS2 engine.

## Features

- Multi-platform (WASM, Windows SDL2)
- Raw KIF archive decryption
- HG-3 image decoding and caching
- Custom-built CatScene recursive-descent parser

## KIF Database Structure

FelineSystem2 implements a custom database of the game's KIF archives and assets, stored as a binary file.

### Archive Table

|       Data Type | Value       | Description                                                                |
| --------------: | :---------- | :------------------------------------------------------------------------- |
|     `char[len]` | ArchiveName | NULL-terminated filename of the KIF archive                                |
|        `uint32` | EntryCount  | Number of entries in archive                                               |
| `unsigned char` | IsEncrypted | `0x01` if archive is encrypted, `0x00` otherwise                           |
|       `byte[4]` | FileKey     | Blowfish key for decrypting entries (Only exists if IsEncrypted is `0x01`) |
|          `0x00` | TableEnd    | Signifies end of table                                                     |

### Archive Item Entry

|   Data Type | Value    | Description                           |
| ----------: | :------- | :------------------------------------ |
| `char[len]` | FileName | NULL-terminated deobfuscated filename |
|    `uint32` | Offset   | The offset to the entry's data        |
|    `uint32` | Length   | The length of the entry's data        |

## Notes

### Parser

FelineSystem2 contains a recursive-descent parser custom-built to parse CatScene scripts.

- If statements must contain a whitespace after the closing parenthesis of the condition
- Operators follow C-Style left-to-right precedence and associativity
- Integers are the only data type that exist
- Variable names can only be integers
- Variable names can by dynamically evaluated

## Special Thanks

- [Trigger](https://github.com/trigger-segfault) for creating such an [amazingly comprehensive knowledgebase](https://github.com/trigger-segfault/TriggersTools.CatSystem2/wiki) of the CS2 engine
- [asmodean](http://asmodean.reverse.net/pages/exkifint.html) for being a pioneer in reversing the CS2 engine
- [Dodder](http://www.doddlercon.com/main/) for [Hacking the Grisaia](http://www.doddlercon.com/main/?p=171) for localization
- [koestl](https://twitter.com/koestl) for his impeccable translations and contributions to localization