#pragma once

#include <filestorm/scenarios/scenario.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ScenarioFactory {
public:
  using Creator = std::function<std::unique_ptr<Scenario>()>;

  static ScenarioFactory& instance();

  void registerScenario(const std::string& name, Creator creator);
  std::unique_ptr<Scenario> create(const std::string& name);
  std::vector<std::string> listScenarios() const;

private:
  std::unordered_map<std::string, Creator> creators;
};
