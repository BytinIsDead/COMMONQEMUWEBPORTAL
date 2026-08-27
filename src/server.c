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
#include "server.h"
#include "control.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BODY 8192
#define MAX_OUT  8192

static const char *mime(const char *p) {
    const char *e = strrchr(p, '.');
    if (e && !strcmp(e, ".css")) return "text/css";
    if (e && !strcmp(e, ".js")) return "application/javascript";
    if (e && !strcmp(e, ".svg")) return "image/svg+xml";
    if (e && !strcmp(e, ".json")) return "application/json";
    return "text/html";
}
static void response(qwm_socket fd, int code, const char *type, const void *body, size_t len) {
    char h[512]; int n = snprintf(h, sizeof(h),
        "HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Connection: close\r\nX-Content-Type-Options: nosniff\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n", code, type, len);
    qwm_send(fd, h, (size_t)n); if (body && len) qwm_send(fd, body, len);
}
static int send_text(qwm_socket fd, const char *body) { size_t n = strlen(body); response(fd, 200, "application/json", body, n); return 0; }

/* Extract the substring between /api/v1/ and the next '/'. */
static int api_segment(const char *path, const char *prefix, char *seg, size_t n) {
    const char *p = strstr(path, prefix);
    if (!p) return 0;
    p += strlen(prefix);
    size_t i = 0;
    while (*p && *p != '/' && i + 1 < n) seg[i++] = *p++;
    seg[i] = 0;
    return (int)i;
}

static void handle_api(qwm_socket fd, const char *path) {
    char out[MAX_OUT];
    char a[256] = {0};    /* primary subresource, e.g. machine id */
    char b[256] = {0};    /* action or query param */
    char m[256] = {0};    /* machine id for /api/v1/machines/<id>/...  */

    if (strstr(path, "/api/v1/health")) { if (!ctl_health(out, sizeof(out))) send_text(fd, out); else response(fd, 500, "application/json", "{\"error\":\"health failed\"}", 24); return; }
    if (strstr(path, "/api/v1/telemetry")) { if (!ctl_telemetry(out, sizeof(out))) send_text(fd, out); else response(fd, 500, "application/json", "{\"error\":\"telemetry failed\"}", 25); return; }
    if (strstr(path, "/api/v1/machines") && strstr(path, "/stop")) {
        int got = api_segment(path, "/api/v1/machines/", m, sizeof(m));
        if (!got) { response(fd, 400, "application/json", "{\"error\":\"bad request\"}", 22); return; }
        if (ctl_stop(m, out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"stop failed\"}", 21); return; }
        send_text(fd, out); return;
    }
    if (strstr(path, "/api/v1/machines") && strstr(path, "/start")) {
        int got = api_segment(path, "/api/v1/machines/", m, sizeof(m));
        if (!got) { response(fd, 400, "application/json", "{\"error\":\"bad request\"}", 22); return; }
        if (ctl_start("{}", out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"start failed\"}", 21); return; }
        send_text(fd, out); return;
    }
    if (strstr(path, "/api/v1/machines") && strstr(path, "/qmp")) {
        int got = api_segment(path, "/api/v1/machines/", m, sizeof(m));
        if (!got) { response(fd, 400, "application/json", "{\"error\":\"bad request\"}", 22); return; }
        const char *execute = strstr(path, "execute=");
        const char *execute_end = execute ? strchr(execute + 8, '&') : NULL;
        if (!execute) { ctl_qmp(m, "query-status", "{}", out, sizeof(out)); send_text(fd, out); return; }
        size_t elen = execute_end ? (size_t)(execute_end - (execute + 8)) : strlen(execute + 8);
        char ex[256]; if (elen >= sizeof(ex)) elen = sizeof(ex) - 1;
        memcpy(ex, execute + 8, elen); ex[elen] = 0;
        if (ctl_qmp(m, ex, "{}", out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"qmp failed\"}", 20); return; }
        send_text(fd, out); return;
    }
    if (strstr(path, "/api/v1/snapshot")) {
        if (sscanf(path, "/api/v1/snapshot/%255[^/]/%255[^/]/%255[^/]", m, a, b) == 3) {
            if (ctl_snapshot(m, a, b, out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"snapshot failed\"}", 26); return; }
            send_text(fd, out); return;
        }
        response(fd, 400, "application/json", "{\"error\":\"snapshot path invalid\"}", 31); return;
    }
    if (strstr(path, "/api/v1/balloon")) {
        if (sscanf(path, "/api/v1/balloon/%255[^/]/%255[^/]", m, b) == 2) {
            unsigned long mb = (unsigned long)strtoul(b, NULL, 10);
            if (ctl_balloon(m, mb, out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"balloon failed\"}", 25); return; }
            send_text(fd, out); return;
        }
        response(fd, 400, "application/json", "{\"error\":\"balloon path invalid\"}", 29); return;
    }
    if (strstr(path, "/api/v1/usb")) {
        if (sscanf(path, "/api/v1/usb/hotplug/%255[^/]/%255[^/]/%255[^/]", m, a, b) == 3) {
            if (ctl_usb_hotplug(m, a, b, out, sizeof(out)) < 0) { response(fd, 500, "application/json", "{\"error\":\"usb hotplug failed\"}", 27); return; }
            send_text(fd, out); return;
        }
        response(fd, 400, "application/json", "{\"error\":\"usb path invalid\"}", 25); return;
    }
    response(fd, 404, "application/json", "{\"error\":\"not found\"}", 18);
}

static void serve(qwm_socket fd, const char *root, const char *request) {
    char path[1024], full[2048];
    if (sscanf(request, "GET %1023s", path) != 1) { response(fd, 400, "text/plain", "bad request", 11); return; }
    if (!strncmp(path, "/api/", 5)) { handle_api(fd, path); return; }
    if (!strcmp(path, "/")) strcpy(path, "/index.html");
    if (strstr(path, "..")) { response(fd, 403, "text/plain", "forbidden", 9); return; }
    snprintf(full, sizeof(full), "%s%s", root, path);
    FILE *f = fopen(full, "rb");
    if (!f) { response(fd, 404, "text/plain", "not found", 9); return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); rewind(f);
    char *body = malloc((size_t)n);
    if (!body) { fclose(f); response(fd, 500, "text/plain", "memory error", 12); return; }
    size_t rd = fread(body, 1, (size_t)n, f); (void)rd; fclose(f);
    response(fd, 200, mime(full), body, (size_t)n); free(body);
}

int server_run(int port, const char *public_dir) {
    if (qwm_sock_startup() < 0) return 1;
    qwm_socket s = qwm_tcp_listen((unsigned int)port);
    if (s == (qwm_socket)-1) { qwm_sock_cleanup(); return 1; }
    for (;;) {
        qwm_socket c = qwm_tcp_accept(s);
        if (c == (qwm_socket)-1) continue;
        char req[MAX_BODY]; int r = qwm_recv(c, req, sizeof(req) - 1);
        if (r > 0) { req[r] = 0; serve(c, public_dir, req); }
        qwm_sock_close(c);
    }
    qwm_sock_close(s); qwm_sock_cleanup(); return 0;
}