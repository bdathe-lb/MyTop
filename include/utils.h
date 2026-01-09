#ifndef UTILS_H
#define UTILS_H

#include "mytop_types.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
  NUM_I32,
  NUM_U32,
  NUM_I64,
  NUM_U64,
} numtype_t;

typedef enum {
  // Bytes
  MEM_B = 0,

  // SI (decimal)
  MEM_KB,  // 10^3
  MEM_MB,  // 10^6
  MEM_GB,  // 10^9
  MEM_TB,  // 10^12

  // IEC (binary)
  MEM_KIB, // 2^10
  MEM_MIB, // 2^20
  MEM_GIB, // 2^30
  MEM_TIB, // 2^40
} mem_uint_t;

/* --------- String & Parsing Utilities --------- */

// Skip spaces.
const char *skip_spaces(const char *p);
// Determine whether the file name consists entirely of digits.
bool is_numeric_name(const char *name);
// Convert a numeric string to a numeric type (e.g., int, unsigned long)
mytop_status_t str_to_num(const char *s, int base, numtype_t type, void *out);

/* --------- Formatting & Conversion Utilities --------- */

// Convert pages to kB (pages * pagesize / 1024), rounding down.
uint64_t pages_to_kb(uint64_t pages, uint64_t pagesize);
// Format CPU time (jiffies) into a human‑readable string.
void format_time_hms(char *out, size_t out_sz, uint64_t jiffies, long hz);
// Perform computer storage unit conversion.
double mem_uint_convert(uint64_t value, mem_uint_t from, mem_uint_t to);

/* --------- Terminal Control & UI Utilities --------- */
// Get the current terminal column width and row.
void get_term_size(int *rows, int *cols);
// Enable/disable raw mode (Raw Mode)
int set_raw_mode(bool enable);
// Check for key input (non-blocking)
bool kbhit();
// Clear screen.
void term_clear();
// Move cursor.
void term_move_cursor(int row, int col);
// Cursor home.
void term_home();
// Hide cursor.
void term_hide_cursor();
// Show cursor.
void term_show_cursor();
// Flush the buffer.
void term_refresh();

#endif // !UTILS_H
