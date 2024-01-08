#include <filestorm/scenarios/scenario.h>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("s", "size", "Size of the file to be created", "1M"));
  addParameter(Parameter("d", "duration", "Duration of the testrun", "1h"));
  addParameter(Parameter("r", "read", "Read the file after creation", "false"));
}

AgingScenario::~AgingScenario() {}