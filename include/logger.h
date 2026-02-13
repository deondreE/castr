#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

typedef enum {
  LOG_TRACE,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL,
} log_lvl;

void log_output(log_lvl level, const char *file, int line, const char *fmt, ...);

#ifdef CASTR_RELEASE
  #define log_trace(...) ((void)0)
  #define log_debug(...) ((void)0)
  #define log_info(...)  ((void)0)
  #define log_warn(...)  ((void)0)
#else
  #define log_trace(...) log_output(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
  #define log_debug(...) log_output(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
  #define log_info(...)  log_output(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
  #define log_warn(...)  log_output(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#endif

#define log_error(...) log_output(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_output(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif