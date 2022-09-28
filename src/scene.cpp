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
        printf("Script not loaded!\n");
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
        printf("End of script!\n");
    }
}

// Return matched command arguments as string vector
// Might not be necessary
std::vector<std::string> SceneManager::getArgsFromMatch(std::smatch matches)
{
    std::vector<std::string> args;
    for (int i = 0; i < matches.size(); i++)
    {
        // std::cout << "ARG " << i << " " << matches[i].str() << std::endl;
        // TODO: Might want to evaluate variable arithmetic here
        args.push_back(matches[i].str());
    }

    return args;
}

// Parse command and dispatch to respective handlers
void SceneManager::handleCommand(std::string cmdString)
{
    LOG << cmdString;
    std::cout << cmdString << std::endl;
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
    else if (std::regex_match(cmdString, matches, std::regex("^(bg|cg|eg)(?: (\\d)(?: ([\\w\\d,]+)(?: ([^%\\s]+)(?: ([^%\\s]+)(?: (\\d)(?: (\\d))?)?)?)?)?)?$")))
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

        // TODO: Evaluate offset variables
        auto xShiftStr = matches[4].str();
        auto yShiftStr = matches[5].str();

        int xShift = 0;
        int yShift = 0;

        // try
        // {
        xShift = xShiftStr.empty() ? 0 : parser.parse(xShiftStr);
        yShift = yShiftStr.empty() ? 0 : parser.parse(yShiftStr);
        // }
        // catch (std::runtime_error)
        // {
        // }

        // if (std::regex_match(xShiftStr, std::regex("^\\d+$")))
        //     xShift = std::stoi(xShiftStr);
        // if (std::regex_match(yShiftStr, std::regex("^\\d+$")))
        //     yShift = std::stoi(yShiftStr);

        imageManager->setImage(type, zIndex, asset, xShift, yShift);

        // for (int i = 0; i < matches.size(); i++)
        // {
        //     std::cout << i << " " << matches[i] << " Empty: " << matches[i].str().empty() << std::endl;
        // }
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
        // std::cout << cmdString << std::endl;
        parser.parse(cmdString);
    }
}
