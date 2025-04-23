#include <filestorm/ioengines/factory.h>
#include <filestorm/result.h>
#include <filestorm/scenarios/scenario.h>
#include <filestorm/utils/logger.h>
#include <fmt/format.h>

#include <filestorm/external/cxxopts.hpp>
#include <iostream>
Scenario::Scenario() {
  addParameter(Parameter("h", "help", "Show help", "false", false));
  addParameter(Parameter("", "save-to", "Save results to a file", "results.json", true));
  // addParameter(Parameter("", "engine", fmt::format("I/O engine to use available engines:\n{}", engine_list), "sync", true));
}

Scenario::~Scenario() {}

void Scenario::setup(int argc, char** argv) {
  cxxopts::Options options("filestorm", fmt::format("Scenario: {}", name()));
  options.custom_help(fmt::format("{} [Options...]", name()));
  for (auto parameter : parameters()) {
    if (parameter.has_value())
      if (parameter.short_name().empty())
        options.add_options()(parameter.long_name(), parameter.description(), cxxopts::value<std::string>()->default_value(parameter.value()));
      else
        options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description(), cxxopts::value<std::string>()->default_value(parameter.value()));
    else
      options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description());
  }

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
  // set values in parameters from parsed result
  for (auto& parameter : _parameters) {
    if (result.count(parameter.long_name())) {
      std::string value = result[parameter.long_name()].as<std::string>();
      logger.debug("Setting {} to {}", parameter.long_name(), value);
      parameter.value(value);
    }
  }
}

void Scenario::run() {}

void Scenario::save() {
  auto filename = getParameter("save-to").get_string();
  logger.info("Saving results to {}", filename);
  Result::save(filename);
}

void Scenario::print() { Result::print(); }

Parameter Scenario::getParameter(const std::string& name) const {
  for (auto parameter : parameters()) {
    if (parameter.long_name() == name) {
      return parameter;
    }
  }
  throw std::invalid_argument(fmt::format("Parameter {} not found.", name));
}

void Scenario::addParameter(Parameter parameter) {
  for (auto& p : _parameters) {
    if (p.long_name() == parameter.long_name() || (p.short_name() == parameter.short_name() && !p.short_name().empty())) {
      throw std::invalid_argument(fmt::format("Parameter {} already exists.", parameter.long_name()));
    }
  }
  if (parameter.long_name().empty()) {
    logger.warn("Parameter long name should not be empty.");
  }
  if (parameter.description().empty()) {
    logger.warn("Parameter description should not be empty.");
  }
  _parameters.push_back(parameter);
}
