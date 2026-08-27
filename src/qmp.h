/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_QMP_H
#define QWM_QMP_H
#include "platform.h"
#include <stddef.h>

typedef struct { qwm_socket fd; char socket_path[512]; } qwm_qmp;

int  qwm_qmp_connect(qwm_qmp *qmp, const char *path);
int  qwm_qmp_send_raw(qwm_qmp *qmp, const char *json, char *response, size_t response_size);
int  qwm_qmp_command(qwm_qmp *qmp, const char *execute, const char *arguments_json, char *response, size_t response_size);
void qwm_qmp_close(qwm_qmp *qmp);

/* Higher-level QMP helpers for the control-plane feature set. */
int qwm_qmp_snapshot(qwm_qmp *qmp, const char *device, const char *name, char *response, size_t n);
int qwm_qmp_balloon(qwm_qmp *qmp, unsigned long target_megabytes, char *response, size_t n);
int qwm_qmp_hotplug_cpu  (qwm_qmp *qmp, int cpu_index, char *response, size_t n);
int qwm_qmp_hotunplug_cpu(qwm_qmp *qmp, int cpu_index, char *response, size_t n);
int qwm_qmp_usb_add (qwm_qmp *qmp, const char *vendor, const char *product, char *response, size_t n);
int qwm_qmp_usb_del (qwm_qmp *qmp, const char *device_id, char *response, size_t n);

#endif