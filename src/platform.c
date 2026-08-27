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
#include "platform.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

int qwm_sock_startup(void) {
    WSADATA wsa; return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
}
void qwm_sock_cleanup(void) { WSACleanup(); }

qwm_socket qwm_tcp_listen(unsigned int port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return (qwm_socket)-1;
    int one = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    struct sockaddr_in a; memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons((unsigned short)port);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0 || listen(s, 32) < 0) { closesocket(s); return (qwm_socket)-1; }
    return (qwm_socket)s;
}
qwm_socket qwm_tcp_accept(qwm_socket s) { return (qwm_socket)accept((SOCKET)s, NULL, NULL); }

/* QMP over a Unix socket on POSIX; over a local TCP loopback socket on Windows. */
int qwm_unix_connect(const char *path, qwm_socket *out) {
    /* On Windows we interpret `path` as host:port so a local QMP TCP proxy can
     * be used (QEMU on Windows emits `-qmp tcp:127.0.0.1:port,...`). */
    unsigned int port = 0; int parsed = sscanf(path, "127.0.0.1:%u", &port);
    if (parsed != 1 || port == 0) return -1;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    struct sockaddr_in a; memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_addr.s_addr = inet_addr("127.0.0.1"); a.sin_port = htons((unsigned short)port);
    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0) { closesocket(s); return -1; }
    *out = (qwm_socket)s; return 0;
}
int qwm_recv(qwm_socket s, char *buf, size_t len) { return recv((SOCKET)s, buf, (int)len, 0); }
int qwm_send(qwm_socket s, const void *buf, size_t len) { return send((SOCKET)s, (const char *)buf, (int)len, 0); }
void qwm_sock_close(qwm_socket s) { closesocket((SOCKET)s); }

int qwm_proc_spawn(const char *const *argv, long *out_pid) {
    if (!argv || !argv[0]) return -1;
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si)); memset(&pi, 0, sizeof(pi)); si.cb = sizeof(si);
    char cmd[4096]; size_t pos = 0;
    for (int i = 0; argv[i]; ++i) {
        const char *a = argv[i]; int quote = strpbrk(a, " \t") != NULL;
        if (i) cmd[pos++] = ' ';
        if (quote) cmd[pos++] = '"';
        size_t l = strlen(a); memcpy(cmd + pos, a, l); pos += l;
        if (quote) cmd[pos++] = '"';
        if (pos >= sizeof(cmd) - 2) return -1;
    }
    cmd[pos] = 0;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) return -1;
    if (out_pid) *out_pid = (long)pi.dwProcessId;
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 0;
}
int qwm_proc_stop(long pid) { (void)pid; return -1; } /* Windows job-object termination is host-managed; placeholder */

const char *qwm_exe_suffix(void) { return ".exe"; }

#else /* POSIX */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

int qwm_sock_startup(void) { return 0; }
void qwm_sock_cleanup(void) {}

qwm_socket qwm_tcp_listen(unsigned int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    int one = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in a; memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons((unsigned short)port);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0 || listen(s, 32) < 0) { close(s); return -1; }
    return s;
}
qwm_socket qwm_tcp_accept(qwm_socket s) { return accept(s, NULL, NULL); }

int qwm_unix_connect(const char *path, qwm_socket *out) {
    if (!path || !out) return -1;
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) return -1;
    struct sockaddr_un a; memset(&a, 0, sizeof(a));
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof(a.sun_path) - 1);
    if (connect(s, (struct sockaddr *)&a, sizeof(a)) < 0) { close(s); return -1; }
    *out = s; return 0;
}
int qwm_recv(qwm_socket s, char *buf, size_t len) { return (int)recv(s, buf, len, 0); }
int qwm_send(qwm_socket s, const void *buf, size_t len) { return (int)send(s, buf, len, 0); }
void qwm_sock_close(qwm_socket s) { close(s); }

int qwm_proc_spawn(const char *const *argv, long *out_pid) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) { execvp(argv[0], (char *const *)argv); _exit(127); }
    if (out_pid) *out_pid = (long)pid;
    return 0;
}
int qwm_proc_stop(long pid) { if (pid <= 0) return -1; kill((pid_t)pid, SIGTERM); return 0; }

const char *qwm_exe_suffix(void) { return ""; }
#endif