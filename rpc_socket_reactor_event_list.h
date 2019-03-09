#ifndef RPC_SOCKET_REACTOR_EVENT_LIST_INCLUDE
#define RPC_SOCKET_REACTOR_EVENT_LIST_INCLUDE

#include <list.h>
#include <stdbool.h>
#include <pthread.h>


typedef struct rpc_reactor_event_list_{
	pthread_mutex_t   mtx;
	pthread_cond_t    cond;
	struct list_head  root;
}rpc_reactor_event_list_, *rpc_reactor_event_list;

typedef struct rpc_reactor_event_
{
	struct list_head node;
	int				 fd;
}*rpc_reactor_event_t;

rpc_reactor_event_list  rpc_reactor_event_list_new();

bool                    rpc_reactor_event_push(rpc_reactor_event_list plist,int fd);

int                     rpc_reactor_event_pop(rpc_reactor_event_list plist);

void                    rpc_reactor_event_list_destory(rpc_reactor_event_list plist);

void                    rpc_reactor_event_signal(rpc_reactor_event_list plist);

#endif 