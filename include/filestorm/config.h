#pragma once

#include <filestorm/scenarios/factory.h>
#include <fmt/format.h>

#include <iostream>
#include <stdexcept>
#include <vector>

class BadScenarioSelected : public std::runtime_error {
public:
  BadScenarioSelected(const char* message) : std::runtime_error(message) {}
  BadScenarioSelected(const std::string& message) : std::runtime_error(message) {}
};

class Config {
public:
  static Config& instance() {
    static Config instance;
    return instance;
  };
  std::vector<std::string> listScenarios() const { return ScenarioFactory::instance().listScenarios(); }
  std::string listScenariosAsString() const {
    std::string result;
    for (const auto& scenario : listScenarios()) {
      result += scenario + ", ";
    }
    return result;
  }
  std::unique_ptr<Scenario> createScenario(const std::string& name) const {
    auto scenario = ScenarioFactory::instance().create(name);
    if (!scenario) {
      throw BadScenarioSelected(fmt::format("Scenario {} not found. Available scenarios: {}", name, listScenariosAsString()));
    }
    return scenario;
  };
};
