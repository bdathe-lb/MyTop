#include "log.h"
#include "mytop.h"
#include "mytop_types.h"
#include "utils.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  g_log_level = LOG_INFO;

  LOG_INFO("Core", "MyTop starting up...");

  proc_list_t *list = create_procs_list(0);
  if (!list) return 1;

  sys_info_t sys = {0};
  mem_info_t mem = {0};
  cpu_stat_t prev = {0}, curr = {0};
  
  // 1. Initial sampling
  parse_version(&sys);
  parse_meminfo(&mem);
  parse_cpu_stat(&prev);
  parse_procs(list);

  // Activacate Raw Mode
  if (set_raw_mode(true) != 0) {
    LOG_WARN("Term", "Failed to enable raw mode");
  }
  
  // Hide the cursor
  term_hide_cursor();

  int running = 1;
  while (running) {
    // 2. Data acquisition
    parse_cpu_stat(&curr);
    parse_meminfo(&mem);

    list->count = 0;
    parse_procs(list);

    // 3. Compute CPU usage percentage
    double cpu_usage = calculate_cpu_usage(&prev, &curr);

    // 4. Simple printing
    // Move cursor to top‑left corner and clear screen
    term_home();
    term_clear();
    // Print system and memory related informations
    print_system_snapshot(&sys, &mem);
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    printf("\n");
    // Print processes informations
    print_procs(list);
    // Force a flush; otherwise, output may be buffered in Raw Mode
    term_refresh();

    // 5. Update
    prev = curr;

    // 6. Intelligent delay (wait for 1 second or until a key press interrupts)
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);

    if (ret > 0) {
      if (FD_ISSET(STDIN_FILENO, &fds)) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
          if (c == 'q' || c == 'Q') {
            running = 0;
          }
        }
      }
    }
  }

  // Restore cursor visibility
  term_show_cursor();

  // Restore terminal mode
  set_raw_mode(false);

  free_procs_list(list);

  LOG_INFO("Core", "MyTop exited gracefully.");

  return 0;
}
