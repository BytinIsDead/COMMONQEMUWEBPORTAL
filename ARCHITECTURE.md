# Architecture

## Modules

```text
src/main.c             argument parsing and daemon bootstrap
src/server.c/.h        HTTP/static serving and API boundary
src/qmp.c/.h           protected QMP JSON line client
src/cli_builder.c/.h   typed configuration to argv generation
src/telemetry.c/.h     host metric sampling
public/                vanilla HTML/CSS/ES modules and SVG sprite
```

The reference daemon is intentionally dependency-light C11. A production deployment may replace the minimal static HTTP loop with a reviewed platform server adapter, while preserving the module contracts.

## Data flow

```text
Browser Fetch/WebSocket
        │
        ▼
HTTP server → request validation → VM registry/policy
        │                         │
        ▼                         ├── CLI builder → fork/exec/CreateProcess adapter
QMP proxy ← protected socket ←───┤
        │                         └── telemetry sampler
        ▼
JSON events / serial / VNC transport
```

## QMP event loop

QMP is a newline-delimited JSON protocol. The client connects to a Unix-domain socket on POSIX hosts or a named-pipe/loopback adapter on Windows. Commands are validated against policy, encoded with `execute` and `arguments`, written atomically, and read into a bounded response buffer. A production async dispatcher should assign monotonically increasing IDs, maintain a pending-response table, and publish asynchronous events to subscribers.

Allowed examples include `query-status`, `query-block`, `device_add`, `device_del`, `blockdev-snapshot-sync`, and `balloon`, subject to role and device policy.

## Process management

The launcher must use an argv vector, not shell interpolation. It owns process handles, captures stderr, records PID/state transitions, applies startup/shutdown timeouts, and reconciles unexpected exit. POSIX uses fork/exec and process groups; Windows uses CreateProcess and Job Objects.

## Proxy design

QMP, VNC, SPICE, serial, and GDB must never be publicly exposed. The HTTP layer authenticates and authorizes a session, then bridges only the requested machine channel. WebSocket connections require origin checks, bounded frames, idle timeouts, backpressure, and tenant/machine binding.

`public/vnc.js` is a canvas transport boundary intended to connect a licensed noVNC/RFB adapter. `public/terminal.js` exposes an xterm.js integration hook while remaining dependency-free until xterm.js is vendored with notices.

## CLI generation

The UI submits a typed VM model. The builder validates architecture, acceleration, resource bounds, device paths, and host capabilities, then emits `char *argv[]` values. It supports architecture-specific binaries, `-accel`, `-smp`, `-m`, `-cpu`, `-M`, `-qmp`, `-S`, `-gdb`, storage buses, display, serial, USB, VFIO, and SD devices.

No user value is evaluated by a shell. Use modern `-blockdev` and `-device` graphs for production snapshots and hotplug operations.

## Telemetry

Host telemetry samples CPU, memory, and disk counters. Guest telemetry is obtained through QMP and qemu-guest-agent when configured. Samples should be timestamped, bounded, downsampled, and broadcast over the machine event channel.

## Security and AGPL

Use TLS termination, authentication, RBAC, audit logging, quotas, path allowlists, IOMMU policy, and least-privileged QEMU processes. Network deployments must offer corresponding source prominently under AGPLv3 Section 13.
