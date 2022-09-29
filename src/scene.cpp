#include <scene.hpp>

SceneManager::SceneManager(MusicPlayer *mp, ImageManager *sm) : musicPlayer{mp}, imageManager{sm}
{
    setScript(SCRIPT_START);
};

void SceneManager::setScript(const std::string name)
{
    auto fpath = ASSETS SCRIPT_PATH + name + SCRIPT_EXT;
    Utils::fetchFileAndProcess(fpath, this, &SceneManager::loadFromBuf, NULL);
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
            break;

        case 0x20: // Display a message
            imageManager->currText = std::string(&stringTable->StringStart);
            imageManager->displayAll();
            break;
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
    LOG << cmdString;
    std::smatch matches;
    if (std::regex_match(cmdString, matches, std::regex("^pcm (\\S+)")))
    {
        if (matches.size() == 2)
        {
            musicPlayer->playPcm(matches[1].str());
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^bgm (\\d+) (\\S+).*")))
    {
        if (matches.size() == 3)
        {
            musicPlayer->setMusic(matches[2].str());
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
    else if (std::regex_match(cmdString, std::regex("^#.*")))
    {
        parser.parse(cmdString);
    }
}
