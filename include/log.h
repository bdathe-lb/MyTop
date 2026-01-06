/*
** log.h -- A simple logging system with the format:
**          [Timestamp] [Log] [Level] [PID] [Module] [Message]
*/

#ifndef __LOG_H__
#define __LOG_H__

#include <unistd.h>

typedef enum {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL
} log_level_t;

// Controls the minimum log level that the current program will output.
// For example:
// IF 'g_log_level = LOG_INFO'; THEN:
// * DEBUG -> not printed
// * INFO/WARN/ERROR -> printed
extern log_level_t g_log_level;

void log_write(log_level_t level,
               const char *module,
               const char *file,
               int line,
               const char *fmt, ...);

#define LOG_DEBUG(module, fmt, ...) \
  log_write(LOG_DEBUG, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(module, fmt, ...) \
  log_write(LOG_INFO, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(module, fmt, ...) \
  log_write(LOG_WARN, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(module, fmt, ...) \
  log_write(LOG_ERROR, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(module, fmt, ...) \
  log_write(LOG_FATAL, module, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif  // !__LOG_H__
