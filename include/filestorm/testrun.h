#pragma once

#include <filestorm/scenario.h>

class Testrun {
private:
  Scenario scenario;

public:
  Testrun(Scenario scenario);
  ~Testrun();
};
