#pragma once

#include <filestorm/data_sizes.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class FileActionAttributes {
private:
  const bool m_time_based;
  const DataSize<DataUnit::B> m_block_size;
  const DataSize<DataUnit::KB> m_file_size;
  const std::string m_file_path;
  const std::chrono::seconds m_time_limit;

public:
  FileActionAttributes(bool time_based, DataSize<DataUnit::B> block_size,
                       DataSize<DataUnit::KB> file_size, std::string file_path,
                       std::chrono::seconds time_limit)
      : m_time_based(time_based),
        m_block_size(block_size),
        m_file_size(file_size),
        m_file_path(file_path),
        m_time_limit(time_limit) {}
  bool is_time_based() const { return m_time_based; }
  DataSize<DataUnit::B> get_block_size() const { return m_block_size; }
  DataSize<DataUnit::KB> get_file_size() const { return m_file_size; }
  inline std::string get_file_path() const { return m_file_path; }
  std::chrono::seconds get_time_limit() const { return m_time_limit; }
};

class ActionMonitor;

/**
 * @brief Virtual class which represents a general action which could be executed and measured its
 * work time.
 *
 */
class VirtualMeasuredAction {
private:
  std::chrono::nanoseconds last_duration;
  bool is_executed;

public:
  VirtualMeasuredAction(/* args */);

  std::chrono::nanoseconds exec();

  virtual void work() { throw "Not implemented"; }
  std::chrono::nanoseconds getLastDuration();
};

VirtualMeasuredAction::VirtualMeasuredAction() {}

std::chrono::nanoseconds VirtualMeasuredAction::exec() {
  auto start = std::chrono::high_resolution_clock::now();
  work();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  last_duration = duration;
  return duration;
}

std::chrono::nanoseconds VirtualMeasuredAction::getLastDuration() {
  if (!is_executed) {
    throw "Action haven't been yet executed";
  }
  return last_duration;
}

/**
 * @brief Virtual class for monitored long-term actions. Action have defined values which should be
 * monitored in specified intervals. This class spawns new thread which monitors action and stores
 * its values.
 *
 */
class VirtualMonitoredAction {
protected:
  std::chrono::milliseconds m_interval;
  std::atomic<bool> m_running = false;
  std::thread m_thread;
  std::map<std::string, std::vector<std::tuple<std::chrono::nanoseconds, float>>> m_monitoredData;
  mutable std::mutex m_monitoredDataMutex;
  std::chrono::nanoseconds m_monitor_started_at;
  // function callback
  std::function<void(VirtualMonitoredAction*)> m_on_log;

public:
  VirtualMonitoredAction(std::chrono::milliseconds monitoring_interval,
                         std::function<void(VirtualMonitoredAction*)> on_log)
      : m_interval(monitoring_interval), m_on_log(on_log){};

  ~VirtualMonitoredAction() {
    if (m_thread.joinable()) {
      stop_monitor();
      m_thread.join();
    }
  }

  void exec() {
    spdlog::debug("VirtualMonitoredAction::exec");
    start_monitor();
    work();
    stop_monitor();
    spdlog::debug("VirtualMonitoredAction::exec finished");
  }

  virtual void work() { throw "Not implemented"; }

  void _log_values() {
    log_values();
    if (m_on_log) {
      m_on_log(this);
    }
  }
  virtual void log_values() { throw "Not implemented"; }

  void start_monitor() {
    m_monitor_started_at = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    spdlog::debug("VirtualMonitoredAction::start_monitor started at: {}",
                  m_monitor_started_at.count());
    if (!m_running) {
      m_running = true;
      m_thread = std::thread([this]() {
        while (m_running) {
          spdlog::debug("VirtualMonitoredAction::start_monitor: logging values");
          _log_values();
          std::this_thread::sleep_for(get_interval());
        }
      });
    }
    spdlog::debug("VirtualMonitoredAction::start_monitor finished");
  }

  void stop_monitor() {
    if (m_running) {
      m_running = false;
      if (m_thread.joinable()) {
        m_thread.join();
      }
    }
  }

  void addMonitoredData(std::string name, float value) {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    if (m_monitoredData.find(name) == m_monitoredData.end()) {
      m_monitoredData[name] = std::vector<std::tuple<std::chrono::nanoseconds, float>>();
    }
    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    auto duration = now - m_monitor_started_at;
    auto value_tuple = std::make_tuple(duration, value);
    m_monitoredData[name].push_back(value_tuple);
  }

  std::chrono::milliseconds get_interval() { return m_interval; }
  std::map<std::string, std::vector<std::tuple<std::chrono::nanoseconds, float>>>
  get_monitored_data() {
    return m_monitoredData;
  }
  std::chrono::nanoseconds get_monitor_started_at() { return m_monitor_started_at; }
};
