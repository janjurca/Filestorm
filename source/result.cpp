#include <filestorm/result.h>

std::vector<Result> Result::results;

std::vector<BasicResult> BasicResult::results;

std::map<std::string, std::string> Result::metas;

std::map<Result::Action, std::chrono::nanoseconds> Result::total_duration_per_action;
std::map<Result::Action, DataSize<DataUnit::B>> Result::total_size_per_action;