
#include "log.h"
#include "mytop.h"
#include "mytop_types.h"
#include <unistd.h>
#include <stdio.h>

int main() {
  g_log_level = LOG_INFO;

  LOG_INFO("Core", "MyTop starting up...");

  sys_info_t sys = {0};
  mem_info_t mem = {0};
  cpu_stat_t prev = {0}, curr = {0};
  proc_list_t *list = create_procs_list(0);
  if (!list) {
    LOG_FATAL("Main", "Failed to allocate memory for process list");
  }

  // 1. Initial sampling
  if (parse_version(&sys) != MYTOP_OK) {
    LOG_FATAL("System", "Failed to parse system info");
  }

  if (parse_meminfo(&mem) != MYTOP_OK) {
    LOG_FATAL("System", "Failed to parse memory info");
  }

  if (parse_cpu_stat(&prev) != MYTOP_OK) {
    LOG_FATAL("CPU", "Cannot read /proc/stat"); 
  }

  if (parse_procs(list) != MYTOP_OK) {
    LOG_FATAL("Process", "Process information parsing failed");
  }

   while (1) {
     // 2. Sampling period: 1 second
     sleep(1);

     // 3. Current sampling
     if (parse_cpu_stat(&curr) != MYTOP_OK) {
       LOG_ERROR("CPU", "Failed to update CPU stats");
       sleep(1);
       continue; 
     }

     if (parse_meminfo(&mem) != MYTOP_OK) {
       LOG_ERROR("System", "Failed to update memory information");
       sleep(1);
       continue;
     }

    if (parse_procs(list) != MYTOP_OK) {
      LOG_ERROR("Process", "Failed to update proc information");
      sleep(1);
      continue;
    }

     // 4. Compute CPU usage percentage
     double cpu_usage = calculate_cpu_usage(&prev, &curr);

     // 5. Simple printing
     printf("\033[2J\033[H");
     print_system_snapshot(&sys, &mem);
     printf("CPU Usage: %.2f%%\n", cpu_usage);
     print_procs(list);

     // 6. Update
     prev = curr;
   }

  return 0;
}
