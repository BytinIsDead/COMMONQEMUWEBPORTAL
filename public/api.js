/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/** Thin, dependency-free REST wrapper for the daemon control-plane API. */
class ApiClient {
  constructor(base = '') { this.base = base; }

  health() { return this.get('/api/v1/health'); }
  telemetry() { return this.get('/api/v1/telemetry'); }
  start(id = 'default') { return this.post(`/api/v1/machines/${id}/start`); }
  stop(id = 'default') { return this.post(`/api/v1/machines/${id}/stop`); }
  qmp(id, execute, args = '{}') {
    return this.get(`/api/v1/machines/${id}/qmp?execute=${encodeURIComponent(execute)}&args=${encodeURIComponent(args)}`);
  }
  snapshot(id, device, name) { return this.get(`/api/v1/snapshot/${id}/${encodeURIComponent(device)}/${encodeURIComponent(name)}`); }
  balloon(id, megabytes) { return this.get(`/api/v1/balloon/${id}/${megabytes}`); }
  usbHotplug(id, vendor, product) { return this.get(`/api/v1/usb/hotplug/${id}/${vendor}/${product}`); }

  async get(path) {
    const response = await fetch(this.base + path);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
  }
  async post(path) {
    const response = await fetch(this.base + path, { method: 'POST' });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.text().then(t => (t ? JSON.parse(t) : {}));
  }
}

export { ApiClient };
export default ApiClient;