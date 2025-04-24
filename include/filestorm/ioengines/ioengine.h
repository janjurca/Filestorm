#pragma once
#include <filestorm/parameter.h>
#include <filestorm/utils/logger.h>
#include <fmt/format.h>

#include <filestorm/external/cxxopts.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class IOEngine {
protected:
  std::vector<Parameter> _parameters = {};
  std::string _name = "unknown";
  std::string _description = "No description available";

public:
  virtual ~IOEngine() = default;
  virtual std::string name() const = 0;
  virtual void read() = 0;
  virtual void write() = 0;
  virtual std::vector<Parameter> parameters() const { return _parameters; };
  virtual std::string description() const { return _description; };
  virtual void addParameter(Parameter parameter) { _parameters.push_back(parameter); };
  virtual std::string setup(int argc, char** argv) {
    cxxopts::Options options("filestorm", fmt::format("IO Engine: {}", name()));
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
      return options.help();
    }
    // set values in parameters from parsed result
    for (auto& parameter : _parameters) {
      if (result.count(parameter.long_name())) {
        std::string value = result[parameter.long_name()].as<std::string>();
        logger.debug("Setting {} to {}", parameter.long_name(), value);
        parameter.value(value);
      }
    }
    return "";
  };
};
