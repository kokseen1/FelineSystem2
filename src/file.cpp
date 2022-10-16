#include <file.hpp>
#include <string.h>
#include <scene.hpp>

FileManager::FileManager(const char *dbpath)
{
    // Initialize manager classes
    static MusicManager musicManager(this);
    static ImageManager imageManager(this);
    static SceneManager sceneManager(&musicManager, &imageManager, this);

    fetchFileAndProcess(dbpath, this, &FileManager::parseKifDb, Utils::null);
}

void FileManager::parseKifDb(byte *buf, size_t sz, int userdata)
{
    std::vector<uint32> entryCountVec;

    // Parse archive table
    for (;;)
    {
        std::string fname(reinterpret_cast<const char *>(buf));
        buf += fname.size() + 1;
        if (fname.size() == 0)
            break;
        entryCountVec.push_back(*(reinterpret_cast<uint32 *>(buf)));
        buf += 4;

        KifTableEntry kte{fname, *(reinterpret_cast<unsigned char *>(buf))};
        buf += 1;
        if (kte.IsEncrypted == '\x01')
        {
            memcpy(kte.FileKey, buf, 4);
            buf += 4;
        }
        kifTable.push_back(kte);
    }

    // Parse archive item entries
    for (std::size_t i = 0; i != entryCountVec.size(); ++i)
    {
        for (uint32 j = 0; j < entryCountVec[i]; j++)
        {
            std::string fname(reinterpret_cast<const char *>(buf));
            buf += fname.size() + 1;
            KifDbEntry kde;
            kde.Offset = *(reinterpret_cast<uint32 *>(buf));
            buf += 4;
            kde.Length = *(reinterpret_cast<uint32 *>(buf));
            buf += 4;
            kde.Index = i;
            kifDb[fname] = kde;
        }
    }

    sceneManager->setScript(SCRIPT_START);
}

// Read a local file and return its contents as a vector
std::vector<byte> FileManager::readFile(const std::string fpath, uint64_t offset, uint64_t length)
{
    // Use C-style for compatibility
    FILE *fp = fopen(fpath.c_str(), "rb");
    if (fp == NULL)
    {
        return {};
    }

    if (offset == -1 || length == -1)
    {
        // Read whole file
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        rewind(fp);
    }
    else
    {
        std::cout << offset << " at " << length << std::endl;
        fseek(fp, offset, SEEK_SET);
    }

    std::vector<byte> buf(length);
    fread(&buf[0], sizeof(byte), length, fp);
    fclose(fp);

    return buf;
}