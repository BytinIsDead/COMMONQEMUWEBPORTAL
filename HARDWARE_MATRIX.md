# Hardware Matrix

Support is capability-driven: the daemon discovers the installed QEMU binary and host backend before enabling controls.

## Target architectures

| Guest target | QEMU binary | Typical machine families |
|---|---|---|
| x86_64 | qemu-system-x86_64 | pc, q35 |
| i386 | qemu-system-i386 | pc, isapc |
| PPC/PPC64 | qemu-system-ppc, qemu-system-ppc64 | mac99, pseries, prep |
| SPARC/SPARC64 | qemu-system-sparc, qemu-system-sparc64 | sun4m, sun4u |
| MIPS/MIPS64 | qemu-system-mips, qemu-system-mips64 | malta, mips |
| ARM/ARM64 | qemu-system-arm, qemu-system-aarch64 | virt, raspi, versatile |
| m68k | qemu-system-m68k | q800 |
| Alpha | qemu-system-alpha | clipper |
| RISC-V | qemu-system-riscv64 | virt |

Exact machine types must be queried from `-machine help` on the selected host.

## Acceleration

| Backend | CLI | Platform | Notes |
|---|---|---|---|
| KVM | `-accel kvm` | Linux | Requires `/dev/kvm`, permissions, and compatible guest/host setup |
| WHPX | `-accel whpx` | Windows | Requires Windows Hypervisor Platform |
| HVF | `-accel hvf` | macOS | Requires Hypervisor.framework and supported hardware |
| NVMM | `-accel nvmm` | NetBSD | Requires NVMM-enabled kernel/QEMU |
| TCG | `-accel tcg` | All | Portable fallback; apply explicit CPU/resource policy |

Threading and execution-speed controls must be emitted only when supported by the installed QEMU version. Unsupported speed flags must fail validation rather than be silently ignored.

## Storage and buses

| Device family | Examples | QEMU strategy |
|---|---|---|
| IDE | PIIX3, PIIX4 | `-drive if=ide` or explicit controller/device |
| SCSI | virtio-scsi, LSI 53C895A | controller plus SCSI drive attachment |
| NVMe | emulated NVMe | `-drive if=none` plus `nvme` device |
| Floppy | FDC | `-drive if=floppy` |
| SD | SD card image | machine/device-specific SD attachment |
| Virtio block | virtio-blk | preferred for modern guests |

Modern builds should prefer `-blockdev` plus explicit `-device` nodes for stable node names, snapshots, and safe hotplug operations.

## Display

Supported requested models:

```text
virtio-vga · std · cirrus · qxl · bochs
```

The UI must query `-device help` and disable unavailable models.

## Audio

Requested models:

```text
AC97 · ES1370 · Sound Blaster 16 · HDA · PC Speaker · CoreAudio
```

Audio device selection and host audio backend are separate capabilities. CoreAudio is a host backend, not a portable guest device model.

## Low-level channels

- QMP: protected Unix socket, named pipe, or loopback TCP.
- GDB: `-S` plus loopback-only `-gdb` endpoint by default.
- Serial: socket-backed chardev bridged to a WebSocket terminal.
- VNC/SPICE: brokered transport with authenticated session binding.
- VFIO/USB: administrator-only, IOMMU and device-ownership validation.

## Host verification

The agent should run:

```text
qemu-system-<arch> -accel help
qemu-system-<arch> -machine help
qemu-system-<arch> -device help
qemu-img --version
```

Results are cached briefly and invalidated when the QEMU binary, host kernel, or platform capability changes.
