#ifndef RPC_TRANSPORT_INCLUDE
#define RPC_TRANSPORT_INCLUDE

#include <rbtree.h>
#include <pthread.h>
#include "rpc_socket.h"
#include "rpc_protocol.h"
#include "rpc_socket_reactor.h"

typedef struct rpc_transport_
{
	pthread_rwlock_t        lock;
	rpc_socket_reactor_t    reactor;
	map *					client_map;
	map *					rpc_callback;
}*rpc_transport_t;

int				rpc_transport_new(rpc_transport_t * trans);

void            rpc_transport_stop(rpc_transport_t transport);

int             rpc_transport_run(rpc_transport_t transport, int listenfd);

void            rpc_transport_destory(rpc_transport_t transport);

#endif