#include <regex>

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
        if (matches.size() == 5)
        {
            // const std::string fpath = ASSETS "/image/" + matches[2].str() + "_" + matches[3].str() + ".hg3";
            // imageManager->setImage(fpath);
        }
    }
    else if (std::regex_match(cmdString, matches, std::regex("^bg (\\d+) (\\S+).*")))
    {
        if (matches.size() == 1)
        {
            // Reset bg
        }
        else if (matches.size() == 2)
        {
            // matches[1].str().c_str()
        }
        else if (matches.size() == 3)
        {
            // TODO: Handle case
            ImageData imageData = {
                matches[2].str(),
                IMAGE_TYPE::IMAGE_BG};

            imageManager->setImage(imageData);
        }
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
