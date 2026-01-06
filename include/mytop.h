/**
 * @file mytop.h
 * @brief My-Top Core Data Structures and Interface Declarations
 *
 * This file contains the data structure definitions for system snapshots
 * (CPU, memory, version) and the function interfaces for parsing the
 * /proc filesystem.
 */

#ifndef MYTOP_H
#define MYTOP_H

#include <stdint.h>

#define BUFFER_SIZE      1024
#define KERNEL_VER_LEN   64
#define MACHINE_ARCH_LEN 32

typedef enum {
  MYTOP_OK = 0,         // Operation successful
  MYTOP_ERR = -1,       // General error
  MYTOP_ERR_IO = -2,    // File read/write error
  MYTOP_ERR_PARSE = -3  // Data parsing error
} mytop_status_t;

// Used to store system memory information.
typedef struct {
  uint64_t total;      // MemTotal: Total physical memory (kB)
  uint64_t free;       // MemFree: Unused memory (kB)
  uint64_t buffers;    // Buffers: Memory used for block device cache (kB)
  uint64_t cached;     // Cached: Memory used for file cache (kB)
  uint64_t available;  // MemAvailable: Estimated available memory (kB)
  
  uint64_t used;       // Memory used (kB) = Total - Free - Buffers - Cached 
  double used_percent; // Memory usage rate (0.0 - 100.0)
} mem_info_t;

// Used to store system version information.
typedef struct {
  char release[KERNEL_VER_LEN];
  char machine[MACHINE_ARCH_LEN];
} sys_info_t;

/**
 * @brief Parse system version information
 *
 * Retrieve information from the 'uname()' system call for greater stability.
 *
 * @param  sys Pointer to the sys_info_t structure for storing the result.
 * @return mytop_status_t Returns a status code 
 */
mytop_status_t parse_version(sys_info_t *sys);

/**
 * @brief Parse memory information
 *
 * Reads the /proc/meminfo file, parses fields such as Total, Free, Buffers,
 * Cached, and automatically calculates the used and used_percent fields.
 *
 * @param  mem Pointer to the mem_info_t structure for storing the result.
 * @return mytop_status_t Returns a status code.
 */
mytop_status_t parse_meminfo(mem_info_t *mem);


/**
 * @brief Print system snapshot to terminal.
 *
 * Formatted output of system version, machine architecture and memory usage.
 * Kernel : [version]
 * Machine: [Arch]
 * Memory : [Used] MB / [Total] MB ([Percent]%)
 *
 * @param sys Pointer to the populated system information structure.
 * @param mem Pointer to the populated memory information structure
 */
void print_system_snapshot(const sys_info_t *sys, const mem_info_t *mem);

#endif // !MYTOP_H
