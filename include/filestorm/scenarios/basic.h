#pragma once

#include <filestorm/scenarios/register.h>
#include <filestorm/scenarios/scenario.h>

class BasicScenario : public Scenario {
public:
  BasicScenario();
  ~BasicScenario();
  void run(std::unique_ptr<IOEngine>& ioengine) override;
  void save() override;
};

REGISTER_SCENARIO(BasicScenario);