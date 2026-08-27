/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/** Polls host and (via QMP) guest metrics, writing into dashboard nodes. */
class TelemetryRenderer {
  constructor(api, intervalMs = 2000) {
    this.api = api; this.intervalMs = intervalMs; this.timer = null;
    this.$hostCpu = document.getElementById('hostCpu');
    this.$hostMem = document.getElementById('hostMem');
    this.$hostRss = document.getElementById('hostRss');
    this.$hostTemp = document.getElementById('hostTemp');
    this.$guestCpu = document.getElementById('guestCpu');
    this.$guestMem = document.getElementById('guestMem');
    this.$memBar = document.getElementById('memBar');
  }

  start() { this.poll(); this.timer = setInterval(() => this.poll(), this.intervalMs); }
  stop() { clearInterval(this.timer); this.timer = null; }

  async poll() {
    try {
      const host = await this.api.telemetry();
      this.renderHost(host);
      const guest = await this.api.qmp('default', 'query-memory-size-summary').catch(() => null);
      if (guest) this.renderGuest(guest);
    } catch (_) { /* daemon offline; leave last known values */ }
  }

  renderHost(h) {
    if (this.$hostCpu) this.$hostCpu.textContent = `${Number(h.cpu_avg ?? 0).toFixed(1)}%`;
    if (this.$hostMem) this.$hostMem.textContent = this.formatBytes(h.mem_used) + ' / ' + this.formatBytes(h.mem_total);
    if (this.$hostRss) this.$hostRss.textContent = `${Number(h.rss_mb ?? 0).toFixed(0)} MB`;
    if (this.$hostTemp) this.$hostTemp.textContent = h.temperature_c > 0 ? `${Number(h.temperature_c).toFixed(1)}°C` : 'n/a';
    if (this.$guestCpu) this.$guestCpu.innerHTML = `${Number(h.cpu_avg ?? 0).toFixed(1)}<span>%</span>`;
  }
  renderGuest(g) {
    const value = g?.return ?? g; const size = value?.size_mb ?? value?.size ?? null;
    if (size && this.$guestMem) this.$guestMem.innerHTML = `${(size / 1024).toFixed(2)}<span> / 8 GB</span>`;
  }
  formatBytes(bytes) {
    if (!bytes) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let i = 0; let v = bytes;
    while (v >= 1024 && i < units.length - 1) { v /= 1024; i++; }
    return `${Number(v).toFixed(v >= 10 || i === 0 ? 0 : 1)} ${units[i]}`;
  }
}

export { TelemetryRenderer };
export default TelemetryRenderer;