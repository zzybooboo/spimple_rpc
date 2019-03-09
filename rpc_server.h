#ifndef RPC_SERVER_INCLUDE
#define RPC_SERVER_INCLUDE 

#include "map.h"
#include "rpc.h"
#include <stdio.h>
//#include <unistd.h>
#include <sys/types.h>
#include "rpc_socket.h"

#define MAX_RPC_NAME_LEN  16

#pragma pack(1)
typedef struct rpc_request_head_t
{
	unsigned short  req_num;
	char			func_name[MAX_RPC_NAME_LEN + 1];
	unsigned int    data_size;
}rpc_request_head_t;

typedef struct rpc_response_head_t
{
	unsigned short er;
	unsigned short ack;
	unsigned int   data_size;
}rpc_response_head_t;

typedef struct rpc_request_t{
	rpc_request_head_t  head;
	void *				data;
}rpc_request_t;

typedef struct rpc_response_t{
	rpc_response_head_t head;
	void *				data;
}rpc_response_t;
#pragma pack()
#endif