/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_CONTROL_H
#define QWM_CONTROL_H
#include <stddef.h>

/* Control-plane operations. String results are plain bytes for HTTP body responses. */
int  ctl_health(char *out, size_t n);
int  ctl_start(const char *machine_json, char *out, size_t n);
int  ctl_stop(const char *machine_id, char *out, size_t n);
int  ctl_qmp(const char *machine_id, const char *execute, const char *args, char *out, size_t n);
int  ctl_telemetry(char *out, size_t n);
int  ctl_snapshot(const char *machine_id, const char *device, const char *name, char *out, size_t n);
int  ctl_balloon(const char *machine_id, unsigned long mb, char *out, size_t n);
int  ctl_usb_hotplug(const char *machine_id, const char *vendor, const char *product, char *out, size_t n);

#endif