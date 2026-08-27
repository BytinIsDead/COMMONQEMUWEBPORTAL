/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
const state={machineId:'lab-x86',connected:false};
const $=selector=>document.querySelector(selector);
const wsProtocol=location.protocol==='https:'?'wss:':'ws:';
function connect(){const ws=new WebSocket(`${wsProtocol}//${location.host}/api/v1/machines/${state.machineId}/events`);ws.onopen=()=>state.connected=true;ws.onmessage=event=>{try{const value=JSON.parse(event.data);if(value.state){const badge=$('h1 mark');if(badge)badge.textContent=value.state.toUpperCase();}}catch(error){console.warn('event decode failed',error);}};ws.onclose=()=>{state.connected=false;setTimeout(connect,3000);};}
$('#stop')?.addEventListener('click',async()=>{await fetch(`/api/v1/machines/${state.machineId}/stop`,{method:'POST'});});
$('#qmp')?.addEventListener('click',async()=>{const response=await fetch(`/api/v1/machines/${state.machineId}/qmp`,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify({execute:'query-status',arguments:{}})});const output=await response.json();const terminal=$('#console');if(terminal)terminal.textContent+=`\nQMP ${JSON.stringify(output)}`;});
for(const control of ['#architecture','#acceleration','#cpuModel','#vcpus','#memory'])$(control)?.addEventListener('change',()=>{document.body.dataset.dirty='true';});
connect();
