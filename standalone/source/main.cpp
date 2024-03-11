#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/version.h>
#include <getopt.h>
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
    std::cout << "    " << scenario->name() << " - " << scenario->description() << std::endl;
  }
  std::cout << "Options:\n"
            << "  -h, --help     Display this help message\n"
            << "  -v, --version  Display version information\n"
            << "  -l, --log level     Set the log level (trace, debug, info, warn, error, critical, off)" << std::endl;
}

void displayVersion() { std::cout << FILESTORM_VERSION << std::endl; }

auto main(int argc, char** argv) -> int {
  spdlog::set_level(spdlog::level::debug);

  if (optind >= argc) {
    spdlog::error("Error: Insufficient arguments.");
    displayHelp();
    return 1;
  }

  const struct option long_options[] = {{"help", no_argument, NULL, 'h'}, {"version", no_argument, NULL, 'v'}, {"log", required_argument, NULL, 'l'}, {NULL, 0, NULL, 0}};
  int opt;
  while ((opt = getopt_long(argc, argv, "+hvl:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'h':
        // Display help message
        displayHelp();
        return 0;
      case 'v':
        // Display version information
        displayVersion();
        return 0;
      case 'l':
        // Set the log level
        spdlog::set_level(spdlog::level::from_str(optarg));
        break;
      case '?':
        // Handle unknown options or missing arguments
        break;
      default:
        break;
    }
  }
  std::string positionalArgument = argv[optind];
  try {
    auto scenario = config.get_scenario(positionalArgument);
    int new_argc = argc - optind;
    char** new_argv = argv + optind;
    for (size_t i = 0; i < new_argc; i++) {
      spdlog::debug("new_argv[{}]: {}", i, new_argv[i]);
    }

    scenario->setup(new_argc, new_argv);
    scenario->run();
    scenario->save();
  } catch (const BadScenarioSelected& e) {
    spdlog::error("Error on main: {}", e.what());
    return 1;
  }

  return 0;
}
