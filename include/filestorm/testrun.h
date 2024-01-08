#pragma once

#include <filestorm/scenarios/scenario.h>

class Testrun {
private:
  Scenario scenario;

public:
  Testrun(Scenario scenario);
  ~Testrun();
};
