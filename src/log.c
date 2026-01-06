/*
** log.c -- Used to implement the interfaces defined in log.h
*/

#include "log.h"
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

log_level_t g_log_level = LOG_DEBUG;

static const char *level_str[] = {
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL"
};

void log_write(log_level_t level,
               const char *module,
               const char *file,
               int line,
               const char *fmt, ...) {
  if (level < g_log_level)
    return;

  // second + microsecond
  struct timeval tv;
  // get timestamps (since 1970-01-01 00:00:00)
  gettimeofday(&tv, NULL);

  struct tm tm;
  // convert UTC seconds to local time
  localtime_r(&tv.tv_sec, &tm);

  char timebuf[64];
  // convert time structure to string
  strftime(timebuf, sizeof(timebuf),
           "%Y-%m-%d %H:%M:%S", &tm);

  fprintf(stderr,
          "[%s.%03ld] [%s] [PID:%u] [%s] ",
          timebuf,
          tv.tv_usec / 1000,
          level_str[level],
          getpid(),
          module
  );

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, " (%s:%d)", file, line);

  if (level >= LOG_ERROR) {
    fprintf(stderr, " | errno=%d (%s)", errno, strerror(errno));
  }

  fprintf(stderr, "\n");

  if (level == LOG_FATAL) {
    fflush(stderr);
    _exit(1);
  }
}
