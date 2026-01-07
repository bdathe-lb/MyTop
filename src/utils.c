#include "utils.h"
#include "mytop_types.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/ioctl.h>

/**
 * @brief Skip spaces.
 */
const char *skip_spaces(const char *p) {
  while (*p && isspace((unsigned char)*p)) ++p;
  return p;
}

/**
 * @brief Determine whether the file name consists entirely of digits.
 */
bool is_numeric_name(const char *name) {
  // Check input parameters
  if (!name) return false;

  for (const char *p = name; *p; ++ p) {
    if (!isdigit(*p)) return false;
  }

  return true;
}

static mytop_status_t parse_i64(const char *s, int base, int64_t *out) {
  if (!s || !out) return MYTOP_ERR_PARAM;
  if (!(base >= 2 && base <= 36)) return MYTOP_ERR_PARAM;

  // Skip leading spaces
  const char *p = skip_spaces(s);
  // Empty or all spaces
  if (*p == '\0') return MYTOP_ERR_PARSE;

  errno = 0;
  char *end = NULL;
  long long val = strtoull((const char *)p, &end, base);

  // No digits consumed
  if (end == p) return MYTOP_ERR_PARSE;
  // Overflow
  if (errno == ERANGE) return MYTOP_ERR_PARSE;

  *out = (int64_t)val;
  return MYTOP_OK;
}

static mytop_status_t parse_u64(const char *s, int base, uint64_t *out) {
  if (!s || !out) return MYTOP_ERR_PARAM;
  if (!(base >= 2 && base <= 36)) return MYTOP_ERR_PARAM;

  // Skip leading spaces
  const char *p = skip_spaces(s);
  // Empty or all spaces
  if (*p == '\0') return MYTOP_ERR_PARSE;

  errno = 0;
  char *end = NULL;
  unsigned long long val = strtoull((const char *)p, &end, base);

  // No digits consumed
  if (end == p) return MYTOP_ERR_PARSE;
  // Overflow
  if (errno == ERANGE) return MYTOP_ERR_PARSE;

  *out = (uint64_t)val;
  return MYTOP_OK;
}

/**
 * @brief Convert numeric string s to a numeric type specified by type.
 *
 * rules:
 *   - base: 0 or 2..36 (same semantics as strto*)
 *   - allows leading whitespace
 *   - allows trailing whitespace (including '\n')
 *   - rejects junk characters after the number (except trailing whitespace)
 *   - for unsigned types, rejects a leading '-' after whitespace
 *
 * out must point to the corresponding type:
 *   NUM_I32 -> int32_t*
 *   NUM_U32 -> uint32_t*
 *   NUM_I64 -> int64_t*
 *   NUM_U64 -> uint64_t*
 */
mytop_status_t str_to_num(const char *s, int base, numtype_t type, void *out) {
  if (!s || !out) return MYTOP_ERR_PARAM;

  switch (type) {
    case NUM_I64: {
      int64_t v = 0;
      mytop_status_t st = parse_i64(s, base, &v);
      if (st != MYTOP_OK) return st;
      *(int64_t *)out = v;
      return MYTOP_OK;
    }
    case NUM_U64: {
      uint64_t u = 0;
      mytop_status_t st = parse_u64(s, base, &u);
      if (st != MYTOP_OK) return st;
      *(uint64_t *)out = u;
      return MYTOP_OK;
    }
    case NUM_I32: {
      int64_t v = 0;
      mytop_status_t st = parse_i64(s, base, &v);
      if (v < INT32_MIN || v > INT64_MAX) return MYTOP_ERR_RANGE;
      if (st != MYTOP_OK) return st;
      *(int32_t *)out = v;
      return MYTOP_OK;
    }
    case NUM_U32: {
      uint64_t u = 0;
      mytop_status_t st = parse_u64(s, base, &u);
      if (st != MYTOP_OK) return st;
      if (u > UINT32_MAX) return MYTOP_ERR_RANGE;
      *(uint64_t *)out = u;
      return MYTOP_OK;
    }
    default:
      return MYTOP_ERR;
  }
}

/**
 * @brief Get the current terminal column width (for dynamic truncation of the COMMAND column).
 *
 * @return Terminal column width; if retrieval fails, returns a conservative default value of 120.
 *
 * Note:
 *   - Only meaningful when stdout is a terminal (isatty).
 *   - Do not treat retrieval failure as an error; printing can proceed normally.
 */
int get_term_cols() {
  int cols = 120;  // Default column
  
  // Check if the standard output is a terminal
  if (!isatty(STDOUT_FILENO)) {
    return cols;
  }
  
  // Get terminal window size
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
    cols = (int)ws.ws_col;
  }

  return cols;
}

/**
 * @brief Convert pages to kB (pages * pagesize / 1024), rounding down.
 *
 * @param pages    RSS field (number of pages)
 * @param pagesize Page size obtained via sysconf(_SC_PAGESIZE) (in bytes)
 */
uint64_t pages_to_kb(uint64_t pages, uint64_t pagesize) {
  /* Note: pages * pagesize can be large, 
            but uint64_t is sufficient for typical machine ranges */
  return (pages * pagesize) / 1024u;
}

/**
 * @brief Format CPU time (jiffies) into a human‑readable string.
 *
 * @param out      Output buffer
 * @param out_sz   Size of the output buffer
 * @param jiffies  Sum of utime + stime
 * @param hz       Value from sysconf(_SC_CLK_TCK) (number of jiffies per second)
 *
 * Formatting strategy (simple and practical):
 *   - Less than 1 hour: MM:SS
 *   - 1 hour or more: HH:MM:SS
 */
void format_time_hms(char *out, size_t out_sz, uint64_t jiffies, long hz) {
  // Check input parameters
  if (!out || out_sz == 0)
    return;

  out[0] = '\0';

  uint64_t total_sec = jiffies / (uint64_t)hz;
  uint64_t hh = total_sec / 3600u;
  uint64_t mm = (total_sec % 3600u) / 60u;
  uint64_t ss = total_sec % 60u;

  if (hh > 0) {
    snprintf(out, out_sz, "%" PRIu64 ":%02" PRIu64 ":%02" PRIu64, hh, mm, ss);
  } else {
    snprintf(out, out_sz, "%02" PRIu64 ":%02" PRIu64, mm, ss);
  }
}

/**
 * @brief Define the "conversion factor" for each unit.
 */
static long double mem_unit_factor_bytes(mem_uint_t u) {
  switch (u) {
    case MEM_B:   return 1.0L;

    // SI
    case MEM_KB:  return 1000.0L;
    case MEM_MB:  return 1000.0L * 1000.0L;
    case MEM_GB:  return 1000.0L * 1000.0L * 1000.0L;
    case MEM_TB:  return 1000.0L * 1000.0L * 1000.0L * 1000.0L;

    // IEC
    case MEM_KIB: return 1024.0L;
    case MEM_MIB: return 1024.0L * 1024.0L;
    case MEM_GIB: return 1024.0L * 1024.0L * 1024.0L;
    case MEM_TIB: return 1024.0L * 1024.0L * 1024.0L * 1024.0L;
    default:      return 1.0L;
  }
}

/**
 * @brief Perform computer storage unit conversion.
 */
double mem_uint_convert(uint64_t value, mem_uint_t from, mem_uint_t to) {
  const long double f_from = mem_unit_factor_bytes(from);
  const long double f_to = mem_unit_factor_bytes(to);

  const long double bytes = (long double)value * f_from;
  const long double out = bytes / f_to;

  return (double)out;
}
