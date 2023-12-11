#pragma once

#include <filestorm/actions/actions.h>

#include <string>

class Scenario {
private:
  std::string _name;

public:
  Scenario(std::string name);
  ~Scenario();
  std::string name() const;
};
