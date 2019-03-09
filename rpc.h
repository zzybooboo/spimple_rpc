#ifndef RPC_INCLUDE
#define RPC_INCLUDE 

#ifdef __cplusplus
#define EXTRERN  extern "C"
#else
#define EXTRERN  
#endif 

#define RPC_OK               0x0
#define PRC_NO_MEMORY        0x1
#define RPC_PROTOCOL_ERROR   0x2
#define PRC_RPOTOCOL_AGAIN   0x3
#define RPC_BROKEN_PIPE      0x4
#define PRC_UNKNOWN_CALLBACK 0x5
#define RPC_BAD_SOCKET       0x6
#define RPC_SOCKET_OPEN      0x7
#define PRC_REACTOR_CREATE   0x8
#define RPC_NO_CALLBACKS     0xa
#define RPC_TIME_OUT         0xb
#define RPC_SERVER_ERROR     0xc
#define RPC_MAX_FUNCNAME     0xd
#define RPC_SOCKET_BIND      0xe
#define RPC_CALLBACK_EXIST   0xf
#define RPC_BAD_CALLBACK     0x10

#include <stdbool.h>
#include <sys/types.h>

typedef bool (*rpc_func) (void *, size_t ,void ** ,size_t *);

typedef struct rpc_server *  rpc_server_t;
typedef struct rpc_client_ *  rpc_client_t;

EXTRERN int			 rpc_server_create(rpc_server_t * server);

EXTRERN int          rpc_server_regiser_func(rpc_server_t, const char * name, rpc_func func);

EXTRERN int          rpc_server_open(rpc_server_t server, const char * socket_path);

EXTRERN void         rpc_server_close(rpc_server_t server);

EXTRERN void         rpc_server_destroy(rpc_server_t server);


EXTRERN int          rpc_client_create(rpc_client_t * client);

EXTRERN int          rpc_client_open(rpc_client_t client, const char * server_addr, int timeout);

EXTRERN int          rpc_client_call_func(rpc_client_t client, const char * name, void * data, size_t len, void  ** ret, size_t * ret_len, int timeout);

EXTRERN void         rpc_client_destroy(rpc_client_t client);
#endif