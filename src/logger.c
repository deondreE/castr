#include "logger.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

static const char* lvl_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char* lvl_colors[] = {
  "\x1b[94m", // TRACE (Blue)
  "\x1b[36m", // DEBUG (Cyan)
  "\x1b[32m", // INFO  (Green)
  "\x1b[33m", // WARN  (Yellow)
  "\x1b[31m", // ERROR (Red)
  "\x1b[35m"  // FATAL (Magenta)
};

void log_output(log_lvl level, const char* file, int line, const char* fmt, ...) {
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  char time_buf[32];
  strftime(time_buf, sizeof(time_buf), "%H:%M:%S", lt);

  fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    time_buf, lvl_colors[level], lvl_strings[level], file, line);

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fprintf(stderr, "\n");
  fflush(stderr);
}