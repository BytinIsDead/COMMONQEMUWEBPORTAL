/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_TELEMETRY_H
#define QWM_TELEMETRY_H
#include <stddef.h>

#define QWM_MAX_CORES 256

typedef struct {
    double cpu_avg;
    double cpu_per_core[QWM_MAX_CORES];
    size_t core_count;
    unsigned long long mem_total;
    unsigned long long mem_used;
    unsigned long long disk_read_bytes;
    unsigned long long disk_write_bytes;
    double rss_megabytes;
    double temperature_c;
} qwm_telemetry;

void telemetry_init(void);
int  telemetry_sample(qwm_telemetry *out);

#endif