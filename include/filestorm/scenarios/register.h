#pragma once

#include <filestorm/scenarios/factory.h>

#include <iostream>  // For debug logging

#define REGISTER_SCENARIO(ScenarioClass)                                                                                                                       \
  namespace {                                                                                                                                                  \
    struct ScenarioClass##Registrator {                                                                                                                        \
      ScenarioClass##Registrator() {                                                                                                                           \
        ScenarioFactory::instance().registerScenario(ScenarioClass().name(), []() -> std::unique_ptr<Scenario> { return std::make_unique<ScenarioClass>(); }); \
      }                                                                                                                                                        \
    };                                                                                                                                                         \
    static ScenarioClass##Registrator global_##ScenarioClass##_registrator;                                                                                    \
  }
