#include "config.h"

#include <fmt/format.h>

const Config config;

Config::Config() {}

const std::vector<Scenario*> Config::get_supported_scenarios() const { return supported_scenarios; }

const std::string Config::get_supported_scenarios_as_string() const {
  std::string result = "";
  for (auto scenario : supported_scenarios) {
    result += fmt::format("{}, ", scenario->name());
  }
  return result;
}

Scenario* Config::get_scenario(const std::string& name) const {
  for (auto scenario : supported_scenarios) {
    if (scenario->name() == name) {
      return scenario;
    }
  }
  throw BadScenarioSelected(fmt::format("Scenario {} not found. Supported scenarios are {}", name, get_supported_scenarios_as_string()));
}