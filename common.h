#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/sendfile.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define U8		unsigned char
/******IPC**********/
#define MSG_R 	0400
#define MSG_W 	0200
#define SVMSG_MODE      (MSG_R | MSG_W | MSG_R>>3 | MSG_R>>6) 
#define DATA_MAX_LENGTH			1500

//#define DE_BUG

extern U8 SRC_ADDR;
extern void sys_info(const char *msg);
extern void sys_err(const char *msg);
extern void sys_exit(const char *msg);
extern void sys_debug(const char *msg);

#endif
