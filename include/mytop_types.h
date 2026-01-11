#ifndef MYTOP_TYPES_H
#define MYTOP_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* --------- Status Codes (0 is success, negetive is failure) --------- */
typedef enum {
  MYTOP_OK          =  0,  // Success
  MYTOP_ERR         = -1,  // General error
  MYTOP_ERR_IO      = -2,  // File I/O failure
  MYTOP_ERR_PARSE   = -3,  // Parsing/format error
  MYTOP_ERR_PARAM   = -4,  // Invalid parameter (e.g., NULL pointer)
  MYTOP_ERR_NOMEM   = -5,  // Memory allocation failure
  MYTOP_ERR_RANGE   = -6,  // Numerical overflow
  MYTOP_NO_FILE     = -7,  // File does not exist
  MYTOP_NO_DATA     = -8,  // File is empty
} mytop_status_t;

/* --------- Constant definition --------- */
#define BUFFER_SIZE      1024
#define KERNEL_VER_LEN   64
#define MACHINE_ARCH_LEN 32
#define MAX_CMD_LEN      256
#define DEFAULT_CAPACITY 512

/* --------- Data structure definition --------- */
// System information
typedef struct {
    char release[KERNEL_VER_LEN];
    char machine[MACHINE_ARCH_LEN];
} sys_info_t;

// Memory information
typedef struct {
  uint64_t total;         // MemTotal: Total physical memory (kB)
  uint64_t free;          // MemFree: Unused memory (kB)
  uint64_t buffers;       // Buffers: Memory used for block device cache (kB)
  uint64_t cached;        // Cached: Memory used for file cache (kB)
  uint64_t available;     // MemAvailable: Estimated available memory (kB)
  
  uint64_t used;          // Memory used (kB) = Total - Free - Buffers - Cached 
  double used_percent;    // Memory usage rate (0.0 - 100.0)
} mem_info_t;

// CPU statistics
typedef struct {
  uint64_t user;          // Time the CPU spends executing regular processes in user mode (jiffies)
  uint64_t nice;          // Time the CPU spends executing low-priority processes in user mode (jiffies)
  uint64_t system;        // Time the CPU spends executing in kernel mode (jiffies)
  uint64_t idle;          // Time the CPU is completely idle (jiffies)
  uint64_t iowait;        // CPU idle time, waiting for I/O completion (jiffies)
  uint64_t irq;           // Time the CPU spends on hard interrupts (jiffies)
  uint64_t softirq;       // Time the CPU spends on soft interrupts (jiffies) 
  uint64_t steal;         // Virtualization (jiffies) 
} cpu_stat_t;

// A single process information
typedef struct {
  uint64_t pid;           // (1) Process ID
  char state;             // (3) Process state (R, S, Z, etc.)
  char cmd[MAX_CMD_LEN];  // (2) Filename of the executable (in parenthess)

  uint64_t ppid;          // (4) Parent PID
  uint64_t pgrp;          // (5) Process Group ID

  uint64_t utime;         // (14) User time (jiffies)
  uint64_t stime;         // (15) Kernel time (jiffies)

  uint64_t vsize;         // (23) Virtual memory size (byte)
  uint64_t rss;           // (24) Resident Set Size (Number of pages of physical memory 
                          //      actually occupied by the process)
  double cpu_percent;     
} proc_info_t;

// Process list container
typedef struct {
  proc_info_t *procs;
  size_t count;
  size_t capacity;
} proc_list_t;

// Sort status
typedef enum {
    SORT_CPU,
    SORT_MEM,
    SORT_PID
} sort_mode_t;

#endif // !MYTOP_TYPES_H
