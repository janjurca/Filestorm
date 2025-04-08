#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/utils/logger.h>
#include <filestorm/version.h>
#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>  // Include for getopts

#include <csignal>
#include <iostream>
#include <string>
#include <unordered_map>

#include "config.h"
#include "interupts.h"

void displayHelp() {
  std::cout << "Usage: filestorm SCENARIO\n"
            << "  SCENARIO: The scenario to run, available:" << std::endl;
  for (const auto& scenario : config.get_supported_scenarios()) {
    std::cout << "    " << scenario->name() << " - " << scenario->description() << std::endl;
  }
  std::cout << "Options:\n"
            << "  -h, --help     Display this help message\n"
            << "  -v, --version  Display version information\n"
            << "  -l, --log level     Set the log level (trace, debug, info, warn, error, critical, off) (default info)\n"
            << "  -s, --seed seed     Set the seed for the random number generator (default 42)" << std::endl;
}

void displayVersion() { std::cout << FILESTORM_VERSION << std::endl; }

auto main(int argc, char** argv) -> int {
  spdlog::set_level(spdlog::level::info);

  if (optind >= argc) {
    logger.error("Error: Insufficient arguments.");
    displayHelp();
    return 1;
  }
  srand(42);

  const struct option long_options[]
      = {{"help", no_argument, NULL, 'h'}, {"version", no_argument, NULL, 'v'}, {"log", required_argument, NULL, 'l'}, {"seed", required_argument, NULL, 's'}, {NULL, 0, NULL, 0}};
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
      case 's':
        // Set the seed for the random number generator
        srand(atoi(optarg));
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
    auto scenario = config.get_and_set_scenario(positionalArgument);
    int new_argc = argc - optind;
    char** new_argv = argv + optind;

    scenario->setup(new_argc, new_argv);
    std::signal(SIGINT, sigint_handler);
    scenario->run();
    std::signal(SIGINT, SIG_DFL);
    scenario->save();
  } catch (const BadScenarioSelected& e) {
    logger.error("Error on main: {}", e.what());
    return 1;
  }

  return 0;
}
