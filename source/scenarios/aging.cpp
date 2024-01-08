#include <filestorm/scenarios/scenario.h>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("d", "directory", "", "/tmp/filestorm/"));
  addParameter(Parameter("r", "depth", "Max directory depth", "5"));
  addParameter(Parameter("n", "ndirs", "Max number of dirs per level", "false"));
  addParameter(Parameter("f", "nfiles", "Max number of files per level", "false"));
  addParameter(Parameter("m", "fs-capacity", "Max overall filesystem size", "full"));
  addParameter(Parameter("s", "fsize", "Max file size", "full"));
  addParameter(Parameter("p", "sdist", "File size probabilistic distribution", "random"));
  addParameter(Parameter(std::string(), "capacity-awareness", "File size probabilistic distribution", "random"));
}

AgingScenario::~AgingScenario() {}

void AgingScenario::run() {}