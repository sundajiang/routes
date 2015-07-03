#include "fhr.h"

static struct fhr_rt_entry 	fhr_rt_cache[MAX_NODES];
static struct fhr_rt_neighbor fhr_rt_neighbor[MAX_NODES];
static int 	static_route_flag = 0;

static void fhr_snd(char *buf, int length)
{
	
	if (buf == NULL | length < 0) {
		sys_info("fhr_snd: parameter is invalid");
		return;	
	}
					
	if (ap_sendto(handle, buf, length, 0) < 0)
		sys_info("fhr_snd faild");

	printf("fhr----fhr_snd: length=%d\n", length);
		
#ifdef DE_BUG
printf("fhr----fhr_snd: length=%d\n", length);
#endif		
}
static int fhr_rcv(char *buf)
{

	if (buf == NULL)
		return -1;
	return ap_rcvfrom(handle, buf, DATA_MAX_LENGTH);
}

static inline void produce_net_sys_rt_entry(rt_entry* rt, U8 dst_addr, U8 hop, U8 next_node)
{
	rt->dst_addr = dst_addr;
	rt->hop = hop;
	rt->path[0] = next_node;
}
static inline void delete_route_entry(U8 dst_addr)
{
	memset(&fhr_rt_cache[dst_addr], 0, sizeof(struct fhr_rt_entry));
	
	rt_entry 	rt;
	produce_net_sys_rt_entry(&rt, 0, 0, 0);
	update_rt_cache(dst_addr, &rt);
	notice_net_system();
	notice_view_process();
}
static inline void add_route_entry(U8 dst_addr, U8 hop, U8 next_node)
{
	#ifdef DE_BUG
	printf("add_route_entry: dst=%d hop=%d next=%d\n", dst_addr, hop, next_node);
	#endif
	
	struct fhr_rt_entry *rt;
	rt = &fhr_rt_cache[dst_addr];
	rt->dst_addr = dst_addr;
	rt->hop = hop;
	rt->next_node = next_node;
	rt->time = time(NULL);
}

static int create_sop(char *sop)
{
	if (sop == NULL)
		return -1;

        int 	i, len;
        char 	*ptr;
        time_t 	ctime;
        struct fhr_rt_entry_opt *opt;
        struct fhr_rt_sop *sop_hdr;

        sop_hdr = (struct fhr_rt_sop*)sop;
        sop_hdr->type = ROUTE_SOP;
        sop_hdr->src_addr = SRC_ADDR;
        sop_hdr->rt_entry_num = 1;
        ptr = sop+3;
        opt = (struct fhr_rt_entry_opt*)ptr;
        opt->dst_addr = SRC_ADDR;
        opt->hop = 0;
        opt->next_node = SRC_ADDR;
        len = 3+3;
        ptr += 3;

        ctime = time(NULL);
	for (i=0; i<MAX_NODES; i++) 
	{
		if (fhr_rt_cache[i].dst_addr == 0)
			continue;
		if (ctime - fhr_rt_cache[i].time > RT_TABLE_TIMEOUT) {
			delete_route_entry(fhr_rt_cache[i].dst_addr);
			continue;
		}
		if (fhr_rt_cache[i].hop > MAX_ZONE_RADIUS)
                 {
                         continue;
                  }
		opt = (struct fhr_rt_entry_opt*)ptr;
		opt->dst_addr = fhr_rt_cache[i].dst_addr;
		opt->hop = fhr_rt_cache[i].hop;
		opt->next_node = fhr_rt_cache[i].next_node;
		ptr += 3;
		len += 3;
		sop_hdr->rt_entry_num++;
	}
	return len;
}

static void broadcast_sop(void)
{
	int length;
	char sop[3*MAX_NODES+3];
	
	length = create_sop(sop);	
	fhr_snd(sop, length);

}

static void sig_broadcast_sop (int signo)
{
	if (static_route_flag == 0) {
		alarm(SOP_INTERVAL);
		broadcast_sop();
	}
}

static void sop_proc(char *pkt, int length)
{

	if (pkt == NULL | length < 0)
		return;
		
	int 	i, update_flag = 0;	
	U8 		dst_addr, j;
	time_t 	ctime;
	struct fhr_rt_entry_opt *opt;
	struct fhr_rt_sop *sop_hdr;	
	rt_entry 	rt;
		
	ctime = time(NULL);	
	sop_hdr = (struct fhr_rt_sop*)pkt;
	opt = (struct fhr_rt_entry_opt*)(pkt+3);	
	dst_addr = sop_hdr->src_addr;
	fhr_rt_neighbor[dst_addr].dst_addr = dst_addr;
	fhr_rt_neighbor[dst_addr].time = time(NULL);
	for(i=0; i<sop_hdr->rt_entry_num; i++) 
	{
		/*if (opt->dst_addr != SRC_ADDR) {
			if (fhr_rt_cache[opt->dst_addr].dst_addr == 0 
				|| fhr_rt_cache[opt->dst_addr].hop > opt->hop+1
				|| ctime-fhr_rt_cache[opt->dst_addr].time > RT_TABLE_TIMEOUT) {
				update_flag = 1;
				
				add_route_entry(opt->dst_addr, opt->hop+1, dst_addr);
				
				produce_net_sys_rt_entry(&rt, opt->dst_addr, opt->hop+1, dst_addr);
				update_rt_cache(opt->dst_addr, &rt);
				
			} 
			if (fhr_rt_cache[opt->dst_addr].dst_addr == opt->dst_addr
				&& fhr_rt_cache[opt->dst_addr].next_node == dst_addr) 
			{
				fhr_rt_cache[opt->dst_addr].time = ctime;
				//printf("sop_proc update time dst=%d\n", opt->dst_addr);
			}
		}
		opt += 1;*/
		
		/* the dest is myself, need not update */ 
		if (opt->dst_addr == SRC_ADDR)
			goto loop;
		/* the next hop is myself, need not update  ,这条路由是我更新过去的，不接受反向更新  */
		if (opt->next_node == SRC_ADDR)
			goto loop;
		/* no route to dest or expires*/
		if ((fhr_rt_cache[opt->dst_addr].dst_addr == 0)||
			(ctime-fhr_rt_cache[opt->dst_addr].time > RT_TABLE_TIMEOUT)||
			(fhr_rt_cache[opt->dst_addr].hop > (opt->hop+1)))
		{
			update_flag = 1;
                        add_route_entry(opt->dst_addr, opt->hop+1, dst_addr);   //添加到fhr路由表
			/*****change system route table*****/
			produce_net_sys_rt_entry(&rt, opt->dst_addr, opt->hop+1, dst_addr);
                        update_rt_cache(opt->dst_addr, &rt);                       //添加到总的路由表（共享内存，没有超时）
		}
		
		if (fhr_rt_cache[opt->dst_addr].dst_addr == opt->dst_addr
			&& fhr_rt_cache[opt->dst_addr].next_node == dst_addr 
			&& opt->hop <= fhr_rt_cache[opt->dst_addr].hop)   //上一跳发来的包会更新时间，但要求跳数不大于原来
		{
			fhr_rt_cache[opt->dst_addr].time = ctime;
			//printf("sop_proc update time dst=%d\n", opt->dst_addr);
		}
		
		loop:
			opt += 1;
	}	
	
	printf("fhr---------------------------------------------------fhr_rcv from: %d  and length=%d\n",dst_addr,length);
	if (update_flag == 1) {
		notice_net_system();
		notice_view_process();
	}
			
}

static void sig_static_route_start(int signo)
{
	printf("static route start\n");
	static_route_flag = 1;
	memset(fhr_rt_cache, 0, sizeof(fhr_rt_cache));
	memset(fhr_rt_neighbor, 0, sizeof(fhr_rt_neighbor));
	clear_rt_cache();
	rt_entry 	rt;
	if(1 == SRC_ADDR)
	{
		produce_net_sys_rt_entry(&rt, 2, 1, 2);
		update_rt_cache(2, &rt);
		produce_net_sys_rt_entry(&rt, 3, 2, 2);
		update_rt_cache(3, &rt);
		produce_net_sys_rt_entry(&rt, 6, 3, 2);
		update_rt_cache(6, &rt);
	}
	if(2 == SRC_ADDR)
	{
		produce_net_sys_rt_entry(&rt, 1, 1, 1);
		update_rt_cache(1, &rt);
		produce_net_sys_rt_entry(&rt, 3, 1, 3);
		update_rt_cache(3, &rt);
		produce_net_sys_rt_entry(&rt, 6, 2, 3);
		update_rt_cache(6, &rt);	
	}
	if(3 == SRC_ADDR)
	{
		produce_net_sys_rt_entry(&rt, 1, 2, 2);
		update_rt_cache(1, &rt);
		produce_net_sys_rt_entry(&rt, 2, 1, 2);
		update_rt_cache(2, &rt);
		produce_net_sys_rt_entry(&rt, 6, 1, 6);
		update_rt_cache(6, &rt);
	}
	if(6 == SRC_ADDR)
	{
		produce_net_sys_rt_entry(&rt, 1, 3, 3);
		update_rt_cache(1, &rt);
		produce_net_sys_rt_entry(&rt, 2, 2, 3);
		update_rt_cache(2, &rt);
		produce_net_sys_rt_entry(&rt, 3, 1, 3);
		update_rt_cache(3, &rt);
	}
	
	notice_net_system();
	notice_view_process();
}
static void sig_dynamic_route_start(int signo)
{
	printf("dynamic route start\n");
	static_route_flag = 0;
	memset(fhr_rt_cache, 0, sizeof(fhr_rt_cache));
	memset(fhr_rt_neighbor, 0, sizeof(fhr_rt_neighbor));
	clear_rt_cache();
	
	rt_entry 	rt;
	memset(&rt, 0, sizeof(rt));
	update_rt_cache(1, &rt);
	update_rt_cache(2, &rt);
	update_rt_cache(3, &rt);
	update_rt_cache(6, &rt);
	
	notice_net_system();
	notice_view_process();
	alarm(1);
}

/*-----------------init---------------------*/
int fhr_init(void)
{
	memset(fhr_rt_cache, 0, sizeof(fhr_rt_cache));
	memset(fhr_rt_neighbor, 0, sizeof(fhr_rt_cache));
	if (signal(SIGALRM, sig_broadcast_sop) == SIG_ERR) {
		sys_err("can't catch SIGALRM");
		return -1;
	}
	if (signal(SIGUSR1, sig_static_route_start) == SIG_ERR) {
		sys_err("can't catch SIGUSR1");
		return -1;
	}
	if (signal(SIGUSR2, sig_dynamic_route_start) == SIG_ERR) {
		sys_err("can't catch SIGUSR1");
		return -1;
	}
	
	return 0;
}

void fhr_exit(void)
{
	
}

void fhr_main(void)
{
	int 	n;
	char 	buf[DATA_MAX_LENGTH];
	char	*type_ptr;
	
	alarm(1);
	while(1) {
		n = fhr_rcv(buf);
		if (n<0)
			continue;
		if (static_route_flag == 1)
			continue;		
				

#ifdef DE_BUG
printf("fhr---------------------------------------------------fhr_rcv: length=%d\n", n);

#endif		
		
		type_ptr = buf;	
		switch(*type_ptr) {
		  case ROUTE_REQ:
		  	break;
		  case ROUTE_REPLY:
		  	break;
		  case ROUTE_ERR:
		  	break;
		  case ROUTE_SOP:
		  	sop_proc(buf, n);
		  	break;
		  default:
		  	sys_info("fhr rcv one invalid package");
		}
	}
}



