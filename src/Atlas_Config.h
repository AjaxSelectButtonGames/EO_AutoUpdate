#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <cstddef>
#include <string>
#include <algorithm>  // For trimming whitespace

class Config : public std::map<std::string, std::map<std::string, std::string>>
{
protected:
    // Config file name
    std::string filename;

public:
    // Max line length in config file
    static const std::size_t MaxLineLength = 4096;

    Config() {}

    Config(const std::string& filename) { Read(filename); }

    bool Read(const std::string& filename, bool nowarn = false);

private:
    // Helper function to trim whitespace
    static std::string trim(const std::string& str);
};
