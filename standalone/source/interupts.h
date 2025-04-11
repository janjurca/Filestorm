#pragma once

#include <filestorm/utils/logger.h>

#include "config.h"

void sigint_handler(int signum) {
  logger.set_progress_bar(nullptr);
  logger.info("SIGINT received. Saving up to now results...");
  config.selected_senario->save();
  config.selected_senario->print();
  exit(signum);
}
