# Architecture

## Runtime topology

```text
Browser ── HTTPS/WSS ── Axum API ── MachineManager ── qemu-system-* process
                                      ├─ QMP broker
                                      ├─ serial/VNC proxy
                                      ├─ telemetry sampler
                                      └─ audit/policy boundary
```

The browser never connects directly to QMP, VNC, SPICE, or GDB. The daemon authenticates and authorizes every channel, then proxies only the selected machine transport.

## State reconciliation

A machine has desired configuration and observed runtime state. Creation validates the typed configuration. Start generates argv, launches the child, and confirms readiness through QMP. Process exit, QMP events, and host capability changes update observed state and are broadcast to WebSocket subscribers.

## Process management

- Launch with `tokio::process::Command` and an argv vector; never construct a shell command.
- Capture stderr into structured logs.
- Track PID and lifecycle state.
- Apply startup and shutdown timeouts.
- Kill the process tree on termination.
- Restart only through an explicit policy.
- Confirm `query-status` after launch.

## QMP event loop

QMP uses a reader task and a command task. The reader classifies the greeting, responses, and asynchronous events. Each command receives a unique correlation ID and a timeout. Events are published through bounded broadcast channels. Reconnect logic re-negotiates capabilities and rehydrates status.

```text
socket read → JSON frame → response map / event bus
command API → authorization → correlation ID → socket write
```

Raw QMP is not unrestricted by default. Read-only queries and lifecycle commands are allowlisted; destructive or device-mutating commands require a privileged policy.

## WebSocket channels

`/api/v1/machines/{id}/events` streams JSON state and telemetry. Serial and VNC endpoints use bounded binary frames and enforce machine/session ownership. Backpressure, idle timeouts, origin validation, and maximum frame sizes are mandatory.

## CLI generation

The UI submits a typed intermediate representation. The generator performs schema, architecture, host-capability, device-conflict, security, and QEMU-version validation. It emits `Vec<String>` argv values. Shell quoting is only used for display in the UI, never for execution.

Mappings include:

- architecture → `qemu-system-*`
- acceleration → `-accel`
- CPU model and flags → `-cpu`
- vCPU count → `-smp`
- memory → `-m`
- QMP → `-qmp`
- GDB → `-S` and protected `-gdb`
- storage and buses → `-drive` or modern `-blockdev`/`-device`
- serial → protected socket-backed `-serial`
- passthrough → policy-gated `vfio-pci` and `usb-host`

## Security boundaries

The daemon should be deployed behind TLS and an identity provider. Host enrollment uses short-lived tokens and mutual TLS. Disk paths are allowlisted. VFIO and USB operations require administrator approval. GDB is loopback-only unless placed behind an authenticated tunnel.

## API shape

```text
GET  /api/v1/health
GET  /api/v1/machines
POST /api/v1/machines
POST /api/v1/machines/{id}/start
POST /api/v1/machines/{id}/stop
POST /api/v1/machines/{id}/qmp
GET  /api/v1/machines/{id}/events   (WebSocket)
```

## AGPLv3 network compliance

The deployed UI must provide a prominent Source link to the corresponding source of the exact running version. Release archives include the source package and required documentation.
