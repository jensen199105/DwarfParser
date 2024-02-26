// DwarfParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>


std::map <std::string, std::vector<std::string>> SplitCategory(std::vector<std::string>& lines);


int main()
{
    std::ifstream dwarfFile("C:\\Users\\borez\\Downloads\\w_option_export");
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(dwarfFile, line))
    {
        lines.push_back(line);
    }

    std::cout << lines.size() << std::endl;
    dwarfFile.close();

    return 0;
}

std::map<std::string, std::vector<std::string>> SplitCategory(std::vector<std::string>& lines)
{
    std::map<std::string, std::vector<std::string>> debugCategory;
    auto token = std::string("Contents of the");
    auto start = lines.begin();

    // try to find "Contents of the" line 
    while ((start = std::find_if(start, lines.end(),
        [&token](std::string line)
        {
            return line.substr(0, token.size()).compare(token) == 0;
        })) != lines.end())
    {
        auto same = std::find_if(start+1, lines.end(), [&token](std::string line) { return line.substr(0, token.size()).compare(token) == 0; });

        auto block = std::vector<std::string>(same - start - 1);
        std::copy(start + 1, same, block.begin());

        auto trimmedKey = TrimString(*start);
        debugCategory[trimmedKey] = block;

        start = same;
    }

    return debugCategory;
}

static std::string TrimString(std::string line)
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

std::map<string, 

