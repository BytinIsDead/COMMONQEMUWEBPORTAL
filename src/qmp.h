/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_QMP_H
#define QWM_QMP_H
#include <stddef.h>
typedef struct { int fd; char socket_path[512]; } qwm_qmp;
int qwm_qmp_connect(qwm_qmp *qmp, const char *path);
int qwm_qmp_command(qwm_qmp *qmp, const char *execute, const char *arguments_json, char *response, size_t response_size);
int qwm_qmp_send_raw(qwm_qmp *qmp, const char *json, char *response, size_t response_size);
void qwm_qmp_close(qwm_qmp *qmp);
#endif
