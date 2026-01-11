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

#include "mytop_types.h"
#include <stdint.h>

/* --------- System Interfaces --------- */
mytop_status_t parse_version(sys_info_t *sys);
mytop_status_t parse_meminfo(mem_info_t *mem);
void print_system_snapshot(const sys_info_t *sys, const mem_info_t *mem);

/* --------- CPU Interfaces --------- */
mytop_status_t parse_cpu_stat(cpu_stat_t *stat);
double calculate_cpu_usage(const cpu_stat_t *prev, const cpu_stat_t *curr, uint64_t *total_delta);

/* --------- Process Interfaces --------- */
proc_list_t *create_procs_list(size_t capacity_hint);
void free_procs_list(proc_list_t *list);
mytop_status_t parse_procs(proc_list_t *list);
void calculate_procs_cpu(const proc_list_t *prev, proc_list_t *curr, uint64_t total_delta);
void sort_procs_by_mode(proc_list_t *list, sort_mode_t mode);
void print_procs(const proc_list_t *list);

#endif // !MYTOP_H
