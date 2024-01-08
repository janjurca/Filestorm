#pragma once

#include <filestorm/actions/actions.h>

#include <string>

class Parameter {
protected:
  std::string _short_name;
  std::string _long_name;
  std::string _description;
  std::string _default_value;
  bool _has_value;

public:
  Parameter(std::string short_name, std::string long_name, std::string description, std::string default_value, bool has_value = true)
      : _short_name(short_name), _long_name(long_name), _description(description), _default_value(default_value), _has_value(has_value){};
  ~Parameter(){};
  std::string short_name() const { return _short_name; };
  std::string long_name() const { return _long_name; };
  std::string description() const { return _description; };
  std::string default_value() const { return _default_value; };
  bool has_value() const { return _has_value; };
};

class Scenario {
protected:
  std::vector<Parameter> _parameters = {};
  std::string _name = "unknown";
  std::string _description = "No description available";

public:
  Scenario();
  ~Scenario();
  std::string name() const { return _name; };
  std::vector<Parameter> parameters() const { return _parameters; };
  std::string description() const { return _description; };
  void setup(int argc, char** argv);
  void addParameter(Parameter parameter) { _parameters.push_back(parameter); };
};

class BasicScenario : public Scenario {
public:
  BasicScenario();
  ~BasicScenario();
};

class AgingScenario : public Scenario {
public:
  AgingScenario();
  ~AgingScenario();
};
