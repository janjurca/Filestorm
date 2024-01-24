#pragma once

#include <filestorm/data_sizes.h>
#include <filestorm/filetree.h>

class Result {
public:
  enum Operation { WRITE, READ, DELETE_FILE, CREATE_FILE, CREATE_DIR };

private:
  int _iteration;
  Operation _operation;
  std::string _path;
  DataSize<DataUnit::B> _size;
  std::chrono::nanoseconds _duration;

public:
  Result(int iteration, Operation operation, std::string path, DataSize<DataUnit::B> size, std::chrono::nanoseconds duration)
      : _iteration(iteration), _operation(operation), _path(path), _size(size), _duration(duration) {}

  int getIteration() const { return _iteration; }
  Operation getOperation() const { return _operation; }
  std::string getPath() const { return _path; }
  DataSize<DataUnit::B> getSize() const { return _size; }
  std::chrono::nanoseconds getDuration() const { return _duration; }
};
