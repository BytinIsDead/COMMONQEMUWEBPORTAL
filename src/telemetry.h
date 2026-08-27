/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_TELEMETRY_H
#define QWM_TELEMETRY_H
typedef struct { double cpu_percent; unsigned long long memory_total; unsigned long long memory_used; unsigned long long disk_read_bytes; unsigned long long disk_write_bytes; } qwm_telemetry;
void telemetry_init(void);
int telemetry_sample(qwm_telemetry *out);
#endif
