# Extreme QEMU Web Manager

An ultra-lightweight cross-platform QEMU web manager and control plane. The daemon is written in Go; the interface is plain HTML5, CSS3, and ES6 JavaScript modules. No Rust, TypeScript, raster assets, or icon fonts are used.

## License

AGPL-3.0-or-later (`SPDX-License-Identifier: AGPL-3.0-or-later`). Every project source file carries the standard FSF notice. The complete license is in [`LICENSE`](LICENSE) and [`LICENSE.md`](LICENSE.md).

### AGPL Section 13

A modified version operated as a network service must prominently offer every remote user a no-charge way to obtain the corresponding source for the exact running version. This deployment includes a Source link in the UI; operators must configure it to their public source repository or immutable release archive and retain build instructions, local changes, generated assets, and workflow files.

## Features

- QEMU lifecycle supervision with safe argv generation.
- x86_64, i386, PPC/PPC64, SPARC/SPARC64, MIPS, ARM/ARM64, m68k, Alpha, and RISC-V target selection.
- KVM, WHPX, HVF, NVMM, and TCG acceleration profiles.
- IDE, SCSI, virtio, NVMe, floppy, SD, display, audio, serial, USB, and VFIO configuration models.
- Policy-gated QMP commands, GDB stub controls, and WebSocket event/console routes.
- Inline SVG icons only.

## Quickstart

Requirements: Go 1.23+, QEMU installed on the host, and optionally Node.js only for frontend asset tooling.

```bash
go mod download
go test ./...
go run ./cmd/qemu-web-manager
```

Open `http://localhost:8080`. The daemon honors the `PORT` environment variable.

Create a VM:

```bash
curl -X POST http://localhost:8080/api/v1/machines \
  -H 'content-type: application/json' \
  -d '{"name":"lab-x86","architecture":"x86_64","acceleration":"tcg","vcpus":2,"memory_mib":2048,"qmp_socket":"/tmp/lab.qmp"}'
```

## Build

```bash
go build -trimpath -ldflags='-s -w' -o bin/qemu-web-manager ./cmd/qemu-web-manager
go test ./...
go vet ./...
```

Cross-build examples:

```bash
GOOS=linux GOARCH=amd64 go build -o bin/qemu-web-manager-linux-amd64 ./cmd/qemu-web-manager
GOOS=linux GOARCH=arm64 go build -o bin/qemu-web-manager-linux-arm64 ./cmd/qemu-web-manager
GOOS=windows GOARCH=amd64 go build -o bin/qemu-web-manager-windows-amd64.exe ./cmd/qemu-web-manager
GOOS=darwin GOARCH=amd64 go build -o bin/qemu-web-manager-macos-amd64 ./cmd/qemu-web-manager
GOOS=darwin GOARCH=arm64 go build -o bin/qemu-web-manager-macos-arm64 ./cmd/qemu-web-manager
GOOS=freebsd GOARCH=amd64 go build -o bin/qemu-web-manager-freebsd-amd64 ./cmd/qemu-web-manager
```

## Platform matrix

| Host | Backends | Notes |
|---|---|---|
| Linux | KVM, TCG | `/dev/kvm`, IOMMU, VFIO, and TAP require host configuration |
| Windows | WHPX, TCG | Enable Windows Hypervisor Platform |
| macOS Intel/Apple Silicon | HVF, TCG | Query guest/host architecture compatibility |
| FreeBSD | TCG; NVMM where supported | Verify NVMM and QEMU package capabilities |

## Production requirements

Use TLS/reverse proxy authentication, RBAC, rate limits, audit storage, protected QMP/GDB transports, path allowlists, image scanning, quotas, and explicit approval for VFIO/USB passthrough. See [`ARCHITECTURE.md`](ARCHITECTURE.md).
