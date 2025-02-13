#include <filestorm/utils.h>

#include <algorithm>
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

std::string toLower(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string toUpper(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;
}

double ceilTo(double value, int decimals) {
  double factor = std::pow(10, decimals);
  return std::ceil(value * factor) / factor;
}

double floorTo(double value, int decimals) {
  double factor = std::pow(10, decimals);
  return std::floor(value * factor) / factor;
}

std::chrono::seconds stringToChrono(const std::string& timeString) {
  char unit = timeString.back();                                         // Get the last character as unit
  std::string numberPart = timeString.substr(0, timeString.size() - 1);  // Extract numeric part
  int timeValue = std::stoi(numberPart);                                 // Convert to integer

  switch (unit) {
    case 'h':  // Hours
      return std::chrono::hours(timeValue);
    case 'm':  // Minutes
      return std::chrono::minutes(timeValue);
    case 's':  // Seconds
      return std::chrono::seconds(timeValue);
    default:
      throw std::invalid_argument("Unsupported time unit");
  }
}

double d_rand(double dMin, double dMax) {
  // Generate a random double between dMin and dMax
  return dMin + (dMax - dMin) * (std::rand() / (RAND_MAX + 1.0));
}

void generate_random_chunk(char* chunk, size_t size) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 255);

  std::generate_n(chunk, size, []() { return dis(gen); });
}
