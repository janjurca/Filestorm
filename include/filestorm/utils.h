#pragma once

#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);
std::string strip(const std::string& str);
std::string strip(const std::string& str, const char stripChar = ' ');

std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);
