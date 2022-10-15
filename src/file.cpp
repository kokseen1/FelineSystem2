#include <file.hpp>

// Read a local file and return its contents as a vector
std::vector<byte> FileManager::readFile(const std::string fpath, long offset, long sz)
{
    // Use C-style for compatibility
    FILE *fp = fopen(fpath.c_str(), "rb");
    if (fp == NULL)
    {
        return {};
    }

    if (offset == -1 || sz == -1)
    {
        // Read whole file
        fseek(fp, 0, SEEK_END);
        sz = ftell(fp);
        rewind(fp);
    }
    else
    {
        fseek(fp, offset, SEEK_SET);
    }

    std::vector<byte> buf(sz);
    fread(&buf[0], sizeof(byte), sz, fp);
    fclose(fp);

    return buf;
}