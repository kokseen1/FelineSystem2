#include <file.hpp>
#include <scene.hpp>

#include <string.h>

#include <fstream>

FileManager::FileManager()
{
}

void FileManager::init(SceneManager *sm)
{
    sceneManager = sm;
    fetchFileAndProcess(KIF_DB, this, &FileManager::parseKifDb, 0);
}

// Parse a raw KIF database file and populate the KIF DB and table
void FileManager::parseKifDb(byte *buf, size_t sz, int userdata)
{
    std::vector<uint32> entryCountVec;

    // Parse archive table
    for (;;)
    {
        std::string fname(reinterpret_cast<const char *>(buf));
        buf += fname.size() + 1;
        if (fname.size() == 0)
            // End of table
            break;

        entryCountVec.push_back(*(reinterpret_cast<uint32 *>(buf)));
        buf += 4;

        KifTableEntry kte{fname, *(reinterpret_cast<unsigned char *>(buf))};
        buf += 1;

        if (kte.IsEncrypted == '\x01')
        {
            // Copy key
            memcpy(kte.FileKey, buf, 4);
            buf += 4;
        }

        kifTable.push_back(kte);
    }

    // Parse archive item entries
    for (std::size_t archiveIdx = 0; archiveIdx < entryCountVec.size(); archiveIdx++)
    {
        for (uint32 itemIdx = 0; itemIdx < entryCountVec[archiveIdx]; itemIdx++)
        {
            std::string fname(reinterpret_cast<const char *>(buf));
            buf += fname.size() + 1;

            KifDbEntry kde;
            kde.Offset = *(reinterpret_cast<uint32 *>(buf));
            buf += 4;
            kde.Length = *(reinterpret_cast<uint32 *>(buf));
            buf += 4;
            kde.Index = archiveIdx;

            kifDb[fname] = kde;
        }
    }

    LOG << "Parsed " << kifDb.size() << " KIF entries";

    // Start game
    sceneManager->start();
}

// Read a local file and return its contents as a vector
// Support optional starting offset and length
std::vector<byte> FileManager::readFile(const std::string &fpath, uint64_t offset, uint64_t length)
{
    // Use C-style for compatibility
    FILE *fp = fopen(fpath.c_str(), "rb");
    if (fp == NULL)
    {
        return {};
    }

    if (length == 0)
    {
        // Read whole file if length not specified
        fseek64(fp, 0, SEEK_END);
        length = ftell64(fp);
        rewind(fp);
    }

    // Seek to starting offset (default 0)
    fseek64(fp, offset, SEEK_SET);

    std::vector<byte> buf(length);
    fread(buf.data(), 1, length, fp);
    fclose(fp);

    return buf;
}
