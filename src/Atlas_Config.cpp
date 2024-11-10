#include "Atlas_Config.h"

// Helper function to trim whitespace
std::string Config::trim(const std::string& str) {
    std::string result = str;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), result.end());
    return result;
}

bool Config::Read(const std::string& filename, bool nowarn)
{
    std::ifstream file(filename.c_str());
    if (!file)
    {
        if (!nowarn)
            std::cerr << "Config::Read: can't open file " << filename << std::endl;
        return false;
    }

    this->filename = filename;
    std::string currentSection;
    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        if (line[0] == '[' && line.back() == ']') {
            // Section header
            currentSection = trim(line.substr(1, line.size() - 2));
        }
        else {
            std::istringstream is(line);
            std::string key;
            if (!std::getline(is, key, '='))
                continue;

            std::string value;
            if (!std::getline(is, value))
                continue;

            // Trim whitespace from key and value
            key = trim(key);
            value = trim(value);

            (*this)[currentSection][key] = value;
        }
    }

    return true;
}
