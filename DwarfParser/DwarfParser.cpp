// DwarfParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

typedef enum _DebugType
{
    DW_TAG_ARRAY_TYPE,
    DW_TAG_CLASS_TYPE,
    DW_TAG_ENUMERATION_TYPE,
    DW_TAG_POINTER_TYPE,
    DW_TAG_STRING_TYPE,
    DW_TAG_STRUCTURE_TYPE,
    DW_TAG_TYPEDEF,
    DW_TAG_UNION_TYPE,

} DebugType;

std::map<std::string, std::vector<std::string>> SplitBlocks(std::vector<std::string>& lines, std::string token);
std::string TrimString(std::string line);
bool IsSubstr(std::string s, std::string sub);
int FindToken(std::vector<std::string>& lines, std::string token, int times);
std::pair<std::string, std::string> IdentifyTypes(std::vector<std::string> &typeLines, std::map<std::string,
    std::string> &existingTypes);
std::pair<std::string, std::string> ExtractNameLocation(std::vector<std::string>& unit, std::map<std::string, std::string> &existingTypes);
std::string ExtractLocation(std::string locationLine);
std::string ExtractName(std::string nameLine);
std::vector<std::vector<std::string>> ExtractUnitInOrder(std::vector<std::string>& lines, std::string token);
std::string ExtractStructureAddress(std::vector<std::string>& typeLines, std::map<std::string, std::string> &existingTypes);
std::string ExtractType(std::string typeLine);
int FindSubstr(std::string s, std::string substr);
std::string SubstituteLocation(std::string &locationStr, std::string oldValue, std::string newValue);

int main()
{
    std::ifstream dwarfFile("C:\\win_share1\\w_option_export");
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(dwarfFile, line))
    {
        lines.push_back(line);
    }

    dwarfFile.close();

    // find debug sections 
    auto blocks = SplitBlocks(lines, "Contents of the");

    auto debugTypesSecKey = "Contents of the .debug_types section:";
    auto debugInfoSecKey = "Contents of the .debug_info section:";

    std::map<std::string, std::string> typeDic;

    if (blocks.find(debugTypesSecKey) != blocks.end())
    {
        auto typeLines = blocks[debugTypesSecKey];

        auto compilationUnits = ExtractUnitInOrder(typeLines, "Compilation Unit");

        for (auto it = compilationUnits.begin(); it != compilationUnits.end(); ++it)
        {
            auto typePair = IdentifyTypes(*it, typeDic);
            typeDic.insert(typePair);
        }
    }

    std::map<std::string, std::string> nameLocationPairs;

    if (blocks.find(debugInfoSecKey) != blocks.end())
    {
        auto infoLines = blocks[debugInfoSecKey];

        auto compileUnits = SplitBlocks(infoLines, "Compilation Unit");

        for (auto it = compileUnits.begin(); it != compileUnits.end(); ++it)
        {
            auto variableBlocks = SplitBlocks(it->second, "DW_TAG_variable");

            for (auto it = variableBlocks.begin(); it != variableBlocks.end(); ++it)
            {
                auto nameLocPair = ExtractNameLocation(it->second, typeDic);
                nameLocationPairs.insert(nameLocPair);
            }
        }
    }

    return 0;
}

std::map<std::string, std::vector<std::string>> SplitBlocks(std::vector<std::string>& lines, std::string token)
{
    std::map<std::string, std::vector<std::string>> debugCategory;
    auto start = lines.begin();

    // try to find "Contents of the" line 
    while ((start = std::find_if(start, lines.end(),
        [&token](std::string line)
        {
            return IsSubstr(line, token);
        })) != lines.end())
    {
        auto same = std::find_if(start + 1, lines.end(), [&token](std::string line) { return IsSubstr(line, token); });

        auto block = std::vector<std::string>(same - start - 1);
        std::copy(start + 1, same, block.begin());

        auto trimmedKey = TrimString(*start);
        debugCategory[trimmedKey] = block;

        start = same;
    }

    return debugCategory;
}

std::string TrimString(std::string line)
{
    int start = 0;
    int end = line.size();

    while (start < end && line[start] == ' ')
    {
        start++;
    }

    while (end > 0 && line[end - 1] == ' ')
    {
        end--;
    }

    if (start < end)
    {
        return line.substr(start, end - start);
    }

    return "";
}

bool IsSubstr(std::string s, std::string sub)
{
    if (s.size() < sub.size())
    {
        return false;
    }

    int big = s.size();
    int small = sub.size();
    int start = 0;
    while (big - small >= 0)
    {
        if (s.substr(start, small).compare(sub) == 0)
        {
            return true;
        }

        start++;
        big--;
    }

    return false;
}

int FindToken(std::vector<std::string>& lines, std::string token, int times)
{
    auto realTimes = 0;
    auto it = lines.begin();
    for (; it != lines.end(); ++it)
    {
        if (IsSubstr(*it, token))
        {
            realTimes++;
        }

        if (realTimes == times)
        {
            return it - lines.begin();
        }
    }
    // can not find token with specified times 
    return -1;
}

std::pair<std::string, std::string> IdentifyTypes(std::vector<std::string>& typeLines, std::map<std::string, std::string> &existingTypes)
{
    auto ret = std::pair<std::string, std::string>();
    // find Signature first 
    auto unary = std::bind(IsSubstr, std::placeholders::_1, std::string("Signature"));

    auto signatureLine = std::find_if(typeLines.begin(), typeLines.end(), unary);

    if (signatureLine != typeLines.end())
    {
        auto theLine = *signatureLine;
        auto startChar = find(theLine.begin(), theLine.end(), '0');

        // 64bit hex string including hex prefix
        if (startChar != theLine.end())
        {
            auto signature = theLine.substr(startChar - theLine.begin(), 18);
            int lineIndex;
            if ((lineIndex = FindToken(typeLines, "Abbrev Number", 2)) != -1)
            {
                auto typeLine = typeLines[lineIndex];
                auto startBrace = std::find(typeLine.begin(), typeLine.end(), '(');
                auto endBrace = std::find(typeLine.begin(), typeLine.end(), ')');

                if (startBrace != typeLine.end() && endBrace != typeLine.end() && endBrace - startBrace > 0)
                {
                    auto typeStr = typeLine.substr(startBrace - typeLine.begin() + 1, endBrace - startBrace - 1);
                    
                    // special case: user defined type
                    if (typeStr == "DW_TAG_typedef")
                    {
                        int secondSignature = FindToken(typeLines, "signature", 1);

                        if (secondSignature != -1)
                        {
                            auto orignalTypeLine = typeLines[secondSignature];

                            int colonIdx = orignalTypeLine.find_last_of(":");

                            auto originalSignature = orignalTypeLine.substr(colonIdx + 2, 18);

                            if (existingTypes.find(originalSignature) != existingTypes.end())
                            {
                                return std::make_pair(signature, existingTypes[originalSignature]);
                            }
                        }
                    }
                    else if (typeStr == "DW_TAG_base_type")
                    {
                        int nameIdx = FindToken(typeLines, "DW_AT_name", 1);

                        if (nameIdx != -1)
                        {
                            auto nameLine = typeLines[nameIdx];

                            int colonIdx = nameLine.find_last_of(":");

                            auto name = nameLine.substr(colonIdx + 2, nameLine.size() - colonIdx - 2);

                            return std::make_pair(signature, "DW_TAG_base_type:" + name);
                        }
                    }
                    else if (typeStr == "DW_TAG_pointer_type")
                    {
                        int secondSignature = FindToken(typeLines, "signature", 1);

                        if (secondSignature != -1)
                        {
                            auto orignalTypeLine = typeLines[secondSignature];

                            int colonIdx = orignalTypeLine.find_last_of(":");

                            auto originalSignature = orignalTypeLine.substr(colonIdx + 2, 18);

                            if (existingTypes.find(originalSignature) != existingTypes.end())
                            {
                                return std::make_pair(signature, "pointer to " + existingTypes[originalSignature]);
                            }
                        }
                    }
                    else if (typeStr.substr(0, 14) == "User TAG value")
                    {
                        int secondSignature = FindToken(typeLines, "signature", 1);

                        if (secondSignature != -1)
                        {
                            auto orignalTypeLine = typeLines[secondSignature];

                            int colonIdx = orignalTypeLine.find_last_of(":");

                            auto originalSignature = orignalTypeLine.substr(colonIdx + 2, 18);

                            if (existingTypes.find(originalSignature) != existingTypes.end())
                            {
                                return std::make_pair(signature, existingTypes[originalSignature]);
                            }
                        }
                    }
                    else if (typeStr == "DW_TAG_structure_type")
                    {
                        auto structureAddr = ExtractStructureAddress(typeLines, existingTypes);

                        return std::make_pair(signature, "DW_TAG_structure_type: " + structureAddr);
                    }

                    return std::make_pair(signature, typeStr);
                }
            }
        }
    }
    return ret;
}

std::pair<std::string, std::string> ExtractNameLocation(std::vector<std::string> &unit, std::map<std::string, std::string> &existingTypes)
{
    int locationIndex = FindToken(unit, "DW_AT_location", 1);
    int nameIndex = FindToken(unit, "DW_AT_name", 1);
    int typeIndex = FindToken(unit, "DW_AT_type", 1);

    auto locationLine = unit[locationIndex];
    auto nameLine = unit[nameIndex];
    auto typeLine = unit[typeIndex];


    auto name = ExtractName(nameLine);
    auto location = ExtractLocation(locationLine);
    auto type = ExtractType(typeLine);

    if (existingTypes.find(type) != existingTypes.end() && IsSubstr(existingTypes[type], "DW_TAG_structure_type"))
    {
        auto locationStr = std::string(existingTypes[type]);

        auto startIdx = FindSubstr(locationStr, "struct_start_addr");

        if (startIdx != -1)
        {
            locationStr.replace(startIdx, std::string("struct_start_addr").size(), location);
        }

        auto exactLoc = SubstituteLocation(locationStr, "DW_OP_addr: ", "0x");

        return std::make_pair(name, exactLoc);
    }

    return std::make_pair(name, SubstituteLocation(location, "DW_OP_addr: ", "0x"));
}

std::string ExtractName(std::string nameLine)
{
    auto idx = nameLine.find_last_of(":");

    if (idx != std::string::npos)
    {
        return TrimString(nameLine.substr(idx + 1, nameLine.end() - nameLine.begin() - idx - 1));
    }

    return "";
}

std::string ExtractLocation(std::string locationLine)
{
    auto startBrace = std::find(locationLine.begin(), locationLine.end(), '(');
    auto endBrace = locationLine.find_last_of(")");
    auto begin = locationLine.begin();

    if (startBrace != locationLine.end() && begin + endBrace != locationLine.end()
        && begin + endBrace - startBrace > 0)
    {
        auto location = locationLine.substr(startBrace - locationLine.begin() + 1, begin + endBrace - startBrace - 1);
        return location;
    }

    return "";
}

std::string ExtractType(std::string typeLine)
{
    int colonIdx = typeLine.find_last_of(":");

    return typeLine.substr(colonIdx + 2, typeLine.size() - colonIdx - 2);
}

std::vector<std::vector<std::string>> ExtractUnitInOrder(std::vector<std::string>& lines, std::string token)
{
    std::vector<std::vector<std::string>> compilationUnits;
    auto start = lines.begin();

    // try to find "Contents of the" line 
    while ((start = std::find_if(start, lines.end(),
        [&token](std::string line)
        {
            return IsSubstr(line, token);
        })) != lines.end())
    {
        auto same = std::find_if(start + 1, lines.end(), [&token](std::string line) { return IsSubstr(line, token); });

        auto block = std::vector<std::string>(same - start - 1);
        std::copy(start + 1, same, block.begin());

        compilationUnits.push_back(block);
        start = same;
    }

    return compilationUnits;
}
/*
*
*
*
* output: member:addr member:addr ... member:addr
*
*/
std::string ExtractStructureAddress(std::vector<std::string>& typeLines, std::map<std::string, std::string>& existingTypes)
{
    auto units = ExtractUnitInOrder(typeLines, "DW_TAG_member");
    auto ret = std::string("");

    for (auto it = units.begin(); it != units.end(); ++it)
    {
        auto nameIdx = FindToken(*it, "DW_AT_name", 1);

        if (nameIdx != -1)
        {
            auto nameLine = (*it)[nameIdx];

            auto colonIdx = nameLine.find_last_of(":");

            auto name = nameLine.substr(colonIdx + 2, nameLine.size() - colonIdx - 2);

            if (it == units.begin())
            {
                ret += name + ":" + "struct_start_addr ";
            }
            else
            {
                auto locationIdx = FindToken(*it, "DW_AT_data_member_location", 1);

                if (locationIdx != -1)
                {
                    auto locationLine = (*it)[locationIdx];
                    auto startBrace = std::find(locationLine.begin(), locationLine.end(), '(');
                    auto endBrace = std::find(locationLine.begin(), locationLine.end(), ')');

                    if (startBrace != locationLine.end() && endBrace != locationLine.end() && endBrace - startBrace > 0)
                    {
                        auto locationStr = locationLine.substr(startBrace - locationLine.begin() + 1, endBrace - startBrace - 1);

                        ret += name + ":" + locationStr + " ";
                    }
                }
            }
        }
    }
    return ret;
}

int FindSubstr(std::string s, std::string substr)
{
    if (s.size() < substr.size())
    {
        return false;
    }

    int big = s.size();
    int small = substr.size();
    int start = 0;
    while (big - small >= 0)
    {
        if (s.substr(start, small).compare(substr) == 0)
        {
            return start;
        }

        start++;
        big--;
    }

    return -1;
}

std::string SubstituteLocation(std::string &locationStr, std::string oldValue, std::string newValue)
{
    if (locationStr.size() < oldValue.size())
    {
        return locationStr;
    }

    int small = oldValue.size();
    int start = 0;

    auto ret = std::string(locationStr);

    while (start + small <= ret.size())
    {
        if (ret.substr(start, small).compare(oldValue) == 0)
        {
            ret.replace(start, small, newValue);
            start += small;
        }
        else
        {
            start++;
        }
    }

    return ret;
}




