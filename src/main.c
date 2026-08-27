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
#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    int port = 8080;
    const char *public_dir = "public";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--public") == 0 && i + 1 < argc) public_dir = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) { printf("Usage: %s [--port N] [--public DIR]\n", argv[0]); return 0; }
    }
    telemetry_init();
    printf("Extreme QEMU Web Manager listening on 0.0.0.0:%d\n", port);
    return server_run(port, public_dir);
}
