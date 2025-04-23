#pragma once
#include <filestorm/utils/logger.h>
#include <fmt/format.h>

#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

class Parameter {
protected:
  std::string _short_name;
  std::string _long_name;
  std::string _description;
  std::string _value;
  bool _has_value;
  std::function<std::string(std::string)> _on_set;
  std::list<std::string> _categories = {};

public:
  Parameter(std::string short_name, std::string long_name, std::string description, std::string value, bool has_value = true, std::function<std::string(std::string)> on_set = nullptr)
      : _short_name(short_name), _long_name(long_name), _description(description), _value(value), _has_value(has_value), _on_set(on_set) {};
  ~Parameter() {};
  std::string short_name() const { return _short_name; };
  std::string long_name() const { return _long_name; };
  std::string description() const { return fmt::format("{}", _description); };
  std::string value() const { return _value; };
  std::string value(const std::string& value) {
    if (_on_set != nullptr) {
      return _value = _on_set(value);
    }
    return _value = value;
  };
  bool has_value() const { return _has_value; };
  int get_int() const { return std::stoi(_value); };
  bool get_bool() const { return _value == "true" || _value == "True" || _value == "t" || _value == "T"; };
  double get_double() const { return std::stod(_value); }
  std::string get_string() const { return _value; };
  bool is_set() const { return _value != ""; };
  std::list<std::string> categories() const { return _categories; };
  void add_category(const std::string& category) { _categories.push_back(category); };

  bool operator==(const Parameter& other) const {
    return _short_name == other._short_name && _long_name == other._long_name && _description == other._description && _value == other._value && _has_value == other._has_value
           && _categories == other._categories;
  }
  bool operator!=(const Parameter& other) const { return !(*this == other); }
};

template <> struct fmt::formatter<Parameter> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext> auto format(const Parameter& p, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "Parameter {{ short_name: {}, long_name: {}, description: {}, value: {}, has_value: {} }}", p.short_name(), p.long_name(), p.description(), p.value(),
                          p.has_value());
  }
};
