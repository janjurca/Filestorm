#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/version.h>
#include <spdlog/spdlog.h>

#include <filestorm/cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include "config.h"

auto main(int argc, char** argv) -> int {
  cxxopts::Options options(*argv, "filestorm - modern metadata extensive storage benchmaring tool");
  // print options
  spdlog::set_level(spdlog::level::debug);
  spdlog::debug("Runninf filestorm with options:");
  for (int i = 0; i < argc; i++) {
    spdlog::debug(" {}: {}", i, argv[i]);
  }

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("f,file", "File name", cxxopts::value<std::string>())
    ("s,scenario",std::string("Define which testing mode you'd like to run. Supported: ") + config.get_supported_scenarios_as_string(), cxxopts::value<std::string>()->default_value(config.get_supported_scenarios().at(0).name()) )
  ;

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>()) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result["version"].as<bool>()) {
    std::cout << FILESTORM_VERSION << std::endl;
    return 0;
  }

  if (!result.count("file")) {
    std::cout << "Please specify directory to test in" << std::endl;
    return 1;
  }

  auto file = result["file"].as<std::string>();  

  WriteAction wa(std::chrono::milliseconds(1000), [](VirtualMonitoredAction* action) {
    auto tuple = action->get_monitored_data().at("write_bytes").back();
    auto duration = std::get<0>(tuple);
    auto value = std::get<1>(tuple);

    static auto last_value = 0.0f;
    static auto last_duration = std::chrono::nanoseconds(0);

    auto diff = value - last_value;
    auto duration_diff = duration - last_duration;

    last_value = value;
    last_duration = duration;

    auto speed = diff / duration_diff.count(); // bytes per nanosecond
    auto speed_mb_s = speed * 1000 * 1000 *1000; // bytes per second
    auto speed_mb = speed_mb_s / 1024 / 1024; // megabytes per second 

    spdlog::debug("WriteAction::on_log: {} MB/s", speed_mb);
    
  }, FileActionAttributes(true, DataSize<DataUnit::KB>(64), DataSize<DataUnit::GB>(3) , file, std::chrono::seconds(10)));

  wa.exec();

  return 0;
}
