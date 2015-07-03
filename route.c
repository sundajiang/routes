#include "route.h"

static pid_t 	net_system_pid = -1;
/****this key for sending route-request to route-protocol module and sharing memory****/
static int		route_msgid = -1, route_shmid = -1, route_view_msgid = -1;
route_table_cache	*rt_cache = NULL;
U8 	SRC_ADDR = -1;
int handle = -1;

void notice_net_system(void)
{
	if (net_system_pid < 0)
		return;
	if (kill(net_system_pid, SIGUSR1) < 0) {
		sys_err("failed to notice net system for update route table");
	}	
	printf("signal to notice net system update route\n");
}


void notice_view_process(void)
{
	int 	i, len;
	U8 	*ptr, rt_num;
	struct rt_view_ipcmsg  rtmsg;
	
	rtmsg.type = rt_cache->src_addr;
	rtmsg.msg[0] = rt_cache->src_addr;
	ptr = rtmsg.msg + 2;
	len = 2;
	rt_num = 0;
	for (i=0; i< MAX_NODES; i++) {
		if (rt_cache->hash_cache[i].dst_addr > 0 && rt_cache->hash_cache[i].dst_addr != rt_cache->src_addr) {
			ptr[0] = rt_cache->hash_cache[i].dst_addr;
			ptr[1] = rt_cache->hash_cache[i].hop;
			ptr[2] = rt_cache->hash_cache[i].path[0]; 
			ptr += 3;
			rt_num++;
			len += 3;
		}
	}
	rtmsg.msg[1] = rt_num;
resnd:
	if (msgsnd(route_view_msgid, &rtmsg, len, 0) < 0) {
		if (errno == EINTR)
			goto resnd;
		else if (errno == EIDRM) {
			sys_err("notic_view_process: message-queue is down");
		}
	}
	
//#ifdef DE_BUG
	printf("route_view_msg:");
	for (i=0; i<len; i++)
		printf(" %x", rtmsg.msg[i]);
	printf("\n");
//#endif
}

void update_rt_cache(U8 dst_addr, rt_entry *rt)
{
	rt_cache->hash_cache[dst_addr] = *rt;
	rt_cache->update_array[dst_addr] = 1;
}

void clear_rt_cache(void)
{
	memset(rt_cache, 0, sizeof(rt_cache));
	rt_cache->rp = FHR;
	rt_cache->src_addr = SRC_ADDR;
}

/******open route-msg-q, route-table share-memory, init rt_cache and open net-system api*****/
static int route_init(void)
{
	key_t 	msg_key, shm_key;
	void	*shmaddr;
	
	msg_key = ftok(KEY_RT_PATH, 1);
	shm_key = ftok(KEY_RT_PATH, 2);
	if (msg_key<0 || shm_key<0) {
		sys_err("failed route ftok:");
		return -1;
	}	
	route_msgid = msgget(msg_key, SVMSG_MODE | IPC_CREAT);
	route_view_msgid = msgget(RT_VIEW_KEY, SVMSG_MODE | IPC_CREAT);
	route_shmid = shmget(shm_key, sizeof(route_table_cache)+1,SVMSG_MODE | IPC_CREAT);
	if (route_msgid<0 || route_shmid<0 || route_view_msgid<0) {
		sys_err("failed msgget or shmget:");
		return -1;
	}
		
	shmaddr = shmat(route_shmid, NULL, 0);
	if (shmaddr == NULL) {
		sys_err("failed shmat:");
		return -1;
	}
	rt_cache = (route_table_cache*)shmaddr;
	memset(rt_cache, 0, sizeof(rt_cache));
	rt_cache->rp = FHR;
	rt_cache->src_addr = SRC_ADDR;
	
	if (fhr_init() < 0) {
		sys_info("fhr init failed");
		return -1;
	}

	handle = ap_socket(RT_PORT);
	if (handle < 0) {
		sys_info("ap_socket failed");
		return -1;
	}
	return 0;
}

static void route_exit(void)
{
	if (route_msgid >= 0)
		if (msgctl(route_msgid, IPC_RMID, NULL) < -1)
			sys_err("failed remove route-message-queue");
	if (route_shmid >= 0)
		if (msgctl(route_shmid, IPC_RMID, NULL) < -1)
			sys_err("failed remove route-share-memory");	
	ap_close(RT_PORT);		
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		sys_info("too few parameter, please input net-system-pid");
		return -1;
	}
	net_system_pid = atoi(argv[1]);
	if (net_system_pid <= 0) {
		sys_info("invalid parameter, exit");
		return -1;
	}
	SRC_ADDR = atoi(argv[2]);
	if (SRC_ADDR<=0 | SRC_ADDR>MAX_NODES) {
		sys_info("invalid parameter, exit");
		return -1;
	}
	if (route_init() < 0) {
		sys_info("route_init failed, exit");
		return -1;
	}
		
	
	switch(rt_cache->rp) {
	  case FHR:
	  	fhr_main();
	  	break;
/*	  case DSR:
	  	dsr_main();
	  	break;
	  case PDQR:
	  	pdqr_main();*/
	}
	
	route_exit();
}
