/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#include "telemetry.h"
#include <stdio.h>
#include <string.h>
void telemetry_init(void) {}
int telemetry_sample(qwm_telemetry *out){if(!out)return -1;memset(out,0,sizeof(*out));
#ifdef __linux__
 FILE *f=fopen("/proc/meminfo","r"); if(f){char key[64];unsigned long long value;char unit[16];while(fscanf(f,"%63s %llu %15s",key,&value,unit)==3){if(!strcmp(key,"MemTotal:"))out->memory_total=value*1024;if(!strcmp(key,"MemAvailable:"))out->memory_used=out->memory_total-value*1024;}fclose(f);}
#endif
 return 0;}
