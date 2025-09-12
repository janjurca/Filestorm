#include <filestorm/result.h>
#include <fstream>

std::vector<Result> Result::results;

std::vector<BasicResult> BasicResult::results;

std::map<std::string, std::string> Result::metas;

void Result::saveAgingState(const std::string& filename, const nlohmann::json& agingState) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error(fmt::format("Failed to open aging state file for writing: {}", filename));
  }
  file << agingState.dump(2);
  file.close();
}

nlohmann::json Result::loadAgingState(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error(fmt::format("Failed to open aging state file for reading: {}", filename));
  }
  nlohmann::json agingState;
  file >> agingState;
  file.close();
  return agingState;
}