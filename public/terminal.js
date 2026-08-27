/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 * Serial/PTY terminal adapter bridged over a WebSocket.
 *
 * When xterm.js is vendored (MIT), call attachXterm() for a full terminal.
 * Without it, output is appended to a <pre> element.
 */
class SerialTerminal {
  constructor(element, url) {
    this.host = element;
    this.url = url;
    this.socket = null;
    this.term = null;
    this.pre = element.querySelector('pre') ?? element;
    this.decoder = new TextDecoder();
  }

  attachXterm(Terminal) {
    if (this.host.querySelector('pre')) this.host.querySelector('pre').remove();
    this.term = new Terminal({ convertEol: true, theme: { background: '#071016', foreground: '#8da7b6' } });
    this.term.open(this.host);
    this.term.onData(data => this.send(data));
  }
  detachXterm() { this.term?.dispose(); this.term = null; }

  connect() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    this.socket = new WebSocket(`${proto}//${location.host}${this.url}`);
    this.socket.binaryType = 'arraybuffer';
    this.socket.onmessage = event => {
      const text = typeof event.data === 'string' ? event.data : this.decoder.decode(event.data);
      if (this.term) this.term.write(text);
      else this.pre.textContent += text;
    };
    return this.socket;
  }

  send(data) {
    if (this.socket?.readyState === WebSocket.OPEN) this.socket.send(data);
  }
  write(text) { if (this.socket?.readyState === WebSocket.OPEN) this.socket.send(text); }
  disconnect() { this.term?.dispose(); this.socket?.close(); this.socket = null; }
}

export { SerialTerminal };
export default SerialTerminal;