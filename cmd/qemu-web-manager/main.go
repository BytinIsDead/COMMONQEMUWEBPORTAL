// Extreme QEMU Web Manager — AGPL-3.0-or-later
// Copyright (C) 2026 Extreme QEMU Web Manager contributors.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See LICENSE.

package main

import (
    "bufio"
    "context"
    "encoding/json"
    "errors"
    "fmt"
    "io"
    "log"
    "net"
    "net/http"
    "os"
    "os/exec"
    "path/filepath"
    "strconv"
    "strings"
    "sync"
    "time"

    "github.com/gorilla/websocket"
)

type VMConfig struct {
    ID           string       `json:"id,omitempty"`
    Name         string       `json:"name"`
    Architecture string       `json:"architecture"`
    MachineType  string       `json:"machine_type,omitempty"`
    Acceleration string       `json:"acceleration"`
    VCPUs        int          `json:"vcpus"`
    MemoryMiB    int          `json:"memory_mib"`
    CPUModel     string       `json:"cpu_model,omitempty"`
    CPUFlags     []string     `json:"cpu_flags,omitempty"`
    QMP          string       `json:"qmp_socket,omitempty"`
    GDB          *GDBConfig   `json:"gdb,omitempty"`
    Devices      []VMDevice   `json:"devices,omitempty"`
}

type GDBConfig struct { Port int `json:"port"`; StopAtStart bool `json:"stop_at_start"` }
type VMDevice struct { Kind string `json:"kind"`; Path string `json:"path,omitempty"`; Bus string `json:"bus,omitempty"`; Model string `json:"model,omitempty"`; Address string `json:"address,omitempty"`; VendorID uint16 `json:"vendor_id,omitempty"`; ProductID uint16 `json:"product_id,omitempty"`; Readonly bool `json:"readonly,omitempty"` }
type QMPCommand struct { Execute string `json:"execute"`; Arguments json.RawMessage `json:"arguments,omitempty"` }
type eventHub struct { mu sync.RWMutex; subscribers map[string]map[chan []byte]struct{} }
type runtime struct { config VMConfig; command *exec.Cmd; qmp *QMPClient; events *eventHub }
type manager struct { mu sync.RWMutex; machines map[string]*runtime }

func main() {
    m := &manager{machines: make(map[string]*runtime)}
    mux := http.NewServeMux()
    mux.HandleFunc("/api/v1/health", func(w http.ResponseWriter, _ *http.Request) { writeJSON(w, http.StatusOK, map[string]string{"status":"ok"}) })
    mux.HandleFunc("/api/v1/machines", m.machinesHandler)
    mux.HandleFunc("/api/v1/machines/", m.machineHandler)
    mux.Handle("/", http.FileServer(http.Dir("web")))
    addr := ":8080"
    if configured := os.Getenv("PORT"); configured != "" { addr = ":" + configured }
    log.Printf("Extreme QEMU Web Manager listening on %s", addr)
    log.Fatal(http.ListenAndServe(addr, securityHeaders(mux)))
}

func (m *manager) machinesHandler(w http.ResponseWriter, r *http.Request) {
    switch r.Method {
    case http.MethodGet:
        m.mu.RLock(); result := make([]VMConfig, 0, len(m.machines)); for _, vm := range m.machines { result = append(result, vm.config) }; m.mu.RUnlock(); writeJSON(w, http.StatusOK, result)
    case http.MethodPost:
        var config VMConfig; if err := json.NewDecoder(r.Body).Decode(&config); err != nil { errorJSON(w, http.StatusBadRequest, err); return }; if err := validateConfig(config); err != nil { errorJSON(w, http.StatusBadRequest, err); return }
        config.ID = newID(); hub := newEventHub(); m.mu.Lock(); m.machines[config.ID] = &runtime{config: config, events: hub}; m.mu.Unlock(); writeJSON(w, http.StatusCreated, config)
    default: w.WriteHeader(http.StatusMethodNotAllowed)
    }
}

func (m *manager) machineHandler(w http.ResponseWriter, r *http.Request) {
    parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/"); if len(parts) < 4 { http.NotFound(w, r); return }; id := parts[3]
    m.mu.RLock(); vm, ok := m.machines[id]; m.mu.RUnlock(); if !ok { http.NotFound(w, r); return }
    action := ""; if len(parts) > 4 { action = parts[4] }
    switch action {
    case "start": if r.Method == http.MethodPost { if err := m.start(r.Context(), vm); err != nil { errorJSON(w, http.StatusBadRequest, err); return }; w.WriteHeader(http.StatusNoContent); return }
    case "stop": if r.Method == http.MethodPost { if err := m.stop(vm); err != nil { errorJSON(w, http.StatusBadRequest, err); return }; w.WriteHeader(http.StatusNoContent); return }
    case "qmp": if r.Method == http.MethodPost { var q QMPCommand; if err := json.NewDecoder(r.Body).Decode(&q); err != nil { errorJSON(w, http.StatusBadRequest, err); return }; result, err := vm.qmp.Execute(q); if err != nil { errorJSON(w, http.StatusBadRequest, err); return }; writeJSON(w, http.StatusOK, result); return }
    case "events": if r.Method == http.MethodGet { m.events(w, r, vm); return }
    case "serial", "vnc": if r.Method == http.MethodGet { m.proxy(w, r, vm, action); return }
    }
    if action == "" && r.Method == http.MethodGet { writeJSON(w, http.StatusOK, vm.config); return }; w.WriteHeader(http.StatusMethodNotAllowed)
}

func (m *manager) start(ctx context.Context, vm *runtime) error {
    m.mu.Lock(); defer m.mu.Unlock(); if vm.command != nil && vm.command.Process != nil { return nil }; argv, err := BuildQEMUArgs(vm.config); if err != nil { return err }; vm.command = exec.CommandContext(ctx, argv[0], argv[1:]...); vm.command.Stderr = os.Stderr; if err := vm.command.Start(); err != nil { vm.command = nil; return err }; vm.events.publish(map[string]string{"type":"state", "state":"running"}); go func() { _ = vm.command.Wait(); m.mu.Lock(); vm.command = nil; m.mu.Unlock(); vm.events.publish(map[string]string{"type":"state", "state":"stopped"}) }(); return nil
}
func (m *manager) stop(vm *runtime) error { m.mu.Lock(); defer m.mu.Unlock(); if vm.command == nil || vm.command.Process == nil { return nil }; if err := vm.command.Process.Kill(); err != nil { return err }; vm.command = nil; return nil }

func (m *manager) events(w http.ResponseWriter, r *http.Request, vm *runtime) {
    conn, err := websocket.Upgrade(w, r, nil, 1024*1024, 1024*1024); if err != nil { return }; ch := vm.events.subscribe(); defer vm.events.unsubscribe(ch); defer conn.Close(); for event := range ch { if err := conn.WriteMessage(websocket.TextMessage, event); err != nil { return } }
}
func (m *manager) proxy(w http.ResponseWriter, r *http.Request, vm *runtime, kind string) {
    conn, err := websocket.Upgrade(w, r, nil, 1024*1024, 1024*1024); if err != nil { return }; defer conn.Close(); path := ""; if kind == "serial" { path = vm.config.QMP }; if path == "" { _ = conn.WriteJSON(map[string]string{"error":"console transport is not configured"}); return }; target, err := net.Dial("unix", path); if err != nil { _ = conn.WriteJSON(map[string]string{"error":err.Error()}); return }; defer target.Close(); go func() { for { _, payload, readErr := conn.ReadMessage(); if readErr != nil { return }; _, _ = target.Write(payload) } }(); _, _ = io.Copy(&wsWriter{conn: conn}, target)
}
type wsWriter struct { conn *websocket.Conn }; func (w *wsWriter) Write(p []byte) (int, error) { return len(p), w.conn.WriteMessage(websocket.BinaryMessage, p) }

func BuildQEMUArgs(c VMConfig) ([]string, error) {
    bins := map[string]string{"x86_64":"x86_64", "i386":"i386", "ppc":"ppc", "ppc64":"ppc64", "sparc":"sparc", "sparc64":"sparc64", "mips":"mips", "mips64":"mips64", "aarch64":"aarch64", "arm":"arm", "m68k":"m68k", "alpha":"alpha", "riscv64":"riscv64"}; arch, ok := bins[c.Architecture]; if !ok { return nil, errors.New("unsupported architecture") }; accel := map[string]string{"kvm":"kvm", "whpx":"whpx", "hvf":"hvf", "nvmm":"nvmm", "tcg":"tcg"}[strings.ToLower(c.Acceleration)]; if accel == "" { return nil, errors.New("unsupported acceleration") }
    args := []string{"qemu-system-" + arch, "-name", c.Name, "-accel", accel, "-smp", strconv.Itoa(c.VCPUs), "-m", strconv.Itoa(c.MemoryMiB)}; if c.MachineType != "" { args = append(args, "-M", c.MachineType) }; if c.CPUModel != "" { cpu := c.CPUModel; if len(c.CPUFlags)>0 { cpu += "," + strings.Join(c.CPUFlags, ",") }; args = append(args, "-cpu", cpu) }; if c.QMP != "" { args = append(args, "-qmp", "unix:"+c.QMP+",server=on,wait=off") }; if c.GDB != nil { if c.GDB.StopAtStart { args = append(args, "-S") }; args = append(args, "-gdb", fmt.Sprintf("tcp:127.0.0.1:%d", c.GDB.Port)) }; for _, d := range c.Devices { switch d.Kind { case "disk": args = append(args, "-drive", fmt.Sprintf("file=%s,if=%s,format=qcow2", d.Path, driveBus(d.Bus))); case "display": args = append(args, "-vga", d.Model); case "network": args = append(args, "-device", d.Model); case "serial": args = append(args, "-serial", "unix:"+d.Path+",server=on,wait=off"); case "usb": args = append(args, "-device", fmt.Sprintf("usb-host,vendorid=0x%04x,productid=0x%04x", d.VendorID, d.ProductID)); case "vfio-pci": args = append(args, "-device", "vfio-pci,host="+d.Address); case "sd": args = append(args, "-drive", "file="+d.Path+",if=sd,format=raw") } }; return args, nil
}
func driveBus(bus string) string { switch bus { case "ide": return "ide"; case "floppy": return "floppy"; case "scsi", "virtio-scsi", "lsi53c895a": return "scsi"; default: return "virtio" } }
func validateConfig(c VMConfig) error { if strings.TrimSpace(c.Name)=="" || c.VCPUs<1 || c.MemoryMiB<16 { return errors.New("name, vcpus, or memory is invalid") }; return nil }

func newEventHub() *eventHub { return &eventHub{subscribers: make(map[string]map[chan []byte]struct{})} }
func (h *eventHub) subscribe() chan []byte { h.mu.Lock(); defer h.mu.Unlock(); ch:=make(chan []byte,64); key:=fmt.Sprintf("%p",ch); h.subscribers[key]=map[chan []byte]struct{}{ch:{}}; return ch }
func (h *eventHub) unsubscribe(ch chan []byte) { h.mu.Lock(); defer h.mu.Unlock(); for key, subs := range h.subscribers { if _, ok:=subs[ch]; ok { delete(h.subscribers,key); close(ch); return } } }
func (h *eventHub) publish(v any) { data,_:=json.Marshal(v); h.mu.RLock(); defer h.mu.RUnlock(); for _, subs:=range h.subscribers { for ch:=range subs { select { case ch<-data: default: } } } }

func (q *QMPClient) Execute(command QMPCommand) (map[string]any, error) { if !safeQMP(command.Execute) { return nil, errors.New("QMP command is not permitted by the default policy") }; return map[string]any{"return":nil,"execute":command.Execute}, nil }
type QMPClient struct { Socket string }
func safeQMP(command string) bool { switch command { case "query-status","query-cpus-fast","query-block","query-memory-size-summary","cont","stop","system_reset": return true; default: return false } }
func writeJSON(w http.ResponseWriter, status int, v any) { w.Header().Set("Content-Type","application/json"); w.WriteHeader(status); _=json.NewEncoder(w).Encode(v) }
func errorJSON(w http.ResponseWriter, status int, err error) { writeJSON(w,status,map[string]string{"error":err.Error()}) }
func newID() string { return strconv.FormatInt(time.Now().UnixNano(), 36) }
func securityHeaders(next http.Handler) http.Handler { return http.HandlerFunc(func(w http.ResponseWriter,r *http.Request) { w.Header().Set("Content-Security-Policy","default-src 'self'; connect-src 'self' ws: wss:; img-src 'self' data:; style-src 'self' 'unsafe-inline'; script-src 'self'"); w.Header().Set("X-Content-Type-Options","nosniff"); next.ServeHTTP(w,r) }) }
var _ = bufio.ErrInvalidUnreadByte
