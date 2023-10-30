#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
private:
  ActionMonitor m_monitor;

public:
  VirtualMonitoredAction(std::chrono::milliseconds monitoring_interval
                         = std::chrono::milliseconds(100))
      : m_monitor(*this, std::chrono::milliseconds(monitoring_interval)){};

  void exec() {
    m_monitor.start();
    work();
    m_monitor.stop();
  }

  virtual void work() { throw "Not implemented"; }

  virtual int monitor() { throw "Not implemented"; }

  virtual std::vector<std::string> monitoredDataNames() { throw "Not implemented"; }
};

/**
 * @brief Class which monitors action and stores its values in specified intervals.
 *
 */
class ActionMonitor {
private:
  VirtualMonitoredAction& m_action;
  std::chrono::milliseconds m_interval;
  std::atomic<bool> m_running;
  std::thread m_thread;
  std::map<std::string, std::vector<float>> m_monitoredData;
  mutable std::mutex m_monitoredDataMutex;

public:
  ActionMonitor(VirtualMonitoredAction& action, std::chrono::milliseconds interval)
      : m_action(action), m_interval(interval), m_running(false) {}

  ~ActionMonitor() {
    if (m_thread.joinable()) {
      stop();
      m_thread.join();
    }
  }

  void start() {
    if (!m_running) {
      m_running = true;
      m_thread = std::thread([this]() {
        while (m_running) {
          m_action.monitor();  // TODO handle which value should be monitored
          std::this_thread::sleep_for(m_interval);
        }
      });
    }
  }

  void stop() {
    if (m_running) {
      m_running = false;
      if (m_thread.joinable()) {
        m_thread.join();
      }
    }
  }

  void initMonitoredDataNames(std::vector<std::string> names) {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    for (auto name : names) {
      m_monitoredData[name] = std::vector<float>();
    }
  }

  void addMonitoredData(std::string name, float value) {
    std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
    m_monitoredData[name].push_back(value);
  }
};