  #ifndef RPC_PROTOCOL_INCLUDE
#define RPC_PROTOCOL_INCLUDE

#include "rpc_server.h"

/*

request 序列化

|_  _ |       _       | _ _ _ _ _ _ _ _| _ _ ...   | .......|
 req    func name len  data len         func name    data
 2byte  1byte          4byte            16byte max   data len
*/

/*
response 序列化

_  _ | _ _| _ _ _ _ _ _ _ _| .......|
rep    ack   data len         data
2byte 2byte 4byte            data len
*/

#define  REQUST_BASIC_LEN    7
typedef struct rpc_server_protocol_
{
	char		   basic_len[8];
	unsigned int   recvd;
	char           status;
	unsigned char  fn_len;
	rpc_request_t  req;
}*rpc_server_protocol_t;

#define  SET_STATUS(proto, offset) do{ proto->status = proto->status | (1 << offset);}while(0)
#define  STATUS_OK(proto, offset)  (proto->status & (1 << offset))

#define  FUNC_NAME_OK(proto)   STATUS_OK(proto,0)
#define  DATA_LEN_OK(proto)    STATUS_OK(proto,1)
#define  DATA_OK(proto)        STATUS_OK(proto,2)

#define  SET_FUNC_NAME_OK(proto)   SET_STATUS(proto,0)
#define  SET_DATA_LEN_OK(proto)    SET_STATUS(proto,1)
#define  SET_DATA_OK(proto)        SET_STATUS(proto,2)


typedef struct rpc_client_protocol_
{
	char		   basic[8];
	unsigned int   recvd;
	bool           basic_ok;
	bool           data_ok;
	rpc_response_t    rsp;
}*rpc_client_protocol_t;

rpc_server_protocol_t   rpc_server_proto_new();
int					    rpc_server_proto_read(rpc_server_protocol_t proto, int fd);
void                    rpc_server_proto_restore(rpc_server_protocol_t proto);
int                     rpc_server_proto_response_write(rpc_response_t * rsp, int fd);
void				    rpc_server_proto_destroy(rpc_server_protocol_t proto);
 
rpc_client_protocol_t   rpc_client_proto_new();
int					    rpc_client_proto_read(rpc_client_protocol_t proto,int fd);
void                    rpc_client_proto_restore(rpc_client_protocol_t proto);
int                     rpc_client_proto_request_write(rpc_request_t * req, int fd);
void				    rpc_client_proto_destroy(rpc_client_protocol_t proto);
#endif