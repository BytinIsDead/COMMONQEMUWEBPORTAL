/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 * Chromium WebUSB client passthrough engine.
 *
 * Workflow:
 *  1. navigator.usb.requestDevice() selects a local peripheral.
 *  2. We open, claim an interface, and select an alternate configuration.
 *  3. Raw endpoint URBs are framed as JSON over a dedicated control WebSocket.
 *  4. The daemon maps the transport to QEMU `device_add usb-host` or the
 *     configured emulated USB/IP server, keeping host hotplug policy intact.
 *
 * WebUSB is available only in Chromium-based browsers; callers should gate
 * this module behind a feature check and present a graceful fallback message.
 */
class ClientUsbBridge {
  constructor() {
    this.usb = navigator.usb;
    this.device = null;
    this.interfaceNumber = null;
    this.endpointsIn = [];
    this.endpointsOut = [];
    this.socket = null;
    this.statusEl = document.getElementById('usbStatus');
  }

  get supported() { return !!this.usb; }

  setStatus(text) { if (this.statusEl) this.statusEl.textContent = text; }

  async select() {
    if (!this.supported) { this.setStatus('WebUSB requires Chromium; enable a secure context.'); return false; }
    this.device = await this.usb.requestDevice({ filters: [] });
    this.setStatus(`Selected ${this.device.productName} (${this.device.vendorId.toString(16)}:${this.device.productId.toString(16)})`);
    return true;
  }

  async claim() {
    if (!this.device) throw new Error('No device selected');
    await this.device.open();
    const configuration = this.device.configurations.find(c => c.interfaces.length > 0);
    if (!configuration) throw new Error('Device exposes no usable interfaces');
    await this.device.selectConfiguration(configuration.configurationValue);
    const iface = configuration.interfaces[0];
    this.interfaceNumber = iface.interfaceNumber;
    await this.device.claimInterface(this.interfaceNumber);
    this.endpointsIn = iface.alternate.endpoints.filter(e => e.direction === 'in');
    this.endpointsOut = iface.alternate.endpoints.filter(e => e.direction === 'out');
    return true;
  }

  async connectForwarding() {
    if (!this.socket) {
      const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
      this.socket = new WebSocket(`${proto}//${location.host}/api/v1/usb/urb`);
    }
    if (this.socket.readyState !== WebSocket.OPEN) {
      await new Promise((resolve, reject) => {
        const onOpen = () => { this.socket.removeEventListener('open', onOpen); resolve(); };
        this.socket.addEventListener('open', onOpen);
        this.socket.addEventListener('error', () => reject(new Error('USB bridge socket failed')));
      });
    }
  }

  beginPolling() {
    if (!this.endpointsIn.length) { this.setStatus('No IN endpoint; passthrough limited to opening the device.'); return; }
    const ep = this.endpointsIn[0];
    this.setStatus(`Polling control-in endpoint ${String.fromCharCode(0x60 + ep.endpointNumber)}${ep.direction === 'in' ? ' IN' : ''}`);
    const loop = async () => {
      while (this.device && this.socket?.readyState === WebSocket.OPEN) {
        try {
          const result = await this.device.transferIn(ep.endpointNumber, 1024);
          if (result.data?.byteLength) {
            this.socket.send(JSON.stringify({ type: 'urb', endpoint: ep.endpointNumber, data: Array.from(new Uint8Array(result.data.buffer)) }));
          }
        } catch (error) {
          this.setStatus(`USB IN stopped: ${error.message}`);
          break;
        }
      }
    };
    loop();
  }

  async forwardUrb() {
    if (this.interfaceNumber == null) await this.claim();
    await this.connectForwarding();
    // Legacy fallback: request vendor-agnostic control transfer and forward.
    this.socket.send(JSON.stringify({ type: 'probe', vendor: this.device.vendorId, product: this.device.productId }));
    this.beginPolling();
  }

  reset() { this.device = null; this.socket?.close(); this.socket = null; }
}

export { ClientUsbBridge };
export default ClientUsbBridge;