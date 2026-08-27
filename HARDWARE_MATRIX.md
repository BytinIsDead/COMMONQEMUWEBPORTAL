# Hardware Matrix

The manager exposes controls only after querying the selected host's QEMU binary with `-accel help`, `-machine help`, and `-device help`.

## Guest targets

| Target | Binary | Representative machine types |
|---|---|---|
| x86_64 | `qemu-system-x86_64` | `pc`, `q35` |
| i386 | `qemu-system-i386` | `pc`, `isapc` |
| PPC/PPC64 | `qemu-system-ppc`, `qemu-system-ppc64` | `mac99`, `pseries`, `prep` |
| SPARC/SPARC64 | `qemu-system-sparc`, `qemu-system-sparc64` | `sun4m`, `sun4u` |
| MIPS/MIPS64 | `qemu-system-mips`, `qemu-system-mips64` | `malta` |
| ARM/ARM64 | `qemu-system-arm`, `qemu-system-aarch64` | `virt`, `versatile`, board-specific |
| m68k | `qemu-system-m68k` | `q800` |
| Alpha | `qemu-system-alpha` | `clipper` |
| RISC-V | `qemu-system-riscv64` | `virt` |

Machine availability varies by QEMU build and must not be hard-coded as universally supported.

## Acceleration

| Backend | CLI | Host |
|---|---|---|
| KVM | `-accel kvm` | Linux with `/dev/kvm` |
| WHPX | `-accel whpx` | Windows Hypervisor Platform |
| HVF | `-accel hvf` | macOS Hypervisor.framework |
| NVMM | `-accel nvmm` | BSD hosts where enabled |
| TCG | `-accel tcg` | Portable fallback |

TCG controls should expose threading and resource throttling only when supported by the installed QEMU version. Unsupported options must produce validation errors.

## Storage buses

| Bus/controller | Typical QEMU representation |
|---|---|
| IDE PIIX3/PIIX4 | `-drive if=ide` or explicit controller/device |
| virtio-scsi | `virtio-scsi` controller plus SCSI disk |
| LSI 53C895A | `lsi53c895a` controller plus SCSI disk |
| NVMe | `-drive if=none` plus NVMe device |
| Floppy/FDC | `-drive if=floppy` |
| SD | machine-specific SD device and raw image |
| Virtio block | explicit `virtio-blk` device; preferred modern path |

Use `-blockdev` graphs for production hotplug and snapshot workflows.

## Display

Requested guest display models:

```text
virtio-vga · std · cirrus · qxl · bochs
```

## Audio

Requested guest audio models/backends:

```text
AC97 · ES1370 · Sound Blaster 16 · HDA · PC Speaker · CoreAudio
```

CoreAudio is a host backend and is available only on compatible macOS builds.

## Low-level interfaces

- QMP: protected Unix socket, named pipe, or loopback TCP.
- GDB: `-S` and loopback-only `-gdb` by default.
- Serial/COM: socket-backed `-serial` bridge to a WebSocket terminal.
- VNC/SPICE: authenticated broker rather than public raw ports.
- VFIO/USB: administrator-only with IOMMU/device ownership checks.
