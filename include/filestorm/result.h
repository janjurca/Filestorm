#pragma once

#include <filestorm/data_sizes.h>
#include <filestorm/filefrag.h>
#include <filestorm/filetree.h>
#include <filestorm/utils/logger.h>

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

class Result {
public:
  enum Action { DELETE_FILE, CREATE_FILE, CREATE_FILE_OVERWRITE, CREATE_FILE_READ, CREATE_DIR, ALTER_SMALLER_TRUNCATE, ALTER_SMALLER_FALLOCATE, ALTER_BIGGER, NONE };
  enum Operation { WRITE, TRIM, OVERWRITE, READ };
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