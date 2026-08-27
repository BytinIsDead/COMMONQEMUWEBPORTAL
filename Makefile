# Extreme QEMU Web Manager — AGPL-3.0-or-later
# Copyright (C) 2026 Extreme QEMU Web Manager contributors.
# Licensed under the GNU Affero General Public License version 3 or later.

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
TARGET  := qemu-web-manager
SOURCES := src/main.c src/server.c src/control.c src/qmp.c src/cli_builder.c src/telemetry.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test format package windows macos freebsd

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

format:
	@command -v clang-format >/dev/null && clang-format -i src/*.c src/*.h || echo "clang-format not installed; skipped"

test: all
	@echo "== build/help =="
	./$(TARGET) --help
	@echo "== start daemon & probe =="
	-@rm -f /tmp/qwm-test.log
	./$(TARGET) --port 18081 --public public >/tmp/qwm-test.log 2>&1 &
	@sleep 1
	@curl -sf http://127.0.0.1:18081/api/v1/health
	@echo
	@curl -sf http://127.0.0.1:18081/api/v1/telemetry
	@echo
	@curl -sf http://127.0.0.1:18081/
	@-kill %1 2>/dev/null || true

package: all
	mkdir -p dist/$(TARGET)/public
	cp $(TARGET) LICENSE LICENSE.md README.md ARCHITECTURE.md HARDWARE_MATRIX.md dist/$(TARGET)/
	cp public/* dist/$(TARGET)/public/
	tar -czf dist/$(TARGET)-source-AGPLv3.tar.gz --exclude=dist . 2>/dev/null || true

windows:
	$(MAKE) CC=x86_64-w64-mingw32-gcc CFLAGS="$(CFLAGS)" TARGET=$(TARGET).exe all
macos:
	$(MAKE) CC=clang CFLAGS="$(CFLAGS)" all
freebsd:
	$(MAKE) CC=cc CFLAGS="$(CFLAGS)" all

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe
	rm -rf dist