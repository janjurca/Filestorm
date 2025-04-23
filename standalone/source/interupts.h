#pragma once

#include <filestorm/result.h>
#include <filestorm/utils/logger.h>
#include <fmt/format.h>

#include <iostream>
#include <string>

#include "config.h"

void sigint_handler(int signum) {
  logger.set_progress_bar(nullptr);
  logger.info("SIGINT received. Here are the results up to now...");
  Result::print();
  std::string filename = fmt::format("results_unfinished_{}.json", std::time(nullptr));
  logger.info("Saving results to {}", filename);
  Result::save(filename);

  exit(signum);
}
