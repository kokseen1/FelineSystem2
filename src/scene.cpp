#include <scene.hpp>
#include <convtable.hpp>

#include <algorithm>

SceneManager::SceneManager(MusicManager *mm, ImageManager *im, FileManager *fm) : musicManager{mm}, imageManager{im}, fileManager{fm}
{
    fileManager->init(this);
};

// Parse a raw CST file from a memory buffer and store the uncompressed script
void SceneManager::loadScript(byte *buf, size_t sz, const std::string &scriptName)
{
    CSTHeader *scriptHeader = reinterpret_cast<CSTHeader *>(buf);

    // Verify signature
    if (strncmp(scriptHeader->FileSignature, SCRIPT_SIGNATURE, sizeof(scriptHeader->FileSignature)) != 0)
    {
        LOG << "Invalid CST file signature!";
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
        currScriptData = Utils::zlibUncompress(scriptHeader->DecompressedSize, scriptDataRaw, scriptHeader->CompressedSize);
        if (currScriptData.empty())
        {
            LOG << "Script uncompress error";
            return;
        }
    }

    // Init script data
    ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(currScriptData.data());
    byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

    // Locate offset table and string table
    stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
    stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

    // Store loaded script name
    currScriptName = scriptName;
}

// Load a script from a raw buffer and parse it from the start
void SceneManager::loadScriptStart(byte *buf, size_t sz, const std::string scriptName)
{
    loadScript(buf, sz, scriptName);
    parseNext();
}

void SceneManager::loadScriptOffset(byte *buf, size_t sz, const SaveData saveData)
{
    loadScript(buf, sz, saveData.scriptName);
    stringOffsetTable = reinterpret_cast<StringOffsetTable *>(stringTableBase - saveData.offsetFromBase);
    parseNext();
}

void SceneManager::setScriptOffset(const SaveData saveData)
{
    fileManager->fetchAssetAndProcess(saveData.scriptName + SCRIPT_EXT, this, &SceneManager::loadScriptOffset, saveData);
}

// Fetch the specified script and begin parsing
void SceneManager::setScript(const std::string name)
{
    fileManager->fetchAssetAndProcess(name + SCRIPT_EXT, this, &SceneManager::loadScriptStart, name);
}

void SceneManager::start()
{
    setScript(SCRIPT_START);
}

std::string SceneManager::sj2utf8(const std::string &input)
{
    std::string output(3 * input.length(), ' '); // ShiftJis won't give 4byte UTF8, so max. 3 byte per input char are needed
    size_t indexInput = 0, indexOutput = 0;

    while (indexInput < input.length())
    {
        char arraySection = ((uint8_t)input[indexInput]) >> 4;

        size_t arrayOffset;
        if (arraySection == 0x8)
            arrayOffset = 0x100; // these are two-byte shiftjis
        else if (arraySection == 0x9)
            arrayOffset = 0x1100;
        else if (arraySection == 0xE)
            arrayOffset = 0x2100;
        else
            arrayOffset = 0; // this is one byte shiftjis

        // determining real array offset
        if (arrayOffset)
        {
            arrayOffset += (((uint8_t)input[indexInput]) & 0xf) << 8;
            indexInput++;
            if (indexInput >= input.length())
                break;
        }
        arrayOffset += (uint8_t)input[indexInput++];
        arrayOffset <<= 1;

        // unicode number is...
        uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) | shiftJIS_convTable[arrayOffset + 1];

        // converting to UTF8
        if (unicodeValue < 0x80)
        {
            output[indexOutput++] = unicodeValue;
        }
        else if (unicodeValue < 0x800)
        {
            output[indexOutput++] = 0xC0 | (unicodeValue >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
        else
        {
            output[indexOutput++] = 0xE0 | (unicodeValue >> 12);
            output[indexOutput++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
    }

    output.resize(indexOutput); // remove the unnecessary bytes
    return output;
}

// Removes formatting symbols from text
std::string SceneManager::cleanText(const std::string &rawText)
{
    std::string text = std::regex_replace(rawText, std::regex("\\[(.*?)\\]"), "$1");
    text = std::regex_replace(text, std::regex("\\\\fn"), "");
    text = std::regex_replace(text, std::regex("\\\\fs"), "");
    text = std::regex_replace(text, std::regex("\\\\"), "");
    text = std::regex_replace(text, std::regex("@"), "");

    return text;
}

// Parse the next line in the script
void SceneManager::parseNext()
{
    if (currScriptData.empty())
    {
        LOG << "Script not loaded!";
        return;
    }

    // Check if pointer exceeded end of script
    if (reinterpret_cast<byte *>(stringOffsetTable) >= stringTableBase)
    {
        LOG << "End of script!";
        return;
    }

    // Parse script
    const std::string scriptName = currScriptName;
    auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

    LOG << stringOffsetTable << "/" << std::hex << (void *)stringTableBase;

    stringOffsetTable++;

    switch (stringTable->Type)
    {
    case 0x02: // Wait for input after message
    case 0x03: // Novel page break and wait for input after message
        // LOG << "BREAK";
        if (autoMode != -1)
        {
            wait(100);
            parseNext();
        }
        break;

    case 0x20: // Display a message
        if (stringTable->StringStart == '\0')
        {
            LOG << "Empty text!";
        }
        else
        {
            if (speakerCounter == 0)
                imageManager->currSpeaker.clear();

            imageManager->currText = cleanText(std::string(&stringTable->StringStart));
            imageManager->displayAll();
            speakerCounter--;
        }
        goto next;

    case 0x21: // Set speaker of the message
        imageManager->currSpeaker = cleanText(std::string(&stringTable->StringStart));
        speakerCounter = 1;
        goto next;

    case 0x30: // Perform any other command
        handleCommand(&stringTable->StringStart);
        if (scriptName != currScriptName)
            return;
        goto next;

    // Debug commands:
    case 0xF0: // Sets the name of the source script file
    case 0xF1: // Marks the current line number in the source script file (as a string)

    next:
        // Might want to avoid recursion
        parseNext();
    }
}

void SceneManager::wait(const int duration)
{
    if (duration > 1000)
        return;
    SDL_Delay(duration);
}

// Parse command and dispatch to respective handlers
void SceneManager::handleCommand(const std::string &cmdString)
{
#ifdef LOG_CMD
    LOG << "'" << cmdString << "'";
#endif
    std::smatch matches;
    if (std::regex_search(cmdString, matches, std::regex("^wait(?: (\\d+))")))
    {
        const std::string &arg = matches[1].str();
        Uint32 ms = arg.empty() ? 1 : std::stoi(arg);
        wait(ms);
    }
    else if (std::regex_search(cmdString, matches, std::regex("^pcm (\\S+)")))
    {
        std::string asset = matches[1].str();
#ifdef LOWERCASE_ASSETS
        Utils::lowercase(asset);
#endif
        musicManager->setPCM(asset);
    }
    else if (std::regex_search(cmdString, matches, std::regex("^bgm (\\d+) (\\S+)")))
    {
        musicManager->setMusic(matches[2].str());
    }
    else if (std::regex_search(cmdString, matches, std::regex("^se (\\d) ([\\w\\d]+)(?: ([\\w\\d]+))?")))
    {
        std::string asset = matches[2].str();
        const std::string &arg2 = matches[3].str();
        const int &channel = std::stoi(matches[1].str());
        int loop = 0;

        if (!arg2.empty())
        {
            if (asset == "end")
            {
                musicManager->stopSound(channel);
                return;
            }

            if (asset == "loop")
            {
                asset = arg2;
                loop = -1;
            }
        }
#ifdef LOWERCASE_ASSETS
        Utils::lowercase(asset);
#endif
        musicManager->setSE(asset, channel, loop);
    }

    // Display images
    else if (std::regex_search(cmdString, matches, std::regex("^(bg|cg|eg|fw)(?: (\\d)(?: ([\\w\\d,]+)(?: ([^a-z][^%\\s]*)(?: ([^a-z][^%\\s]*)(?: (\\d+)(?: (\\d+))?)?)?)?)?)?")))
    {
        // bg 0 BG15_d 0 0 0
        // cg 0 Tchi01m,1,1,g,g #(950+#300) #(955+0) 1 0

        // Get image type enum based on identifier
        auto it = imageManager->imageTypes.find(matches[1].str());
        if (it == imageManager->imageTypes.end())
        {
            LOG << "Unknown image type identifier!";
            return;
        }
        const IMAGE_TYPE imageType = it->second;

        const std::string &zIndexStr = matches[2].str();
        if (zIndexStr.empty())
        {
            imageManager->clearImageType(imageType);
            return;
        }

        int zIndex = std::stoi(zIndexStr);
        const std::string &asset = matches[3].str();
        if (asset.empty() || asset == "0")
        {
            imageManager->clearZIndex(imageType, zIndex);
            return;
        }

        // Support for @ symbol referring to previous value
        const auto &imageData = imageManager->getImageData(imageType, zIndex);
        auto prevXShift = imageData.xShift;
        auto prevYShift = imageData.yShift;

        auto xShiftStr = matches[4].str();
        auto yShiftStr = matches[5].str();

        int xShift = xShiftStr.empty() ? 0 : parser.parse(xShiftStr, prevXShift);
        int yShift = yShiftStr.empty() ? 0 : parser.parse(yShiftStr, prevYShift);

        imageManager->setImage(imageType, zIndex, asset, xShift, yShift);
    }

    // Conditional statement
    else if (std::regex_search(cmdString, matches, std::regex("^if\\s*\\((.+)\\)\\s+(.+)")))
    {
        std::string cond = matches[1].str();
        if (parser.parse(cond) == 1)
        {
            handleCommand(matches[2].str());
        }
    }

    // Next scene
    else if (std::regex_search(cmdString, matches, std::regex("^next (\\S+)")))
    {
        setScript(matches[1].str());
    }

    // Choice options
    else if (std::regex_match(cmdString, matches, std::regex("^(\\d+) (\\w+) (.+)")))
    {
        currChoices.push_back(std::make_pair(matches[2].str(), matches[3].str()));

        for (int i = 0; i < currChoices.size(); i++)
        {
            std::cout << "[" << i + 1 << "] " << currChoices[i].second << std::endl;
        }
    }

    // Auto mode
    else if (std::regex_search(cmdString, matches, std::regex("^auto (\\w+)")))
    {
        const std::string &autoStatus = matches[1].str();
        if (autoStatus == "on")
        {
            autoMode = 0;
        }
        else if (autoStatus == "off")
        {
            autoMode = -1;
        }
    }
    // Non-capturing regex

    // Variable assignment
    else if (std::regex_match(cmdString, std::regex("^#.+")))
    {
        parser.parse(cmdString);
    }
    else if (std::regex_search(cmdString, std::regex("^fselect")))
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

void SceneManager::saveState(const int saveSlot)
{
    saveData[saveSlot].scriptName = currScriptName;
    saveData[saveSlot].offsetFromBase = reinterpret_cast<byte *>(stringTableBase - reinterpret_cast<byte *>(stringOffsetTable));
}

void SceneManager::loadState(const int saveSlot)
{
    auto &loadData = saveData[saveSlot];
    if (loadData.scriptName.empty())
    {
        LOG << "Empty savedata!";
        return;
    }

    setScriptOffset(loadData);
}