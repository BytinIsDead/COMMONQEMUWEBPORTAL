# Extreme QEMU Web Manager — AGPL-3.0-or-later
# Copyright (C) 2026 Extreme QEMU Web Manager contributors.
# Licensed under the GNU Affero General Public License version 3 or later.

CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
TARGET := qemu-web-manager
SOURCES := src/main.c src/server.c src/qmp.c src/cli_builder.c src/telemetry.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test linux windows macos freebsd package
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
%.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@
test: $(TARGET)
	./$(TARGET) --help
linux:
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all
windows:
	$(MAKE) CC=x86_64-w64-mingw32-gcc CFLAGS="$(CFLAGS)" TARGET=$(TARGET).exe all
macos:
	$(MAKE) CC=clang CFLAGS="$(CFLAGS)" all
freebsd:
	$(MAKE) CC=cc CFLAGS="$(CFLAGS)" all
package: all
	mkdir -p dist/$(TARGET)/public
	cp $(TARGET) LICENSE LICENSE.md README.md ARCHITECTURE.md HARDWARE_MATRIX.md dist/$(TARGET)/
	cp public/* dist/$(TARGET)/public/
	tar -czf dist/$(TARGET)-source-AGPLv3.tar.gz --exclude=dist .
clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe
	rm -rf dist
