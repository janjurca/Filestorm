#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class FileActionStrategy {
private:
  bool m_time_based;
  std::chrono::milliseconds m_interval;

public:
  FileActionStrategy(bool time_based, std::chrono::milliseconds interval)
      : m_time_based(time_based), m_interval(interval) {}
  bool is_time_based() { return m_time_based; }
  std::chrono::milliseconds get_interval() { return m_interval; }
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
  std::atomic<bool> m_running;
  std::thread m_thread;
  std::map<std::string, std::vector<float>> m_monitoredData;
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
    start_monitor();
    work();
    stop_monitor();
  }

  virtual void work() { throw "Not implemented"; }

  void _log_values() {
    if (m_on_log) {
      m_on_log(this);
    }
    log_values();
  }
  virtual void log_values() { throw "Not implemented"; }

  void start_monitor() {
    m_monitor_started_at = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());

    if (!m_running) {
      m_running = true;
      m_thread = std::thread([this]() {
        while (m_running) {
          _log_values();
          std::this_thread::sleep_for(m_interval);
        }
      });
    }
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
    m_monitoredData[name].push_back(value);
  }
};
