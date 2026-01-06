
#include "log.h"
#include "mytop.h"
#include <unistd.h>
#include <stdio.h>

int main() {
  sys_info_t sys = {0};
  mem_info_t mem = {0};
  cpu_stat_t prev = {0}, curr = {0};

  // 1. Initial sampling
  if (parse_version(&sys) != MYTOP_OK) {
    LOG_ERROR("System", "Failed to parse system info");
    return 1;
  }

  if (parse_meminfo(&mem) != MYTOP_OK) {
    LOG_ERROR("System", "Failed to parse memory info");
    return 1;
  }

  if (parse_cpu_stat(&prev) != MYTOP_OK) {
    LOG_FATAL("CPU", "Cannot read /proc/stat"); 
  }

  while (1) {
    // 2. Sampling period: 1 second
    sleep(1);

    // 3. Current sampling
    if (parse_cpu_stat(&curr) != MYTOP_OK) {
      LOG_ERROR("CPU", "Read failed");
      continue; 
    }

    if (parse_meminfo(&mem) != MYTOP_OK) {
      LOG_ERROR("System", "Failed to parse memory info");
      continue;
    }

    // 4. Compute CPU usage percentage
    double cpu_usage = calculate_cpu_usage(&prev, &curr);

    // 5. Simple printing
    printf("\033[2J\033[H");
    print_system_snapshot(&sys, &mem);
    printf("CPU Usage: %.2f%%\n", cpu_usage);

    // 6. Update
    prev = curr;
  }

  return 0;
}
