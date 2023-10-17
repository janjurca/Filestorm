#pragma once

#include <string>

class Scenario {
private:
  std::string _name;

public:
  Scenario(std::string name);
  ~Scenario();
  std::string name() const;
};
