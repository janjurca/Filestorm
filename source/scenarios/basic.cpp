#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/scenarios/scenario.h>
#include <spdlog/spdlog.h>

BasicScenario::BasicScenario() {
  _name = "basic";
  _description = "Basic scenario for testing rw operations";
  addParameter(Parameter("a", "operation", "What operation to perform. Supported: read, write, randread", "randwrite"));
  addParameter(Parameter("b", "blocksize", "Block size", "4096"));
  addParameter(Parameter("s", "filesize", "File size (Example format: 1g, 512k, 1gb ..)", "1gb"));
  // addParameter(Parameter("t", "threads", "Number of threads", "1"));
  addParameter(Parameter("f", "filename", "File which should be used for testing.", "/tmp/filestorm.tmp"));
  addParameter(Parameter("t", "time", "Time in seconds to run the test", "10"));
}

BasicScenario::~BasicScenario() {}

void BasicScenario::run() {
  std::cout << "Running basic scenario" << std::endl;
  auto attributes = FileActionAttributes(true, DataSize<DataUnit::B>::fromString(getParameter("blocksize").get_string()), DataSize<DataUnit::KB>::fromString(getParameter("filesize").get_string()),
                                         getParameter("filename").get_string(), stringToChrono(getParameter("time").get_string()));
  std::string operation = getParameter("operation").get_string();
  RWAction* action;
  std::function<void(VirtualMonitoredAction*)> log_func = [](VirtualMonitoredAction* action) {
    auto monitored_data = action->get_monitored_data();
    for (const auto& pair : monitored_data) {
      auto key = pair.first;
      if (monitored_data.at(key).size() < 2) {
        continue;
      }
      auto tuple = monitored_data.at(key).back();
      auto prev_tuple = monitored_data.at(key).at(monitored_data.at(key).size() - 2);

      auto duration = std::get<0>(tuple);
      auto value = std::get<1>(tuple);
      auto prev_duration = std::get<0>(prev_tuple);
      auto prev_value = std::get<1>(prev_tuple);

      auto diff = value - prev_value;
      auto duration_diff = duration - prev_duration;

      auto speed = diff / duration_diff.count();     // bytes per nanosecond
      auto speed_mb_s = speed * 1000 * 1000 * 1000;  // bytes per second
      auto speed_mb = speed_mb_s / 1024 / 1024;      // megabytes per second
      spdlog::debug("Action: {} | Value: {} | Duration: {} | Speed: {} MB/s", key, diff, duration_diff.count(), speed_mb);
    }
  };
  if (operation == "read") {
    spdlog::info("Read operation");
    action = new ReadMonitoredAction(std::chrono::milliseconds(1000), log_func, attributes);
  } else if (operation == "write") {
    spdlog::info("Write operation");
    action = new WriteMonitoredAction(std::chrono::milliseconds(1000), log_func, attributes);
  } else if (operation == "randread") {
    spdlog::info("Read-Write operation");
    action = new RandomReadMonitoredAction(std::chrono::milliseconds(1000), log_func, attributes);
  } else if (operation == "randwrite") {
    spdlog::info("Read-Write operation");
    action = new RandomWriteMonitoredAction(std::chrono::milliseconds(1000), log_func, attributes);
  } else {
    spdlog::error("Invalid operation");
    return;
  }
  action->exec();
  delete action;
}
