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
#ifndef QWM_PLATFORM_H
#define QWM_PLATFORM_H
#include <stddef.h>

#ifdef _WIN32
typedef intptr_t qwm_socket;          /* Windows SOCKET is a pointer-sized handle */
#else
typedef int qwm_socket;
#endif

/* Sockets */
int  qwm_sock_startup(void);
void qwm_sock_cleanup(void);
qwm_socket qwm_tcp_listen(unsigned int port);              /* -1 on failure */
qwm_socket qwm_tcp_accept(qwm_socket s);
int  qwm_unix_connect(const char *path, qwm_socket *out);  /* returns 0 on success, -1 on failure */
int  qwm_recv(qwm_socket s, char *buf, size_t len);
int  qwm_send(qwm_socket s, const void *buf, size_t len);
void qwm_sock_close(qwm_socket s);

/* Processes */
int qwm_proc_spawn(const char *const *argv, long *out_pid); /* 0 ok, -1 err; pid usable by qwm_proc_stop */
int qwm_proc_stop(long pid);                                 /* 0 ok, -1 if unsupported (returns -1 on win) */
const char *qwm_exe_suffix(void);                            /* returns "" on POSIX, ".exe" on Windows */

#endif