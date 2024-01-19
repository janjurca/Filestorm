#include <filestorm/utils.h>

#include <sstream>

std::vector<std::string> split(const std::string& str, char delimiter) {
  // split the string by the delimiter and return the vector of strings
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    result.push_back(token);
  }
  return result;
}

std::string strip(const std::string& str) {
  size_t start = 0;
  size_t end = str.length();

  // Find the index of the first non-whitespace character from the beginning
  while (start < end && std::isspace(str[start])) {
    ++start;
  }

  // Find the index of the first non-whitespace character from the end
  while (end > start && std::isspace(str[end - 1])) {
    --end;
  }

  // Return the stripped substring
  return str.substr(start, end - start);
}

std::string strip(const std::string& str, const char stripChar) {
  size_t start = 0;
  size_t end = str.length();

  // Find the index of the first non-stripChar character from the beginning
  while (start < end && str[start] == stripChar) {
    ++start;
  }

  // Find the index of the first non-stripChar character from the end
  while (end > start && str[end - 1] == stripChar) {
    --end;
  }

  // Return the stripped substring
  return str.substr(start, end - start);
}
