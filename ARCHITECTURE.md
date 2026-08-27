# Architecture

## Runtime topology

```text
Browser (HTML/CSS/ES6) ── HTTPS/WSS ── Go HTTP daemon ── qemu-system-* child
                                          ├─ lifecycle supervisor
                                          ├─ typed CLI generator
                                          ├─ QMP policy/client
                                          ├─ serial/VNC WebSocket proxy
                                          └─ event hub / telemetry
```

The daemon is intentionally stateless at the HTTP layer. A production deployment should add durable configuration, authentication, RBAC, and append-only audit storage around the in-memory reference manager.

## Process lifecycle

1. Validate VM configuration and host capability.
2. Resolve the architecture-specific `qemu-system-*` binary.
3. Generate an argv vector without shell evaluation.
4. Launch with `exec.CommandContext`.
5. Connect to QMP and confirm readiness.
6. Broadcast state and telemetry events.
7. Capture process exit and reconcile observed state.
8. Stop through QMP when available, then use a bounded process termination fallback.

## QMP event loop

The production QMP client uses a persistent Unix socket, Windows named pipe, or protected loopback TCP connection. A reader goroutine classifies the greeting, correlated responses, and asynchronous events. A command writer assigns unique IDs and waits on a bounded response map.

```text
command request → authorization → ID assignment → JSON line write
socket read → JSON decode → response waiter OR event hub
EOF/error → reconnect policy → status rehydration
```

Raw commands must be separately authorized. The reference implementation allows safe status and lifecycle commands only.

## WebSocket proxies

- `/api/v1/machines/{id}/events`: JSON lifecycle/telemetry events.
- `/api/v1/machines/{id}/serial`: authenticated serial byte proxy.
- `/api/v1/machines/{id}/vnc`: authenticated VNC transport boundary.

Never expose QMP, VNC, SPICE, or GDB directly to the public network. Enforce Origin checks, session ownership, frame limits, idle timeouts, backpressure, and audit records.

## CLI generation engine

The REST payload is a typed intermediate representation. Validation checks architecture, acceleration, device compatibility, path policy, resource limits, and privilege requirements. The generator emits `[]string` argv values.

Mappings:

```text
architecture → qemu-system-* 
acceleration → -accel
CPU → -cpu
vcpus → -smp
memory → -m
machine type → -M
QMP → -qmp
GDB → -S and protected -gdb
storage → -drive or blockdev/device graph
serial → socket-backed -serial
USB/VFIO → policy-gated device arguments
```

User-controlled paths and flags must never be passed through a shell. Modern production implementations should prefer `-blockdev` graphs over legacy `-drive` strings for hotplug, snapshots, and node identity.

## Frontend

The frontend is pure HTML, CSS, and JavaScript. It uses an inline SVG symbol sprite and native WebSocket/Fetch APIs. noVNC and xterm.js may be vendored as JavaScript assets when their licenses and source notices are included; no third-party frontend dependency is required for the dashboard shell.

## Security boundaries

Use TLS, OIDC/MFA, tenant-scoped authorization, host enrollment certificates, QEMU least privilege, seccomp/AppArmor/SELinux where available, disk path allowlists, IOMMU policy, and quotas. GDB and raw QMP are critical controls and should require an administrator role plus a short-lived session.

## Section 13 compliance

The deployed UI must include a prominent Source link to the corresponding source of the exact running version at no charge, including modifications, build scripts, documentation, and generated frontend assets.
