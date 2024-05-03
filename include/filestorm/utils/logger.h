#pragma once
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

class ProgressBar {
  int total;
  int width;
  bool active = true;
  int current = 0;
  std::chrono::time_point<std::chrono::steady_clock> start;
  std::string label;
  enum class UnitType { Time, Count };

  UnitType unit_type;

public:
  ProgressBar(int total, const std::string label) : total(total), label(label) {
    unit_type = UnitType::Count;
    print_bar();
  }
  ProgressBar(std::chrono::seconds total, const std::string label) : total(total.count()), label(label) {
    unit_type = UnitType::Time;
    print_bar();
  }

  void clear_line();
  void disable() { active = false; }

  void update(int current);
  bool is_active() { return active; }
  void print_bar();

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
  template <typename... Args> inline void trace(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->trace(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void debug(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->debug(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void info(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->info(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->warn(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void error(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->error(fmt, std::forward<Args>(args)...)); }
  template <typename... Args> inline void critical(spdlog::format_string_t<Args...> fmt, Args &&...args) { PRINT_WITH_CLEAR(logger->critical(fmt, std::forward<Args>(args)...)); }

  template <typename T> inline void trace(const T &msg) { PRINT_WITH_CLEAR(logger->trace(msg);) }
  template <typename T> inline void debug(const T &msg) { PRINT_WITH_CLEAR(logger->debug(msg);) }
  template <typename T> inline void info(const T &msg) { PRINT_WITH_CLEAR(logger->info(msg);) }
  template <typename T> inline void warn(const T &msg) { PRINT_WITH_CLEAR(logger->warn(msg);) }
  template <typename T> inline void error(const T &msg) { PRINT_WITH_CLEAR(logger->error(msg);) }
  template <typename T> inline void critical(const T &msg) { PRINT_WITH_CLEAR(logger->critical(msg);) }

  void set_progress_bar(ProgressBar *progress_bar) {
    m_progress_bar = progress_bar;
    logger->flush();
  }
};

extern FilestormLogger logger;