# Extreme QEMU Web Manager

A lightweight cross-platform web control plane for creating, supervising, inspecting, and debugging QEMU instances. The daemon is written in Rust with Tokio and Axum; the UI is zero-dependency TypeScript with inline SVG icons.

## License

This project is licensed under **AGPL-3.0-only**. The complete license text is in [`LICENSE`](LICENSE), with project guidance in [`LICENSE.md`](LICENSE.md).

If you modify and operate the software as a network service, AGPLv3 Section 13 requires a prominent, no-charge offer of the corresponding source to remote users. Configure the UI's Source link to the exact public source archive for the running build, including local modifications and build scripts.

## Architecture

- Rust daemon: REST API, process supervisor, typed CLI generator, QMP policy boundary, event fan-out.
- QEMU: separate supervised child process; QMP and console transports remain brokered.
- Frontend: static vanilla TypeScript, inline SVG sprite, responsive machine dashboard.
- Security boundary: raw QMP, GDB, VFIO, and USB passthrough require explicit policy approval.

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`HARDWARE_MATRIX.md`](HARDWARE_MATRIX.md).

## Quickstart

```bash
cargo test
cargo run
```

The daemon listens on `0.0.0.0:8080`. Set the port configuration before production deployment; the development default is intentionally simple.

Build the UI:

```bash
cd frontend
npm install
npm run build
```

The current UI is a static control-panel shell. Connect API calls and console transports through the documented routes before production use.

## Supported build targets

| Platform | Target | Acceleration |
|---|---|---|
| Linux | x86_64, aarch64 | KVM, TCG |
| Windows | x86_64 | WHPX, TCG |
| macOS | x86_64, aarch64 | HVF, TCG |
| FreeBSD | x86_64 | NVMM where available, TCG |

## Production checklist

- Run QEMU under a dedicated least-privileged account.
- Bind QMP and GDB to protected local transports.
- Configure authentication, RBAC, quotas, and audit storage.
- Publish the exact corresponding source archive.
- Preserve upstream QEMU and frontend dependency notices.
- Test passthrough only on isolated hosts with IOMMU policy.
