#include "log.h"
#include "mytop.h"
#include "mytop_types.h"
#include "utils.h"
#include <signal.h>
#include <stdint.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  g_log_level = LOG_INFO;

  LOG_INFO("Core", "MyTop starting up...");

  sort_mode_t sort_mode = SORT_CPU;

  proc_list_t *prev_procs_list = create_procs_list(0);
  if (!prev_procs_list) return 1;
  proc_list_t *curr_procs_list = create_procs_list(0);
  if (!curr_procs_list) {
    free_procs_list(prev_procs_list);
    return 1;
  }

  sys_info_t sys_info = {0};
  mem_info_t mem_info = {0};
  cpu_stat_t prev_cpu_info = {0}, curr_cpu_info = {0};
  
  // 1. Initial sampling
  parse_version(&sys_info);
  parse_meminfo(&mem_info);
  parse_cpu_stat(&prev_cpu_info);
  parse_procs(prev_procs_list);

  // Activacate Raw Mode
  if (set_raw_mode(true) != 0) {
    LOG_WARN("Term", "Failed to enable raw mode");
  }
  
  // Hide the cursor
  term_hide_cursor();

  sleep(1);

  uint64_t total_delta;
  int running = 1;
  while (running) {
    // 2. Data acquisition
    parse_cpu_stat(&curr_cpu_info);
    parse_meminfo(&mem_info);

    curr_procs_list->count = 0;
    parse_procs(curr_procs_list);

    // 3. Compute CPU usage percentage and processes CPU usage percentage
    double cpu_usage = calculate_cpu_usage(&prev_cpu_info, &curr_cpu_info, &total_delta);
    calculate_procs_cpu(prev_procs_list, curr_procs_list, total_delta);

    // 4. Sort by CPU usage percentage
    sort_procs_by_mode(curr_procs_list, sort_mode);

    // 5. Simple printing
    // Move cursor to top‑left corner and clear screen
    term_home();
    term_clear_screen();
    // Print system and memory related informations
    print_system_snapshot(&sys_info, &mem_info);
    printf("CPU Usage: %.2f%%\n", cpu_usage);
    printf("\n");
    // Print processes informations
    print_procs(curr_procs_list);
    // Force a flush; otherwise, output may be buffered in Raw Mode
    term_refresh();

    // 6. Update
    prev_cpu_info = curr_cpu_info;
    
    proc_list_t *temp = prev_procs_list;
    prev_procs_list = curr_procs_list;
    curr_procs_list = temp;

    // 7. Intelligent delay (wait for 1 second or until a key press interrupts)
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
          else if (c == 'm' || c == 'M') {
            sort_mode = SORT_MEM;
          }
          else if (c == 'p' || c == 'P') {
            sort_mode = SORT_PID;
          }
          else if (c == 'c' || c == 'C') {
            sort_mode = SORT_CPU;
          }
          else if (c == 'k' || c == 'K') {
            // 1. Move cursor to the bottom‑most position (rows, 1)
            int rows, cols;
            get_term_size(&rows, &cols);
            term_move_cursor(rows, 1);
            // 2. Switch blocking mode
            if (set_raw_mode(false) != 0) {
              LOG_WARN("Term", "Failed to enable normal mode");
            }
            // 3. Clear line
            term_clear_line();
            // 4. Print promt
            printf("PID to kill: ");
            term_refresh();
            // 5. Read input
            char pid_buf[32];
            term_show_cursor();
            term_read_line(pid_buf, sizeof(pid_buf));
            term_hide_cursor();
            // 6. Kill
            int64_t pid;
            str_to_num(pid_buf, 10, NUM_I64,  &pid);
            if (pid > 0) {
              if (kill(pid, SIGTERM) == 0) {
                printf("\nSignal sent to PID %d", (int)pid);
              } else {
                LOG_ERROR("Main", "Failed to send signal");
              }
            }
            // 7. Pause for a moment to let the user see the results
            term_refresh();
            sleep(1);

            // 8. Switch blocking mode
            if (set_raw_mode(true) != 0) {
              LOG_WARN("Term", "Failed to enable raw mode");
            }
          }
        }
      }
    }
  }

  // Restore cursor visibility
  term_show_cursor();

  // Restore terminal mode
  set_raw_mode(false);

  free_procs_list(prev_procs_list);
  free_procs_list(curr_procs_list);

  LOG_INFO("Core", "MyTop exited gracefully.");

  return 0;
}
