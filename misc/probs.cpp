#include <iostream>

double CAF(double x) { return sqrt(1 - (x * x)); }

int main() {
  double capacity = 1024;
  double free = 1024;
  while (free > 0) {
    std::cout << "capacity: " << capacity << " free: " << free << " pC= " << CAF((float(capacity - free) / float(capacity))) << std::endl;
    free -= 1;
  }
  return 0;
}