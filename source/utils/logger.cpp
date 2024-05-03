#include <filestorm/utils/logger.h>

FilestormLogger logger;

void ProgressBar::clear_line() {
  std::cout << "\r";
  for (int i = 0; i < width; i++) {
    std::cout << " ";
  }
  std::cout << "\r";
}

void ProgressBar::update(int current) {
  this->current = current;
  clear_line();
  print_bar();
}

void ProgressBar::print_bar() {
  if (!is_active()) {
    return;
  }

  if (current == 0) {
    start = std::chrono::steady_clock::now();
  }
  auto end = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  int previous_width = width;
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
  struct winsize size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
  width = size.ws_col;
#endif
  if (previous_width != 0) {
    clear_line();
  }
  float progress = (float)current / total;
  std::string infos = "";
  if (unit_type == UnitType::Count) {
    std::chrono::duration<double> remaining = std::chrono::duration<double>((total - current) / (current / (elapsed / 1000.0)));
    double it_per_s = current / (elapsed / 1000.0);

    infos = fmt::format("[{}/{}][{:.1f} it/s][{:.1f}s Remaining]", current, total, it_per_s, remaining.count());
  } else {
    std::chrono::duration<int> done = std::chrono::duration<int>(current);
    std::chrono::duration<int> total_duration = std::chrono::duration<int>(total);
    infos = fmt::format("[{}/{}]", fmt::format("{:%H:%M:%S}", done), fmt::format("{:%H:%M:%S}", total_duration));
  }
  std::string prefix_infos = fmt::format("{} {} %", label, (int)(progress * 100));
  int bar_width = width - prefix_infos.size() - infos.size() - 2;
  int progress_width = (int)(progress * bar_width);
  std::string bar = "";
  for (int i = 0; i < progress_width; i++) {
    bar += "=";
  }
  for (int i = progress_width; i < bar_width; i++) {
    bar += " ";
  }
  std::cout << prefix_infos << "[" << bar << "]" << infos;
  std::cout.flush();
  if (current == total) {
    return;
    disable();
  }
}

ProgressBar& ProgressBar::operator++() {
  current++;
  clear_line();
  print_bar();
  return *this;
}

ProgressBar ProgressBar::operator++(int) {
  ProgressBar temp = *this;
  ++*this;
  return temp;
}