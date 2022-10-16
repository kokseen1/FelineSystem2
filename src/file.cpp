#include <file.hpp>
#include <string.h>
#include <scene.hpp>

FileManager::FileManager(const char *dbpath)
{
    static MusicManager musicManager(this);
    static ImageManager imageManager(this);
    static SceneManager sceneManager(&musicManager, &imageManager, this);
    fetchFileAndProcess(dbpath, this, &FileManager::parseKifDb, Utils::null);
}

void FileManager::parseKifDb(byte *buf, size_t sz, int userdata)
{
    auto end = buf + sz;

    for (;;)
    {
        std::string fname(reinterpret_cast<const char *>(buf));
        buf += fname.size() + 1;
        if (fname.size() == 0)
            break;
        kifTable.push_back(fname);
    }

    for (;;)
    {
        std::string fname(reinterpret_cast<const char *>(buf));
        buf += fname.size() + 1;
        kifDb[fname] = *(reinterpret_cast<KifDbEntry *>(buf));
        buf += SIZEOF_KDE;
        if (buf == end)
            break;
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