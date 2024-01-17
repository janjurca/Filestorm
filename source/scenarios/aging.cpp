#include <filestorm/filetree.h>
#include <filestorm/scenarios/scenario.h>
#include <filestorm/utils/fs.h>
#include <filestorm/utils/progress_bar.h>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("d", "directory", "", "/tmp/filestorm/"));
  addParameter(Parameter("r", "depth", "Max directory depth", "5"));
  addParameter(Parameter("n", "ndirs", "Max number of dirs per level", "false"));
  addParameter(Parameter("f", "nfiles", "Max number of files per level", "false"));
  addParameter(Parameter("m", "fs-capacity", "Max overall filesystem size", "full"));  // TODO: add support for this
  addParameter(Parameter("s", "fsize", "Max file size", "full"));
  addParameter(Parameter("p", "sdist", "File size probabilistic distribution", "1000"));
  addParameter(Parameter("i", "iterations", "Iterations to run", "1000"));
}

AgingScenario::~AgingScenario() {}

void AgingScenario::run() {
  FileTree tree(getParameter("directory").get_string());

  std::function<double()> pC = [&]() {
    // CAF - Capacity Awareness Factor
    auto fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());
    return CAF(fs_status.capacity / (fs_status.capacity - fs_status.free));
  };
  std::function<double()> pD = [&]() { return 0.1 * (1 / pC()); };
  std::function<double()> pA = [&]() { return 1 - pC() - pD(); };
  std::function<double()> pAM = []() { return 0.1; };
  std::function<double()> pAB = pC;
  std::function<double()> pAS = [&]() { return 1 - pAM() - pAB(); };
  std::function<double()> pDD = []() { return 0.01; };
  std::function<double()> pDF = [&]() { return 1 - pDD(); };
  std::function<double()> pCF = [&]() { return (tree.getDirectoryCount() / getParameter("ndirs").get_int()); };
  std::function<double()> pCD = [&]() { return 1 - pCF(); };
  std::function<double()> p1 = [=]() { return 1.0; };

  std::map<std::string, State> states;
  states.emplace("S", State("S"));
  states.emplace("CREATE", State("CREATE"));
  states.emplace("ALTER", State("ALTER"));
  states.emplace("DELETE", State("DELETE"));
  states.emplace("CREATE_FILE", State("CREATE_FILE"));
  states.emplace("CREATE_DIR", State("CREATE_DIR"));
  states.emplace("ALTER_SMALLER", State("ALTER_SMALLER"));
  states.emplace("ALTER_BIGGER", State("ALTER_BIGGER"));
  states.emplace("DELETE_FILE", State("DELETE_FILE"));
  states.emplace("DELETE_DIR", State("DELETE_DIR"));
  states.emplace("END", State("END"));

  std::map<std::string, Transition> transtions;
  transtions.emplace("S->CREATE", Transition(states.at("S"), states.at("CREATE"), pC));
  transtions.emplace("S->ALTER", Transition(states.at("S"), states.at("ALTER"), pA));
  transtions.emplace("S->DELETE", Transition(states.at("S"), states.at("DELETE"), pD));
  transtions.emplace("CREATE->CREATE_FILE", Transition(states.at("CREATE"), states.at("CREATE_FILE"), pCF));
  transtions.emplace("CREATE->CREATE_DIR", Transition(states.at("CREATE"), states.at("CREATE_DIR"), pCD));
  transtions.emplace("ALTER->ALTER_SMALLER", Transition(states.at("ALTER"), states.at("ALTER_SMALLER"), pAS));
  transtions.emplace("ALTER->ALTER_BIGGER", Transition(states.at("ALTER"), states.at("ALTER_BIGGER"), pAB));
  transtions.emplace("DELETE->DELETE_FILE", Transition(states.at("DELETE"), states.at("DELETE_FILE"), pDF));
  transtions.emplace("DELETE->DELETE_DIR", Transition(states.at("DELETE"), states.at("DELETE_DIR"), pDD));
  transtions.emplace("CREATE_FILE->END", Transition(states.at("CREATE_FILE"), states.at("END"), p1));
  transtions.emplace("CREATE_DIR->END", Transition(states.at("CREATE_DIR"), states.at("END"), p1));
  transtions.emplace("ALTER_SMALLER->END", Transition(states.at("ALTER_SMALLER"), states.at("END"), p1));
  transtions.emplace("ALTER_BIGGER->END", Transition(states.at("ALTER_BIGGER"), states.at("END"), p1));
  transtions.emplace("DELETE_FILE->END", Transition(states.at("DELETE_FILE"), states.at("END"), p1));
  transtions.emplace("DELETE_DIR->END", Transition(states.at("DELETE_DIR"), states.at("END"), p1));
  transtions.emplace("END->S", Transition(states.at("END"), states.at("S"), p1));

  int iteration = 0;
  progressbar bar(getParameter("iterations").get_int());

  ProbabilisticStateMachine psm(states, transtions, states.at("S"));

  while (iteration < getParameter("iterations").get_int()) {
    bar.update();
    iteration++;
  }
}