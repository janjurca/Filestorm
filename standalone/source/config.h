#pragma once

#include <filestorm/scenario.h>
#include <filestorm/testrun.h>

#include <vector>

class Config {
private:
  const std::vector<Scenario> supported_scenarios = {
      Scenario("full"),
      Scenario("partial"),
  };

public:
  Config();
  const std::vector<Scenario> get_supported_scenarios() const;
  const std::string get_supported_scenarios_as_string() const;
};

extern const Config config;