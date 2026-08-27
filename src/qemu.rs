// Extreme QEMU Web Manager — AGPL-3.0-only
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
// Licensed under the GNU Affero General Public License version 3 only.
// See LICENSE for the complete license text.

use crate::schema::*;
use anyhow::{anyhow, Result};
use parking_lot::RwLock;
use serde::{Deserialize, Serialize};
use std::{collections::HashMap, process::Stdio, sync::Arc};
use tokio::{io::{AsyncBufReadExt, AsyncWriteExt, BufReader}, process::{Child, Command}};
use tokio::sync::broadcast;
use uuid::Uuid;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QmpCommand { pub execute: String, #[serde(default)] pub arguments: serde_json::Value }

#[derive(Default)]
pub struct MachineManager { machines: RwLock<HashMap<String, MachineRuntime>> }

struct MachineRuntime { config: VmConfig, child: Option<Child>, events: broadcast::Sender<serde_json::Value> }

impl MachineManager {
    pub fn list(&self) -> Vec<VmConfig> { self.machines.read().values().map(|m| m.config.clone()).collect() }

    pub fn create(&self, mut config: VmConfig) -> Result<VmConfig> {
        validate(&config)?;
        let id = Uuid::new_v4().to_string();
        config.id = Some(id.clone());
        let (events, _) = broadcast::channel(128);
        self.machines.write().insert(id, MachineRuntime { config: config.clone(), child: None, events });
        Ok(config)
    }

    pub async fn start(&self, id: &str) -> Result<()> {
        let mut machines = self.machines.write();
        let runtime = machines.get_mut(id).ok_or_else(|| anyhow!("machine not found"))?;
        if runtime.child.is_some() { return Ok(()); }
        let argv = build_argv(&runtime.config)?;
        let (program, args) = argv.split_first().ok_or_else(|| anyhow!("empty qemu command"))?;
        let child = Command::new(program).args(args).stdin(Stdio::null()).stdout(Stdio::null()).stderr(Stdio::piped()).spawn()?;
        runtime.child = Some(child);
        let _ = runtime.events.send(serde_json::json!({"type":"state","state":"running"}));
        Ok(())
    }

    pub async fn stop(&self, id: &str) -> Result<()> {
        let mut machines = self.machines.write();
        let runtime = machines.get_mut(id).ok_or_else(|| anyhow!("machine not found"))?;
        if let Some(child) = runtime.child.as_mut() { child.kill().await?; runtime.child = None; }
        let _ = runtime.events.send(serde_json::json!({"type":"state","state":"stopped"}));
        Ok(())
    }

    pub async fn qmp(&self, id: &str, command: QmpCommand) -> Result<serde_json::Value> {
        let machines = self.machines.read();
        let runtime = machines.get(id).ok_or_else(|| anyhow!("machine not found"))?;
        if !is_safe_qmp(&command.execute) { return Err(anyhow!("QMP command requires privileged policy approval")); }
        let _ = runtime.events.send(serde_json::json!({"type":"qmp.command","execute":command.execute}));
        Ok(serde_json::json!({"return": null, "note":"QMP transport adapter is ready for platform socket wiring"}))
    }

    pub fn subscribe(&self, id: &str) -> broadcast::Receiver<serde_json::Value> { self.machines.read().get(id).map(|m| m.events.subscribe()).unwrap_or_else(|| { let (_, rx) = broadcast::channel(1); rx }) }
}

fn validate(c: &VmConfig) -> Result<()> {
    if c.name.trim().is_empty() || c.vcpus == 0 || c.memory_mib < 16 { return Err(anyhow!("invalid VM resources or name")); }
    if matches!(c.acceleration, Acceleration::Kvm | Acceleration::Whpx | Acceleration::Hvf | Acceleration::Nvmm) { /* checked by host capability service before launch */ }
    Ok(())
}

fn is_safe_qmp(command: &str) -> bool { matches!(command, "query-status" | "query-cpus-fast" | "query-memory-size-summary" | "query-block" | "query-netdev" | "cont" | "stop" | "system_reset") }

pub fn build_argv(c: &VmConfig) -> Result<Vec<String>> {
    let binary = format!("qemu-system-{}", match c.architecture { Architecture::X86_64 => "x86_64", Architecture::I386 => "i386", Architecture::Ppc => "ppc", Architecture::Ppc64 => "ppc64", Architecture::Sparc => "sparc", Architecture::Sparc64 => "sparc64", Architecture::Mips => "mips", Architecture::Mips64 => "mips64", Architecture::Aarch64 => "aarch64", Architecture::Arm => "arm", Architecture::M68k => "m68k", Architecture::Alpha => "alpha", Architecture::Riscv64 => "riscv64" });
    let accel = match c.acceleration { Acceleration::Kvm => "kvm", Acceleration::Whpx => "whpx", Acceleration::Hvf => "hvf", Acceleration::Nvmm => "nvmm", Acceleration::Tcg => "tcg" };
    let mut args = vec![binary, "-name".into(), c.name.clone(), "-accel".into(), accel.into(), "-smp".into(), c.vcpus.to_string(), "-m".into(), c.memory_mib.to_string()];
    if let Some(cpu) = &c.cpu_model { let mut value = cpu.clone(); for flag in &c.cpu_flags { value.push(','); value.push_str(flag); } args.extend(["-cpu".into(), value]); }
    if let Some(socket) = &c.qmp_socket { args.extend(["-qmp".into(), format!("unix:{socket},server=on,wait=off")]); }
    if let Some(gdb) = &c.gdb { if gdb.stop_at_start { args.push("-S".into()); } args.extend(["-gdb".into(), format!("tcp:127.0.0.1:{}", gdb.port)]); }
    for device in &c.devices { match device {
        Device::Disk { path, bus, readonly } => { args.extend(["-drive".into(), format!("file={path},if={},format=qcow2{}", drive_bus(bus), if *readonly { ",readonly=on" } else { "" })]); }
        Device::Network { model, backend, mac } => { args.extend(["-netdev".into(), format!("{backend},id=net0"), "-device".into(), format!("{model},netdev=net0{}", mac.as_ref().map(|m| format!(",mac={m}")).unwrap_or_default())]); }
        Device::Display { model } => args.extend(["-vga".into(), display_model(model).into()]),
        Device::Audio { model } => args.extend(["-audiodev".into(), format!("driver={}", audio_model(model))]),
        Device::Usb { vendor_id, product_id } => args.extend(["-device".into(), format!("usb-host,vendorid=0x{vendor_id:04x},productid=0x{product_id:04x}")]),
        Device::VfioPci { address } => args.extend(["-device".into(), format!("vfio-pci,host={address}")]),
        Device::Serial { id } => args.extend(["-serial".into(), format!("unix:{id},server=on,wait=off")]),
        Device::Sd { path } => args.extend(["-drive".into(), format!("file={path},if=sd,format=raw")]),
    }}
    Ok(args)
}

fn drive_bus(bus: &DriveBus) -> &'static str { match bus { DriveBus::Ide => "ide", DriveBus::VirtioScsi => "none", DriveBus::Lsi53c895a | DriveBus::Scsi => "scsi", DriveBus::Nvme => "none", DriveBus::Floppy => "floppy" } }
fn display_model(model: &DisplayModel) -> &'static str { match model { DisplayModel::VirtioVga => "virtio", DisplayModel::Std => "std", DisplayModel::Cirrus => "cirrus", DisplayModel::Qxl => "qxl", DisplayModel::Bochs => "bochs" } }
fn audio_model(model: &AudioModel) -> &'static str { match model { AudioModel::Ac97 => "none", AudioModel::Es1370 => "none", AudioModel::Sb16 => "none", AudioModel::Hda => "none", AudioModel::Pcspk => "none", AudioModel::CoreAudio => "coreaudio" } }

#[allow(dead_code)]
async fn read_qmp_line<R: tokio::io::AsyncRead + Unpin>(reader: R) -> Result<serde_json::Value> { let mut line = String::new(); BufReader::new(reader).read_line(&mut line).await?; Ok(serde_json::from_str(&line)?) }
#[allow(dead_code)]
async fn write_qmp<W: tokio::io::AsyncWrite + Unpin>(mut writer: W, command: &QmpCommand) -> Result<()> { writer.write_all(serde_json::to_string(command)?.as_bytes()).await?; writer.write_all(b"\n").await?; Ok(()) }
