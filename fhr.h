#ifndef FHR_H_
#define FHR_H_

#include "common.h"
#include "route.h"

#define ROUTE_REQ          1
#define ROUTE_REPLY        2
#define ROUTE_ERR          3
#define DATA               4
#define ROUTE_SOP          5
#define ACK 		       6
#define OTHER_NET		   7

#define SOP_INTERVAL                1     	   	//s
#define RT_TABLE_TIMEOUT			5  	   	    //seconds

#define MAX_ZONE_RADIUS             3
#define MIN_ZONE_RADIUS             2


struct fhr_rt_entry_opt {
	U8 	dst_addr;
	U8	hop;
	U8 	next_node;
};

struct fhr_rt_entry {
	U8 	dst_addr;
	U8	hop;
	U8 	next_node;
	time_t 	time;
};

struct fhr_rt_neighbor {
	U8 	dst_addr;
	time_t 	time;
};

struct fhr_rt_sop {
	U8 type;
	U8 src_addr;
	U8 rt_entry_num;
};



extern int fhr_init(void);
extern void fhr_exit(void);
extern void fhr_main(void);

#endif 
