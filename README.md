# ![icon](https://i.imgur.com/fgr54yA.png) FelineSystem2


## KIF Database Structure

### Archive Table

|Data Type|Value|Description|
|--:|:--|:--|
|`char[len]`|ArchiveName|NULL-terminated filename of the KIF archive|
|`uint32`|EntryCount|Number of entries in archive|
|`unsigned char`|IsEncrypted|`0x01` if archive is encrypted, `0x00` otherwise|
|`byte[4]`|FileKey|Blowfish key for decrypting entries (Only exists if IsEncrypted is `0x01`)|
|`0x00`|TableEnd|Signifies end of table|

### Archive Item Entry

|Data Type|Value|Description|
|--:|:--|:--|
|`char[len]`|FileName|NULL-terminated deobfuscated filename|
|`uint32`|Offset|The offset to the entry's data|
|`uint32`|Length|The length of the entry's data|