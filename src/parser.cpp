#include <parser.hpp>

ScriptParser::ScriptParser(MusicPlayer *mp, ImageManager *sm) : musicPlayer{mp}, imageManager{sm}
{
    setScript(SCRIPT_START);
};

void ScriptParser::setScript(const std::string name)
{
    auto fpath = ASSETS SCRIPT_PATH + name + SCRIPT_EXT;
    Utils::fetchFileAndProcess(fpath, this, &ScriptParser::loadFromBuf, NULL);
}

void ScriptParser::parseNext()
{
    if (currScriptData.empty())
    {
        printf("Script not loaded!\n");
        return;
    }

    if (currStringEntry < stringEntryCount)
    {
        auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

        char buf[1000] = "console.log('";

        stringOffsetTable++;
        currStringEntry++;

        switch (stringTable->Type)
        {
        case 0x02: // Wait for input after message
        case 0x03: // Novel page break and wait for input after message
            break;

        case 0x20: // Display a message
        case 0x21: // Set speaker of the message
#ifdef DIALOGUE
            std::cout << &stringTable->StringStart << std::endl;
#endif
            goto next;

        case 0x30: // Perform any other command
#ifdef __EMSCRIPTEN__
            strcat(buf, &stringTable->StringStart);
            strcat(buf, "')");
            emscripten_run_script(buf);
#endif
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
std::vector<std::string> ScriptParser::getArgsFromMatch(std::smatch matches)
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
void ScriptParser::handleCommand(std::string cmdString)
{
    // std::cout << cmdString << std::endl;
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
    else if (std::regex_match(cmdString, matches, std::regex("^cg (\\d+)\\s?(?:([\\w\\d]+),(\\d+),([\\d]+),([\\w\\d]),([\\w\\d])|(\\w+))?.*")))
    {
        imageManager->setImage(getArgsFromMatch(matches), IMAGE_TYPE::IMAGE_CG);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^eg (\\d+)\\s?(\\S+)?.*")))
    {
        imageManager->setImage(getArgsFromMatch(matches), IMAGE_TYPE::IMAGE_EG);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^bg (\\d+)\\s?(\\S+)?.*")))
    {
        // TODO: Handle case
        // TODO: Range validation
        imageManager->setImage(getArgsFromMatch(matches), IMAGE_TYPE::IMAGE_BG);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^next (\\S+)")))
    {
        if (matches.size() == 2)
        {
            setScript(matches[1].str());
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^#(\\d+)=(\\d+)")))
    {
        if (matches.size() == 3)
        {
            scriptVars[std::stoi(matches[1].str())] = std::stoi(matches[2].str());
        }
    }
}
