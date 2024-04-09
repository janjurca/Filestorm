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
  FileActionAttributes(bool time_based, DataSize<DataUnit::B> block_size, DataSize<DataUnit::KB> file_size, std::string file_path, std::chrono::seconds time_limit);

  DataSize<DataUnit::B> get_block_size() const;
  DataSize<DataUnit::KB> get_file_size() const;
  std::string get_file_path() const;
  std::chrono::milliseconds get_time_limit() const;
  bool is_time_based() const;
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

class MeasuredCBAction : public VirtualMeasuredAction {
public:
  std::function<void()> m_cb;

  MeasuredCBAction(std::function<void()> cb) : m_cb(cb) {}

  void work() override { m_cb(); }
};

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
  VirtualMonitoredAction(std::chrono::milliseconds monitoring_interval, std::function<void(VirtualMonitoredAction*)> on_log);

  ~VirtualMonitoredAction();

  void exec();
  virtual void work();
  void _log_values();
  virtual void log_values();
  void start_monitor();

  void stop_monitor();

  void addMonitoredData(std::string name, float value);

  std::chrono::milliseconds get_interval();
  std::map<std::string, std::vector<std::tuple<std::chrono::nanoseconds, float>>> get_monitored_data();
  std::chrono::nanoseconds get_monitor_started_at();
};
