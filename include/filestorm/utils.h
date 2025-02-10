#pragma once

#include <algorithm>  // Make sure this is included
#include <chrono>
#include <random>
#include <random>  // Assuming you're using random number generation
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);
std::string strip(const std::string& str);
std::string strip(const std::string& str, const char stripChar);

std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);

void generate_random_chunk(char* chunk, size_t size);
double ceilTo(double value, int decimals);
double floorTo(double value, int decimals);
std::chrono::seconds stringToChrono(const std::string& timeString);

double d_rand(double dMin, double dMax);