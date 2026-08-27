/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See LICENSE.
 */
#include "qmp.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

static int allowed(const char *s) {
    static const char *ok[] = {
        "query-status", "query-cpus-fast", "query-block", "query-memory-size-summary",
        "cont", "stop", "system_reset", "device_add", "device_del",
        "blockdev-snapshot-sync", "savevm", "loadvm", "balloon",
        "human-monitor-command", NULL };
    for (int i = 0; ok[i]; ++i) if (!strcmp(s, ok[i])) return 1;
    return 0;
}

int qwm_qmp_connect(qwm_qmp *q, const char *path) {
    if (!q || !path) return -1;
    memset(q, 0, sizeof(*q));
    q->fd = -1;
    if (qwm_sock_startup() < 0) return -1;
    qwm_socket s;
    if (qwm_unix_connect(path, &s) < 0) return -1;
    q->fd = s;
    strncpy(q->socket_path, path, sizeof(q->socket_path) - 1);
    return 0;
}

int qwm_qmp_send_raw(qwm_qmp *q, const char *json, char *response, size_t size) {
    if (!q || q->fd < 0 || !json || !response || size < 2) return -1;
    size_t len = strlen(json);
    if (qwm_send(q->fd, json, len) < 0 || qwm_send(q->fd, "\n", 1) < 0) return -1;
    size_t used = 0;
    while (used + 1 < size) {
        int n = qwm_recv(q->fd, response + used, size - used - 1);
        if (n <= 0) break;
        used += (size_t)n;
        if (response[used - 1] == '\n') break;
    }
    response[used] = 0;
    return used ? 0 : -1;
}

int qwm_qmp_command(qwm_qmp *q, const char *execute, const char *args, char *response, size_t size) {
    if (!allowed(execute)) return -2;
    char json[8192];
    snprintf(json, sizeof(json), "{\"execute\":\"%s\",\"arguments\":%s}", execute, args && *args ? args : "{}");
    return qwm_qmp_send_raw(q, json, response, size);
}

void qwm_qmp_close(qwm_qmp *q) { if (q && q->fd >= 0) { qwm_sock_close(q->fd); q->fd = -1; } }

int qwm_qmp_snapshot(qwm_qmp *q, const char *device, const char *name, char *response, size_t n) {
    char args[1024];
    if (device && *device) snprintf(args, sizeof(args), "{\"device\":\"%s\",\"snapshot-file\":\"%s.qcow2\",\"format\":\"qcow2\"}", device, name);
    else snprintf(args, sizeof(args), "{\"tag\":\"%s\"}", name);
    return qwm_qmp_command(q, device && *device ? "blockdev-snapshot-sync" : "savevm", args, response, n);
}

int qwm_qmp_balloon(qwm_qmp *q, unsigned long target_megabytes, char *response, size_t n) {
    char args[64]; snprintf(args, sizeof(args), "{\"value\":%lu}", target_megabytes * 1024UL * 1024UL);
    return qwm_qmp_command(q, "balloon", args, response, n);
}

int qwm_qmp_hotplug_cpu(qwm_qmp *q, int cpu_index, char *response, size_t n) {
    char args[64]; snprintf(args, sizeof(args), "{\"vcpu\":%d}", cpu_index);
    return qwm_qmp_command(q, "device_add", args, response, n);
}

int qwm_qmp_hotunplug_cpu(qwm_qmp *q, int cpu_index, char *response, size_t n) {
    char args[64]; snprintf(args, sizeof(args), "{\"id\":\"cpu%d\"}", cpu_index);
    return qwm_qmp_command(q, "device_del", args, response, n);
}

int qwm_qmp_usb_add(qwm_qmp *q, const char *vendor, const char *product, char *response, size_t n) {
    char args[160]; snprintf(args, sizeof(args), "{\"driver\":\"usb-host\",\"id\":\"usb-%s-%s\",\"vendorid\":%s,\"productid\":%s}", vendor, product, vendor, product);
    return qwm_qmp_command(q, "device_add", args, response, n);
}

int qwm_qmp_usb_del(qwm_qmp *q, const char *device_id, char *response, size_t n) {
    char args[128]; snprintf(args, sizeof(args), "{\"id\":\"%s\"}", device_id);
    return qwm_qmp_command(q, "device_del", args, response, n);
}