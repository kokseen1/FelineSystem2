#include <scene.hpp>
#include <algorithm>

SceneManager::SceneManager(MusicManager *mm, ImageManager *im, FileManager *fm) : musicManager{mm}, imageManager{im}, fileManager{fm}
{
    fileManager->init(this);
};

// Read a script from a memory buffer
void SceneManager::loadScript(byte *buf, size_t sz, int userdata)
{
    CSTHeader *scriptHeader = reinterpret_cast<CSTHeader *>(buf);

    // Verify signature
    if (strncmp(scriptHeader->FileSignature, SCRIPT_SIGNATURE, sizeof(scriptHeader->FileSignature)) != 0)
    {
        std::cout << "Invalid CST file signature!" << std::endl;
        return;
    }

    // Get pointer to start of raw data
    auto *scriptDataRaw = reinterpret_cast<byte *>(scriptHeader + 1);

    if (scriptHeader->CompressedSize == 0)
    {
        // Uncompressed script
        currScriptData = std::vector<byte>(scriptDataRaw, scriptDataRaw + scriptHeader->DecompressedSize);
    }
    else
    {
        auto scriptDataDecompressed = Utils::zlibUncompress(scriptHeader->DecompressedSize, scriptDataRaw, scriptHeader->CompressedSize);
        if (scriptDataDecompressed.empty())
        {
            std::cout << "Script uncompress error" << std::endl;
            return;
        }
        currScriptData = scriptDataDecompressed;
    }

    // Init script data

    ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(currScriptData.data());
    byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

    // Locate offset table and string table
    stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
    stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

    // Calculate entry count
    stringEntryCount = (stringTableBase - reinterpret_cast<byte *>(stringOffsetTable)) / sizeof(StringOffsetTable);
    currStringEntry = 0;

    parseNext();
}

void SceneManager::setScript(const std::string name)
{
    fileManager->fetchAssetAndProcess(name + SCRIPT_EXT, this, &SceneManager::loadScript, 0);
}

void SceneManager::start()
{
    setScript(SCRIPT_START);
}

void SceneManager::parseNext()
{
    if (currScriptData.empty())
    {
        std::cout << "Script not loaded!" << std::endl;
        return;
    }

    if (currStringEntry < stringEntryCount)
    {
        auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

        stringOffsetTable++;
        currStringEntry++;

        switch (stringTable->Type)
        {
        case 0x02: // Wait for input after message
        case 0x03: // Novel page break and wait for input after message
            // LOG << "BREAK";
            break;

        case 0x20: // Display a message
            if (stringTable->StringStart == '\0')
            {
                LOG << "Empty text!";
            }
            else
            {
                imageManager->currText = std::string(&stringTable->StringStart);
                imageManager->currText.erase(std::remove(imageManager->currText.begin(), imageManager->currText.end(), '['), imageManager->currText.end());
                imageManager->currText.erase(std::remove(imageManager->currText.begin(), imageManager->currText.end(), ']'), imageManager->currText.end());
                imageManager->displayAll();
            }
            goto next;
        case 0x21: // Set speaker of the message
#ifdef DIALOGUE_ENABLE
            std::cout << &stringTable->StringStart << std::endl;
#endif
            goto next;

        case 0x30: // Perform any other command
            handleCommand(&stringTable->StringStart);
            goto next;

        // Debug commands:
        case 0xF0: // Sets the name of the source script file
        case 0xF1: // Marks the current line number in the source script file (as a string)

        next:
            // Might want to avoid recursion
            parseNext();
        }
    }
    else
    {
        std::cout << "End of script!" << std::endl;
    }
}

// Parse command and dispatch to respective handlers
void SceneManager::handleCommand(std::string cmdString)
{
#ifdef LOG_CMD
    LOG << cmdString;
#endif
    std::smatch matches;
    if (std::regex_match(cmdString, matches, std::regex("^pcm (\\S+)")))
    {
        if (matches.size() == 2)
        {
            musicManager->playPcm(matches[1].str());
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^bgm (\\d+) (\\S+).*")))
    {
        if (matches.size() == 3)
        {
            musicManager->setMusic(matches[2].str());
        }
    }
    // bg 0 BG15_d 0 0 0
    // cg 0 Tchi01m,1,1,g,g #(950+#300) #(955+0) 1 0
    else if (std::regex_match(cmdString, matches, std::regex("^(bg|cg|eg)(?: (\\d)(?: ([\\w\\d,]+)(?: ([^a-z][^%\\s]*)(?: ([^a-z][^%\\s]*)(?: (\\d)(?: (\\d))?)?)?)?)?)?$")))
    {
        IMAGE_TYPE type;
        const std::string &typeStr = matches[1].str();
        if (typeStr == "bg")
            type = IMAGE_TYPE::IMAGE_BG;
        else if (typeStr == "cg")
            type = IMAGE_TYPE::IMAGE_CG;
        else if (typeStr == "eg")
            type = IMAGE_TYPE::IMAGE_EG;

        const std::string &zIndexStr = matches[2].str();
        if (zIndexStr.empty())
        {
            imageManager->clearAllImage(type);
            return;
        }

        int zIndex = std::stoi(zIndexStr);
        const std::string &asset = matches[3].str();
        if (asset.empty())
        {
            imageManager->clearZIndex(type, zIndex);
            return;
        }

        if (asset == "blend")
        {
            return;
        }

        auto xShiftStr = matches[4].str();
        auto yShiftStr = matches[5].str();

        int xShift = xShiftStr.empty() ? 0 : parser.parse(xShiftStr);
        int yShift = yShiftStr.empty() ? 0 : parser.parse(yShiftStr);

        imageManager->setImage(type, zIndex, asset, xShift, yShift);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^if\\s*\\((.+)\\)\\s+(.+)")))
    {
        if (matches.size() == 3)
        {
            std::string cond = matches[1].str();
            if (parser.parse(cond) == 1)
            {
                handleCommand(matches[2].str());
            }
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^next (\\S+)")))
    {
        if (matches.size() == 2)
        {
            setScript(matches[1].str());
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^(\\d+) (\\w+) (.+)")))
    {
        if (matches.size() == 4)
        {
            currChoices.push_back(std::make_pair(matches[2].str(), matches[3].str()));
        }

        for (int i = 0; i < currChoices.size(); i++)
        {
            std::cout << "[" << i + 1 << "] " << currChoices[i].second << std::endl;
        }
    }
    // Non-capturing regex
    else if (std::regex_match(cmdString, std::regex("^#.*")))
    {
        parser.parse(cmdString);
    }
    else if (std::regex_match(cmdString, std::regex("^fselect$")))
    {
        LOG << "FSELECT";
        currChoices.clear();
    }
}

void SceneManager::selectChoice(int idx)
{
    if (idx < currChoices.size())
    {
        setScript(currChoices[idx].first);
        currChoices.clear();
    }
    else
    {
        LOG << "Selected choice out of bounds!";
    }
}