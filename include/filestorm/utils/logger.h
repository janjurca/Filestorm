#pragma once
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <map>

class ProgressBar {
  int total;
  int width;
  bool active = true;
  int current = 0;
  std::chrono::time_point<std::chrono::steady_clock> start;
  std::string label;
  enum class UnitType { Time, Count };

  UnitType unit_type;
  bool speeds_line_printed = false;

  std::map<std::string, std::string> metas = {};
  std::map<std::string, double> operation_speeds = {};
  struct OperationSpeedContainer {
    double sum = 0;
    int count = 0;
  };
  std::map<std::string, OperationSpeedContainer> operation_speeds_container;

public:
  ProgressBar(int total, const std::string label) : total(total), label(label) {
    unit_type = UnitType::Count;
    print_bar();
  }
  ProgressBar(std::chrono::seconds total, const std::string label) : total(total.count()), label(label) {
    unit_type = UnitType::Time;
    print_bar();
  }
  ProgressBar(const std::string label) : total(0), label(label) {}

  void clear_line(bool overwrite = true);
  void disable() { active = false; }

  void update(int current);
  void update(std::chrono::seconds current) { update(current.count()); }
  bool is_active() { return active; }
  void print_bar(bool clear = false);
  void set_meta(const std::string &key, const std::string &value) { metas[key] = value; }
  void get_meta(const std::string &key) { metas[key]; }
  void clear_meta(const std::string &key) { metas.erase(key); }
  void set_operation_speed(const std::string &operation, double speed_mb_s) {
    operation_speeds[operation] = speed_mb_s;
    operation_speeds_container[operation].sum += speed_mb_s;
    operation_speeds_container[operation].count++;
  }
  void clear_operation_speeds() {
    operation_speeds.clear();
    operation_speeds_container.clear();
  }
  void set_total(int total);
  void set_total(std::chrono::seconds total);

  ProgressBar &operator++();
  ProgressBar operator++(int);
};

#define PRINT_WITH_CLEAR(...)            \
  {                                      \
    if (m_progress_bar != nullptr) {     \
      if (m_progress_bar->is_active()) { \
        m_progress_bar->clear_line();    \
        __VA_ARGS__;                     \
        logger->flush();                 \
        m_progress_bar->print_bar();     \
      } else {                           \
        __VA_ARGS__;                     \
      }                                  \
    } else {                             \
      __VA_ARGS__;                       \
    }                                    \
  }

class FilestormLogger {
protected:
  ProgressBar *m_progress_bar = nullptr;
  std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("filestorm");

public:
  std::shared_ptr<spdlog::logger> get_logger() { return logger; }
  template <typename... Args> inline void trace(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->trace(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void debug(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->debug(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void info(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->info(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void warn(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->warn(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void error(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->error(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void critical(const std::string fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->critical(fmt, std::forward<Args>(args)...)); }

  template <typename T> inline void trace(const T &msg) { PRINT_WITH_CLEAR(logger->trace(msg);) }
  template <typename T> inline void debug(const T &msg) { PRINT_WITH_CLEAR(logger->debug(msg);) }
  template <typename T> inline void info(const T &msg) { PRINT_WITH_CLEAR(logger->info(msg);) }
  template <typename T> inline void warn(const T &msg) { PRINT_WITH_CLEAR(logger->warn(msg);) }
  template <typename T> inline void error(const T &msg) { PRINT_WITH_CLEAR(logger->error(msg);) }
  template <typename T> inline void critical(const T &msg) { PRINT_WITH_CLEAR(logger->critical(msg);) }

  void set_progress_bar(ProgressBar *progress_bar) {
    m_progress_bar = progress_bar;
    if (!isatty(fileno(stdout)) && m_progress_bar != nullptr) {
      m_progress_bar->disable();
      warn("Progress bar disabled because stdout is not a tty");
    }
    logger->flush();
  }
};

extern FilestormLogger logger;