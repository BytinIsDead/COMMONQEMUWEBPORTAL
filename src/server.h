/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#ifndef QWM_SERVER_H
#define QWM_SERVER_H

/* Returns 0 on clean shutdown, nonzero on fatal start error. */
int server_run(int port, const char *public_dir);

#endif