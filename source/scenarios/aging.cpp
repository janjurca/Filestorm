#include <fcntl.h>  // for open
#include <filestorm/actions/actions.h>
#include <filestorm/data_sizes.h>
#include <filestorm/filetree.h>
#include <filestorm/result.h>
#include <filestorm/scenarios/aging.h>
#include <filestorm/scenarios/register.h>
#include <filestorm/utils.h>
#include <filestorm/utils/fs.h>
#include <filestorm/utils/logger.h>
#include <fmt/format.h>
#include <sys/stat.h>  // for S_IRWXU
#include <sys/types.h>
#include <unistd.h>  // for close

#include <algorithm>
#include <cerrno>  // for errno
#include <chrono>
#include <cstring>  // for strerror
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iostream>  // for std::cerr
#include <memory>    // for std::unique_ptr
#include <random>
#include <string_view>

AgingScenario::AgingScenario() {
  _name = "aging";
  _description = "Scenario for testing filesystem aging.";
  addParameter(Parameter("d", "directory", "Set the target directory where the testing will be performed", "/tmp/filestorm/"));
  addParameter(Parameter("r", "depth", "Max directory depth", "5"));
  addParameter(Parameter("n", "ndirs", "Max number of dirs", "200"));
  addParameter(Parameter("m", "fs-capacity", "Max overall filesystem size", "full"));  // TODO: add support for this
  addParameter(Parameter("S", "maxfsize", "Max file size", "1G"));
  addParameter(Parameter("s", "minfsize", "Min file size", "10KB"));
  addParameter(Parameter("p", "sdist", "File size probabilistic distribution", "uniform"));
  addParameter(Parameter("i", "iterations", "Iterations to run", "-1"));
  addParameter(Parameter("b", "blocksize", "RW operations blocksize", "64k"));
  addParameter(Parameter("y", "sync", "Sync after each write", "false"));
  addParameter(Parameter("o", "direct_io", "Use direct IO", "false"));
  addParameter(Parameter("t", "time", "Time to run", "2h"));
  addParameter(Parameter("", "rapid-aging",
                         "The testing will try to age the filesystem at first by only allocating files without data writes in order to get the aged state much faster than the using normal aging "
                         "process. downside of this is that you wont get the data performance for the for the initial aging stage.",
                         "false"));

  addParameter(Parameter("", "create-dir", "If testing directory doesn't exists try to create it.", "false"));
  addParameter(Parameter("", "features-punch-hole", "Whether to do hole punching in file", "true"));
  addParameter(Parameter("", "features-log-probs", "Should log probabilities", "false"));
  addParameter(Parameter("", "cleanup", "Should clean up files/folders after the test is done", "true"));
  addParameter(Parameter("", "rapid-aging-threshold", "Set threshold for rapid aging testing 90-0.where 90 is rapid aging essentially turned off and at 0  will probably never ends.", "37"));
  addParameter(Parameter("", "rapid-aging-min-time", "Minimal time to run rapid aging in seconds", "5s"));
  addParameter(Parameter("", "settings-safe-margin",
                         "When new file is computed and there is not enough space the new file size is shrinked to available size but in some cases the fs has a file size overhead because of "
                         "metadata writes which are hard to predict and compute. So the safe margin is introduced which specify what is a minimal space amount that should be left available",
                         "1MB"));
}

AgingScenario::~AgingScenario() {}

void AgingScenario::run(std::unique_ptr<IOEngine>& ioengine) {
  if (!std::filesystem::exists(getParameter("directory").get_string())) {
    if (getParameter("create-dir").get_bool()) {
      std::filesystem::create_directories(getParameter("directory").get_string());
    } else {
      throw std::runtime_error(fmt::format("Directory {} does not exist!", getParameter("directory").get_string()));
    }
  }
  if (!std::filesystem::is_directory(getParameter("directory").get_string())) {
    throw std::runtime_error(fmt::format("{} is not a directory!", getParameter("directory").get_string()));
  }
  if (!std::filesystem::is_empty(getParameter("directory").get_string())) {
    logger.warn("Directory {} is not empty!", getParameter("directory").get_string());
    throw std::runtime_error(fmt::format("{} is not empty!", getParameter("directory").get_string()));
  }
  rapid_aging = getParameter("rapid-aging").get_bool();

  logger.debug("Rapid aging is: {}", rapid_aging);

  FileTree tree(getParameter("directory").get_string(), getParameter("depth").get_int());
  std::map<std::string, Transition> transitions;
  transitions.emplace("S->CREATE", Transition(S, CREATE, "pC"));
  transitions.emplace("S->ALTER", Transition(S, ALTER, "pA"));
  transitions.emplace("S->DELETE", Transition(S, DELETE, "pD"));
  transitions.emplace("CREATE->CREATE_FILE", Transition(CREATE, CREATE_FILE, "pCF"));
  transitions.emplace("CREATE->CREATE_FILE_FALLOCATE", Transition(CREATE, CREATE_FILE_FALLOCATE, "pCFF"));
  transitions.emplace("CREATE_FILE_FALLOCATE->END", Transition(CREATE_FILE_FALLOCATE, END, "p1"));
  transitions.emplace("CREATE->CREATE_DIR", Transition(CREATE, CREATE_DIR, "pCD"));
  transitions.emplace("ALTER->ALTER_SMALLER", Transition(ALTER, ALTER_SMALLER, "pAS"));
  transitions.emplace("ALTER->ALTER_BIGGER", Transition(ALTER, ALTER_BIGGER, "pAB"));
  transitions.emplace("ALTER->ALTER_METADATA", Transition(ALTER, ALTER_METADATA, "pAM"));
  transitions.emplace("DELETE->DELETE_FILE", Transition(DELETE, DELETE_FILE, "pDF"));
  transitions.emplace("DELETE->DELETE_DIR", Transition(DELETE, DELETE_DIR, "pDD"));
  transitions.emplace("CREATE_FILE->CREATE_FILE_OVERWRITE", Transition(CREATE_FILE, CREATE_FILE_OVERWRITE, "pCFO"));
  transitions.emplace("CREATE_FILE->CREATE_FILE_READ", Transition(CREATE_FILE, CREATE_FILE_READ, "pCFR"));
  transitions.emplace("CREATE_FILE->END", Transition(CREATE_FILE, END, "pCFE"));
  transitions.emplace("CREATE_FILE_OVERWRITE->END", Transition(CREATE_FILE_OVERWRITE, END, "p1"));
  transitions.emplace("CREATE_FILE_READ->END", Transition(CREATE_FILE_READ, END, "p1"));
  transitions.emplace("CREATE_DIR->END", Transition(CREATE_DIR, END, "p1"));
  transitions.emplace("ALTER_SMALLER->ALTER_SMALLER_TRUNCATE", Transition(ALTER_SMALLER, ALTER_SMALLER_TRUNCATE, "pAST"));
  transitions.emplace("ALTER_SMALLER->ALTER_SMALLER_FALLOCATE", Transition(ALTER_SMALLER, ALTER_SMALLER_FALLOCATE, "pASF"));
  transitions.emplace("ALTER_SMALLER_TRUNCATE->END", Transition(ALTER_SMALLER_TRUNCATE, END, "p1"));
  transitions.emplace("ALTER_SMALLER_FALLOCATE->END", Transition(ALTER_SMALLER_FALLOCATE, END, "p1"));
  transitions.emplace("ALTER_BIGGER->ALTER_BIGGER_WRITE", Transition(ALTER_BIGGER, ALTER_BIGGER_WRITE, "pABW"));
  transitions.emplace("ALTER_BIGGER->ALTER_BIGGER_FALLOCATE", Transition(ALTER_BIGGER, ALTER_BIGGER_FALLOCATE, "pABF"));
  transitions.emplace("ALTER_BIGGER_WRITE->END", Transition(ALTER_BIGGER_WRITE, END, "p1"));
  transitions.emplace("ALTER_BIGGER_FALLOCATE->END", Transition(ALTER_BIGGER_FALLOCATE, END, "p1"));
  transitions.emplace("ALTER_METADATA->END", Transition(ALTER_METADATA, END, "p1"));
  transitions.emplace("DELETE_FILE->END", Transition(DELETE_FILE, END, "p1"));
  transitions.emplace("DELETE_DIR->END", Transition(DELETE_DIR, END, "p1"));
  transitions.emplace("END->S", Transition(END, S, "p1"));

  int iteration = 0;
  std::chrono::seconds max_time = stringToChrono(getParameter("time").get_string());
  auto start = std::chrono::high_resolution_clock::now();
  // progressbar bar(getParameter("iterations").get_int());

  std::vector<Result> results;
  ProbabilisticStateMachine psm(transitions, S);
  std::map<std::string, double> probabilities;
  std::vector<FileTree::Nodeptr> touched_files;
  Result result;

  ///////// LOGGER settings

  ProgressBar bar("Aging Scenario");
  if (getParameter("iterations").get_int() != -1) {
    logger.warn("Iterations are set to {}", getParameter("iterations").get_int());
    bar.set_total(getParameter("iterations").get_int());
  } else {
    logger.warn("Time is set to {}", getParameter("time").get_string());
    bar.set_total(std::chrono::duration_cast<std::chrono::seconds>(max_time));
  }
  logger.set_progress_bar(&bar);

  PolyCurve extents_curve(1, 10);

  while ((iteration < getParameter("iterations").get_int() || getParameter("iterations").get_int() == -1)
         && (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) < max_time || getParameter("iterations").get_int() != -1)) {
    result.setIteration(iteration);

    if (rapid_aging) {
      if (extents_curve.getPointCount() > 10
          && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start)
                 > std::chrono::duration_cast<std::chrono::seconds>(stringToChrono(getParameter("rapid-aging-min-time").get_string()))) {
        extents_curve.fitPolyCurve();
        logger.debug("Extents curve angle: {}", extents_curve.slopeAngle());
        if (extents_curve.slopeAngle() < getParameter("rapid-aging-threshold").get_int()) {
          rapid_aging = false;
          logger.debug("Rapid aging - disabling");
        }
      }
    }

    compute_probabilities(probabilities, tree, extents_curve);
    psm.performTransition(probabilities);
    switch (psm.getCurrentState()) {
      case S:
        logger.debug("S");
        break;
      case CREATE:
        // logger.debug("CREATE");
        break;
      case ALTER:
        // logger.debug("ALTER");
        break;
      case DELETE:
        // logger.debug("DELETE");
        break;

      case CREATE_FILE: {
        FileTree::Nodeptr file_node = tree.mkfile(tree.newFilePath());
        logger.debug(fmt::format("CREATE_FILE {}", file_node->path(true)));

        DataSize<DataUnit::B> file_size = get_file_size();
        size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

        // Allocate aligned memory
        void* aligned_buf = nullptr;
        int alignment = 4096;  // Typically required for O_DIRECT
        if (posix_memalign(&aligned_buf, alignment, block_size) != 0) {
          throw std::runtime_error("posix_memalign failed for aligned buffer");
        }

        generate_random_chunk(static_cast<char*>(aligned_buf), block_size);

        int fd = ioengine->open_file(file_node->path(true).c_str(), O_WRONLY | O_CREAT | O_TRUNC, getParameter("direct_io").get_bool());

        MeasuredCBAction action([&]() {
          off_t offset = 0;
          uint64_t written_bytes_optimistic = 0;
          uint64_t written_bytes_true = 0;

          while (written_bytes_optimistic < file_size.get_value()) {
            ssize_t _written_bytes = ioengine->write(fd, aligned_buf, block_size, offset);
            if (_written_bytes == -1) {
              perror("Error writing to file");
              ioengine->close(fd);
              free(aligned_buf);
              throw std::runtime_error(fmt::format("Error writing to file {}", file_node->path(true)));
            }
            offset += _written_bytes;
            written_bytes_optimistic += block_size;
            written_bytes_true += _written_bytes;
          }

          written_bytes_true += ioengine->complete();

          if (written_bytes_true != written_bytes_optimistic) {
            logger.warn(fmt::format("Written bytes {} != written_bytes_optimistic {}", written_bytes_true, written_bytes_optimistic));
          }
        });

        auto duration = action.exec();
        ioengine->close(fd);
        free(aligned_buf);  // Free aligned memory

        logger.debug(fmt::format("CREATE_FILE {} Wrote {} MB in {} ms | Speed {} MB/s", file_node->path(true), int(file_size.get_value() / 1024. / 1024.), duration.count() / 1000000.0,
                                 (file_size.get_value() / 1024. / 1024.) / (duration.count() / 1000000000.0)));

        touched_files.push_back(file_node);

        result.setAction(Result::Action::CREATE_FILE);
        result.setOperation(Result::Operation::WRITE);
        result.setPath(file_node->path(true));
        result.setSize(file_size);
        result.setDuration(duration);
        break;
      }

      case CREATE_FILE_FALLOCATE: {
        FileTree::Nodeptr file_node = tree.mkfile(tree.newFilePath());
        logger.debug(fmt::format("CREATE_FILE_FALLOCATE {}", file_node->path(true)));
        DataSize<DataUnit::B> file_size = get_file_size();

        int fd = ioengine->open_file(file_node->path(true).c_str(), O_RDWR | O_CREAT, getParameter("direct_io").get_bool());
        MeasuredCBAction action([&]() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
          if (ftruncate(fd, file_size.get_value()) == -1) {
            throw std::runtime_error(fmt::format("ftruncate failed: {}", strerror(errno)));
          }
#elif defined(__linux__)
          if (fallocate(fd, 0, 0, file_size.get_value()) == -1) {
            throw std::runtime_error(fmt::format("Fallocate failed: {}", strerror(errno)));
          }
#endif
        });
        auto duration = action.exec();
        ioengine->close(fd);
        logger.debug(fmt::format("CREATE_FILE_FALLOCATE {} Wrote {} MB in {} ms | Speed {} MB/s", file_node->path(true), int(file_size.get_value() / 1024. / 1024.), duration.count() / 1000000.0,
                                 (file_size.get_value() / 1024. / 1024.) / (duration.count() / 1000000000.0)));
        touched_files.push_back(file_node);

        result.setAction(Result::Action::CREATE_FILE_FALLOCATE);
        result.setOperation(Result::Operation::FALLOCATE);
        result.setPath(file_node->path(true));
        result.setSize(file_size);
        result.setDuration(duration);
        break;
      }
      case CREATE_FILE_OVERWRITE: {
        auto prev_file = touched_files.back();
        assert(prev_file != nullptr);
        assert(prev_file->getFallocationCount() == 0);

        auto prev_file_path = prev_file->path(true);
        auto file_size = fs_utils::file_size(prev_file_path);
        logger.debug("CREATE_FILE_OVERWRITE {} size {}", prev_file_path, file_size);

        size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

        // Allocate aligned memory for O_DIRECT
        void* aligned_buf = nullptr;
        int alignment = 4096;  // common page size for O_DIRECT, verify against your FS/ioengine
        if (posix_memalign(&aligned_buf, alignment, block_size) != 0) {
          throw std::runtime_error("posix_memalign failed for aligned buffer");
        }

        generate_random_chunk(static_cast<char*>(aligned_buf), block_size);

        int fd = ioengine->open_file(prev_file_path.c_str(), O_WRONLY, getParameter("direct_io").get_bool());

        MeasuredCBAction action([&]() {
          off_t offset = 0;
          uint64_t written_bytes_optimistic = 0;
          uint64_t written_bytes_true = 0;
          while (written_bytes_optimistic < file_size) {
            ssize_t written_bytes_ = ioengine->write(fd, aligned_buf, block_size, offset);
            if (written_bytes_ == -1) {
              perror("Error writing to file");
              ioengine->close(fd);
              free(aligned_buf);
              throw std::runtime_error(fmt::format("Error writing to file {}", prev_file_path));
            }
            offset += written_bytes_;
            written_bytes_optimistic += block_size;
            written_bytes_true += written_bytes_;
          }
          written_bytes_true += ioengine->complete();
          if (written_bytes_true != written_bytes_optimistic) {
            logger.warn(fmt::format("Written bytes {} != file size {}", written_bytes_true, written_bytes_optimistic));
          }
        });

        auto duration = action.exec();
        ioengine->close(fd);
        free(aligned_buf);  // free aligned memory

        logger.debug(fmt::format("CREATE_FILE_OVERWRITE {} Wrote {} MB in {} ms | Speed {} MB/s", prev_file_path, int(file_size / 1024. / 1024.), duration.count() / 1000000.0,
                                 (file_size / 1024. / 1024.) / (duration.count() / 1000000000.0)));

        result.setAction(Result::Action::CREATE_FILE_OVERWRITE);
        result.setOperation(Result::Operation::OVERWRITE);
        result.setPath(prev_file_path);
        result.setSize(file_size);
        result.setDuration(duration);
        break;
      }

      case CREATE_FILE_READ: {
        auto prev_file = touched_files.back();
        assert(prev_file != nullptr);
        assert(prev_file->getFallocationCount() == 0);
        auto prev_file_path = prev_file->path(true);
        auto file_size = fs_utils::file_size(prev_file_path);
        logger.debug("CREATE_FILE_READ {} size {}", prev_file_path, file_size);

        int fd = ioengine->open_file(prev_file_path.c_str(), O_RDONLY, getParameter("direct_io").get_bool());

        MeasuredCBAction action([&]() {
          size_t block_size = get_block_size().convert<DataUnit::B>().get_value();

          // Allocate aligned memory
          void* aligned_buf = nullptr;
          int alignment = 4096;  // Typically required for O_DIRECT
          if (posix_memalign(&aligned_buf, alignment, block_size) != 0) {
            throw std::runtime_error("posix_memalign failed for aligned buffer");
          }

          off_t offset = 0;
          uint64_t read_bytes_optimistic = 0;
          uint64_t read_bytes_true = 0;

          while (read_bytes_optimistic < file_size) {
            ssize_t read_bytes_ = ioengine->read(fd, aligned_buf, block_size, offset);
            if (read_bytes_ == -1) {
              perror("Error reading from file");
              ioengine->close(fd);
              free(aligned_buf);
              throw std::runtime_error(fmt::format("Error reading from file {}", prev_file_path));
            }
            offset += read_bytes_;
            read_bytes_optimistic += block_size;
            read_bytes_true += read_bytes_;
          }

          read_bytes_true += ioengine->complete();
          free(aligned_buf);  // Always free allocated memory

          if (read_bytes_true != file_size) {
            logger.warn(fmt::format("Read bytes {} != file size {}", read_bytes_true, file_size));
          }
        });

        auto duration = action.exec();
        ioengine->close(fd);

        logger.debug(fmt::format("CREATE_FILE_READ {} Read {} MB in {} ms | Speed {} MB/s", prev_file_path, int(file_size / 1024. / 1024.), duration.count() / 1000000.0,
                                 (file_size / 1024. / 1024.) / (duration.count() / 1000000000.0)));

        result.setAction(Result::Action::CREATE_FILE_READ);
        result.setOperation(Result::Operation::READ);
        result.setPath(prev_file_path);
        result.setSize(DataSize<DataUnit::B>(file_size));
        result.setDuration(duration);
        break;
      }

      case CREATE_DIR: {
        logger.debug("CREATE_DIR");
        auto new_dir_path = tree.newDirectoryPath();
        logger.debug("CREATE_DIR {}", new_dir_path);
        FileTree::Nodeptr dir_node = tree.mkdir(new_dir_path);
        auto dir_path = dir_node->path(true);

        MeasuredCBAction action([&]() { std::filesystem::create_directory(dir_path); });
        auto duration = action.exec();
        result.setAction(Result::Action::CREATE_DIR);
        result.setPath(dir_path);
        result.setDuration(duration);
        break;
      }
      case ALTER_SMALLER: {
        logger.debug("ALTER_SMALLER");
        break;
      }
      case ALTER_SMALLER_TRUNCATE: {
        logger.debug("ALTER_SMALLER_TRUNCATE");
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        auto actual_file_size = fs_utils::file_size(random_file_path);
        // auto new_file_size = get_file_size(0, actual_file_size, false);
        std::uintmax_t blocksize = get_block_size().convert<DataUnit::B>().get_value();
        auto new_file_size = get_file_size(std::max(actual_file_size / 2, blocksize), actual_file_size, false);  // TODO check this for error, remove the magic constant
        logger.debug("ALTER_SMALLER_TRUNCATE {} from {} kB to {} kB ({})", random_file_path, actual_file_size / 1024, new_file_size.get_value() / 1024, new_file_size.get_value());
        bool fallocatable = random_file->isPunchable(blocksize);
        random_file->truncate(blocksize, new_file_size.get_value());
        MeasuredCBAction action([&]() { truncate(random_file_path.c_str(), new_file_size.convert<DataUnit::B>().get_value()); });
        touched_files.push_back(random_file);
        auto duration = action.exec();
        if (fallocatable && !random_file->isPunchable(blocksize)) {
          tree.removeFromPunchableFiles(random_file);
        }
        result.setAction(Result::Action::ALTER_SMALLER_TRUNCATE);
        result.setPath(random_file_path);
        result.setSize(new_file_size);
        result.setDuration(duration);
        break;
      }
      case ALTER_SMALLER_FALLOCATE: {
        logger.debug("ALTER_SMALLER_FALLOCATE");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
#  warning "FALLOCATE not supported on macOS"
        throw std::runtime_error("FALLOCATE not supported on this system");
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
        auto random_file = tree.randomPunchableFile();
        auto random_file_path = random_file->path(true);
        auto block_size = get_block_size().convert<DataUnit::B>().get_value();
        std::tuple<size_t, size_t> hole_address = random_file->getHoleAddress(block_size, true);
        // Round to modulo blocksize
        logger.debug("ALTER_SMALLER_FALLOCATE {} with size {} punched hole {} - {}", random_file_path, random_file->size(), std::get<0>(hole_address), std::get<1>(hole_address));
        int fd = ioengine->open_file(random_file_path.c_str(), O_RDWR, false);
        if (fd == -1) {
          std::cerr << "Error opening file: " << strerror(errno) << std::endl;
          throw std::runtime_error(fmt::format("Error opening file {}", random_file_path));
        }
        MeasuredCBAction action([&]() {
          fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, std::get<0>(hole_address), std::get<1>(hole_address) - std::get<0>(hole_address));
          ioengine->close(fd);
        });
        touched_files.push_back(random_file);
        auto duration = action.exec();
        if (!random_file->isPunchable(block_size)) {
          logger.debug("File {} is not punchable anymore", random_file_path);
          tree.removeFromPunchableFiles(random_file);
        }
        result.setAction(Result::Action::ALTER_SMALLER_FALLOCATE);
        result.setPath(random_file_path);
        result.setSize(DataSize<DataUnit::B>(std::get<1>(hole_address) - std::get<0>(hole_address)));
        result.setDuration(duration);
#else
        throw std::runtime_error("FALLOCATE not supported on this system");
#endif
        break;
      }
      case ALTER_BIGGER: {
        logger.debug("ALTER_BIGGER");
        break;
      }
      case ALTER_BIGGER_FALLOCATE: {
        logger.debug("ALTER_BIGGER_FALLOCATE");
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        auto actual_file_size = fs_utils::file_size(random_file_path);
        auto new_file_size = get_file_size(actual_file_size, DataSize<DataUnit::B>::fromString(getParameter("maxfsize").get_string()).convert<DataUnit::B>().get_value());
        logger.debug("ALTER_BIGGER_FALLOCATE {} from {} to {}", random_file_path, actual_file_size, new_file_size);
        int fd = ioengine->open_file(random_file_path.c_str(), O_RDWR, getParameter("direct_io").get_bool());
        MeasuredCBAction action([&]() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
#  warning "FALLOCATE not supported on macOS"
          throw std::runtime_error("FALLOCATE not supported on this system");

#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
          if (new_file_size.get_value() > actual_file_size) {  // Only expand, never shrink
            if (fallocate(fd, 0, actual_file_size, new_file_size.get_value() - actual_file_size) == -1) {
              perror("fallocate");
              throw std::runtime_error("fallocate failed for file: " + random_file_path);
            }
          } else {
            logger.warn("File {} is already bigger ({}) than new size ({})", random_file_path, actual_file_size, new_file_size.get_value());
          }
#endif
          ioengine->close(fd);
        });
        auto duration = action.exec();
        touched_files.push_back(random_file);
        result.setAction(Result::Action::ALTER_BIGGER_FALLOCATE);
        result.setOperation(Result::Operation::FALLOCATE);
        result.setPath(random_file_path);
        result.setSize(new_file_size - DataSize<DataUnit::B>(actual_file_size));
        result.setDuration(duration);
        break;
      }
      case ALTER_BIGGER_WRITE: {
        logger.debug("ALTER_BIGGER");

        size_t block_size = get_block_size().convert<DataUnit::B>().get_value();
        size_t alignment = 4096;  // Common alignment for O_DIRECT (check with `stat -f`)

        // Ensure block_size is aligned to 4096 bytes
        if (block_size % alignment != 0) {
          block_size = (block_size / alignment) * alignment;  // Round down to nearest 4KB
          logger.debug("Block size adjusted to {} for O_DIRECT", block_size);
        }

        // Allocate aligned memory for O_DIRECT
        void* buf = nullptr;
        if (posix_memalign(&buf, alignment, block_size) != 0) {
          throw std::runtime_error("Failed to allocate aligned memory");
        }

        generate_random_chunk(static_cast<char*>(buf), block_size);

        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        auto actual_file_size = fs_utils::file_size(random_file_path);
        auto new_file_size = get_file_size(actual_file_size, DataSize<DataUnit::B>::fromString(getParameter("maxfsize").get_string()).convert<DataUnit::B>().get_value());

        logger.debug("ALTER_BIGGER_WRITE {} from {} to {}", random_file_path, actual_file_size, new_file_size);

        // Open file without O_APPEND (O_APPEND conflicts with O_DIRECT)
        int fd = ioengine->open_file(random_file_path.c_str(), O_WRONLY, getParameter("direct_io").get_bool());
        if (fd == -1) {
          perror("Error opening file with O_DIRECT");
          free(buf);
          throw std::runtime_error(fmt::format("Error opening file {}", random_file_path));
        }

        auto current_size = lseek(fd, 0, SEEK_END);
        if (current_size == -1) {
          perror("Error getting file size");
          ioengine->close(fd);
          free(buf);
          throw std::runtime_error(fmt::format("Error getting file size {}", random_file_path));
        }

        // Ensure write offset is aligned
        uint64_t write_offset = actual_file_size;
        if (write_offset % alignment != 0) {
          write_offset = (write_offset / alignment) * alignment;  // Round down to nearest 4KB
          logger.debug("Aligning write offset to {}", write_offset);
        }

        MeasuredCBAction action([&]() {
          for (; write_offset < new_file_size.get_value();) {
            // Use pwrite() to avoid O_APPEND issues and ensure aligned writes
            ssize_t written_bytes_ = pwrite(fd, buf, block_size, write_offset);  // todo use ioengine->write
            if (written_bytes_ == -1) {
              perror("Error writing to file");
              ioengine->close(fd);
              free(buf);
              throw std::runtime_error(fmt::format("Error writing to file {}", random_file_path));
            }
            write_offset += written_bytes_;
          }
          ioengine->close(fd);
        });

        auto duration = action.exec();

        logger.debug("ALTER_BIGGER {} from {} to {} in {} ms | Speed {} MB/s", random_file_path, actual_file_size, new_file_size.get_value(), duration.count() / 1000000.0,
                     ((new_file_size.get_value() - actual_file_size) / 1024. / 1024.) / (duration.count() / 1000000000.0));

        free(buf);  // Free aligned memory

        touched_files.push_back(random_file);
        result.setAction(Result::Action::ALTER_BIGGER_WRITE);
        result.setOperation(Result::Operation::WRITE);
        result.setPath(random_file_path);
        result.setSize(new_file_size - DataSize<DataUnit::B>(actual_file_size));
        result.setDuration(duration);
        break;
      }

      case ALTER_METADATA:
        logger.debug("ALTER_METADATA");
        break;
      case DELETE_FILE: {
        auto random_file = tree.randomFile();
        auto random_file_path = random_file->path(true);
        logger.debug("DELETE_FILE {}", random_file_path);
        auto file_extents = random_file->getExtents(true).size();

        MeasuredCBAction action([&]() { std::filesystem::remove(random_file_path); });
        action.exec();
        tree.rm(random_file->path(false));
        result.setAction(Result::Action::DELETE_FILE);
        result.setPath(random_file_path);
        tree.total_extents_count -= file_extents;
        break;
      }
      case DELETE_DIR: {
        logger.debug("DELETE_DIR");
        auto random_dir = tree.randomDirectory();
        auto random_dir_path = random_dir->path(true);
        logger.debug("DELETE_DIR {}", random_dir_path);
        // TODO: remove directory Is it necesary and good idea at all?
        break;
      }
      case END:
        logger.debug("END");
        if (getParameter("sync").get_bool()) {
          logger.debug("Syncing...");
          sync();
        }
        for (auto& file : touched_files) {
          auto original_extents = file->getExtents(false).size();
          auto updated_extents = file->getExtents(true);
          int updated_extents_size = updated_extents.size();

          logger.debug("File {} extents: {} -> {}", file->path(true), original_extents, updated_extents_size);
          tree.total_extents_count += updated_extents_size - original_extents;
          if (!file->isPunchable(get_block_size().convert<DataUnit::B>().get_value())) {
            tree.removeFromPunchableFiles(file);
          }
          if (file->path(true) == result.getPath()) {
            result.setFileExtentCount(updated_extents_size);
          }
        }
        logger.debug("Total extents count: {}, File Count {}, F avail files {}, free space {} MB", tree.total_extents_count, tree.all_files.size(), tree.files_for_fallocate.size(),
                     fs_utils::get_fs_status(getParameter("directory").get_string()).available / 1024 / 1024);
        result.setExtentsCount(tree.total_extents_count);
        result.commit();
        result = Result();
        if (tree.findNullPointer()) {
          throw std::runtime_error("Null pointer found");
        }
        iteration++;
        bar.set_meta("extents", fmt::format("{}", tree.total_extents_count));
        bar.set_meta("f-count", fmt::format("{}", tree.all_files.size()));
        if (extents_curve.isFitted()) {
          bar.set_meta("slope", fmt::format("{:.3f}", extents_curve.slopeAngle()));
        }
        if (getParameter("iterations").get_int() != -1) {
          bar.update(iteration);
        } else {
          bar.update(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start));
        }
        if (touched_files.size() > 0) {
          extents_curve.addPoint(tree.total_extents_count);
        }
        touched_files.clear();
        break;
      default:
        break;
    }
  }

  // Count files and total extent count

  int file_count = 0;
  int total_extents = 0;
  for (auto& file : tree.all_files) {
    file_count++;
    total_extents += file->getExtents().size();
  }
  logger.set_progress_bar(nullptr);
  logger.info("File count: {}, total extents: {}", file_count, total_extents);
  if (getParameter("cleanup").get_bool()) {
    for (auto& file : tree.all_files) {
      std::filesystem::remove(file->path(true));
    }
    tree.bottomUpDirWalk(tree.getRoot(), [&](FileTree::Nodeptr dir) { std::filesystem::remove(dir->path(true)); });
  } else {
    logger.info("Not cleaning up, files will remain in the directory");
  }
}

void AgingScenario::compute_probabilities(std::map<std::string, double>& probabilities, FileTree& tree, PolyCurve& curve) {
  probabilities.clear();
  auto fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());

  // double CAF(double x) { return sqrt(1 - (x * x)); }
  // logger.debug("Capacity: {}, available: {}", fs_status.capacity, fs_status.available);
  // logger.debug("CAF input: {}", ((float(fs_status.capacity - fs_status.available) / float(fs_status.capacity))));

  // Handle case when available space is less than block size. If this happens, we can't write any more data. Even thought the drive isnt completely full.
  auto safe_margin = DataSize<DataUnit::B>::fromString(getParameter("settings-safe-margin").get_string());

  if (fs_status.available <= (safe_margin.get_value() + get_block_size().get_value())) {
    fs_status.available = 0;
  }

  double caf = CAF((float(fs_status.capacity - fs_status.available) / float(fs_status.capacity)));
  caf = floorTo(caf, 3);

  // When there is no files in the system, we must write first file
  // and then we can start to delete files. So we set pC to 1.0
  if (tree.getFileCount() == 0) {
    caf = 1.;
  }
  probabilities["pC"] = caf;
  probabilities["pD"] = 0.1 * (1.0 - caf);
  probabilities["pA"] = 1 - caf - probabilities["pD"];
  probabilities["pAM"] = 0.1;
  probabilities["pAB"] = caf - probabilities["pAM"];

  probabilities["pABF"] = 0;
  probabilities["pABW"] = 1 - probabilities["pABF"];

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
  probabilities["pCFF"] = 0;
  if (rapid_aging) {
    probabilities["pCFF"] = probabilities["pCF"];
    probabilities["pCF"] = 0;
    probabilities["pABF"] = 1;
    probabilities["pABW"] = 0;
  }
  probabilities["pCD"] = 1 - probabilities["pCF"] - probabilities["pCFF"];
  probabilities["pCFO"] = 0.3;
  probabilities["pCFR"] = 0.3;
  probabilities["pCFE"] = 1 - probabilities["pCFO"] - probabilities["pCFR"];
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
    logger.debug(logMessage);
  }
}

DataSize<DataUnit::B> AgingScenario::get_file_size(uint64_t range_from, uint64_t range_to, bool safe) {
  logger.debug("get_file_size({},{},{})", range_from, range_to, safe);
  std::random_device rand_dev;
  std::mt19937 generator(rand_dev());
  DataSize<DataUnit::B> return_size(0);

  if (getParameter("sdist").get_string() == "uniform") {
    std::uniform_int_distribution<uint64_t> distr(range_from, range_to);
    auto d = distr(generator);
    return_size = DataSize<DataUnit::B>(d);
  } else if (getParameter("sdist").get_string() == "normal") {
    double mean = range_to / 2.0;
    double stddev = mean / 2.0;
    std::normal_distribution<double> distr(mean, stddev);
    return_size = DataSize<DataUnit::B>(distr(generator));
  } else {
    throw std::runtime_error("Invalid file size distribution!");
  }

  if (safe) {
    std::filesystem::space_info fs_status = fs_utils::get_fs_status(getParameter("directory").get_string());
    auto block_size = get_block_size().convert<DataUnit::B>().get_value();
    auto safe_margin = DataSize<DataUnit::B>::fromString(getParameter("settings-safe-margin").get_string());
    // Subtract one block as a safety margin from the available space.
    uint64_t safe_available = (fs_status.available > safe_margin.get_value() ? fs_status.available - safe_margin.get_value() : 0);

    logger.debug("return size= {}, free space = {}, safe available = {}", return_size.get_value(), fs_status.available, safe_available);

    if (return_size.get_value() > safe_available) {
      // Align file size to block boundary using the safe available space.
      uint64_t aligned_size = safe_available - (safe_available % block_size);
      return_size = DataSize<DataUnit::B>(aligned_size);
    }
  }

  logger.debug("Return get_file_size = {}", return_size);
  return return_size;
}

DataSize<DataUnit::B> AgingScenario::get_file_size() {
  auto max_size = DataSize<DataUnit::B>::fromString(getParameter("maxfsize").get_string()).convert<DataUnit::B>();
  auto min_size = DataSize<DataUnit::B>::fromString(getParameter("minfsize").get_string()).convert<DataUnit::B>();
  auto fsize = get_file_size(min_size.get_value(), max_size.get_value());
  // logger.debug("get_file_size: {} - {} : {} ", min_size.get_value(), max_size.get_value(), fsize.get_value());
  return fsize;
}
