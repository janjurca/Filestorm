#pragma once

#include <filestorm/actions/actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/filefrag.h>
#include <filestorm/filetree.h>
#include <filestorm/parameter.h>
#include <filestorm/polycurve.h>
#include <filestorm/utils/psm.h>

#include <list>
#include <map>
#include <string>

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
  void addParameter(Parameter parameter);
  Parameter getParameter(const std::string& name) const;
  virtual void run();
  virtual void save();
  virtual void print();
};
