
#include "mytop.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>

typedef enum {
  UNIT_KB,
  UNIT_MB,
  UNIT_GB,
  UNIT_TB,
} mem_uint_t;

enum {
  F_MEMTOTAL     = 1u << 0,
  F_MEMFREE      = 1u << 1,
  F_MEMAVAILABLE = 1u << 2,
  F_BUFFERS      = 1u << 3,
  F_CACHED       = 1u << 4,
};

#define ALL_MEM_BITS \
   (F_MEMTOTAL | F_MEMFREE | F_MEMAVAILABLE | F_BUFFERS | F_CACHED)

struct field {
  char    *name;       // Field name
  size_t   name_len;   // Field name length
  size_t   offset;     // The offset of this field within mem_info_t
  uint64_t bit;        // Bit flags (to avoid duplicate counting/repeated writes)
} fields[] = {
  {"MemTotal", sizeof("MemTotal") - 1, offsetof(mem_info_t, total), F_MEMTOTAL},
  {"MemFree", sizeof("MemFree") - 1, offsetof(mem_info_t, free), F_MEMFREE},
  {"MemAvailable", sizeof("MemAvailable") - 1, offsetof(mem_info_t, available), F_MEMAVAILABLE},
  {"Buffers", sizeof("Buffers") - 1, offsetof(mem_info_t, buffers), F_BUFFERS},
  {"Cached", sizeof("Cached") - 1, offsetof(mem_info_t, cached), F_CACHED},
};

/**
 * @brief Define the "conversion factor" for each unit.
 */
static uint64_t unit_factor(mem_uint_t unit) {
  uint64_t fac = 0;
  switch (unit) {
    case UNIT_KB: fac = 1;                  break;
    case UNIT_MB: fac = 1024;               break;
    case UNIT_GB: fac = 1024 * 1024;        break;
    case UNIT_TB: fac = 1024 * 1024 * 1024; break;
    default: break;;
  }

  return fac;
}

/**
 * @brief Perform computer storage unit conversion.
 */
static double mem_uint_convert(uint64_t value, mem_uint_t from, mem_uint_t to) {
  return (double)(value * unit_factor(from)) / unit_factor(to);
}

/**
 * @brief Extract memory information from a line of /proc/meminfo.
 *
 * Parses a line from /proc/meminfo in the format "FieldName: 123456 kB"
 * and extracts the value if the field is in the target list.
 *
 * @param mem        Pointer to memory info structure to store results.
 * @param buffer     Null-terminated input line from /proc/meminfo.
 * @param fields     Array of field names to extract.
 * @param parsed_cnt Pointer to counter for successfully parsed fields.
 *
 * @return
 *   - MYTOP_OK          Field parsed successfully or not in target list.
 *   - MYTOP_ERR_PARSE   Failed to parse numeric value.
 *   - MYTOP_ERR         Numeric overflow or other error.
 */
static mytop_status_t extract_field_info(mem_info_t *mem, char *line, 
                                         const struct field *fields, 
                                         size_t fields_len, uint32_t *mask) {
  // e.g. 
  // "MemTotal: 15716420 kB\n"

  // Check input parameters
  if (!mem || !line || !fields || !mask)
    return MYTOP_ERR_PARAM;

  char *p = line;
  // Skip spaces
  for (; *p != '\0' && *p == ' '; ++ p);
  // Locate the field string
  char *colon = strchr(line, ':');
  if (!colon)
    return MYTOP_OK;

  char *key_start = p;
  char *key_end = colon;
  size_t key_len = key_end - key_start;

  for (size_t i = 0; i < fields_len; ++ i) {
    if (key_len == fields[i].name_len && memcmp(key_start, fields[i].name, key_len) == 0) {
      // Parse value field
      // Skip spaces
      p = colon + 1;
      for (; *p != '\0' && *p == ' '; ++ p);
      // Value string translate to number
      errno = 0;
      char *end;
      uint64_t value = strtoul(p, &end, 10);
      if (p == end) 
        return MYTOP_ERR_PARSE;
      else if (errno == ERANGE)
        return MYTOP_ERR;

      if (!((*mask) & fields[i].bit)) {
        (*mask) |= fields[i].bit;
        
        uint64_t *dst = (uint64_t *)((char *)mem + fields[i].offset);
        *dst = value;

        break;
      }
    }
  }

  return MYTOP_OK;
}

/**
 * @brief Parse system version information
 *
 * Retrieve information from the 'uname()' system call for greater stability.
 *
 * @param  sys Pointer to the sys_info_t structure for storing the result.
 *
 * @return mytop_status_t Returns a status code 
 */
mytop_status_t parse_version(sys_info_t *sys) {
  // Check input parameters
  if (!sys)
    return MYTOP_ERR_PARAM;

  struct utsname buf;
  if (uname(&buf) == -1) {
    return MYTOP_ERR;
  }

  strncpy(sys->release, buf.release, KERNEL_VER_LEN - 1);
  sys->release[KERNEL_VER_LEN - 1] = '\0';
  strncpy(sys->machine, buf.machine, MACHINE_ARCH_LEN - 1);
  sys->machine[MACHINE_ARCH_LEN - 1] = '\0';

  return MYTOP_OK;
}

/**
 * @brief Parse memory information
 *
 * Reads the /proc/meminfo file, parses fields such as Total, Free, Buffers,
 * Cached, and automatically calculates the used and used_percent fields.
 *
 * @param  mem Pointer to the mem_info_t structure for storing the result.
 *
 * @return mytop_status_t Returns a status code.
 */
mytop_status_t parse_meminfo(mem_info_t *mem) {
  // Check input parameters
  if (!mem)
    return MYTOP_ERR_PARAM;

  FILE *fp = fopen("/proc/meminfo", "r");
  if (!fp)
    return MYTOP_ERR_IO;

  uint32_t mask = 0;
  size_t fields_len = sizeof(fields) / sizeof(fields[0]);
  char line[BUFFER_SIZE];
  while (fgets(line, sizeof line, fp)) {
    mytop_status_t ret = extract_field_info(mem, line, fields,fields_len,  &mask);
    if (ret != MYTOP_OK) {
      fclose(fp);
      return ret;
    }

    if ((mask & ALL_MEM_BITS) == ALL_MEM_BITS) {
      fclose(fp);
      break;
    }
  }

  // For backward compatibility, choose the classic calculation formula
  mem->used = mem->total - mem->free - mem->buffers - mem->cached;
  mem->used_percent = ((double)mem->used / mem->total) * 100;

  return MYTOP_OK;
}

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
void print_system_snapshot(const sys_info_t *sys, const mem_info_t *mem) {
  // Check input parameters 
  if (!sys || !mem)
    return;

  printf("Kernel : %s\n", sys->release);
  printf("Machine: %s\n", sys->machine);
  printf("Memory : %.2lf GB / %.2lf GB (%.2lf%%)\n",
         mem_uint_convert(mem->used, UNIT_KB, UNIT_GB),
         mem_uint_convert(mem->total, UNIT_KB, UNIT_GB),
         mem->used_percent);
}
