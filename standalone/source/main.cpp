#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/version.h>
#include <spdlog/spdlog.h>
#include <unistd.h>  // Include for getopts

#include <iostream>
#include <string>
#include <unordered_map>

#include "config.h"

void displayHelp() {
  std::cout << "Usage: filestorm SCENARIO\n"
            << "  SCENARIO: The scenario to run, available:" << std::endl;
  for (const auto& scenario : config.get_supported_scenarios()) {
    std::cout << "    " << scenario.name() << " - " << scenario.description() << std::endl;
  }
  std::cout << "Options:\n"
            << "  -h, --help     Display this help message\n"
            << "  -v, --version  Display version information\n";
}

void displayVersion() { std::cout << FILESTORM_VERSION << std::endl; }

auto main(int argc, char** argv) -> int {
  spdlog::set_level(spdlog::level::debug);
  int option;
  while ((option = getopt(argc, argv, "hv")) != -1) {
    switch (option) {
      case 'h':
        displayHelp();
        return 0;
      case 'v':
        displayVersion();
        return 0;
      default:
        spdlog::error("Error: Invalid option.");
        displayHelp();
        return 1;
    }
  }

  if (optind >= argc) {
    spdlog::error("Error: Insufficient arguments.");
    displayHelp();
    return 1;
  }

  std::string positionalArgument = argv[optind];
  try {
    auto scenario = config.get_scenario(positionalArgument);
    int new_argc = argc - optind;
    char** new_argv = argv + optind;
    scenario.setup(new_argc, new_argv);
  } catch (const std::invalid_argument& e) {
    spdlog::error("Error: {}", e.what());
    return 1;
  }

  return 0;
}
