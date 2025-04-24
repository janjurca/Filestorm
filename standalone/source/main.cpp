#include <filestorm/actions/actions.h>
#include <filestorm/actions/rw_actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/ioengines/factory.h>
#include <filestorm/utils/logger.h>
#include <filestorm/version.h>
#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "config.h"
#include "interupts.h"

void displayHelp() {
  std::cout << "Usage: filestorm [-hvls] IOENGINE [engine params] SCENARIO [scenario params]\n\n"
            << "  IOENGINE: The IO engine to use, available:" << std::endl;

  for (const auto& scenario : IOEngineFactory::instance().listEngines()) {
    auto scenario_ptr = IOEngineFactory::instance().create(scenario);
    if (!scenario_ptr) {
      std::cerr << "Error creating IO engine: " << scenario << std::endl;
      continue;
    }
    std::cout << "    " << scenario_ptr->name() << " - " << scenario_ptr->description() << std::endl;
  }

  std::cout << std::endl << "  SCENARIO: The scenario to run, available:" << std::endl;
  for (const auto& scenario : Config::instance().listScenarios()) {
    auto scenario_ptr = Config::instance().createScenario(scenario);
    if (!scenario_ptr) {
      std::cerr << "Error creating scenario: " << scenario << std::endl;
      continue;
    }
    std::cout << "    " << scenario_ptr->name() << " - " << scenario_ptr->description() << std::endl;
  }
  std::cout << "\nOptions:\n"
            << "  -h, --help     Display this help message\n"
            << "  -v, --version  Display version information\n"
            << "  -l, --log level     Set the log level (trace, debug, info, warn, error, critical, off) (default info)\n"
            << "  -s, --seed seed     Set the seed for the random number generator (default 42)" << std::endl;
}

void displayVersion() { std::cout << FILESTORM_VERSION << std::endl; }

auto main(int argc, char** argv) -> int {
  spdlog::set_level(spdlog::level::info);
  const struct option long_options[]
      = {{"help", no_argument, NULL, 'h'}, {"version", no_argument, NULL, 'v'}, {"log", required_argument, NULL, 'l'}, {"seed", required_argument, NULL, 's'}, {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "+hvl:s:", long_options, nullptr)) != -1) {
    switch (opt) {
      case 'h':
        displayHelp();
        return 0;
      case 'v':
        std::cout << FILESTORM_VERSION << "\n";
        return 0;
      case 'l':
        spdlog::set_level(spdlog::level::from_str(optarg));
        break;
      case 's':
        srand(std::atoi(optarg));
        break;
      case '?': /* unknown global, fall through */
        break;
      default:
        break;
    }
  }

  // 2) make lists of valid names
  auto engineNames = IOEngineFactory::instance().listEngines();
  auto scenarioNames = Config::instance().listScenarios();

  // 3) grab IOENGINE name + its flags
  if (optind >= argc) {
    logger.error("Missing IOENGINE");
    displayHelp();
    return 1;
  }
  std::string engineName = argv[optind++];
  if (std::find(engineNames.begin(), engineNames.end(), engineName) == engineNames.end()) {
    logger.error("Unknown IO engine “{}”", engineName);
    displayHelp();
    return 1;
  }
  std::vector<char*> engineArgv;
  engineArgv.push_back(const_cast<char*>(engineName.c_str()));
  while (optind < argc && std::find(scenarioNames.begin(), scenarioNames.end(), argv[optind]) == scenarioNames.end()) {
    engineArgv.push_back(argv[optind++]);
  }
  int engineArgc = (int)engineArgv.size();

  auto ioengine = IOEngineFactory::instance().create(engineName);
  if (!ioengine) {
    logger.error("Failed to create IO engine “{}”", engineName);
    return 1;
  }
  auto ioengine_help = ioengine->setup(engineArgc, engineArgv.data());
  if (!ioengine_help.empty()) {
    displayHelp();
    std::cout << "\n" << ioengine_help << std::endl;
    return 0;
  }

  // 4) grab SCENARIO name + its flags
  if (optind >= argc) {
    logger.error("Missing SCENARIO");
    displayHelp();
    return 1;
  }
  std::string scenarioName = argv[optind++];
  if (std::find(scenarioNames.begin(), scenarioNames.end(), scenarioName) == scenarioNames.end()) {
    logger.error("Unknown scenario “{}”", scenarioName);
    return 1;
  }
  std::vector<char*> scenarioArgv;
  scenarioArgv.push_back(const_cast<char*>(scenarioName.c_str()));
  while (optind < argc) {
    scenarioArgv.push_back(argv[optind++]);
  }
  int scenarioArgc = (int)scenarioArgv.size();

  auto scenario = Config::instance().createScenario(scenarioName);
  scenario->setup(scenarioArgc, scenarioArgv.data());

  std::signal(SIGINT, sigint_handler);
  scenario->run(ioengine);
  std::signal(SIGINT, SIG_DFL);

  scenario->save();
  scenario->print();
  return 0;
}
