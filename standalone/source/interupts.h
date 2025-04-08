#pragma once

#include <filestorm/utils/logger.h>

#include "config.h"

void sigint_handler(int signum) {
  logger.error("SIGINT received. Saving up to now results...");
  config.selected_senario->save();
  logger.error("SIGINT received. Exiting...");
  exit(signum);
}
