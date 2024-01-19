#include <filestorm/filetree.h>
#include <filestorm/scenarios/scenario.h>
#include <filestorm/utils/fs.h>
#include <filestorm/utils/progress_bar.h>
#include <fmt/format.h>

#include <iostream>
#include <string_view>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("d", "directory", "", "/tmp/filestorm/"));
  addParameter(Parameter("r", "depth", "Max directory depth", "5"));
  addParameter(Parameter("n", "ndirs", "Max number of dirs", "200"));
  addParameter(Parameter("f", "nfiles", "Max number of files", "2000"));
  addParameter(Parameter("m", "fs-capacity", "Max overall filesystem size", "full"));  // TODO: add support for this
  addParameter(Parameter("s", "fsize", "Max file size", "full"));
  addParameter(Parameter("p", "sdist", "File size probabilistic distribution", "1000"));
  addParameter(Parameter("i", "iterations", "Iterations to run", "1000"));
}

AgingScenario::~AgingScenario() {}

void AgingScenario::run() {
  FileTree tree(getParameter("directory").get_string(), getParameter("depth").get_int());
  std::map<std::string, Transition> transtions;
  transtions.emplace("S->CREATE", Transition(S, CREATE, "pC"));
  transtions.emplace("S->ALTER", Transition(S, ALTER, "pA"));
  transtions.emplace("S->DELETE", Transition(S, DELETE, "pD"));
  transtions.emplace("CREATE->CREATE_FILE", Transition(CREATE, CREATE_FILE, "pCF"));
  transtions.emplace("CREATE->CREATE_DIR", Transition(CREATE, CREATE_DIR, "pCD"));
  transtions.emplace("ALTER->ALTER_SMALLER", Transition(ALTER, ALTER_SMALLER, "pAS"));
  transtions.emplace("ALTER->ALTER_BIGGER", Transition(ALTER, ALTER_BIGGER, "pAB"));
  transtions.emplace("ALTER->ALTER_METADATA", Transition(ALTER, ALTER_METADATA, "pAM"));
  transtions.emplace("DELETE->DELETE_FILE", Transition(DELETE, DELETE_FILE, "pDF"));
  transtions.emplace("DELETE->DELETE_DIR", Transition(DELETE, DELETE_DIR, "pDD"));
  transtions.emplace("CREATE_FILE->END", Transition(CREATE_FILE, END, "p1"));
  transtions.emplace("CREATE_DIR->END", Transition(CREATE_DIR, END, "p1"));
  transtions.emplace("ALTER_SMALLER->END", Transition(ALTER_SMALLER, END, "p1"));
  transtions.emplace("ALTER_BIGGER->END", Transition(ALTER_BIGGER, END, "p1"));
  transtions.emplace("ALTER_METADATA->END", Transition(ALTER_METADATA, END, "p1"));
  transtions.emplace("DELETE_FILE->END", Transition(DELETE_FILE, END, "p1"));
  transtions.emplace("DELETE_DIR->END", Transition(DELETE_DIR, END, "p1"));
  transtions.emplace("END->S", Transition(END, S, "p1"));

  int iteration = 0;
  // progressbar bar(getParameter("iterations").get_int());

  ProbabilisticStateMachine psm(transtions, S);
  std::map<std::string, double> probabilities;

  while (iteration < getParameter("iterations").get_int()) {
    compute_probabilities(probabilities, tree);
    psm.performTransition(probabilities);
    switch (psm.getCurrentState()) {
      case S:
        spdlog::debug("S");
        break;
      case CREATE:
        spdlog::debug("CREATE");
        break;
      case ALTER:
        spdlog::debug("ALTER");
        break;
      case DELETE:
        spdlog::debug("DELETE");
        break;
      case CREATE_FILE: {
        auto new_file_path = tree.newFilePath();
        spdlog::debug("CREATE_FILE {}", new_file_path);
        FileTree::Node* file_node = tree.mkfile(new_file_path);
        spdlog::debug(fmt::format("CREATE_FILE {} {}", new_file_path, file_node->path(true)));

        break;
      }
      case CREATE_DIR: {
        auto new_dir_path = tree.newDirectoryPath();
        spdlog::debug("CREATE_DIR {}", new_dir_path);
        FileTree::Node* dir_node = tree.mkdir(new_dir_path);
        spdlog::debug(fmt::format("CREATE_DIR {}", new_dir_path, dir_node->path(true)));
        break;
      }
      case ALTER_SMALLER:
        spdlog::debug("ALTER_SMALLER");
        break;
      case ALTER_BIGGER:
        spdlog::debug("ALTER_BIGGER");
        break;
      case ALTER_METADATA:
        spdlog::debug("ALTER_METADATA");
        break;
      case DELETE_FILE:
        spdlog::debug("DELETE_FILE");
        break;
      case DELETE_DIR:
        spdlog::debug("DELETE_DIR");
        break;
      case END:
        spdlog::debug("END");
        iteration++;
        break;

      default:
        break;
    }

    // bar.update();
    iteration++;
  }
}

void AgingScenario::compute_probabilities(std::map<std::string, double>& probabilities, FileTree& tree) {
  probabilities.clear();
  auto fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());

  // double CAF(double x) { return sqrt(1 - (x * x)); }
  // spdlog::debug("Capacity: {}, free: {}", fs_status.capacity, fs_status.free);
  // spdlog::debug("CAF input: {}", ((float(fs_status.capacity - fs_status.free) / float(fs_status.capacity))));

  double caf = CAF((float(fs_status.capacity - fs_status.free) / float(fs_status.capacity)));

  probabilities["pC"] = caf;
  probabilities["pD"] = 0.1 * (1.0 - caf);
  probabilities["pA"] = 1 - caf - probabilities["pD"];
  std::string logMessage = fmt::format("pC: {:.2f}, pD: {:.2f}, pA: {:.2f}, sum p {:.2f} ", probabilities["pC"], probabilities["pD"], probabilities["pA"],
                                       probabilities["pC"] + probabilities["pD"] + probabilities["pA"]);
  probabilities["pAM"] = 0.1;
  probabilities["pAB"] = caf;
  probabilities["pAS"] = 1 - probabilities["pAM"] - probabilities["pAB"];
  logMessage += fmt::format("pAM: {:.2f}, pAB: {:.2f}, pAS: {:.2f}, sum pA {:.2f} ", probabilities["pAM"], probabilities["pAB"], probabilities["pAS"],
                            probabilities["pAM"] + probabilities["pAB"] + probabilities["pAS"]);
  probabilities["pDD"] = 0.01;
  probabilities["pDF"] = 1 - probabilities["pDD"];
  logMessage += fmt::format("pDD: {:.2f}, pDF: {:.2f}, sum pD {:.2f} ", probabilities["pDD"], probabilities["pDF"], probabilities["pDD"] + probabilities["pDF"]);
  probabilities["pCF"] = (tree.getDirectoryCount() / getParameter("ndirs").get_int());
  probabilities["pCD"] = 1 - probabilities["pCF"];
  logMessage += fmt::format("pCF: {:.2f}, pCD: {:.2f}, sum pC {:.2f} ", probabilities["pCF"], probabilities["pCD"], probabilities["pCF"] + probabilities["pCD"]);
  probabilities["p1"] = 1.0;
  // spdlog::debug(logMessage);
}