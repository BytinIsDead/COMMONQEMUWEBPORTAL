/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#include "cli_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static char *dupstr(const char *s) { size_t n = strlen(s) + 1; char *p = malloc(n); if (p) memcpy(p, s, n); return p; }
static const char *arch_bin(const char *a) { const char *valid[] = {"x86_64","i386","ppc","ppc64","sparc","sparc64","mips","mips64","arm","aarch64","m68k","alpha","riscv64",NULL}; for (int i=0;valid[i];++i) if (!strcmp(a,valid[i])) return valid[i]; return NULL; }
static int accel_valid(const char *a) { return !strcmp(a,"kvm") || !strcmp(a,"whpx") || !strcmp(a,"hvf") || !strcmp(a,"nvmm") || !strcmp(a,"tcg"); }
static const char *bus_value(const char *b) { if (!b) return "virtio"; if (!strcmp(b,"ide")) return "ide"; if (!strcmp(b,"floppy")) return "floppy"; if (!strcmp(b,"scsi") || !strcmp(b,"virtio-scsi") || !strcmp(b,"lsi53c895a")) return "scsi"; return "virtio"; }
static int add(char **v, size_t cap, int n, const char *s) { if ((size_t)n + 1 >= cap) return -1; v[n] = dupstr(s); return v[n] ? n + 1 : -1; }
static int addf(char **v, size_t cap, int n, const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap,fmt); int r=vsnprintf(buf,sizeof(buf),fmt,ap); va_end(ap); return r<0 || (size_t)r>=sizeof(buf) ? -1 : add(v,cap,n,buf); }
int qwm_build_argv(const qwm_vm_config *c, char **v, size_t cap) {
    if (!c || !c->name || !c->architecture || !c->acceleration || c->vcpus < 1 || c->memory_mib < 16 || !arch_bin(c->architecture) || !accel_valid(c->acceleration)) return -1;
    int n=0; char cpu[1024]; n=addf(v,cap,n,"qemu-system-%s",arch_bin(c->architecture)); n=add(v,cap,n,"-name"); n=add(v,cap,n,c->name); n=add(v,cap,n,"-accel"); n=add(v,cap,n,c->acceleration); n=add(v,cap,n,"-smp"); n=addf(v,cap,n,"%d",c->vcpus); n=add(v,cap,n,"-m"); n=addf(v,cap,n,"%d",c->memory_mib); if (n<0) return -1;
    if (c->machine) { n=add(v,cap,n,"-M"); n=add(v,cap,n,c->machine); }
    if (c->cpu) { snprintf(cpu,sizeof(cpu),"%s",c->cpu); for(size_t i=0;i<c->cpu_flag_count;i++){ strncat(cpu,",",sizeof(cpu)-strlen(cpu)-1); strncat(cpu,c->cpu_flags[i],sizeof(cpu)-strlen(cpu)-1); } n=add(v,cap,n,"-cpu"); n=add(v,cap,n,cpu); }
    if (c->qmp_socket) { n=add(v,cap,n,"-qmp"); n=addf(v,cap,n,"unix:%s,server=on,wait=off",c->qmp_socket); }
    if (c->gdb_port > 0) { if(c->gdb_stop) n=add(v,cap,n,"-S"); n=add(v,cap,n,"-gdb"); n=addf(v,cap,n,"tcp:127.0.0.1:%d",c->gdb_port); }
    for(size_t i=0;i<c->device_count;i++){ const qwm_device *d=&c->devices[i]; if(!d->kind) continue; if(!strcmp(d->kind,"disk")){ n=add(v,cap,n,"-drive"); n=addf(v,cap,n,"file=%s,if=%s,format=qcow2%s",d->path?d->path:"",bus_value(d->bus),d->readonly?",readonly=on":""); } else if(!strcmp(d->kind,"display")){ n=add(v,cap,n,"-vga"); n=add(v,cap,n,d->model?d->model:"std"); } else if(!strcmp(d->kind,"serial")){ n=add(v,cap,n,"-serial"); n=addf(v,cap,n,"unix:%s,server=on,wait=off",d->path?d->path:""); } else if(!strcmp(d->kind,"usb")){ n=add(v,cap,n,"-device"); n=addf(v,cap,n,"usb-host,vendorid=0x%04x,productid=0x%04x",d->vendor_id,d->product_id); } else if(!strcmp(d->kind,"vfio-pci")){ n=add(v,cap,n,"-device"); n=addf(v,cap,n,"vfio-pci,host=%s",d->address?d->address:""); } else if(!strcmp(d->kind,"sd")){ n=add(v,cap,n,"-drive"); n=addf(v,cap,n,"file=%s,if=sd,format=raw",d->path?d->path:""); } }
    if((size_t)n>=cap) return -1; v[n]=NULL; return n;
}
void qwm_free_argv(char **v,int count){for(int i=0;i<count;i++)free(v[i]);}
