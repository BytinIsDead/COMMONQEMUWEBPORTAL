/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
export class QemuVncCanvas {
  constructor(canvas, url) { this.canvas=canvas; this.url=url; this.socket=null; this.context=canvas.getContext('2d'); }
  connect() { this.socket=new WebSocket(this.url); this.socket.binaryType='arraybuffer'; this.socket.onmessage=event=>this.handleFrame(new Uint8Array(event.data)); }
  handleFrame(frame) { if(frame.length<8) return; /* QEMU framebuffer codec negotiation belongs to the vendored noVNC adapter. */ }
  sendPointer(x,y,buttons=0) { if(this.socket?.readyState===WebSocket.OPEN)this.socket.send(JSON.stringify({type:'pointer',x,y,buttons})); }
  disconnect() { this.socket?.close(); this.socket=null; }
}
