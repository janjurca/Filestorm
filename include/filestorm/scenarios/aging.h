#pragma once

#include <filestorm/scenarios/register.h>
#include <filestorm/scenarios/scenario.h>

class AgingScenario : public Scenario {
protected:
  enum States {
    S,
    CREATE,
    ALTER,
    DELETE,
    CREATE_FILE,
    CREATE_FILE_FALLOCATE,
    CREATE_FILE_OVERWRITE,
    CREATE_FILE_READ,
    CREATE_DIR,
    ALTER_SMALLER,
    ALTER_SMALLER_TRUNCATE,
    ALTER_SMALLER_FALLOCATE,
    ALTER_BIGGER,
    ALTER_BIGGER_WRITE,
    ALTER_BIGGER_FALLOCATE,
    ALTER_METADATA,
    DELETE_FILE,
    DELETE_DIR,
    END,
  };

  bool rapid_aging = false;
  void compute_probabilities(std::map<std::string, double>& probabilities, FileTree& tree, PolyCurve& curve);
  int open_file(const char* path, int flags, bool direct_io);

public:
  AgingScenario();
  ~AgingScenario();
  void run(std::unique_ptr<IOEngine>& ioengine) override;
  double CAF(double x) { return sqrt(1 - (x * x)); }
  DataSize<DataUnit::B> get_file_size();
  DataSize<DataUnit::B> get_file_size(uint64_t min, uint64_t max, bool safe = true);
  DataSize<DataUnit::B> get_block_size() { return DataSize<DataUnit::B>::fromString(getParameter("blocksize").get_string()); };
};

REGISTER_SCENARIO(AgingScenario);