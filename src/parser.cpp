#include <parser.hpp>

ScriptParser::ScriptParser(MusicPlayer *mp, ImageManager *sm) : musicPlayer{mp}, imageManager{sm} {};

void ScriptParser::setScript(const std::string fpath)
{
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

        stringOffsetTable++;
        currStringEntry++;

        switch (stringTable->Type)
        {
        case 0x02: // Wait for input after message
        case 0x03: // Novel page break and wait for input after message
            break;

        case 0x20: // Display a message
                   // setMessage();
        case 0x21: // Set speaker of the message
            // setSpeaker();
            printf("%s\n", &stringTable->StringStart);
            goto next;

        case 0x30: // Perform any other command
            handleCommand(&stringTable->StringStart);
            goto next;

        // Debug:
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

std::vector<std::string> ScriptParser::getArgsFromMatch(std::smatch matches)
{
    std::vector<std::string> args;
    for (int i = 0; i < matches.size(); i++)
    {
        args.push_back(matches[i].str());
    }

    return args;
}

void ScriptParser::handleCommand(std::string cmdString)
{
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
            const std::string fpath = ASSETS "/bgm/" + matches[2].str() + ".ogg";
            musicPlayer->setMusic(fpath);
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^cg (\\d+) ([\\w\\d]+),(\\d),\\d,(\\w),\\w.*")))
    {
        ImageData imageData = {
            getArgsFromMatch(matches),
            IMAGE_TYPE::IMAGE_CG};

        imageManager->setImage(imageData);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^bg (\\d+) (\\S+).*")))
    {
        // TODO: Handle case
        ImageData imageData = {
            getArgsFromMatch(matches),
            IMAGE_TYPE::IMAGE_BG};

        imageManager->setImage(imageData);
    }
    else if (std::regex_match(cmdString, matches, std::regex("^next (\\S+)")))
    {
        if (matches.size() == 2)
        {
            const std::string fpath = ASSETS "/scene/" + matches[1].str() + ".cst";
            setScript(fpath);
        }
    }
}
