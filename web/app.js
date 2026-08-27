// Extreme QEMU Web Manager — AGPL-3.0-or-later
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

const id = 'lab-x86';
const badge = document.querySelector('h1 mark');
const consoleOutput = document.querySelector('pre');
const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';

function connectEvents() {
  const socket = new WebSocket(`${protocol}//${location.host}/api/v1/machines/${id}/events`);
  socket.onmessage = ({ data }) => {
    const event = JSON.parse(data);
    if (badge && event.state) badge.textContent = event.state.toUpperCase();
  };
  socket.onclose = () => setTimeout(connectEvents, 3000);
}

document.querySelector('.danger')?.addEventListener('click', async () => {
  await fetch(`/api/v1/machines/${id}/stop`, { method: 'POST' });
});
document.querySelector('.ghost')?.addEventListener('click', async () => {
  const response = await fetch(`/api/v1/machines/${id}/qmp`, { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ execute: 'query-status' }) });
  const result = await response.json();
  if (consoleOutput) consoleOutput.textContent += `\nQMP: ${JSON.stringify(result)}`;
});
if (consoleOutput) consoleOutput.setAttribute('aria-label', 'Serial console output');
connectEvents();
