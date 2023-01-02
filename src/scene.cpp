#include <scene.hpp>
#include <convtable.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <algorithm>

SceneManager::SceneManager(AudioManager &mm, ImageManager &im, FileManager &fm, std::vector<Choice> &currChoices) : audioManager{mm}, imageManager{im}, fileManager{fm}, currChoices{currChoices}
{
    fileManager.init(this);
}

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
void SceneManager::loadScriptStart(byte *buf, size_t sz, const std::string &scriptName)
{
    loadScript(buf, sz, scriptName);

    // Allow ticker to start parsing
    parseScript = true;
}

void SceneManager::loadScriptOffset(byte *buf, size_t sz, const SaveData &saveData)
{
    loadScript(buf, sz, saveData.scriptName);
    stringOffsetTable = reinterpret_cast<StringOffsetTable *>(stringTableBase - saveData.offsetFromBase);
}

// Fetch and load script and offset specified in SaveData
void SceneManager::setScriptOffset(const SaveData &saveData)
{
    fileManager.fetchAssetAndProcess(saveData.scriptName + SCRIPT_EXT, this, &SceneManager::loadScriptOffset, saveData);
}

// Fetch the specified script and begin parsing
void SceneManager::setScript(const std::string &name)
{
    fileManager.fetchAssetAndProcess(name + SCRIPT_EXT, this, &SceneManager::loadScriptStart, name);
}

// Fetch and parse entrypoint script
void SceneManager::start()
{
    setScript(SCRIPT_ENTRYPOINT);
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
// TODO: Move to utils
std::string SceneManager::cleanText(const std::string &rawText)
{
    std::string text = std::regex_replace(sj2utf8(rawText), std::regex("\\[(.*?)\\]"), "$1");
    text = std::regex_replace(text, std::regex("¥fn"), "");
    text = std::regex_replace(text, std::regex("¥fs"), " ");
    text = std::regex_replace(text, std::regex("¥n"), " ");
    text = std::regex_replace(text, std::regex("¥@"), "");
    // text = std::regex_replace(text, std::regex("\\\\fn"), "");
    // text = std::regex_replace(text, std::regex("\\\\fs"), "");
    // text = std::regex_replace(text, std::regex("\\\\"), "");
    // text = std::regex_replace(text, std::regex("@"), "");
    return text;
}

// Block script from proceeding for a number of frames
void SceneManager::wait(unsigned int frames)
{
    waitTargetFrames = imageManager.getFramestamp() + frames;
}

bool SceneManager::rdrawWaited()
{
    return imageManager.getFramestamp() > (imageManager.getRdrawStart() + imageManager.getGlobalRdraw());
}

bool SceneManager::canProceed()
{
    return parseScript && (imageManager.getFramestamp() >= waitTargetFrames);
}

// Called from main loop to proceed script if needed
void SceneManager::tickScript()
{
    // LOG << imageManager.getFramestamp() << " : " << waitTargetFrames;

    if (canProceed())
    {
        LOG << "Start section";

        // Hide text when transitioning (follows CS2 behaviour)
        // Show when reaching break
        // imageManager.setHideText();
    }
    else
        return;

    // Parse a `section` of commands until encountering break or wait command
    // Iterative instead of recursive to avoid stack overflow
    while (canProceed())
        parseLine();

    LOG << "End section";
    // End of section
    imageManager.setRdraw(sectionRdraw);
    sectionRdraw = 0;
}

// User input initiated parse
void SceneManager::parse()
{
    if (!imageManager.getShowMwnd())
    {
        imageManager.setShowMwnd();
        return;
    }

    // Skip timer if any
    waitTargetFrames = 0;

    // Skip any remaining transitions/animations
    // imageManager.killRdraw();

    // Indicate to parse the next line
    parseScript = true;
}

// Parse the current line of the script
// Return 0 on success
void SceneManager::parseLine()
{
    if (currScriptData.empty())
    {
        LOG << "Script not loaded!";

        // Break loop to prevent blocking other processes (e.g. fetching next script)
        parseScript = false;
        return;
    }

    // Check if pointer exceeded end of script
    if (reinterpret_cast<byte *>(stringOffsetTable) >= stringTableBase)
    {
        LOG << "End of script!";
        parseScript = false;
        return;
    }

    // Parse script
    auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

    stringOffsetTable++;

    switch (stringTable->Type)
    {
    case 0x02: // Wait for input after message
    case 0x03: // Novel page break and wait for input after message
        imageManager.setShowText();
        imageManager.setShowMwnd();
        LOG << "Break";
        switch (autoMode)
        {
        case -1:
            // Auto off
            parseScript = false;
            break;
        case 0:
        default:
            // TODO: Support various auto mode speeds
            wait(60);
            break;
        }
        break;

    case 0x20: // Display a message
        if (stringTable->StringStart == '\0')
            break;

        if (speakerCounter == 0)
            imageManager.currSpeaker.clear();

        imageManager.currText = cleanText(std::string(&stringTable->StringStart));
        speakerCounter--;

        // Also show here in cases of appended text without break in-between
        imageManager.setShowText();
        imageManager.setShowMwnd();

        LOG << imageManager.currText;
        break;

    case 0x21: // Set speaker of the message
        imageManager.currSpeaker = cleanText(std::string(&stringTable->StringStart));
        speakerCounter = 1;
        break;

    case 0x30: // Perform any other command
        handleCommand(&stringTable->StringStart);
        break;

    // Debug commands:
    case 0xF0: // Sets the name of the source script file
    case 0xF1: // Marks the current line number in the source script file (as a string)
        break;
    }

    return;
}

// Wait without args to wait for frame-consuming actions within the current section
void SceneManager::wait()
{
    waitTargetFrames = maxWaitFramestamp;
}

// Determine the framestamp when the longest animation of the current section ends
void SceneManager::addSectionFrames(unsigned int rdraw)
{
    Uint64 waitFramestamp = imageManager.getFramestamp() + rdraw;
    if (waitFramestamp > maxWaitFramestamp)
        maxWaitFramestamp = waitFramestamp;
}

// Parse command and dispatch to respective handlers
void SceneManager::handleCommand(const std::string &cmdString)
{

#ifdef LOG_CMD
    LOG << "'" << cmdString << "'";
#endif
    std::smatch matches;
    if (std::regex_search(cmdString, matches, std::regex("^wait\\s?(\\d*)")))
    {
        const std::string &arg = matches[1].str();
        if (!arg.empty())
        {
            // Wait for x frames
            wait(std::stoi(arg));
        }
        else
        {
            addSectionFrames(sectionRdraw);

            // Wait until longest existing animation completes
            wait();
        }
    }
    else if (std::regex_search(cmdString, matches, std::regex("^frameon(?: (\\w+) (\\d+))?")))
    {
        const auto &framesStr = matches[2].str();
        if (framesStr.empty())
            imageManager.setShowMwnd();

        unsigned int frames = std::stoi(framesStr);
        const auto &mode = matches[1].str();

        // Assume only fade
        imageManager.setFrameon(frames);
        addSectionFrames(frames);
        wait();
    }
    else if (std::regex_search(cmdString, matches, std::regex("^frameoff(?: (\\w+) (\\d+))?")))
    {
        const auto &framesStr = matches[2].str();
        if (framesStr.empty())
            imageManager.setHideMwnd();
        else
        {
            unsigned int frames = std::stoi(framesStr);
            const auto &mode = matches[1].str();

            // Assume only fade
            imageManager.setFrameoff(frames);
            addSectionFrames(frames);
            wait();
        }
    }
    else if (std::regex_search(cmdString, matches, std::regex("^rdraw (\\d+)")))
    {
        // Number of frames to spend fading from one sprite to the next
        // Default is 1 frame - no fade effect (alpha 0 to 255 within 1 frame)
        // Set frames taken to transition images
        // currRdraw = std::stoi(matches[1].str());
        unsigned int rdraw = std::stoi(matches[1].str());
        sectionRdraw = rdraw;
        imageManager.setHideText();
    }
    else if (std::regex_search(cmdString, matches, std::regex("^r?wipe2? (\\w+) (\\d+)")))
    {
        // Use transition for wipes
        sectionRdraw = std::stoi(matches[2].str());
        imageManager.setHideText();
    }
    else if (std::regex_search(cmdString, matches, std::regex("^pcm (\\S+)")))
    {
        std::string asset = matches[1].str();
#ifdef LOWERCASE_ASSETS
        Utils::lowercase(asset);
#endif
        audioManager.setPCM(asset);
    }
    else if (std::regex_search(cmdString, matches, std::regex("^bgm (\\d+) (\\S+)")))
    {
        audioManager.setMusic(matches[2].str());
    }
    else if (std::regex_search(cmdString, matches, std::regex("^se (\\d) ([\\w]+)(?: ([\\w]+))?")))
    {
        std::string asset = matches[2].str();
        const std::string &arg2 = matches[3].str();
        const int &channel = std::stoi(matches[1].str());
        int loop = 0;

        if (!arg2.empty())
        {
            if (asset == "end")
            {
                audioManager.stopSound(channel);
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
        audioManager.setSE(asset, channel, loop);
    }

    // Display images
    else if (std::regex_search(cmdString, matches, std::regex("^(bg|eg|fg|cg|fw)(?: (\\d)(?: ([\\w,$]+)(?: ([^\\s]*)(?: ([^\\s]*)(?: ([^\\s]*)(?: ([^\\s]*))?)?)?)?)?)?")))
    {
        // bg 0 BG15_d 0 0 0
        // cg 0 Tchi01m,1,1,g,g #(950+#300) #(955+0) 1 0

        // Get image type enum based on identifier
        IMAGE_TYPE imageType;
        const auto &t = matches[1].str();
        if (t == "bg")
            imageType = IMAGE_TYPE::BG;
        else if (t == "eg")
            imageType = IMAGE_TYPE::EG;
        else if (t == "fg")
            imageType = IMAGE_TYPE::FG;
        else if (t == "cg")
            imageType = IMAGE_TYPE::CG;
        else if (t == "fw")
            imageType = IMAGE_TYPE::FW;
        else if (t == "pl")
        {
            return;
        }
        else
        {
            LOG << "Unknown image type identifier!";
            return;
        }

        const std::string &zIndexStr = matches[2].str();
        if (zIndexStr.empty())
        {
            imageManager.clearImageType(imageType);
            return;
        }

        int zIndex = std::stoi(zIndexStr);
        const std::string &asset = matches[3].str();
        if (asset.empty() || asset == "0")
        {
            imageManager.clearZIndex(imageType, zIndex);
            return;
        }

        // Support for @ symbol referring to previous value
        const auto &prevShifts = imageManager.getShifts(imageType, zIndex);
        const auto &prevXShift = prevShifts.first;
        const auto &prevYShift = prevShifts.second;

        if (asset == "blend")
        {
            imageManager.setBlend(imageType, zIndex, parser.parse(matches[4].str()));
        }

        else if (asset == "attr")
        {
            int attr = std::stoi(matches[4].str());
            const auto &xStr = matches[5].str();
            const auto &yStr = matches[6].str();

            const auto &stdinfo = imageManager.getStdinfo(imageType, zIndex);

            switch (attr)
            {
            case 1:
                parser.setVar(xStr, stdinfo.BaseX);
                parser.setVar(yStr, stdinfo.BaseY - 350);
                break;

            case 2:
                parser.setVar(xStr, stdinfo.BaseX);
                parser.setVar(yStr, stdinfo.BaseY);
                break;
            }
        }

        else if (asset == "disp")
        {
        }

        else if (asset == "fade")
        {
            // eg 5 fade 240 255 0
            const unsigned int frames = std::stoi(matches[4].str());
            const Uint8 startAlpha = std::stoi(matches[5].str());
            const Uint8 targetAlpha = std::stoi(matches[6].str());

            addSectionFrames(frames);

            imageManager.setFade(imageType, zIndex, frames, startAlpha, targetAlpha);
        }

        else if (asset == "move")
        {
            const auto &rdrawStr = matches[4].str();
            const auto &xShiftStr = matches[5].str();
            auto yShiftStr = matches[6].str();

            // No movement on axis if empty
            if (yShiftStr.empty())
                yShiftStr = "@";

            unsigned int rdraw = std::stoi(rdrawStr);
            int targetXShift = parser.parse(xShiftStr, prevXShift);
            int targetYShift = parser.parse(yShiftStr, prevYShift);

            // If there are multiple animations `wait` will wait for the longest one
            addSectionFrames(rdraw);

            imageManager.setMove(imageType, zIndex, rdraw, targetXShift, targetYShift);
        }

        // eg 5 amove1 240 100+96 300-144
        else if (asset == "amove1")
        {
        }
        else if (asset == "amove2")
        {
        }
        else if (asset == "amove3")
        {
        }

        else if (asset == "scale")
        {
            // eg 4 scale 100% 100%
        }

        else if (asset == "mode")
        {
            // imageManager.setMode(imageType, zIndex, matches[4].str());
        }
        else if (asset[0] == '$')
        {
            // $AARRGGBB
            if (asset.length() < 9)
                return;

            const auto &widthStr = matches[4].str();
            const auto &heightStr = matches[5].str();
            int width = widthStr.empty() ? WINDOW_WIDTH : parser.parse(widthStr);
            int height = heightStr.empty() ? WINDOW_HEIGHT : parser.parse(heightStr);

            Uint32 color = std::stoul(&asset[1], nullptr, 16);

            imageManager.createSolid(asset, width, height, color);

            const auto &xShiftStr = matches[6].str();
            const auto &yShiftStr = matches[7].str();

            int xShift = xShiftStr.empty() ? 0 : parser.parse(xShiftStr, prevXShift);
            int yShift = yShiftStr.empty() ? 0 : parser.parse(yShiftStr, prevYShift);

            imageManager.setImage(imageType, zIndex, asset, xShift, yShift);
        }

        else
        {

            const auto &xShiftStr = matches[4].str();
            const auto &yShiftStr = matches[5].str();
            const auto &opt1 = matches[6].str();
            const auto &opt2 = matches[7].str();

            int xShift = xShiftStr.empty() ? 0 : parser.parse(xShiftStr, prevXShift);
            int yShift = yShiftStr.empty() ? 0 : parser.parse(yShiftStr, prevYShift);

            imageManager.setImage(imageType, zIndex, asset, xShift, yShift);
        }
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

    // Non-capturing regex
    // Indicates start of choices
    else if (std::regex_search(cmdString, std::regex("^fselect")))
    {
        // Should not be required, but clear just to be sure
        currChoices.clear();
    }

    // Choice options
    else if (std::regex_match(cmdString, matches, std::regex("^(\\d+) (\\w+) (.+)")))
    {
        currChoices.push_back({imageManager, cleanText(matches[2].str()), cleanText(matches[3].str())});
    }

    // Auto mode
    else if (std::regex_search(cmdString, matches, std::regex("^auto (\\w+)")))
    {
        const std::string &status = matches[1].str();
        if (status == "on")
            autoMode = 0;
        else if (status == "off")
            autoMode = -1;
    }

    // Non-capturing regex
    // Variable assignment
    else if (std::regex_match(cmdString, std::regex("^#.+")))
    {
        try
        {
            parser.parse(cmdString);
        }
        catch (const std::runtime_error& error)
        {
            // Ignore parse errors
        }
    }
}

void SceneManager::selectChoice(int idx)
{
    if (idx < currChoices.size())
    {
        setScript(currChoices[idx].target);
        currChoices.clear();
    }
    else
    {
        LOG << "Selected choice out of bounds!";
    }
}

void SceneManager::saveState(const int saveSlot)
{
    json j;
    j[KEY_IMAGE] = imageManager.dump();
    j[KEY_AUDIO] = audioManager.dump();
    j[KEY_TEXT] = imageManager.currText;
    j[KEY_SPEAKER] = imageManager.currSpeaker;

    json &jScene = j[KEY_SCENE];
    jScene[KEY_SYMBOL_TABLE] = json(parser.getSymbolTable());
    jScene[KEY_SCRIPT_NAME] = currScriptName;
    jScene[KEY_OFFSET] = stringTableBase - reinterpret_cast<byte *>(stringOffsetTable);

    json &jChoices = jScene[KEY_CHOICE];
    for (const auto &c : currChoices)
        jChoices[c.target] = c.prompt;

    Utils::save(std::to_string(saveSlot), j);
}

void SceneManager::loadState(const int saveSlot)
{
    const auto &j = Utils::load(std::to_string(saveSlot));
    if (j.empty())
    {
        LOG << "No save data on slot " << saveSlot;
        return;
    }

    try
    {
        const json &jScene = j.at(KEY_SCENE);
        const json &jImage = j.at(KEY_IMAGE);
        const json &jAudio = j.at(KEY_AUDIO);

        setScriptOffset({jScene.at(KEY_SCRIPT_NAME), reinterpret_cast<byte *>(jScene.at(KEY_OFFSET).get<Uint64>())});
        parser.setSymbolTable(jScene.at(KEY_SYMBOL_TABLE).get<SymbolTable>());
        imageManager.currText = j.at(KEY_TEXT);
        imageManager.currSpeaker = j.at(KEY_SPEAKER);
        imageManager.loadDump(jImage);
        audioManager.loadDump(jAudio);

        autoMode = -1;

        currChoices.clear();

        // Populate choices from savedata
        for (auto &el : jScene.at(KEY_CHOICE).items())
            currChoices.push_back({imageManager, el.key(), el.value()});
    }
    catch (const json::out_of_range &e)
    {
        LOG << "Invalid save data on slot " << saveSlot;
    }
}
