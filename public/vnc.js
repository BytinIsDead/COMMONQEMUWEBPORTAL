/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 * Dependency-free canvas VNC/RFB display transport boundary.
 *
 * The production build should vendor noVNC (MPL 2.0) for a full RFB codec
 * stack. This adapter provides the canvas surface, WebSocket plumbing, and a
 * single binary frame hook so a vendored adapter can be dropped in without
 * touching the rest of the dashboard.
 */
class VncDisplay {
  constructor(canvas, url) {
    this.canvas = canvas;
    this.url = url;
    this.ctx = canvas.getContext('2d');
    this.socket = null;
  }

  connect() {
    this.socket = new WebSocket(this.url);
    this.socket.binaryType = 'arraybuffer';
    this.socket.onmessage = event => this.handleFrame(event.data);
    this.socket.onopen = () => this.sendHandshake();
  }

  sendHandshake() {
    // QEMU RFB handshake is negotiated in the vendor adapter. We signal that
    // the canvas is ready to the daemon-side proxy.
    this.sendBinary('R');
  }

  sendBinary(data) {
    if (this.socket?.readyState === WebSocket.OPEN) {
      if (typeof data === 'string') this.socket.send(data);
      else this.socket.send(data);
    }
  }

  handleFrame(payload) {
    const bytes = payload instanceof ArrayBuffer ? new Uint8Array(payload) : payload;
    // Pixel codec (Raw, Tight, ZRLE) is handled by noVNC. This hook trace only
    // the frame length for debugging; a bundled adapter replaces this body.
    if (bytes.byteLength >= 8) {
      // Example simple RGB fill for readiness verification.
      this.ctx.fillStyle = '#050a0e';
      this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
    }
  }

  pointer(x, y, buttons = 0) {
    if (this.socket?.readyState === WebSocket.OPEN) {
      const rect = this.canvas.getBoundingClientRect();
      const sx = (x - rect.left) / rect.width * this.canvas.width;
      const sy = (y - rect.top) / rect.height * this.canvas.height;
      this.socket.send(JSON.stringify({ type: 'pointer', x: sx, y: sy, buttons }));
    }
  }

  disconnect() { this.socket?.close(); this.socket = null; }
}

export { VncDisplay };
export default VncDisplay;