#include <filestorm/scenarios/scenario.h>

BasicScenario::BasicScenario() {
  _name = "basic";
  _description = "Basic scenario for testing rw operations";
  addParameter(Parameter("a", "operation", "What operation to perform. Supported: rd, rw, rdrw", "rdwr"));
  addParameter(Parameter("b", "blocksize", "Block size in bytes", "4096"));
  addParameter(Parameter("s", "filesize", "File size (Example format: 1g, 512k, 1gb ..)", "1gb"));
  // addParameter(Parameter("t", "threads", "Number of threads", "1"));
  addParameter(Parameter("f", "filename", "File which should be used for testing.", "/tmp/filestorm.tmp"));
  addParameter(Parameter("t", "time", "Time in seconds to run the test", "10"));
}

BasicScenario::~BasicScenario() {}
