#pragma once

#include <filestorm/actions/actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/filefrag.h>
#include <filestorm/filetree.h>
#include <filestorm/polycurve.h>
#include <filestorm/utils/psm.h>

#include <list>
#include <map>
#include <string>

class Parameter {
protected:
  std::string _short_name;
  std::string _long_name;
  std::string _description;
  std::string _value;
  bool _has_value;
  std::function<std::string(std::string)> _on_set;

public:
  Parameter(std::string short_name, std::string long_name, std::string description, std::string value, bool has_value = true, std::function<std::string(std::string)> on_set = nullptr)
      : _short_name(short_name), _long_name(long_name), _description(description), _value(value), _has_value(has_value), _on_set(on_set) {};
  ~Parameter() {};
  std::string short_name() const { return _short_name; };
  std::string long_name() const { return _long_name; };
  std::string description() const { return fmt::format("{} (default: {})", _description, _value); };
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
  virtual void save();
};

class BasicScenario : public Scenario {
public:
  BasicScenario();
  ~BasicScenario();
  void run() override;
  void save() override;
};

class AgingScenario : public Scenario {
protected:
  enum States {
    S,
    CREATE,
    ALTER,
    DELETE,
    CREATE_FILE,
    CREATE_FILE_FALLOCATE,
    CREATE_FILE_OVERWRITE,
    CREATE_FILE_READ,
    CREATE_DIR,
    ALTER_SMALLER,
    ALTER_SMALLER_TRUNCATE,
    ALTER_SMALLER_FALLOCATE,
    ALTER_BIGGER,
    ALTER_METADATA,
    DELETE_FILE,
    DELETE_DIR,
    END,
  };

  bool rapid_aging = false;
  void compute_probabilities(std::map<std::string, double>& probabilities, FileTree& tree, PolyCurve& curve);
  int open_file(const char* path, int flags);

public:
  AgingScenario();
  ~AgingScenario();
  void run() override;
  double CAF(double x) { return sqrt(1 - (x * x)); }
  DataSize<DataUnit::B> get_file_size();
  DataSize<DataUnit::B> get_file_size(uint64_t min, uint64_t max, bool safe = true);
  DataSize<DataUnit::B> get_block_size() { return DataSize<DataUnit::B>::fromString(getParameter("blocksize").get_string()); };
};
