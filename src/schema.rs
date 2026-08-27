// Extreme QEMU Web Manager — AGPL-3.0-only
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
// Licensed under the GNU Affero General Public License version 3 only.
// See LICENSE for the complete license text.

use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct VmConfig {
    pub id: Option<String>,
    pub name: String,
    pub architecture: Architecture,
    pub machine_type: Option<String>,
    pub acceleration: Acceleration,
    pub vcpus: u16,
    pub memory_mib: u64,
    pub cpu_model: Option<String>,
    pub cpu_flags: Vec<String>,
    pub devices: Vec<Device>,
    pub gdb: Option<GdbConfig>,
    pub qmp_socket: Option<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum Architecture { X86_64, I386, Ppc, Ppc64, Sparc, Sparc64, Mips, Mips64, Aarch64, Arm, M68k, Alpha, Riscv64 }

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Acceleration { Kvm, Whpx, Hvf, Nvmm, Tcg }

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(tag = "kind", rename_all = "kebab-case")]
pub enum Device {
    Disk { path: String, bus: DriveBus, readonly: bool },
    Network { model: String, backend: String, mac: Option<String> },
    Display { model: DisplayModel },
    Audio { model: AudioModel },
    Serial { id: String },
    Usb { vendor_id: u16, product_id: u16 },
    VfioPci { address: String },
    Sd { path: String },
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum DriveBus { Ide, VirtioScsi, Lsi53c895a, Nvme, Floppy, Scsi }

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum DisplayModel { VirtioVga, Std, Cirrus, Qxl, Bochs }

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum AudioModel { Ac97, Es1370, Sb16, Hda, Pcspk, CoreAudio }

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct GdbConfig { pub port: u16, pub stop_at_start: bool }
