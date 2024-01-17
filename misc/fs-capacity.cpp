#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace std::chrono;

int main() {
  try {
    auto start = high_resolution_clock::now();

    // Replace "C:" with the drive or path you want to check
    fs::space_info spaceInfo = fs::space("/Users/jjurca/Nextcloud");

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    std::cout << "Available space: " << spaceInfo.available << " bytes\n";

    // Convert capacity to gigabytes
    double capacityGB = static_cast<double>(spaceInfo.capacity) / (1 << 30);
    std::cout << "Total space: " << capacityGB << " GB\n";

    // Convert free space to gigabytes
    double freeGB = static_cast<double>(spaceInfo.free) / (1 << 30);
    std::cout << "Free space: " << freeGB << " GB\n";

    std::cout << "Time taken by function: " << duration.count() << " microseconds\n";
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
  }

  return 0;
}
