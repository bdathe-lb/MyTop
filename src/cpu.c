#include "log.h"
#include "mytop.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Parse /proc/stat to obtain global CPU data.
 *
 * Only reads the first line (starting with "cpu "), 
 * filling the first 8 numerical values into the structure.
 *
 * @param stat Stores the parsing result.
 *
 * @return mytop_status_t 
 */
mytop_status_t parse_cpu_stat(cpu_stat_t *stat) {
  // Check input parameters
  if (!stat)
    return MYTOP_ERR_PARSE;
  
  FILE *fp = fopen("/proc/stat", "r");
  if (!fp) {
    int err = errno;
    LOG_ERROR("CPU", "Cannot open /proc/stat file: %s", strerror(err));
    return MYTOP_ERR_IO;
  }

  // Read the first line
  char line[BUFFER_SIZE];
  char label[16];
  char *p;
  if (fgets(line, sizeof line, fp)) {

    // Skip spaces
    for (p = line; *p && *p == ' '; ++ p);

    // Verify it starts with "cpu"
    if (strncmp(p, "cpu ", 4) != 0) {
      fclose(fp);
      return MYTOP_ERR_PARSE;
    }

    // Parse fields
    int n = sscanf(
      line,
      "%15s %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
              " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64,
      label,
      &stat->user,
      &stat->nice,
      &stat->system,
      &stat->idle,
      &stat->iowait,
      &stat->irq,
      &stat->softirq,
      &stat->steal
    );

    if (n != 9) {
      LOG_WARN("CPU", "Unexpected format in /proc/stat. Expected 9 fields, got %d", n);
      fclose(fp);
      return MYTOP_ERR_PARSE;
    }
  }

  return MYTOP_OK;
}

/**
 * @brief Calculate CPU usage based on two samples.
 *
 * @param prev        Data from the previous sample.
 * @param curr        Data from the current sample.
 * @param total_delta Total CPU time interval, used for subsequent calculation
 *                    of process CPU usage percentage. [out]
 *
 * @return double CPU usage percentage (0.0 - 100.0)
 */
double calculate_cpu_usage(const cpu_stat_t *prev, const cpu_stat_t *curr, uint64_t *total_delta) {
  // 1. Compute idle time and total time of prev
  uint64_t prev_idle = prev->idle + prev->iowait;
  uint64_t prev_nonidle = prev->user + prev->nice + prev->system +
                          prev->irq + prev->softirq + prev->steal; 
  uint64_t prev_total = prev_idle + prev_nonidle;

  // 2. Compute idle time and total time of curr
  uint64_t curr_idle = curr->idle + curr->iowait;
  uint64_t curr_nonidle = curr->user + curr->nice + curr->system +
                          curr->irq + curr->softirq + curr->steal; 
  uint64_t curr_total = curr_idle + curr_nonidle;

  // 3. Compute the delta_idle time and delta_total time
  //    The contents of `/proc/stat` are a set of monotonically
  //    increasing counters, where the values represent cumulative totals
  uint64_t delta_idle = curr_idle - prev_idle;
  uint64_t delta_total = curr_total - prev_total;
  
  *total_delta = delta_total;

  if (delta_total == 0) 
    return 0.0;

  // 4. Compute cpu usage percentage
  return ((double)(delta_total - delta_idle) / delta_total) * 100;
}
