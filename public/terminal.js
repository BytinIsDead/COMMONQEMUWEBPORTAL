/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
export class SerialTerminal {
  constructor(element, url) { this.element=element; this.url=url; this.socket=null; this.term=null; }
  attachXterm(Terminal) { this.term=new Terminal({convertEol:true,theme:{background:'#071016',foreground:'#8da7b6'}}); this.term.open(this.element); this.term.onData(data=>this.send(data)); }
  connect() { this.socket=new WebSocket(this.url); this.socket.binaryType='arraybuffer'; this.socket.onmessage=event=>this.term?.write(typeof event.data==='string'?event.data:new TextDecoder().decode(event.data)); }
  send(data) { if(this.socket?.readyState===WebSocket.OPEN)this.socket.send(data); }
  disconnect() { this.socket?.close(); this.socket=null; }
}
