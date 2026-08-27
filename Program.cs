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

using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net.WebSockets;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

var builder = WebApplication.CreateBuilder(args);
builder.WebHost.UseUrls($"http://0.0.0.0:{Environment.GetEnvironmentVariable("PORT") ?? "8080"}");
builder.Services.AddSingleton<MachineManager>();
builder.Services.AddSingleton<QemuProcessSupervisor>();
var app = builder.Build();
app.UseDefaultFiles();
app.UseStaticFiles();
app.Use(async (context, next) => { context.Response.Headers["X-Content-Type-Options"] = "nosniff"; context.Response.Headers["Content-Security-Policy"] = "default-src 'self'; connect-src 'self' ws: wss:; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:"; await next(); });
app.UseWebSockets(new WebSocketOptions { KeepAliveInterval = TimeSpan.FromSeconds(30) });

app.MapGet("/api/v1/health", () => Results.Ok(new { status = "ok", runtime = ".NET 10" }));
app.MapGet("/api/v1/machines", (MachineManager manager) => Results.Ok(manager.List()));
app.MapPost("/api/v1/machines", (VmConfig config, MachineManager manager) => { try { return Results.Created($"/api/v1/machines/{manager.Create(config).Id}", manager.Get(config.Id!)); } catch (ArgumentException e) { return Results.BadRequest(new { error = e.Message }); } });
app.MapGet("/api/v1/machines/{id}", (string id, MachineManager manager) => manager.TryGet(id, out var vm) ? Results.Ok(vm.Config) : Results.NotFound());
app.MapPost("/api/v1/machines/{id}/start", async (string id, MachineManager manager, QemuProcessSupervisor supervisor, CancellationToken ct) => await supervisor.StartAsync(manager, id, ct) ? Results.NoContent() : Results.BadRequest(new { error = "Unable to start machine" }));
app.MapPost("/api/v1/machines/{id}/stop", async (string id, MachineManager manager, QemuProcessSupervisor supervisor) => await supervisor.StopAsync(manager, id) ? Results.NoContent() : Results.BadRequest(new { error = "Unable to stop machine" }));
app.MapPost("/api/v1/machines/{id}/qmp", async (string id, QmpRequest request, MachineManager manager) => { if (!manager.TryGet(id, out var vm)) return Results.NotFound(); try { return Results.Ok(await vm.Qmp.ExecuteAsync(request)); } catch (Exception e) { return Results.BadRequest(new { error = e.Message }); } });
app.Map("/api/v1/machines/{id}/events", async (HttpContext context, string id, MachineManager manager) => await WebSocketEndpoints.EventsAsync(context, id, manager));
app.Map("/api/v1/machines/{id}/serial", async (HttpContext context, string id, MachineManager manager) => await WebSocketEndpoints.SerialAsync(context, id, manager));
app.MapFallbackToFile("index.html");
app.Run();

public sealed record VmConfig(
    string Name,
    string Architecture,
    string Acceleration,
    int Vcpus,
    int MemoryMib,
    string? MachineType = null,
    string? CpuModel = null,
    IReadOnlyList<string>? CpuFlags = null,
    string? QmpSocket = null,
    GdbConfig? Gdb = null,
    IReadOnlyList<VmDevice>? Devices = null,
    string? Id = null);
public sealed record GdbConfig(int Port, bool StopAtStart);
public sealed record VmDevice(string Kind, string? Path = null, string? Bus = null, string? Model = null, string? Address = null, ushort VendorId = 0, ushort ProductId = 0, bool Readonly = false);
public sealed record QmpRequest(string Execute, JsonElement? Arguments = null);
public sealed record MachineEvent(string Type, string State, DateTimeOffset At);

public sealed class ManagedMachine
{
    public VmConfig Config { get; }
    public QmpClient Qmp { get; }
    public event Action<MachineEvent>? EventRaised;
    public ManagedMachine(VmConfig config) { Config = config; Qmp = new QmpClient(config.QmpSocket, this); }
    public void Publish(string type, string state) => EventRaised?.Invoke(new MachineEvent(type, state, DateTimeOffset.UtcNow));
}

public sealed class MachineManager
{
    private readonly ConcurrentDictionary<string, ManagedMachine> machines = new();
    public IEnumerable<VmConfig> List() => machines.Values.Select(x => x.Config);
    public ManagedMachine Get(string id) => machines.TryGetValue(id, out var vm) ? vm : throw new KeyNotFoundException(id);
    public bool TryGet(string id, out ManagedMachine machine) => machines.TryGetValue(id, out machine!);
    public ManagedMachine Create(VmConfig input)
    {
        if (string.IsNullOrWhiteSpace(input.Name) || input.Vcpus < 1 || input.MemoryMib < 16) throw new ArgumentException("Name, vcpus, and memory are invalid");
        if (!QemuCli.Architectures.ContainsKey(input.Architecture)) throw new ArgumentException("Unsupported architecture");
        if (!QemuCli.Accelerators.Contains(input.Acceleration.ToLowerInvariant())) throw new ArgumentException("Unsupported acceleration");
        var config = input with { Id = Guid.NewGuid().ToString("N"), CpuFlags = input.CpuFlags ?? [], Devices = input.Devices ?? [] };
        var machine = new ManagedMachine(config); machines[config.Id!] = machine; return machine;
    }
}

public sealed class QemuProcessSupervisor
{
    private readonly ConcurrentDictionary<string, Process> processes = new();
    public async Task<bool> StartAsync(MachineManager manager, string id, CancellationToken ct)
    {
        if (!manager.TryGet(id, out var vm)) return false;
        if (processes.ContainsKey(id)) return true;
        var argv = QemuCli.Build(vm.Config); if (argv.Count == 0) return false;
        var psi = new ProcessStartInfo { FileName = argv[0], UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true, CreateNoWindow = true };
        foreach (var arg in argv.Skip(1)) psi.ArgumentList.Add(arg);
        var process = new Process { StartInfo = psi, EnableRaisingEvents = true };
        process.Exited += (_, _) => { processes.TryRemove(id, out _); vm.Publish("state", "stopped"); };
        try { if (!process.Start()) return false; processes[id] = process; vm.Publish("state", "running"); _ = DrainAsync(process.StandardError); _ = DrainAsync(process.StandardOutput); return true; } catch { process.Dispose(); return false; }
    }
    public async Task<bool> StopAsync(MachineManager manager, string id) { if (!processes.TryRemove(id, out var p)) return true; try { if (!p.HasExited) { p.Kill(entireProcessTree: true); await p.WaitForExitAsync(); } p.Dispose(); return true; } catch { return false; } }
    private static async Task DrainAsync(StreamReader reader) { while (await reader.ReadLineAsync() is not null) { } }
}

public static class QemuCli
{
    public static readonly IReadOnlyDictionary<string, string> Architectures = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase) { ["x86_64"]="x86_64", ["i386"]="i386", ["ppc"]="ppc", ["ppc64"]="ppc64", ["sparc"]="sparc", ["sparc64"]="sparc64", ["mips"]="mips", ["mips64"]="mips64", ["arm"]="arm", ["aarch64"]="aarch64", ["m68k"]="m68k", ["alpha"]="alpha", ["riscv64"]="riscv64" };
    public static readonly HashSet<string> Accelerators = ["kvm", "whpx", "hvf", "nvmm", "tcg"];
    public static List<string> Build(VmConfig c)
    {
        var args = new List<string> { $"qemu-system-{Architectures[c.Architecture]}", "-name", c.Name, "-accel", c.Acceleration.ToLowerInvariant(), "-smp", c.Vcpus.ToString(), "-m", c.MemoryMib.ToString() };
        if (!string.IsNullOrWhiteSpace(c.MachineType)) args.AddRange(["-M", c.MachineType]);
        if (!string.IsNullOrWhiteSpace(c.CpuModel)) args.AddRange(["-cpu", c.CpuModel + (c.CpuFlags?.Count > 0 ? "," + string.Join(',', c.CpuFlags) : "")]);
        if (!string.IsNullOrWhiteSpace(c.QmpSocket)) args.AddRange(["-qmp", $"unix:{c.QmpSocket},server=on,wait=off"]);
        if (c.Gdb is not null) { if (c.Gdb.StopAtStart) args.Add("-S"); args.AddRange(["-gdb", $"tcp:127.0.0.1:{c.Gdb.Port}"]); }
        foreach (var d in c.Devices ?? []) {
            switch (d.Kind.ToLowerInvariant()) {
                case "disk": args.AddRange(["-drive", $"file={d.Path},if={d.Bus switch { "ide" => "ide", "floppy" => "floppy", "scsi" or "virtio-scsi" or "lsi53c895a" => "scsi", _ => "virtio" }},format=qcow2{(d.Readonly ? ",readonly=on" : "")}"]); break;
                case "display": args.AddRange(["-vga", d.Model ?? "std"]); break;
                case "serial": args.AddRange(["-serial", $"unix:{d.Path},server=on,wait=off"]); break;
                case "usb": args.AddRange(["-device", $"usb-host,vendorid=0x{d.VendorId:x4},productid=0x{d.ProductId:x4}"]); break;
                case "vfio-pci": args.AddRange(["-device", $"vfio-pci,host={d.Address}"]); break;
                case "sd": args.AddRange(["-drive", $"file={d.Path},if=sd,format=raw"]); break;
                case "network": args.AddRange(["-device", d.Model ?? "virtio-net-pci"]); break;
            }
        }
        return args;
    }
}

public sealed class QmpClient
{
    private readonly string? socket; private readonly ManagedMachine owner;
    private static readonly HashSet<string> Allowed = ["query-status", "query-cpus-fast", "query-block", "query-memory-size-summary", "cont", "stop", "system_reset"];
    public QmpClient(string? socket, ManagedMachine owner) { this.socket = socket; this.owner = owner; }
    public async Task<object> ExecuteAsync(QmpRequest request)
    {
        if (!Allowed.Contains(request.Execute)) throw new InvalidOperationException("QMP command is not permitted by the default policy");
        owner.Publish("qmp.command", request.Execute);
        if (string.IsNullOrWhiteSpace(socket)) return new { @return = (object?)null, execute = request.Execute, simulated = true };
        await Task.CompletedTask; return new { @return = (object?)null, execute = request.Execute, socket };
    }
}

public static class WebSocketEndpoints
{
    public static async Task EventsAsync(HttpContext context, string id, MachineManager manager)
    {
        if (!context.WebSockets.IsWebSocketRequest || !manager.TryGet(id, out var machine)) { context.Response.StatusCode = 400; return; }
        using var socket = await context.WebSockets.AcceptWebSocketAsync(); var channel = System.Threading.Channels.Channel.CreateBounded<MachineEvent>(64); void Handler(MachineEvent e) => channel.Writer.TryWrite(e); machine.EventRaised += Handler;
        try { await foreach (var e in channel.Reader.ReadAllAsync(context.RequestAborted)) { var data = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(e)); await socket.SendAsync(data, WebSocketMessageType.Text, true, context.RequestAborted); } } catch (OperationCanceledException) { } finally { machine.EventRaised -= Handler; }
    }
    public static async Task SerialAsync(HttpContext context, string id, MachineManager manager)
    {
        if (!context.WebSockets.IsWebSocketRequest || !manager.TryGet(id, out var machine)) { context.Response.StatusCode = 400; return; }
        using var socket = await context.WebSockets.AcceptWebSocketAsync(); var buffer = new byte[8192]; while (socket.State == WebSocketState.Open && !context.RequestAborted.IsCancellationRequested) { var result = await socket.ReceiveAsync(buffer, context.RequestAborted); if (result.MessageType == WebSocketMessageType.Close) break; }
    }
}
