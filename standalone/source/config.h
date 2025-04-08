#pragma once

#include <filestorm/scenarios/scenario.h>

#include <iostream>
#include <stdexcept>
#include <vector>

class Config {
private:
  const std::vector<Scenario*> supported_scenarios = {
      new BasicScenario(),
      new AgingScenario(),
  };

public:
  Config();
  const std::vector<Scenario*> get_supported_scenarios() const;
  const std::string get_supported_scenarios_as_string() const;
  Scenario* get_and_set_scenario(const std::string& name) const;
  static Scenario* selected_senario;
};

extern const Config config;

// Custom exception class

class BadScenarioSelected : public std::runtime_error {
public:
  BadScenarioSelected(const char* message) : std::runtime_error(message) {}
  BadScenarioSelected(const std::string& message) : std::runtime_error(message) {}
};
