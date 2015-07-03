#ifndef ROUTE_H_
#define ROUTE_H_


#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/shm.h>
#include<signal.h>
#include "common.h"
#include "fhr.h"
#include "../app_interface/TX_RX_interface.h"

#define RT_PORT				8
#define KEY_RT_PATH			"/etc/passwd"
#define RT_VIEW_KEY			10007
#define MAX_NODES			64
#define MAX_HOPS			MAX_NODES


enum route_protocol {
	DSR=1,
	PDQR=2,
	FHR=3
};

struct rt_view_ipcmsg {
	long type;
	U8 	msg[200];
};

typedef struct {
	U8 dst_addr;
	U8 hop;
	U8 path[MAX_HOPS]; 
}rt_entry;

typedef struct {
	enum route_protocol		rp;            /*when rp == 0, indicate that route-process is not run*********/
	U8	src_addr;
	U8 	update_array[MAX_NODES];
	rt_entry 	hash_cache[MAX_NODES];
}route_table_cache;

extern U8 SRC_ADDR;
extern int 	handle;
extern void notice_net_system(void);
extern void notice_view_process(void);
extern void update_rt_cache(U8 dst_addr, rt_entry *rt);
extern void clear_rt_cache(void);
#endif
