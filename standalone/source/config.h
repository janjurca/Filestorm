#pragma once

#include <filestorm/scenarios/scenario.h>
#include <filestorm/testrun.h>

#include <vector>

class Config {
private:
  const std::vector<Scenario> supported_scenarios = {
      BasicScenario(),
      AgingScenario(),
  };

public:
  Config();
  const std::vector<Scenario> get_supported_scenarios() const;
  const std::string get_supported_scenarios_as_string() const;
  const Scenario get_scenario(const std::string& name) const;
};

extern const Config config;