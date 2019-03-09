#ifndef RPC_SOCKET_INCLUDE
#define RPC_SOCKET_INCLUDE
#include "rpc.h"

typedef int rpc_socket_t;

typedef bool (*rpc_server_aeccptor)(rpc_server_t server);

bool				  rpc_socket_setopt(rpc_socket_t sock);

int				      rpc_unix_socket_create(rpc_socket_t * sock);

int				      rpc_unix_socket_bind(rpc_socket_t sock, const char * socket_path);

int                   rpc_unix_socket_listen(rpc_socket_t sock);

void                  rpc_unix_socket_close(rpc_socket_t sock);

int                   rpc_unix_socket_connect(rpc_socket_t sock, const char * addr, int timeout);


#endif