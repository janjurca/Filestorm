#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/version.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include "config.h"

auto main(int argc, char** argv) -> int {
  cxxopts::Options options(*argv, "Filestorm - modern metadata extensive storage benchmaring tool");

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("f,file", "Specify file which should be tested")
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

  WriteAction wa(std::chrono::milliseconds(100), [](VirtualMonitoredAction* action) {
    std::cout << "WriteAction: " << action->get_monitored_data().at("write_bytes").at(0) << std::endl;
  }, FileActionAttributes(true, 4096, 4096*1024,  result["file"].as<std::string>()));

  return 0;
}
