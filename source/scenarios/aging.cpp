#include <fcntl.h>  // for open
#include <filestorm/actions/actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/filetree.h>
#include <filestorm/result.h>
#include <filestorm/scenarios/scenario.h>
#include <filestorm/utils.h>
#include <filestorm/utils/fs.h>
#include <filestorm/utils/progress_bar.h>
#include <fmt/format.h>
#include <sys/stat.h>  // for S_IRWXU
#include <unistd.h>    // for close

#include <algorithm>
#include <cerrno>  // for errno
#include <chrono>
#include <cstring>  // for strerror
#include <filesystem>
#include <fstream>
#include <iostream>  // for std::cerr
#include <memory>    // for std::unique_ptr
#include <random>
#include <string_view>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("d", "directory", "", "/tmp/filestorm/"));
  addParameter(Parameter("r", "depth", "Max directory depth", "5"));
  addParameter(Parameter("n", "ndirs", "Max number of dirs", "200"));
  addParameter(Parameter("f", "nfiles", "Max number of files", "2000"));
  addParameter(Parameter("m", "fs-capacity", "Max overall filesystem size", "full"));  // TODO: add support for this
  addParameter(Parameter("S", "maxfsize", "Max file size", "1G"));
  addParameter(Parameter("s", "minfsize", "Min file size", "10KB"));
  addParameter(Parameter("p", "sdist", "File size probabilistic distribution", "uniform"));
  addParameter(Parameter("i", "iterations", "Iterations to run", "-1"));
  addParameter(Parameter("b", "blocksize", "RW operations blocksize", "64k"));
  addParameter(Parameter("y", "sync", "Sync after each write", "false"));
  addParameter(Parameter("o", "direct_io", "Use direct IO", "false"));
  addParameter(Parameter("t", "time", "Max Time to run", "20m"));
  addParameter(Parameter("", "features-punch-hole", "Whether to do hole punching in file", "true"));
  addParameter(Parameter("", "features-log-probs", "Should log probabilities", "false"));
}

AgingScenario::~AgingScenario() {}

void AgingScenario::run() {
  if (!std::filesystem::exists(getParameter("directory").get_string())) {
    throw std::runtime_error(fmt::format("Directory {} does not exist!", getParameter("directory").get_string()));
  }
  if (!std::filesystem::is_directory(getParameter("directory").get_string())) {
    throw std::runtime_error(fmt::format("{} is not a directory!", getParameter("directory").get_string()));
  }
  if (!std::filesystem::is_empty(getParameter("directory").get_string())) {
    spdlog::warn("Directory {} is not empty!", getParameter("directory").get_string());
    throw std::runtime_error(fmt::format("{} is not empty!", getParameter("directory").get_string()));
  }

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
  transtions.emplace("ALTER_SMALLER->ALTER_SMALLER_TRUNCATE", Transition(ALTER_SMALLER, ALTER_SMALLER_TRUNCATE, "pAST"));
  transtions.emplace("ALTER_SMALLER->ALTER_SMALLER_FALLOCATE", Transition(ALTER_SMALLER, ALTER_SMALLER_FALLOCATE, "pASF"));
  transtions.emplace("ALTER_SMALLER_TRUNCATE->END", Transition(ALTER_SMALLER_TRUNCATE, END, "p1"));
  transtions.emplace("ALTER_SMALLER_FALLOCATE->END", Transition(ALTER_SMALLER_FALLOCATE, END, "p1"));
  transtions.emplace("ALTER_BIGGER->END", Transition(ALTER_BIGGER, END, "p1"));
  transtions.emplace("ALTER_METADATA->END", Transition(ALTER_METADATA, END, "p1"));
  transtions.emplace("DELETE_FILE->END", Transition(DELETE_FILE, END, "p1"));
  transtions.emplace("DELETE_DIR->END", Transition(DELETE_DIR, END, "p1"));
  transtions.emplace("END->S", Transition(END, S, "p1"));

  int iteration = 0;
  std::chrono::seconds max_time = stringToChrono(getParameter("time").get_string());
  auto start = std::chrono::high_resolution_clock::now();
  // progressbar bar(getParameter("iterations").get_int());

  std::vector<Result> results;

  ProbabilisticStateMachine psm(transtions, S);
  std::map<std::string, double> probabilities;

  std::vector<FileTree::Node*> touched_files;
  Result result;
  while ((iteration < getParameter("iterations").get_int() || getParameter("iterations").get_int() == -1)
         && (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) < max_time || getParameter("iterations").get_int() != -1)) {
    result.setIteration(iteration);
    compute_probabilities(probabilities, tree);
    psm.performTransition(probabilities);
    switch (psm.getCurrentState()) {
      case S:
        // spdlog::debug("S");
        break;
      case CREATE:
        // spdlog::debug("CREATE");
        break;
      case ALTER:
        // spdlog::debug("ALTER");
        break;
      case DELETE:
        // spdlog::debug("DELETE");
        break;
      case CREATE_FILE: {
        FileTree::Node* file_node = tree.mkfile(tree.newFilePath());
        spdlog::debug(fmt::format("CREATE_FILE {}", file_node->path(true)));
        DataSize<DataUnit::B> file_size = get_file_size();

        int fd;
        std::unique_ptr<char[]> line(new char[get_block_size().get_value()]);
        size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
        generate_random_chunk(line.get(), block_size);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
        fd = open(file_node->path(true).c_str(), O_RDWR | O_CREAT, S_IRWXU);
        if (getParameter("direct_io").get_bool() && fd != -1) {
          if (fcntl(fd, F_NOCACHE, 1) == -1) {
            close(fd);
            throw std::runtime_error("WriteMonitoredAction::work: error on fcntl F_NOCACHE");
          }
        }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
        int flags = O_RDWR | O_CREAT;
        if (getParameter("direct_io").get_bool()) {
          flags |= O_DIRECT;
        }
        fd = open(file_node->path(true).c_str(), flags, S_IRWXU);
#else
#  error "Unknown system"
#endif
        if (fd == -1) {
          std::cerr << "Error opening file: " << strerror(errno) << std::endl;
          if (errno == EINVAL) {
            throw std::runtime_error("Error: Direct IO not supported");
          }
          throw std::runtime_error(fmt::format("Error opening file {}", file_node->path(true)));
        }
        MeasuredCBAction action([&]() {
          for (uint64_t written_bytes = 0; written_bytes < file_size.get_value(); written_bytes += block_size) {
            write(fd, line.get(), block_size);
          }
        });
        auto duration = action.exec();
        close(fd);
        spdlog::debug(fmt::format("CREATE_FILE {} Wrote {} MB in {} ms | Speed {} MB/s", file_node->path(true), int(file_size.get_value() / 1024. / 1024.), duration.count() / 1000000.0,
                                  (file_size.get_value() / 1024. / 1024.) / (duration.count() / 1000000000.0)));
        touched_files.push_back(file_node);
        result.setAction(Result::Action::CREATE_FILE);
        result.setOperation(Result::Operation::WRITE);
        result.setPath(file_node->path(true));
        result.setSize(file_size);
        result.setDuration(duration);
        break;
      }
      case CREATE_DIR: {
        auto new_dir_path = tree.newDirectoryPath();
        spdlog::debug("CREATE_DIR {}", new_dir_path);
        FileTree::Node* dir_node = tree.mkdir(new_dir_path);
        auto dir_path = dir_node->path(true);

        MeasuredCBAction action([&]() { std::filesystem::create_directory(dir_path); });
        auto duration = action.exec();
        result.setAction(Result::Action::CREATE_DIR);
        result.setPath(dir_path);
        result.setDuration(duration);
        break;
      }
      case ALTER_SMALLER: {
        spdlog::debug("ALTER_SMALLER");
        break;
      }
      case ALTER_SMALLER_TRUNCATE: {
        spdlog::debug("ALTER_SMALLER_TRUNCATE");
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        auto actual_file_size = fs_utils::file_size(random_file_path);
        // auto new_file_size = get_file_size(0, actual_file_size, false);
        std::uintmax_t blocksize = get_block_size().convert<DataUnit::B>().get_value();
        auto new_file_size = get_file_size(std::max(actual_file_size / 2, 10 * blocksize), actual_file_size, false);
        spdlog::debug("ALTER_SMALLER_TRUNCATE {} from {} kB to {} kB ({})", random_file_path, actual_file_size / 1024, new_file_size.get_value() / 1024, new_file_size.get_value());
        random_file->truncate(new_file_size.get_value());
        MeasuredCBAction action([&]() { truncate(random_file_path.c_str(), new_file_size.convert<DataUnit::B>().get_value()); });
        touched_files.push_back(random_file);
        auto duration = action.exec();
        result.setAction(Result::Action::ALTER_SMALLER_TRUNCATE);
        result.setPath(random_file_path);
        result.setSize(new_file_size);
        result.setDuration(duration);
        break;
      }
      case ALTER_SMALLER_FALLOCATE: {
        spdlog::debug("ALTER_SMALLER_FALLOCATE");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
#  warning "FALLOCATE not supported on macOS"
        throw std::runtime_error("FALLOCATE not supported on this system");
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
        auto random_file = tree.randomPunchableFile();
        auto random_file_path = random_file->path(true);
        auto block_size = get_block_size().convert<DataUnit::B>().get_value();
        std::tuple<size_t, size_t> hole_adress;
        try {
          random_file->getHoleAdress(block_size, true);
        } catch (std::runtime_error& e) {
          for (auto& f in tree.files_for_fallocate) {
            std::cout << f->path(true) << std::endl;
          }
          exit(1);
        }
        // Round to modulo blocksize
        spdlog::debug("ALTER_SMALLER_FALLOCATE {} with size {} punched hole {} - {}", random_file_path, random_file->size(), std::get<0>(hole_adress), std::get<1>(hole_adress));
        MeasuredCBAction action([&]() {
          int fd = open(random_file_path.c_str(), O_RDWR);
          if (fd == -1) {
            std::cerr << "Error opening file: " << strerror(errno) << std::endl;
            throw std::runtime_error(fmt::format("Error opening file {}", random_file_path));
          }
          fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, std::get<0>(hole_adress), std::get<1>(hole_adress) - std::get<0>(hole_adress));
          close(fd);
        });
        touched_files.push_back(random_file);
        auto duration = action.exec();
        if (!random_file->isPunchable(block_size)) {
          spdlog::warn("File {} is not punchable anymore", random_file_path);
          tree.removeFromPunchableFiles(random_file);
        }
        result.setAction(Result::Action::ALTER_SMALLER_FALLOCATE);
        result.setPath(random_file_path);
        result.setSize(DataSize<DataUnit::B>(std::get<1>(hole_adress) - std::get<0>(hole_adress)));
        result.setDuration(duration);
#else
        throw std::runtime_error("FALLOCATE not supported on this system");
#endif
        break;
      }
      case ALTER_BIGGER: {
        std::unique_ptr<char[]> line(new char[get_block_size().get_value()]);
        size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
        generate_random_chunk(line.get(), block_size);
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        auto actual_file_size = fs_utils::file_size(random_file_path);
        auto new_file_size = get_file_size(actual_file_size, DataSize<DataUnit::B>::fromString(getParameter("maxfsize").get_string()).convert<DataUnit::B>().get_value());
        spdlog::debug("ALTER_BIGGER {} from {} to {}", random_file_path, actual_file_size, new_file_size);
        int fd;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
        fd = open(random_file_path.c_str(), O_WRONLY | O_APPEND, S_IRWXU);
        if (fcntl(fd, F_NOCACHE, 1) == -1 && fd != -1) {
          close(fd);
          throw std::runtime_error("WriteMonitoredAction::work: error on fcntl F_NOCACHE");
        }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
        fd = open(random_file_path.c_str(), O_DIRECT | O_WRONLY | O_APPEND, S_IRWXU);
#else
#  error "Unknown system"
#endif
        if (fd == -1) {
          std::cerr << "Error opening file: " << strerror(errno) << std::endl;
          if (errno == EINVAL) {
            throw std::runtime_error("Error: Direct IO not supported");
          }
          throw std::runtime_error(fmt::format("Error opening file {}", random_file_path));
        }
        auto current_size = lseek(fd, 0, SEEK_END);
        if (current_size == -1) {
          perror("Error getting file size");
          close(fd);
          throw std::runtime_error(fmt::format("Error getting file size {}", random_file_path));
        }

        MeasuredCBAction action([&]() {
          for (uint64_t written_bytes = actual_file_size; written_bytes < new_file_size.get_value(); written_bytes += block_size) {
            write(fd, line.get(), block_size);
          }
          close(fd);
        });
        auto duration = action.exec();
        spdlog::debug("ALTER_BIGGER {} from {} to {} in {} ms | Speed {} MB/s", random_file_path, actual_file_size, new_file_size.get_value(), duration.count() / 1000000.0,
                      ((new_file_size.get_value() - actual_file_size) / 1024. / 1024.) / (duration.count() / 1000000000.0));
        touched_files.push_back(random_file);
        result.setAction(Result::Action::ALTER_BIGGER);
        result.setOperation(Result::Operation::WRITE);
        result.setPath(random_file_path);
        result.setSize(new_file_size - DataSize<DataUnit::B>(actual_file_size));
        result.setDuration(duration);
        break;
      }
      case ALTER_METADATA:
        spdlog::debug("ALTER_METADATA");
        break;
      case DELETE_FILE: {
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        spdlog::debug("DELETE_FILE {}", random_file_path);
        auto file_extents = random_file->getExtentsCount(true);

        MeasuredCBAction action([&]() { std::filesystem::remove(random_file_path); });
        action.exec();
        tree.rm(random_file->path(false));
        result.setAction(Result::Action::DELETE_FILE);
        result.setPath(random_file_path);
        tree.total_extents_count -= file_extents;
        break;
      }
      case DELETE_DIR: {
        auto random_dir = tree.randomDirectory();
        auto random_dir_path = random_dir->path(true);
        spdlog::debug("DELETE_DIR {}", random_dir_path);
        // TODO: remove directory Is it necesary and good idea at all?
        break;
      }
      case END:
        spdlog::debug("END");
        if (getParameter("sync").get_bool()) {
          spdlog::debug("Syncing...");
          sync();
        }
        for (auto& file : touched_files) {
          auto original_extents = file->getExtentsCount(false);
          auto updated_extents = file->getExtentsCount(true);
          spdlog::debug("File {} extents: {} -> {}", file->path(true), original_extents, updated_extents);
          tree.total_extents_count += updated_extents - original_extents;
        }
        spdlog::debug("Total extents count: {}, File Count {}, F avail files {}, free space {} MB", tree.total_extents_count, tree.all_files.size(), tree.files_for_fallocate.size(),
                      fs_utils::get_fs_status(getParameter("directory").get_string()).available / 1024 / 1024);
        touched_files.clear();
        result.commit();
        result = Result();

        iteration++;
        break;

      default:
        break;
    }

    // bar.update();
  }

  // Count files and total extent count

  int file_count = 0;
  int total_extents = 0;
  for (auto& file : tree.all_files) {
    file_count++;
    total_extents += file->getExtentsCount();
  }
  spdlog::info("File count: {}, total extents: {}", file_count, total_extents);

  // cleanup
  for (auto& file : tree.all_files) {
    std::filesystem::remove(file->path(true));
  }
  tree.bottomUpDirWalk(tree.getRoot(), [&](FileTree::Node* dir) { std::filesystem::remove(dir->path(true)); });
}

void AgingScenario::compute_probabilities(std::map<std::string, double>& probabilities, FileTree& tree) {
  probabilities.clear();
  auto fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());

  // double CAF(double x) { return sqrt(1 - (x * x)); }
  // spdlog::debug("Capacity: {}, available: {}", fs_status.capacity, fs_status.available);
  // spdlog::debug("CAF input: {}", ((float(fs_status.capacity - fs_status.available) / float(fs_status.capacity))));

  // Handle case when available space is less than block size. If this happens, we can't write any more data. Even thought the drive isnt completely full.
  if (fs_status.available <= get_block_size().convert<DataUnit::B>().get_value()) {
    fs_status.available = 0;
  }

  double caf = CAF((float(fs_status.capacity - fs_status.available) / float(fs_status.capacity)));
  caf = ceilTo(caf, 3);
  probabilities["pC"] = caf;
  probabilities["pD"] = 0.1 * (1.0 - caf);
  probabilities["pA"] = 1 - caf - probabilities["pD"];
  probabilities["pAM"] = 0.1;
  probabilities["pAB"] = caf - probabilities["pAM"];
  probabilities["pAS"] = 1 - probabilities["pAM"] - probabilities["pAB"];
#if __linux__
  if (getParameter("features-punch-hole").get_bool() && tree.hasPunchableFiles()) {
    probabilities["pAST"] = 0.1;
  } else {
    probabilities["pAST"] = 1;
  }
  probabilities["pASF"] = 1 - probabilities["pAST"];
#else
  probabilities["pAST"] = 1;
  probabilities["pASF"] = 0;
#endif
  probabilities["pDD"] = 0.01;
  probabilities["pDF"] = 1 - probabilities["pDD"];
  probabilities["pCF"] = (tree.getDirectoryCount() / getParameter("ndirs").get_int());
  probabilities["pCD"] = 1 - probabilities["pCF"];
  probabilities["p1"] = 1.0;

  if (getParameter("features-log-probs").get_bool()) {
    std::string logMessage
        = fmt::format("Capacity: {} MB | available: {} MB ({}) | pC: {:.4f}, pD: {:.4f}, pA: {:.4f}, sum p {:.4f} ", fs_status.capacity / 1024 / 1024, fs_status.available / 1024 / 1024,
                      fs_status.available, probabilities["pC"], probabilities["pD"], probabilities["pA"], probabilities["pC"] + probabilities["pD"] + probabilities["pA"]);
    logMessage += fmt::format("Capacity: {} B | available: {} B ", fs_status.capacity, fs_status.available);
    logMessage += fmt::format("pAM: {:.2f}, pAB: {:.2f}, pAS: {:.2f}, sum pA {:.2f} ", probabilities["pAM"], probabilities["pAB"], probabilities["pAS"],
                              probabilities["pAM"] + probabilities["pAB"] + probabilities["pAS"]);
    logMessage += fmt::format("pAST: {:.2f}, pASF: {:.2f}, sum pAS {:.2f} ", probabilities["pAST"], probabilities["pASF"], probabilities["pAST"] + probabilities["pASF"]);
    logMessage += fmt::format("pDD: {:.2f}, pDF: {:.2f}, sum pD {:.2f} ", probabilities["pDD"], probabilities["pDF"], probabilities["pDD"] + probabilities["pDF"]);
    logMessage += fmt::format("pCF: {:.2f}, pCD: {:.2f}, sum pC {:.2f} ", probabilities["pCF"], probabilities["pCD"], probabilities["pCF"] + probabilities["pCD"]);
    spdlog::debug(logMessage);
  }
}

DataSize<DataUnit::B> AgingScenario::get_file_size(uint64_t range_from, uint64_t range_to, bool safe) {
  std::random_device rand_dev;
  std::mt19937 generator(rand_dev());
  DataSize<DataUnit::B> return_size(0);
  if (getParameter("sdist").get_string() == "uniform") {
    std::uniform_int_distribution<uint64_t> distr(range_from, range_to);
    auto d = distr(generator);
    return_size = DataSize<DataUnit::B>(d);
  } else if (getParameter("sdist").get_string() == "normal") {
    double mean = range_to / 2;
    double stddev = mean / 2;
    std::normal_distribution<double> distr(mean, stddev);
    return_size = DataSize<DataUnit::B>(distr(generator));
  } else {
    throw std::runtime_error("Invalid file size distribution!");
  }
  if (safe) {
    std::filesystem::space_info fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());
    if (return_size.get_value() > fs_status.available) {
      return_size = DataSize<DataUnit::B>(fs_status.available);
      auto block_size = get_block_size().convert<DataUnit::B>().get_value();
      return_size = DataSize<DataUnit::B>(return_size.get_value() - (return_size.get_value() % block_size));
    }
  }
  return return_size;
}

DataSize<DataUnit::B> AgingScenario::get_file_size() {
  auto max_size = DataSize<DataUnit::B>::fromString(getParameter("maxfsize").get_string()).convert<DataUnit::B>();
  auto min_size = DataSize<DataUnit::B>::fromString(getParameter("minfsize").get_string()).convert<DataUnit::B>();
  auto fsize = get_file_size(min_size.get_value(), max_size.get_value());
  // spdlog::debug("get_file_size: {} - {} : {} ", min_size.get_value(), max_size.get_value(), fsize.get_value());
  return fsize;
}
