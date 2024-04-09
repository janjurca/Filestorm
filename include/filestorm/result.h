#pragma once

#include <filestorm/data_sizes.h>
#include <filestorm/filefrag.h>
#include <filestorm/filetree.h>

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

class Result {
public:
  enum Action { DELETE_FILE, CREATE_FILE, CREATE_DIR, ALTER_SMALLER_TRUNCATE, ALTER_SMALLER_FALLOCATE, ALTER_BIGGER };
  enum Operation { WRITE, TRIM };
  static std::vector<Result> results;

private:
  int _iteration;
  Action _action;
  Operation _operation;
  std::string _path;
  DataSize<DataUnit::B> _size;
  std::chrono::nanoseconds _duration;
  int64_t _extents_count;
  std::vector<extents> _extents;

public:
  Result(int iteration, Action action, Operation operation, std::string path, DataSize<DataUnit::B> size, std::chrono::nanoseconds duration, int64_t extents_count)
      : _iteration(iteration), _action(action), _operation(operation), _path(path), _size(size), _duration(duration), _extents_count(extents_count) {}
  Result() = default;

  int getIteration() const { return _iteration; }
  Action getAction() const { return _action; }
  Operation getOperation() const { return _operation; }
  std::string getPath() const { return _path; }
  DataSize<DataUnit::B> getSize() const { return _size; }
  std::chrono::nanoseconds getDuration() const { return _duration; }
  int64_t getExtentsCount() const { return _extents_count; }

  // setters
  void setIteration(int iteration) { _iteration = iteration; }
  void setAction(Action action) { _action = action; }
  void setOperation(Operation operation) { _operation = operation; }
  void setPath(std::string path) { _path = path; }
  void setSize(DataSize<DataUnit::B> size) { _size = size; }
  void setDuration(std::chrono::nanoseconds duration) { _duration = duration; }
  void setExtentsCount(int64_t extents_count) { _extents_count = extents_count; }
  void setExtents(std::vector<extents> extents) {
    for (auto& extent : extents) {
      _extents.push_back(extent);
    }
  }

  void commit() { results.push_back(*this); }

  static constexpr const char* actionToString(Action action) {
    switch (action) {
      case DELETE_FILE:
        return "DELETE_FILE";
      case CREATE_FILE:
        return "CREATE_FILE";
      case CREATE_DIR:
        return "CREATE_DIR";
      case ALTER_SMALLER_TRUNCATE:
        return "ALTER_SMALLER_TRUNCATE";
      case ALTER_SMALLER_FALLOCATE:
        return "ALTER_SMALLER_FALLOCATE";
      case ALTER_BIGGER:
        return "ALTER_BIGGER";
      default:
        return "Unknown Action";
    }
  }

  static constexpr const char* operationToString(Operation operation) {
    switch (operation) {
      case WRITE:
        return "WRITE";
      case TRIM:
        return "TRIM";
      default:
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
        jsonResult["extents_count"] = result.getExtentsCount();
        jsonResults.push_back(jsonResult);
      }
      file << jsonResults.dump(2);
      file.close();
    }
  }
};
