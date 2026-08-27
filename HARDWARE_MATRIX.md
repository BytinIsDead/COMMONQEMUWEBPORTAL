# Hardware Matrix

Controls are capability-driven. The daemon should query `qemu-system-* -accel help`, `-machine help`, and `-device help` on every selected host.

## Guest targets

| Guest | Binary | Common machine families |
|---|---|---|
| x86_64 | qemu-system-x86_64 | pc, q35 |
| i386 | qemu-system-i386 | pc, isapc |
| PPC/PPC64 | qemu-system-ppc/ppc64 | mac99, pseries, prep |
| SPARC/SPARC64 | qemu-system-sparc/sparc64 | sun4m, sun4u |
| MIPS/MIPS64 | qemu-system-mips/mips64 | malta |
| ARM/ARM64 | qemu-system-arm/aarch64 | virt, versatile, board-specific |
| m68k | qemu-system-m68k | q800 |
| Alpha | qemu-system-alpha | clipper |
| RISC-V | qemu-system-riscv64 | virt |

## Acceleration

| Backend | QEMU option | Host |
|---|---|---|
| KVM | `-accel kvm` | Linux with `/dev/kvm` |
| WHPX | `-accel whpx` | Windows Hypervisor Platform |
| HVF | `-accel hvf` | macOS Hypervisor.framework |
| NVMM | `-accel nvmm` | BSD where enabled |
| TCG | `-accel tcg` | Portable software emulation |

TCG thread and speed controls are version-dependent. Emit only options confirmed by the selected QEMU binary.

## Storage and controllers

- IDE: PIIX3/PIIX4 and `if=ide`.
- SCSI: virtio-scsi or `lsi53c895a` controller plus attached disk.
- NVMe: explicit NVMe device with a block backend.
- Floppy: FDC and `if=floppy`.
- SD: board-specific SD controller and raw image.
- Virtio block: preferred modern guest storage path.

Prefer QMP block nodes and `-blockdev` for live snapshots, hotplug, and node identity.

## Display and audio

Display models: `virtio-vga`, `std`, `cirrus`, `qxl`, `bochs`.

Audio models/backends: AC97, ES1370, Sound Blaster 16, HDA, PC Speaker, and CoreAudio where supported. CoreAudio is a host backend and is not portable across hosts.

## Low-level controls

- QMP raw JSON terminal: privileged, audited, policy-gated.
- GDB: `-S` plus loopback-only `-gdb` by default.
- Serial/COM: socket-backed chardev bridged to WebSocket.
- USB hotplug: `device_add`/`device_del` after policy validation.
- PCI/VFIO: IOMMU, device ownership, and administrator approval required.
- TAP/SLIRP: host interface policy and port-forward validation required.

## Host verification

```text
qemu-system-<arch> -accel help
qemu-system-<arch> -machine help
qemu-system-<arch> -device help
qemu-img --version
```

Availability differs between QEMU builds and host kernels; disabled controls must explain the missing capability.
