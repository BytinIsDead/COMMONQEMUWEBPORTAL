/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_CLI_BUILDER_H
#define QWM_CLI_BUILDER_H
#include <stddef.h>
#define QWM_MAX_ARGS 256
#define QWM_MAX_DEVICES 64

typedef struct { const char *kind; const char *path; const char *bus; const char *model; const char *address; unsigned vendor_id; unsigned product_id; int readonly; } qwm_device;
typedef struct { const char *name; const char *architecture; const char *acceleration; const char *machine; const char *cpu; const char **cpu_flags; size_t cpu_flag_count; int vcpus; int memory_mib; const char *qmp_socket; int gdb_port; int gdb_stop; qwm_device devices[QWM_MAX_DEVICES]; size_t device_count; } qwm_vm_config;
int qwm_build_argv(const qwm_vm_config *config, char **argv, size_t capacity);
void qwm_free_argv(char **argv, int count);
#endif
