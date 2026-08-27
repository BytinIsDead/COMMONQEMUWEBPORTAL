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
#include "control.h"
#include "cli_builder.h"
#include "qmp.h"
#include "telemetry.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define MAX_OUT 8192

int ctl_health(char *out, size_t n) { return snprintf(out, n, "{\"status\":\"ok\",\"license\":\"AGPL-3.0-or-later\"}") >= 0 ? 0 : -1; }

int ctl_telemetry(char *out, size_t n) {
    qwm_telemetry t; if (telemetry_sample(&t) < 0) return -1;
    return snprintf(out, n,
        "{\"cpu_avg\":%.1f,\"core_count\":%zu,\"mem_total\":%llu,\"mem_used\":%llu,\"rss_mb\":%.1f,\"temperature_c\":%.1f,\"license\":\"AGPL-3.0-or-later\"}",
        t.cpu_avg, t.core_count, t.mem_total, t.mem_used, t.rss_megabytes, t.temperature_c) >= 0 ? 0 : -1;
}

/* Minimal single-VM launcher/stopper using QMP-optional lifecycle. The
 * process is recorded in a fixed path as PID for a control-plane reference.
 * Production hosts replace this with a per-user supervisor and an audit log. */
static const char *pid_path = "/tmp/qwm.pid";
static int write_pid(pid_t pid) { FILE *f = fopen(pid_path, "w"); if (!f) return -1; int rc = fprintf(f, "%ld", (long)pid); fclose(f); return rc < 0 ? -1 : 0; }

int ctl_start(const char *machine_json, char *out, size_t n) {
    (void)machine_json;
#ifdef _WIN32
    return snprintf(out, n, "{\"error\":\"Windows lifecycle uses CreateProcess adapter; see control host port\"}") >= 0 ? 1 : -1;
#else
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        qwm_vm_config cfg; memset(&cfg, 0, sizeof(cfg));
        cfg.name = "qcow2-vm"; cfg.architecture = "x86_64"; cfg.acceleration = "tcg"; cfg.vcpus = 2; cfg.memory_mib = 2048;
        char *argv[QWM_MAX_ARGS]; int argc = qwm_build_argv(&cfg, argv, QWM_MAX_ARGS);
        if (argc < 0) _exit(127);
        execvp(argv[0], argv);
        perror("exec"); _exit(127);
    }
    if (write_pid(pid) < 0) return -1;
    return snprintf(out, n, "{\"pid\":%ld}", (long)pid) >= 0 ? 0 : -1;
#endif
}

int ctl_stop(const char *machine_id, char *out, size_t n) {
    (void)machine_id;
#ifndef _WIN32
    FILE *f = fopen(pid_path, "r"); if (f) { long pid; if (fscanf(f, "%ld", &pid) == 1) kill((pid_t)pid, SIGTERM); fclose(f); remove(pid_path); }
#endif
    return snprintf(out, n, "{\"state\":\"stopped\"}") >= 0 ? 0 : -1;
}

int ctl_qmp(const char *machine_id, const char *execute, const char *args, char *out, size_t n) {
    (void)machine_id;
    char qmp_path[512]; snprintf(qmp_path, sizeof(qmp_path), "/tmp/qwm-%s.qmp", machine_id ? machine_id : "default");
    qwm_qmp q; if (qwm_qmp_connect(&q, qmp_path) < 0) return snprintf(out, n, "{\"error\":\"QMP socket unavailable\"}") >= 0 ? 1 : -1;
    char response[MAX_OUT]; int rc = qwm_qmp_command(&q, execute, args ? args : "{}", response, sizeof(response));
    qwm_qmp_close(&q);
    if (rc == -2) return snprintf(out, n, "{\"error\":\"command forbidden by policy\"}") >= 0 ? 1 : -1;
    return snprintf(out, n, "%s", response) >= 0 ? 0 : -1;
}

int ctl_snapshot(const char *machine_id, const char *device, const char *name, char *out, size_t n) {
    char qmp_path[512]; snprintf(qmp_path, sizeof(qmp_path), "/tmp/qwm-%s.qmp", machine_id ? machine_id : "default");
    qwm_qmp q; if (qwm_qmp_connect(&q, qmp_path) < 0) return snprintf(out, n, "{\"error\":\"QMP socket unavailable\"}") >= 0 ? 1 : -1;
    char response[MAX_OUT]; int rc = qwm_qmp_snapshot(&q, device, name, response, sizeof(response));
    qwm_qmp_close(&q); return snprintf(out, n, "%s", response) >= 0 ? rc : -1;
}

int ctl_balloon(const char *machine_id, unsigned long mb, char *out, size_t n) {
    char qmp_path[512]; snprintf(qmp_path, sizeof(qmp_path), "/tmp/qwm-%s.qmp", machine_id ? machine_id : "default");
    qwm_qmp q; if (qwm_qmp_connect(&q, qmp_path) < 0) return snprintf(out, n, "{\"error\":\"QMP socket unavailable\"}") >= 0 ? 1 : -1;
    char response[MAX_OUT]; int rc = qwm_qmp_balloon(&q, mb, response, sizeof(response));
    qwm_qmp_close(&q); return snprintf(out, n, "%s", response) >= 0 ? rc : -1;
}

int ctl_usb_hotplug(const char *machine_id, const char *vendor, const char *product, char *out, size_t n) {
    char qmp_path[512]; snprintf(qmp_path, sizeof(qmp_path), "/tmp/qwm-%s.qmp", machine_id ? machine_id : "default");
    qwm_qmp q; if (qwm_qmp_connect(&q, qmp_path) < 0) return snprintf(out, n, "{\"error\":\"QMP socket unavailable\"}") >= 0 ? 1 : -1;
    char response[MAX_OUT]; int rc = qwm_qmp_usb_add(&q, vendor, product, response, sizeof(response));
    qwm_qmp_close(&q); return snprintf(out, n, "%s", response) >= 0 ? rc : -1;
}