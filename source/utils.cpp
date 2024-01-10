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