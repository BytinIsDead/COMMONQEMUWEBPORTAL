/* Extreme QEMU Web Manager — AGPL-3.0-or-later
 * Copyright (C) 2026 Extreme QEMU Web Manager contributors.
 * Licensed under the GNU Affero General Public License version 3 or later.
 */
#include "server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
static const char *mime(const char *p){const char *e=strrchr(p,'.');if(e&&!strcmp(e,".css"))return "text/css";if(e&&!strcmp(e,".js"))return "application/javascript";if(e&&!strcmp(e,".svg"))return "image/svg+xml";return "text/html";}
static void response(int fd,int code,const char *type,const char *body,size_t len){char h[512];int n=snprintf(h,sizeof(h),"HTTP/1.1 %d OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n",code,type,len);send(fd,h,(size_t)n,0);send(fd,body,len,0);}
static void serve(int fd,const char *root,const char *request){char path[1024],full[2048];if(sscanf(request,"GET %1023s",path)!=1){response(fd,400,"text/plain","bad request",11);return;}if(!strcmp(path,"/api/v1/health")){response(fd,200,"application/json","{\"status\":\"ok\"}",15);return;}if(!strcmp(path,"/"))strcpy(path,"/index.html");if(strstr(path,"..")){response(fd,403,"text/plain","forbidden",9);return;}snprintf(full,sizeof(full),"%s%s",root,path);FILE *f=fopen(full,"rb");if(!f){response(fd,404,"text/plain","not found",9);return;}fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char *body=malloc((size_t)n);if(!body){fclose(f);response(fd,500,"text/plain","memory error",12);return;}fread(body,1,(size_t)n,f);fclose(f);response(fd,200,mime(full),body,(size_t)n);free(body);}
int server_run(int port,const char *public_dir){int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return 1;int one=1;setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));struct sockaddr_in a;memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons((unsigned short)port);if(bind(s,(struct sockaddr*)&a,sizeof(a))<0||listen(s,32)<0){close(s);return 1;}for(;;){int c=accept(s,NULL,NULL);if(c<0){if(errno==EINTR)continue;break;}char req[4096];ssize_t n=recv(c,req,sizeof(req)-1,0);if(n>0){req[n]=0;serve(c,public_dir,req);}close(c);}close(s);return 0;}
