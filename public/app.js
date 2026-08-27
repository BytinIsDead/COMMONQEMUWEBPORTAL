/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
import { ApiClient } from '/api.js';
import { TelemetryRenderer } from '/telemetry.js';
import { ClientUsbBridge } from '/webusb.js';
import { VncDisplay } from '/vnc.js';
import { SerialTerminal } from '/terminal.js';

const api = new ApiClient();
const machineId = 'default';

/* ---- daemon health / state ---- */
const healthDot = document.getElementById('healthDot');
const healthLabel = document.getElementById('healthLabel');
const stateBadge = document.getElementById('stateBadge');

async function refreshHealth() {
  try {
    const h = await api.health();
    healthDot?.classList.add('online');
    if (healthLabel) healthLabel.textContent = `CONTROL PLANE ONLINE · ${h.license ?? ''}`.trim();
    if (stateBadge) { stateBadge.textContent = 'RUNNING'; stateBadge.classList.remove('off'); }
  } catch (_) {
    healthDot?.classList.remove('online');
    if (healthLabel) healthLabel.textContent = 'CONTROL PLANE OFFLINE';
    if (stateBadge) { stateBadge.textContent = 'OFFLINE'; stateBadge.classList.add('off'); }
  }
}
setInterval(refreshHealth, 5000); refreshHealth();

/* ---- lifecycle ---- */
document.getElementById('btnStart')?.addEventListener('click', async () => {
  try { const r = await api.start(machineId); console.info('started', r); refreshHealth(); }
  catch (e) { alert(`Start failed: ${e.message}`); }
});
document.getElementById('btnStop')?.addEventListener('click', async () => {
  try { const r = await api.stop(machineId); console.info('stopped', r); refreshHealth(); }
  catch (e) { alert(`Stop failed: ${e.message}`); }
});

/* ---- QMP + advanced state ops ---- */
document.getElementById('btnQmp')?.addEventListener('click', async () => {
  try {
    const execute = prompt('QMP command (e.g. query-status):', 'query-status') ?? 'query-status';
    const result = await api.qmp(machineId, execute);
    console.info('QMP', result);
  } catch (e) { alert(`QMP failed: ${e.message}`); }
});
document.getElementById('btnSnapshot')?.addEventListener('click', async () => {
  const name = prompt('Snapshot name:', `snap-${Date.now()}`);
  if (!name) return;
  try { const r = await api.snapshot(machineId, '', name); console.info('snapshot', r); }
  catch (e) { alert(`Snapshot failed: ${e.message}`); }
});
document.getElementById('btnBalloonDown')?.addEventListener('click', async () => {
  try { const r = await api.balloon(machineId, 4096); console.info('balloon', r); }
  catch (e) { alert(`Balloon failed: ${e.message}`); }
});
document.getElementById('btnHotplug')?.addEventListener('click', async () => {
  try { const r = await api.qmp(machineId, 'device_add', '{"driver":"host-x86_64-cpu","id":"cpu1","socket-id":0,"core-id":4}'); console.info('hotplug', r); }
  catch (e) { alert(`Hotplug failed: ${e.message}`); }
});

/* ---- instrumentation wiring from the hardware profile ---- */
for (const id of ['architecture', 'acceleration', 'cpuModel', 'vcpus', 'memory', 'machineType']) {
  document.getElementById(id)?.addEventListener('change', () => { document.body.dataset.dirty = 'true'; });
}

/* ---- telemetry ---- */
const telemetry = new TelemetryRenderer(api, 2000);
telemetry.start();

/* ---- serial terminal ---- */
const serial = new SerialTerminal(document.getElementById('terminalContainer'), `/api/v1/machines/${machineId}/serial`);
serial.connect();

/* ---- VNC display ---- */
const vnc = new VncDisplay(document.getElementById('vncCanvas'), `/api/v1/machines/${machineId}/vnc`);
vnc.connect();

/* ---- client USB passthrough (Chromium) ---- */
const usb = new ClientUsbBridge();
const usbStatus = document.getElementById('usbStatus');
if (!usb.supported && usbStatus) usbStatus.textContent = 'WebUSB requires a Chromium browser in a secure context.';
document.getElementById('btnWebUsbSet')?.addEventListener('click', async () => { try { await usb.select(); } catch (e) { if (usbStatus) usbStatus.textContent = `Selection cancelled or failed: ${e.message}`; } });
document.getElementById('btnWebUsbForward')?.addEventListener('click', async () => {
  try { await usb.forwardUrb(); }
  catch (e) { if (usbStatus) usbStatus.textContent = `Forwarding failed: ${e.message}`; }
});