#pragma once
#include <fcntl.h>
#include <filestorm/parameter.h>
#include <filestorm/utils/logger.h>
#include <fmt/format.h>
#include <sys/stat.h>  // for S_IRWXU
#include <sys/types.h>
#include <unistd.h>  // for close

#include <filestorm/external/cxxopts.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class IOEngine {
protected:
  std::vector<Parameter> _parameters = {};

public:
  virtual ~IOEngine() = default;
  virtual std::string name() const = 0;
  virtual std::string description() const = 0;
  virtual ssize_t read(int fd, void* buf, size_t count) = 0;
  virtual ssize_t write(int fd, void* buf, size_t count) = 0;
  virtual void sync(int fd) {
    if (fsync(fd) == -1) {
      perror("Error syncing file");
      throw std::runtime_error("Error syncing file");
    }
  }
  virtual int close(int fp) {
    if (fp != -1) {
      return ::close(fp);
    }
    return EBADF;
  };

  std::vector<Parameter> parameters() const { return _parameters; };
  void addParameter(Parameter parameter) { _parameters.push_back(parameter); };

  Parameter getParameter(const std::string& name) const {
    for (auto parameter : parameters()) {
      if (parameter.long_name() == name) {
        return parameter;
      }
    }
    throw std::invalid_argument(fmt::format("Parameter {} not found.", name));
  }

  virtual std::string setup(int argc, char** argv) {
    cxxopts::Options options("filestorm", fmt::format("IO Engine: {}", name()));
    options.custom_help(fmt::format("{} [Options...]", name()));
    for (auto parameter : parameters()) {
      if (parameter.has_value())
        if (parameter.short_name().empty())
          options.add_options()(parameter.long_name(), parameter.description(), cxxopts::value<std::string>()->default_value(parameter.value()));
        else
          options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description(), cxxopts::value<std::string>()->default_value(parameter.value()));
      else
        options.add_options()(fmt::format("{},{}", parameter.short_name(), parameter.long_name()), parameter.description());
    }

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      return options.help();
    }
    // set values in parameters from parsed result
    for (auto& parameter : _parameters) {
      if (result.count(parameter.long_name())) {
        std::string value = result[parameter.long_name()].as<std::string>();
        logger.debug("Setting {} to {}", parameter.long_name(), value);
        parameter.value(value);
      }
    }
    return "";
  };

  int open_file(const char* path, int flags, bool direct_io) {
    int fd;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  error "Windows is not supported"
#elif __APPLE__
    fd = open(path, flags, S_IRWXU);
    if (direct_io && fd != -1) {
      if (fcntl(fd, F_NOCACHE, 1) == -1) {
        ::close(fd);
        throw std::runtime_error("WriteMonitoredAction::work: error on fcntl F_NOCACHE");
      }
    }
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    if (direct_io) {
      flags |= O_DIRECT;
    }
    fd = open(path, flags, S_IRWXU);
#else
#  error "Unknown system"
#endif
    if (fd == -1) {
      std::cerr << "Error opening file: " << strerror(errno) << std::endl;
      if (errno == EINVAL) {
        throw std::runtime_error("Error: Direct IO not supported");
      }
      throw std::runtime_error(fmt::format("Error opening file {}", path));
    }
    return fd;
  }
};
