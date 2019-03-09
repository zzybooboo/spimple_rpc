#ifndef RPC_SOCKET_REACTOR_INCLUDE
#define RPC_SOCKET_REACTOR_INCLUDE


#include <stdbool.h>
#include <sys/types.h>

typedef bool(*rpc_reactor_on_recv)(void * , int);
typedef bool(*rpc_reactor_on_accept)(void * , int , void * );

typedef struct rpc_reactor_handler_{
	rpc_reactor_on_recv  recv_func;
	rpc_reactor_on_accept accept_func;
}rpc_reactor_handler_t;

typedef struct rpc_socket_reactor_  *rpc_socket_reactor_t;

rpc_socket_reactor_t   rpc_reactor_new(void * param, rpc_reactor_handler_t * handlers);

int                    rpc_reactor_run(rpc_socket_reactor_t reactor, int listenfd);

void                   rpc_reactor_canncel_regsiter(rpc_socket_reactor_t reactor,int fd);

void                   rpc_reactor_stop(rpc_socket_reactor_t reactor);

void                   rpc_reactor_destory(rpc_socket_reactor_t reactor);
#endif