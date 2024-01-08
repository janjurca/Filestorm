#pragma once

#include <filestorm/actions/actions.h>

#include <string>

class Parameter {
protected:
  std::string _short_name;
  std::string _long_name;
  std::string _description;
  std::string _value;
  bool _has_value;

public:
  Parameter(std::string short_name, std::string long_name, std::string description, std::string value, bool has_value = true)
      : _short_name(short_name), _long_name(long_name), _description(description), _value(value), _has_value(has_value){};
  ~Parameter(){};
  std::string short_name() const { return _short_name; };
  std::string long_name() const { return _long_name; };
  std::string description() const { return _description; };
  std::string value() const { return _value; };
  std::string value(const std::string& value) { return _value = value; };
  bool has_value() const { return _has_value; };
  int get_int() const { return std::stoi(_value); };
  bool get_bool() const { return _value == "true" || _value == "True" || _value == "t" || _value == "T"; };
  double get_double() const { return std::stod(_value); }
  std::string get_string() const { return _value; };
  bool is_set() const { return _value != ""; };
};

template <> struct fmt::formatter<Parameter> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext> auto format(const Parameter& p, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "Parameter {{ short_name: {}, long_name: {}, description: {}, value: {}, has_value: {} }}", p.short_name(), p.long_name(), p.description(), p.value(),
                          p.has_value());
  }
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
  const std::vector<Parameter>& parameters() const { return _parameters; };
  std::string description() const { return _description; };
  void setup(int argc, char** argv);
  void addParameter(Parameter parameter) { _parameters.push_back(parameter); };
  Parameter getParameter(const std::string& name) const;
  virtual void run();
};

class BasicScenario : public Scenario {
public:
  BasicScenario();
  ~BasicScenario();
  void run() override;
};

class AgingScenario : public Scenario {
public:
  AgingScenario();
  ~AgingScenario();
  void run() override;
};
