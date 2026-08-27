// Extreme QEMU Web Manager — AGPL-3.0-or-later
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
// Licensed under the GNU Affero General Public License version 3 or later.

const source = document.querySelector('.source');
const stateBadge = document.querySelector('h1 mark');
const stopButton = document.querySelector('.danger');
const qmpButton = document.querySelector('.ghost');
const machineId = 'lab-x86';

function connectEvents() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const socket = new WebSocket(`${protocol}//${location.host}/api/v1/machines/${machineId}/events`);
  socket.onmessage = ({ data }) => {
    try {
      const event = JSON.parse(data);
      if (event.state && stateBadge) stateBadge.textContent = event.state.toUpperCase();
    } catch (_) { /* Ignore malformed telemetry frames. */ }
  };
  socket.onclose = () => setTimeout(connectEvents, 3000);
}

stopButton?.addEventListener('click', async () => {
  await fetch(`/api/v1/machines/${machineId}/stop`, { method: 'POST' });
});
qmpButton?.addEventListener('click', async () => {
  const response = await fetch(`/api/v1/machines/${machineId}/qmp`, { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({execute: 'query-status'}) });
  const result = await response.json();
  window.alert(JSON.stringify(result, null, 2));
});
if (source) source.setAttribute('title', 'AGPLv3 Section 13 corresponding source');
if (location.protocol === 'http:' || location.protocol === 'https:') connectEvents();
