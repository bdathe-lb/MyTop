
#include "log.h"
#include "mytop.h"

int main() {
  sys_info_t sys = {0};
  if (parse_version(&sys) != MYTOP_OK) {
    LOG_ERROR("System", "Failed to parse system info");
    return 1;
  }
  
  mem_info_t mem = {0};
  if (parse_meminfo(&mem) != MYTOP_OK) {
    LOG_ERROR("System", "Failed to parse memory info");
    return 1;
  }

  print_system_snapshot(&sys, &mem);

  return 0;
}
