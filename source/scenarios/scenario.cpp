#include <filestorm/scenarios/scenario.h>
#include <fmt/format.h>

#include <filestorm/cxxopts.hpp>
#include <iostream>

Scenario::Scenario() { addParameter(Parameter("h", "help", "Show help", "false", false)); }

Scenario::~Scenario() {}

void Scenario::setup(int argc, char** argv) {
  cxxopts::Options options("filestorm", fmt::format("Scenario: {}", name()));
  options.custom_help(fmt::format("{} [Options...]", name()));
  for (auto parameter : parameters()) {
    if (parameter.has_value())
      options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description(), cxxopts::value<std::string>()->default_value(parameter.default_value()));
    else
      options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description());
  }

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
}