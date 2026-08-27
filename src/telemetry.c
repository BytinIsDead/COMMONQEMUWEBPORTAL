/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#include "telemetry.h"
#include <stdio.h>
#include <string.h>

static unsigned long long parse_ull(FILE *f, const char *label) {
    char key[64]; unsigned long long v; char unit[16];
    rewind(f);
    while (fscanf(f, "%63s %llu %15s", key, &v, unit) == 3)
        if (!strcmp(key, label)) return v;
    return 0;
}

void telemetry_init(void) {}

int telemetry_sample(qwm_telemetry *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    out->temperature_c = -1.0;

#ifdef __linux__
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        out->mem_total = parse_ull(f, "MemTotal:") * 1024ULL;
        out->mem_used  = out->mem_total - parse_ull(f, "MemAvailable:") * 1024ULL;
        fclose(f);
    }
    f = fopen("/proc/self/status", "r");
    if (f) {
        char key[32]; unsigned long v;
        while (fscanf(f, "%31s %lu", key, &v) == 2)
            if (!strcmp(key, "VmRSS:")) { out->rss_megabytes = (double)v / 1024.0; break; }
        fclose(f);
    }
    /* Per-core usage from /proc/stat. Compute average delta-free so a single
     * sample remains usable; production samples twice for real utilization. */
    f = fopen("/proc/stat", "r");
    if (f) {
        char line[256];
        double total = 0.0, idle = 0.0; int count = 0;
        while (fgets(line, sizeof(line), f) && count < QWM_MAX_CORES) {
            unsigned long long us, ni, sy, id, io, irq, si;
            if (sscanf(line, "cpu%*u %llu %llu %llu %llu %llu %llu %llu", &us, &ni, &sy, &id, &io, &irq, &si) != 7) continue;
            double busy = (double)(us + ni + sy + io + irq + si);
            double all = busy + (double)id;
            if (all > 0.0) out->cpu_per_core[count] = busy / all * 100.0;
            total += busy; idle += all; count++;
        }
        out->core_count = (size_t)count;
        out->cpu_avg = idle > 0.0 ? total / idle * 100.0 : 0.0;
        fclose(f);
    }
    f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (f) {
        unsigned long milli; if (fscanf(f, "%lu", &milli) == 1 && milli > 0) out->temperature_c = (double)milli / 1000.0;
        fclose(f);
    }
#endif
    return 0;
}