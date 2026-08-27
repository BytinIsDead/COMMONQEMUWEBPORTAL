# Extreme QEMU Web Manager

Pure C11 daemon and pure vanilla JavaScript control panel for cross-platform QEMU management. The project contains no Rust, Go, C++, C#, TypeScript, UI framework, raster image, or icon-font dependency.

## License

`AGPL-3.0-or-later` (`SPDX-License-Identifier: AGPL-3.0-or-later`). The complete unmodified license text is in [`LICENSE`](LICENSE). Project guidance is in [`LICENSE.md`](LICENSE.md).

Every network deployment of a modified version must prominently offer remote users a no-charge corresponding-source download or repository link under AGPLv3 Section 13. Configure the Source link in `public/index.html` to the exact immutable deployed source.

## Build

POSIX/Linux/macOS/BSD:

```bash
make
make test
./qemu-web-manager --port 8080 --public public
```

Windows with MinGW:

```bash
make windows
```

Cross-compilation depends on the installed toolchain:

```bash
CC=x86_64-w64-mingw32-gcc make windows
CC=clang make macos
CC=cc make freebsd
```

## Layout

- `src/main.c`: daemon entry point.
- `src/server.c`: dependency-free HTTP/static serving boundary.
- `src/qmp.c`: POSIX QMP JSON-line client.
- `src/cli_builder.c`: validated QEMU argv generator.
- `src/telemetry.c`: host resource sampler.
- `public/`: HTML, CSS, ES6 modules, SVG sprite, VNC and terminal adapters.

## API

```text
GET  /api/v1/health
GET  /api/v1/machines
POST /api/v1/machines/{id}/start
POST /api/v1/machines/{id}/stop
POST /api/v1/machines/{id}/qmp
GET  /api/v1/machines/{id}/events
GET  /api/v1/machines/{id}/serial
GET  /api/v1/machines/{id}/vnc
```

The minimal daemon serves static files and health status. Production deployments should add authenticated VM persistence, process supervision adapters, full WebSocket framing, QMP event dispatch, TLS, RBAC, audit storage, quotas, and host capability discovery.

## Platform matrix

| Platform | Targets | Accelerator |
|---|---|---|
| Linux | x86_64, ARM64 | KVM, TCG |
| Windows | x86_64 | WHPX, TCG |
| macOS | Intel, Apple Silicon | HVF, TCG |
| FreeBSD | x86_64 | TCG; NVMM adapter where supported |

## Security

Never expose QMP, VNC, SPICE, or GDB directly. Protect QEMU sockets, validate paths, isolate passthrough devices, use process groups/Job Objects, and require administrator authorization for VFIO, USB, raw QMP, snapshots, and guest-agent operations.
