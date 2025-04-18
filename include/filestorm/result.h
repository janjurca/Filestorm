#pragma once

#include <filestorm/data_sizes.h>
#include <filestorm/filefrag.h>
#include <filestorm/filetree.h>
#include <filestorm/utils/logger.h>

#include <chrono>
#include <filestorm/external/tabulate.hpp>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>

template <typename T> struct Statistics {
  T min;
  T max;
  T mean;
  T median;
  T stddev;
  T q1;
  T q3;
};

class Result {
public:
  enum Action {
    DELETE_FILE,
    CREATE_FILE,
    CREATE_FILE_FALLOCATE,
    CREATE_FILE_OVERWRITE,
    CREATE_FILE_READ,
    CREATE_DIR,
    ALTER_SMALLER_TRUNCATE,
    ALTER_SMALLER_FALLOCATE,
    ALTER_BIGGER,
    ALTER_BIGGER_WRITE,
    ALTER_BIGGER_FALLOCATE,
    NONE,
  };
  enum Operation { WRITE, TRIM, OVERWRITE, READ, FALLOCATE };
  static std::vector<Result> results;

private:
  int _iteration;
  Action _action;
  Operation _operation;
  std::string _path;
  DataSize<DataUnit::B> _size;
  std::chrono::nanoseconds _duration;
  int64_t _total_extents_count;
  int64_t _file_extent_count;

public:
  Result(int iteration, Action action, Operation operation, std::string path, DataSize<DataUnit::B> size, std::chrono::nanoseconds duration, int64_t extents_count, int64_t file_extent_count)
      : _iteration(iteration), _action(action), _operation(operation), _path(path), _size(size), _duration(duration), _total_extents_count(extents_count), _file_extent_count(file_extent_count) {}
  Result() = default;

  int getIteration() const { return _iteration; }
  Action getAction() const { return _action; }
  Operation getOperation() const { return _operation; }
  std::string getPath() const { return _path; }
  DataSize<DataUnit::B> getSize() const { return _size; }
  std::chrono::nanoseconds getDuration() const { return _duration; }
  int64_t getExtentsCount() const { return _total_extents_count; }
  int64_t getFileExtentCount() const { return _file_extent_count; }
  DataSize<DataUnit::B> getThroughput() const {
    if (_duration.count() == 0) {
      return DataSize<DataUnit::B>(0);
    }
    return DataSize<DataUnit::B>(((double)_size.get_value()) / ((double)_duration.count() / 1'000'000'000.0));
  }

  // setters
  void setIteration(int iteration) { _iteration = iteration; }
  void setAction(Action action) { _action = action; }
  void setOperation(Operation operation) { _operation = operation; }
  void setPath(std::string path) { _path = path; }
  void setSize(DataSize<DataUnit::B> size) { _size = size; }
  void setDuration(std::chrono::nanoseconds duration) { _duration = duration; }
  void setExtentsCount(int64_t extents_count) { _total_extents_count = extents_count; }
  void setFileExtentCount(int64_t file_extent_count) { _file_extent_count = file_extent_count; }

  void commit() { results.push_back(*this); }

  static constexpr const char* actionToString(Action action) {
    switch (action) {
      case DELETE_FILE:
        return "DELETE_FILE";
      case CREATE_FILE:
        return "CREATE_FILE";
      case CREATE_FILE_FALLOCATE:
        return "CREATE_FILE_FALLOCATE";
      case CREATE_DIR:
        return "CREATE_DIR";
      case CREATE_FILE_OVERWRITE:
        return "CREATE_FILE_OVERWRITE";
      case CREATE_FILE_READ:
        return "CREATE_FILE_READ";
      case ALTER_SMALLER_TRUNCATE:
        return "ALTER_SMALLER_TRUNCATE";
      case ALTER_SMALLER_FALLOCATE:
        return "ALTER_SMALLER_FALLOCATE";
      case ALTER_BIGGER:
        return "ALTER_BIGGER";
      case ALTER_BIGGER_WRITE:
        return "ALTER_BIGGER_WRITE";
      case ALTER_BIGGER_FALLOCATE:
        return "ALTER_BIGGER_FALLOCATE";
      case NONE:
        return "NONE";
      default:
        throw std::runtime_error("Unknown Action");
    }
  }

  static constexpr const char* operationToString(Operation operation) {
    switch (operation) {
      case WRITE:
        return "WRITE";
      case TRIM:
        return "TRIM";
      case OVERWRITE:
        return "OVERWRITE";
      case READ:
        return "READ";
      case FALLOCATE:
        return "FALLOCATE";
      default:
        logger.debug("Unknown Operation id {}", (int)operation);
        return "Unknown Operation";
    }
  }

  static void save(const std::string& filename) {
    // Save all results to a JSON file
    std::ofstream file(filename);
    if (file.is_open()) {
      nlohmann::json jsonResults;
      for (const auto& result : results) {
        nlohmann::json jsonResult;
        jsonResult["iteration"] = result.getIteration();
        jsonResult["action"] = actionToString(result.getAction());
        jsonResult["operation"] = operationToString(result.getOperation());
        jsonResult["path"] = result.getPath();
        jsonResult["size"] = result.getSize().get_value();
        jsonResult["duration"] = result.getDuration().count();
        jsonResult["total_extents_count"] = result.getExtentsCount();
        jsonResult["file_extent_count"] = result.getFileExtentCount();
        jsonResults.push_back(jsonResult);
      }
      file << jsonResults.dump(2);
      file.close();
    }
  }

  static void clear() { results.clear(); }

  static std::set<Action> getUsedActions() {
    std::set<Action> usedActions;
    for (const auto& result : results) {
      usedActions.insert(result.getAction());
    }
    return usedActions;
  }

  static void print() {
    std::set<Action> usedActions = getUsedActions();
    tabulate::Table table;
    table.add_row({"Action", "Mean (MB/s)", "Min (MB/s)", "Q1 (MB/s)", "Median (MB/s)", "Q3 (MB/s)", "Max (MB/s)", "Stddev (MB/s)"});

    for (const auto& action : usedActions) {
      std::vector<Result> actionResults = getActionResults(action);
      if (actionResults.empty()) {
        continue;
      }

      Statistics<double> stats_throughput = getStatistics<double>(actionResults, "throughput");

      auto formatNumber = [](double value) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << value;
        return stream.str();
      };

      table.add_row({actionToString(action), formatNumber(stats_throughput.mean / (1024 * 1024)), formatNumber(stats_throughput.min / (1024 * 1024)), formatNumber(stats_throughput.q1 / (1024 * 1024)),
                     formatNumber(stats_throughput.median / (1024 * 1024)), formatNumber(stats_throughput.q3 / (1024 * 1024)), formatNumber(stats_throughput.max / (1024 * 1024)),
                     formatNumber(stats_throughput.stddev / (1024 * 1024))});
    }

    table.format().font_align(tabulate::FontAlign::center).border_left("|").border_right("|");

    std::cout << table << std::endl;
  }

  static std::vector<Result> getActionResults(Action action) {
    std::vector<Result> actionResults;
    for (const auto& result : results) {
      if (result.getAction() == action) {
        actionResults.push_back(result);
      }
    }
    return actionResults;
  }

  template <typename T> static Statistics<T> getStatistics(std::vector<Result> results, std::string unit) {
    std::vector<T> data;
    for (const auto& result : results) {
      if (unit == "duration") {
        data.push_back(static_cast<T>(result.getDuration().count()));
      } else if (unit == "throughput") {
        DataSize<DataUnit::B> throughput = result.getThroughput();
        data.push_back(static_cast<T>(throughput.get_value()));
      } else {
        throw std::runtime_error("Unknown unit");
      }
    }
    if (data.empty()) {
      throw std::runtime_error("No data for the given action");
    }
    Statistics<T> stats = getStatistics(data);
    return stats;
  }

  template <typename T> static Statistics<T> getStatistics(std::vector<T> data) {
    Statistics<T> stats;
    if (data.empty()) {
      throw std::runtime_error("No data for the given action");
    }
    std::sort(data.begin(), data.end());
    stats.min = data.front();
    stats.max = data.back();
    stats.mean = std::accumulate(data.begin(), data.end(), static_cast<T>(0)) / data.size();
    stats.median = (data.size() % 2 == 0) ? (data[data.size() / 2 - 1] + data[data.size() / 2]) / 2 : data[data.size() / 2];
    T sum = T(0);
    for (const auto& data_item : data) {
      sum += (data_item - stats.mean) * (data_item - stats.mean);
    }
    stats.stddev = std::sqrt((static_cast<double>(sum)) / static_cast<double>(data.size()));
    stats.q1 = data[data.size() / 4];
    stats.q3 = data[3 * data.size() / 4];
    return stats;
  }
};

class BasicResult {
protected:
  std::map<std::string, std::string> _result;
  std::chrono::nanoseconds timestamp;

public:
  static std::vector<BasicResult> results;
  BasicResult() = default;
  void addResult(std::string key, std::string value) { _result[key] = value; }
  void setTimestamp(std::chrono::nanoseconds timestamp) { this->timestamp = timestamp; }

  void commit() { results.push_back(*this); }

  static void save(const std::string& filename) {
    // Save all results to a JSON file
    std::ofstream file(filename);
    if (file.is_open()) {
      nlohmann::json jsonResults;
      for (const auto& result : results) {
        nlohmann::json jsonResult;
        jsonResult["timestamp"] = result.timestamp.count();
        for (const auto& pair : result._result) {
          jsonResult[pair.first] = pair.second;
          logger.debug("Timestamp: {} | Action: {} | Value: {}", result.timestamp.count(), pair.first, pair.second);
        }
        jsonResults.push_back(jsonResult);
      }
      file << jsonResults.dump(2);
      file.close();
    }
  }
};