#include "log.h"
#include "mytop.h"
#include "mytop_types.h"
#include "utils.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>

/**
 * Helper function
 *
 * @brief Reads the /proc/[pid]/cmdline file to 
 *        obtain the process's command line.
 *
 * @param path   File name.
 * @param out    Buffer to write the cmdline into upon successful read.
 * @param out_sz Size of the out buffer.
 *
 * @return 
 *  - MYTOP_OK on success.
 *  - MYTOP_NO_FILE if the file does not exist.
 *  - MYTOP_NO_DATA if the file content is empty.
 *  - MYTOP_ERR_PARAM on parameter error.
 *  - MYTOP_ERR for other errors.
 */
static mytop_status_t read_cmdline(const char *path, char *out, size_t out_sz) {
  // Check input parameters
  if (!path || !out || out_sz == 0)
    return MYTOP_ERR_PARAM;

  out[0] = '\0';

  FILE *fp = fopen(path, "r");
  if (!fp) {
    if (errno == ENOENT || errno == EACCES)
      return MYTOP_NO_FILE;
     int err = errno;
     LOG_ERROR("Process", "Cannot open /proc/stat file: %s", strerror(err));
     return MYTOP_ERR_IO;
  }

  char buf[BUFFER_SIZE];
  size_t n = fread(buf, 1, sizeof(buf) - 1, fp);

  if (n == 0) {
    // Error
    if (ferror(fp)) {
      fclose(fp);
      return MYTOP_ERR;
    }
    // Content is empty
    fclose(fp);
    return MYTOP_NO_DATA;
  }

  fclose(fp);

  buf[n] = '\0';

  // Replace the middle '\0' with a space
  for (size_t i = 0; i < n; i++) {
    if (buf[i] == '\0') buf[i] = ' ';
  }

  // Remove trailing spaces (typically the last \0 is replaced with ' ')
  while (n > 0 && buf[n - 1] == ' ') {
    buf[n - 1] = '\0';
    n--;
  }

  // Copy to output
  snprintf(out, out_sz, "%s", buf);

  if (out[0] == '\0')
    return MYTOP_NO_DATA;

  return MYTOP_OK;
}

/**
 * Helper function
 *
 * @brief Reads the /proc/[pid]/comm file to 
 *        obtain the process's comm info.
 *
 * @param path   File name.
 * @param out    Buffer to write the cmdline into upon successful read.
 * @param out_sz Size of the out buffer.
 *
 * @return 
 *  - MYTOP_OK on success.
 *  - MYTOP_NO_FILE if the file does not exist.
 *  - MYTOP_NO_DATA if the file content is empty.
 *  - MYTOP_ERR_PARAM on parameter error.
 *  - MYTOP_ERR for other errors.
 */
static mytop_status_t read_comm(char *path, char *out, size_t out_sz) {
  // Check input parameters
  if (!path || !out || out_sz == 0)
    return MYTOP_ERR_PARAM;
  
  out[0] = '\0';

  FILE *fp = fopen(path, "r");
  if (!fp) {
    int err = errno;
    LOG_ERROR("Process", "Cannot open /proc/stat file: %s", strerror(err));
    return MYTOP_ERR_IO;
  }

  char buf[BUFFER_SIZE];
  size_t n = fread(buf, 1, sizeof(buf) - 1, fp);

  if (n == 0) {
    // Error
    if (ferror(fp)) {
      fclose(fp);
      return MYTOP_ERR;
    }
    // Content is empty
    fclose(fp);
    return MYTOP_NO_DATA;
  }

  fclose(fp);

  // Note that there is a \n at the end of comm
  buf[n-1] = '\0';

  // Copy to output
  snprintf(out, out_sz, "%s", buf);

  if (out[0] == '\0')
    return MYTOP_NO_DATA;

  return MYTOP_OK;
}

/**
 * Helper function
 *
 * Reads the /proc/[pid]/stat file to obtain process information 
 * such as status, pid, ppid, etc.
 *
 * @param path  File name.
 * @param info  Structure to store the parsed process information.
 *
 * @return 
 *  - MYTOP_OK on success.
 *  - MYTOP_ERR_PARAM on parameter error.
 *  - MYTOP_ERR for other errors.
 */
static mytop_status_t read_stat(const char *path, proc_info_t *info) {
  // Check input parameters
  if (!path || !info)
     return MYTOP_ERR_PARAM;

  FILE *fp = fopen(path, "r");
  if (!fp) {
    int err = errno;
    LOG_ERROR("Process", "Cannot open /proc/stat file: %s", strerror(err));
    return MYTOP_ERR;
  }

  char buf[BUFFER_SIZE];
  size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
  if (n == 0) {
    fclose(fp);
    return MYTOP_ERR;
  }
  fclose(fp);

  buf[n] = '\0';
  
  // Find the last )
  char *end_paren = strrchr(buf, ')');
  // Invalid input or no matching minimum parentheses
  if (!end_paren || end_paren - buf < 2)
    return MYTOP_ERR_PARSE;

  char *rest = end_paren + 2;

  // Skip the first two fields
  int field_index = 3;

  // Parse the remaining fields
  mytop_status_t ret;
  char *save = NULL;
  char *token = strtok_r(rest, " ", &save);

  while (token && *token) {
    switch (field_index) {
      case 3: 
        info->state = token[0]; 
        break;
      case 4: 
        ret = str_to_num(token, 10, NUM_U64, &info->ppid);
        if (ret != MYTOP_OK) return ret;
        break;
      case 5:
        ret = str_to_num(token, 10, NUM_U64, &info->pgrp);
        if (ret != MYTOP_OK) return ret;
        break;
      case 14:
        ret = str_to_num(token, 10, NUM_U64, &info->utime);
        if (ret != MYTOP_OK) return ret;
        break;
      case 15:
        ret = str_to_num(token, 10, NUM_U64, &info->stime);
        if (ret != MYTOP_OK) return ret;
        break;
      case 23:
        ret = str_to_num(token, 10, NUM_U64, &info->vsize);
        if (ret != MYTOP_OK) return ret;
        break;
      case 24:
        ret = str_to_num(token, 10, NUM_U64, &info->rss);
        if (ret != MYTOP_OK) return ret;
        break;
      default: 
        break;
    }

    field_index ++;

    token = strtok_r(NULL, " ", &save);
  }

  return MYTOP_OK;
}

/**
 * @brief Create a new stored process information list.
 */
proc_list_t *create_procs_list(size_t capacity_hint) {
  size_t capacity = capacity_hint == 0 ?
                    DEFAULT_CAPACITY   :
                    capacity_hint;

  proc_list_t *list = malloc(sizeof(proc_list_t));
  if (!list)
    return NULL;

  list->procs = malloc(sizeof(proc_info_t) * capacity);
  if (!list->procs) {
    free(list);
    return NULL;
  }

  list->capacity = capacity;
  list->count = 0;

  return list;
}

/**
 * @brief Free memory occupied by the process list.
 */
void free_procs_list(proc_list_t *list) {
  if (!list)
    return;

  // Free array
  free(list->procs);

  // Free list
  free(list);
}

/**
 * @brief Scan and parse all current processes
 *
 * 1. Traverse the /proc directory.
 * 2. Filter out numeric directories.
 * 3. Read /proc/[pid]/stat to parse detailed information.
 * 4. Store results in the list container (automatically expands as needed).
 *
 * @param list Result storage container (must be initialized before calling, or pass an existing list to reuse memory)
 * @return mytop_status_t
 */
mytop_status_t parse_procs(proc_list_t *list) {
  // Check input parameters
  if (!list)
    return MYTOP_ERR_PARAM;

  // 1. Traverse the /proc directories
  // Open /proc directory
  DIR *dir = opendir("/proc");
  if (!dir) {
    int err = errno;
    LOG_ERROR("Process", "Cannot open /proc directory: %s", strerror(err));
    return MYTOP_ERR_IO;
  }
  
  // Loop to read directory entries
  struct dirent *dt;
  while ((dt = readdir(dir)) != NULL) {
    // Filter out ./ and ../ directories
    if (strcmp(dt->d_name, ".") == 0 ||
        strcmp(dt->d_name, "..") == 0)
      continue;

    // Filter out directories that are not purely numeric in name
    if (!is_numeric_name(dt->d_name))
      continue;

    /* ------ 1. Read /proc/[pid]/cmdline file --------- */
    // Concatenate paths
    mytop_status_t ret;
    char file[64];
    int n;
   
    n = snprintf(file, sizeof(file), 
                     "/proc/%s/%s", dt->d_name, "cmdline");
    if (n < 0)
      return MYTOP_ERR;

    // Try to read cmdline
    ret = read_cmdline(file, list->procs[list->count].cmd, MAX_CMD_LEN);
    
    // Error
    if (ret == MYTOP_ERR || ret == MYTOP_ERR_PARAM) {
      return ret;
    }
    // File not is exit
    else if (ret == MYTOP_NO_FILE) {
      continue;
    }
    // Need to read /proc/[pid]/comm file
    else if (ret == MYTOP_NO_DATA) {
      memset(file, 0, sizeof(file));
      n = snprintf(file, sizeof(file), 
                     "/proc/%s/%s", dt->d_name, "comm");
      if (n < 0)
        return MYTOP_ERR;

      ret = read_comm(file, list->procs[list->count].cmd, MAX_CMD_LEN);
      if (ret != MYTOP_OK) 
        return ret;
    }

    // Store pid field
    ret = str_to_num(dt->d_name, 10, NUM_U64, &list->procs[list->count].pid);
    if (ret != MYTOP_OK)
      return ret;

    /* ------ 2. Read /proc/[pid]/cmdline stat --------- */
    // Concatenate paths
    memset(file, 0, sizeof(file));
    
    n = snprintf(file, sizeof(file), 
                    "/proc/%s/%s", dt->d_name, "stat");
    if (n < 0)
      return MYTOP_ERR;

    ret = read_stat(file, &list->procs[list->count]);
    if (ret != MYTOP_OK) 
      return ret;
    
    // Capacity full
    if (list->count >= list->capacity) {
      size_t new_cap = list->capacity * 2;
      proc_info_t *new_arr = realloc(list->procs, sizeof(proc_info_t) * new_cap);
      if (!new_arr) 
        return MYTOP_ERR_NOMEM;

      list->procs = new_arr;
      list->capacity = new_cap;
    }
    list->count ++;
  }

  closedir(dir);
  return MYTOP_OK;
}

/**
 * @brief Debug print: Output information of the top N processes.
 */
void print_procs(const proc_list_t *list) {
  if (!list) 
    return;

  int rows, cols;
  // Get terminal width and length
  get_term_size(&rows, &cols);
  int reserved_lines = 3 + 1 + 1;
  int max_procs_to_show = rows - reserved_lines;
  if (max_procs_to_show < 0) max_procs_to_show = 0;

  // Read system time unit and page size
  const long hz = sysconf(_SC_CLK_TCK);
  const long pagesize_l = sysconf(_SC_PAGESIZE);
  // sysconf fails and returns -1
  const uint64_t pagesize = (pagesize_l > 0) ? (uint64_t)pagesize_l : 4096u;
  // Fixed column width definition
  const int W_PID   = 6;
  const int W_PPID  = 6;
  const int W_PGRP  = 6;
  const int W_VIRT  = 8;
  const int W_RES   = 8;
  const int W_TIME  = 10;

  int fixed_width =
      W_PID + 1 +   /* PID + space */
      1     + 1 +   /* S + space   */
      W_PPID+ 1 +   /* ...         */
      W_PGRP+ 1 +
      W_VIRT+ 1 +
      W_RES + 1 +
      W_TIME+ 1;
  
  // Compute COMMAND field width
  int cmd_width = cols - fixed_width;
  if (cmd_width < 10) cmd_width = 10;      
  if (cmd_width > 80) cmd_width = 80;

  // Print table header
  printf("%*s %s %*s %*s %*s %*s %*s %s\n",
           W_PID,  "PID",
           "S",
           W_PPID, "PPID",
           W_PGRP, "PGRP",
           W_VIRT, "VIRT",
           W_RES,  "RES",
           W_TIME, "TIME+",
           "COMMAND");
  
  size_t limit = (size_t)max_procs_to_show;
  if (limit > list->count) limit = list->count;

  for (size_t i = 0; i < limit; i++) {
    const proc_info_t *p = &list->procs[i];
    uint64_t virt_kb = mem_uint_convert(p->vsize, MEM_B, MEM_KIB);
    uint64_t res_kb  = pages_to_kb(p->rss, pagesize);

    char timebuf[16];
    format_time_hms(timebuf, sizeof(timebuf), p->utime + p->stime, hz);
    printf("%*" PRIu64 " %c %*" PRIu64 " %*" PRIu64 " %*" PRIu64 " %*" PRIu64 " %*s %-.*s\n",
            W_PID,  p->pid,
            p->state,
            W_PPID, p->ppid,
            W_PGRP, p->pgrp,
            W_VIRT, virt_kb,
            W_RES,  res_kb,
            W_TIME, timebuf,
            cmd_width, p->cmd);
  }
}
