#pragma once

#include <algorithm>  // Make sure this is included
#include <random>
#include <random>  // Assuming you're using random number generation
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);
std::string strip(const std::string& str);
std::string strip(const std::string& str, const char stripChar = ' ');

std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);

inline void generate_random_chunk(char* chunk, size_t size) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 255);

  std::generate_n(chunk, size, []() { return dis(gen); });
}

double ceilTo(double value, int decimals);