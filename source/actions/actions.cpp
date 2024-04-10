#include <filestorm/actions/actions.h>

FileActionAttributes::FileActionAttributes(bool time_based, DataSize<DataUnit::B> block_size, DataSize<DataUnit::KB> file_size, std::string file_path, std::chrono::seconds time_limit)
    : m_time_based(time_based), m_block_size(block_size), m_file_size(file_size), m_file_path(file_path), m_time_limit(time_limit) {}

bool FileActionAttributes::is_time_based() const { return m_time_based; }
DataSize<DataUnit::B> FileActionAttributes::get_block_size() const { return m_block_size; }
DataSize<DataUnit::KB> FileActionAttributes::get_file_size() const { return m_file_size; }

std::string FileActionAttributes::get_file_path() const { return m_file_path; }
std::chrono::milliseconds FileActionAttributes::get_time_limit() const { return m_time_limit; }

VirtualMeasuredAction::VirtualMeasuredAction() {}

std::chrono::nanoseconds VirtualMeasuredAction::exec() {
  auto start = std::chrono::high_resolution_clock::now();
  work();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  last_duration = duration;
  return duration;
}

std::chrono::nanoseconds VirtualMeasuredAction::VirtualMeasuredAction::getLastDuration() {
  if (!is_executed) {
    throw "Action haven't been yet executed";
  }
  return last_duration;
}

VirtualMonitoredAction::VirtualMonitoredAction(std::chrono::milliseconds monitoring_interval, std::function<void(VirtualMonitoredAction*)> on_log)
    : m_interval(monitoring_interval), m_on_log(on_log){};

VirtualMonitoredAction::~VirtualMonitoredAction() {
  if (m_thread.joinable()) {
    stop_monitor();
    m_thread.join();
  }
}

void VirtualMonitoredAction::exec() {
  start_monitor();
  work();
  stop_monitor();
}

void VirtualMonitoredAction::work() { throw "Not implemented"; }

void VirtualMonitoredAction::_log_values() {
  log_values();
  if (m_on_log) {
    m_on_log(this);
  }
}
void VirtualMonitoredAction::log_values() { throw "Not implemented"; }

void VirtualMonitoredAction::start_monitor() {
  m_monitor_started_at = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
  spdlog::debug("VirtualMonitoredAction::start_monitor started at: {}", m_monitor_started_at.count());
  if (!m_running) {
    m_running = true;
    m_thread = std::thread([this]() {
      while (m_running) {
        _log_values();
        std::this_thread::sleep_for(get_interval());
      }
    });
  }
  spdlog::debug("VirtualMonitoredAction::start_monitor finished");
}

void VirtualMonitoredAction::stop_monitor() {
  if (m_running) {
    m_running = false;
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }
}

void VirtualMonitoredAction::addMonitoredData(std::string name, float value) {
  std::lock_guard<std::mutex> lock(m_monitoredDataMutex);
  if (m_monitoredData.find(name) == m_monitoredData.end()) {
    m_monitoredData[name] = std::vector<std::tuple<std::chrono::nanoseconds, float>>();
  }
  auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
  auto duration = now - m_monitor_started_at;
  auto value_tuple = std::make_tuple(duration, value);
  m_monitoredData[name].push_back(value_tuple);
}

std::chrono::milliseconds VirtualMonitoredAction::get_interval() { return m_interval; }
std::map<std::string, std::vector<std::tuple<std::chrono::nanoseconds, float>>> VirtualMonitoredAction::get_monitored_data() { return m_monitoredData; }
std::chrono::nanoseconds VirtualMonitoredAction::get_monitor_started_at() { return m_monitor_started_at; }
