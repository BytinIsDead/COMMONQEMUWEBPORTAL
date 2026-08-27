/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#include "qmp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
static int allowed(const char *s){return !strcmp(s,"query-status")||!strcmp(s,"query-cpus-fast")||!strcmp(s,"query-block")||!strcmp(s,"query-memory-size-summary")||!strcmp(s,"cont")||!strcmp(s,"stop")||!strcmp(s,"system_reset")||!strcmp(s,"device_add")||!strcmp(s,"device_del")||!strcmp(s,"blockdev-snapshot-sync")||!strcmp(s,"balloon");}
int qwm_qmp_connect(qwm_qmp *q,const char *path){if(!q||!path)return -1; memset(q,0,sizeof(*q)); q->fd=socket(AF_UNIX,SOCK_STREAM,0); if(q->fd<0)return -1; struct sockaddr_un a; memset(&a,0,sizeof(a)); a.sun_family=AF_UNIX; strncpy(a.sun_path,path,sizeof(a.sun_path)-1); if(connect(q->fd,(struct sockaddr*)&a,sizeof(a))<0){close(q->fd);q->fd=-1;return -1;} strncpy(q->socket_path,path,sizeof(q->socket_path)-1); return 0;}
int qwm_qmp_send_raw(qwm_qmp *q,const char *json,char *response,size_t size){if(!q||q->fd<0||!json||!response||size<2)return -1; size_t len=strlen(json); if(write(q->fd,json,len)!=(ssize_t)len||write(q->fd,"\n",1)!=1)return -1; size_t used=0; while(used+1<size){ssize_t n=read(q->fd,response+used,size-used-1);if(n<=0)break;used+=(size_t)n;if(response[used-1]=='\n')break;}response[used]=0;return used?0:-1;}
int qwm_qmp_command(qwm_qmp *q,const char *execute,const char *args,char *response,size_t size){if(!allowed(execute))return -2; char json[4096]; snprintf(json,sizeof(json),"{\"execute\":\"%s\",\"arguments\":%s}",execute,args&&*args?args:"{}");return qwm_qmp_send_raw(q,json,response,size);}
void qwm_qmp_close(qwm_qmp *q){if(q&&q->fd>=0){close(q->fd);q->fd=-1;}}
