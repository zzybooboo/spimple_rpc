#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "rpc_protocol.h"


typedef int(*rpc_proto_read_flow) (rpc_server_protocol_t, int);
typedef int(*rpc_proto_client_read_flow) (rpc_client_protocol_t, int);

rpc_server_protocol_t rpc_server_proto_new()
{
	return  (rpc_server_protocol_t)calloc(1, sizeof(struct rpc_server_protocol_));
}


static inline int _rpc_protocol_write(int fd, void * buffer, size_t len)
{
	int left = len;
	char * pos = (char *)buffer;
	while (left > 0)
	{
		int wlen = write(fd, pos, left);
		if (wlen == 0)
			return RPC_BROKEN_PIPE;

		if (wlen < 0)
		{
			if ((errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
				continue;

			return RPC_BROKEN_PIPE;
		}
		 
		left -= wlen;
		pos += wlen;

	}
	return RPC_OK;
}

static inline int _rpc_protocol_read(int fd, void * buffer, size_t len, size_t * finished)
{
	size_t offset = 0;
	size_t left = len;
	while (left > 0)
	{
		int rlen = read(fd, (char *)buffer + offset, left);
		if (rlen == 0)	
			return RPC_BROKEN_PIPE;

		if (rlen < 0)
		{
			if (errno == EAGAIN)
			{
				// 没有接收完全,socket buffer 空了
				return PRC_RPOTOCOL_AGAIN;
			}
			//error
			return RPC_BROKEN_PIPE;
		}
		offset += rlen;
		left -= rlen;
		*finished = offset;
	}
	return RPC_OK;
}

/*

request 序列化

|_  _ |       _       | _ _ _ _ _ _ _ _| _ _ ...   | .......|
req    func name len  data len         func name    data
2byte  1byte          4byte            16byte max   data len
*/


//len 的大小是固定的，因此集中一起读
static inline int _rpc_read_request_proto_basic_len(rpc_server_protocol_t proto, int fd)
{
	 
	size_t finished = 0;
	char * offset = (char *)&proto->basic_len + proto->recvd;
	int error = _rpc_protocol_read(fd, offset, REQUST_BASIC_LEN - proto->recvd, &finished);

	proto->recvd += finished;

	if (error != RPC_OK)
		return error;

	memcpy(&proto->req.head.req_num, proto->basic_len, 2);
	proto->fn_len = proto->basic_len[2];
	memcpy(&proto->req.head.data_size, proto->basic_len + 3, 4);
	
	//转换字节序
	proto->req.head.req_num = ntohs(proto->req.head.req_num);
	proto->req.head.data_size = ntohl(proto->req.head.data_size);

	if (proto->fn_len > MAX_RPC_NAME_LEN)
		return RPC_PROTOCOL_ERROR;

	proto->recvd = 0;
	SET_DATA_LEN_OK(proto);
	return RPC_OK;
}

static inline int _rpc_read_request_proto_func_name(rpc_server_protocol_t proto, int fd)
{	
	size_t finished = 0;
	char * offset = (char *)&proto->req.head.func_name + proto->recvd;
	int error = _rpc_protocol_read(fd, offset, proto->fn_len - proto->recvd, &finished);
 
	proto->recvd += finished;
	if (error != RPC_OK)
		return error;

	
	SET_FUNC_NAME_OK(proto);
	proto->recvd = 0;
	return error;
}

static inline int _prc_read_request_proto_data(rpc_server_protocol_t proto, int fd)
{
	if (!proto->req.data  && proto->req.head.data_size > 0)
	{
 
		proto->req.data = calloc(proto->req.head.data_size + 1, sizeof(char));
		if (!proto->req.data)
			return PRC_NO_MEMORY;
	}

	size_t finished = 0;
	char * offset = (char *)proto->req.data + proto->recvd;
	int error = _rpc_protocol_read(fd, offset, proto->req.head.data_size - proto->recvd, &finished);
 
	proto->recvd += finished;
	if (error != RPC_OK)
		return error;

	SET_DATA_OK(proto);
	proto->recvd = 0;
	return error;
}

int	 rpc_server_proto_read(rpc_server_protocol_t proto, int fd)
{
	if (!proto)
		return RPC_SERVER_ERROR;
	
	bool  finish[] = { DATA_LEN_OK(proto), FUNC_NAME_OK(proto), DATA_OK(proto) };
	rpc_proto_read_flow flows[] = {
		_rpc_read_request_proto_basic_len,
		_rpc_read_request_proto_func_name,
		_prc_read_request_proto_data
	};

	int i = 0, error = 0;
	for (; i < sizeof(flows) / sizeof(flows[0]); i++)
	{
		if (finish[i])
			continue;

		error = (*flows[i])(proto, fd);
		if (error != RPC_OK)
			return error;
		
		finish[i] = true;
	}

	return RPC_OK;
}

/*
response 序列化

_  _ | _ _ | _ _ _ _ _ _ _ _| .......|
er    ack   data len         data
2byte 2byte 4byte            data len
*/

static inline void _rpc_server_proto_marshal_response_head(rpc_response_t * rsp, char * buffer)
{
 
	unsigned short er = htons(rsp->head.er);
	unsigned short ack = htons(rsp->head.ack);
	unsigned int data_size = htonl(rsp->head.data_size);

	memcpy(buffer, &er, sizeof(er));
	memcpy(buffer + 2, &ack, sizeof(ack));
	memcpy(buffer + 4, &data_size, sizeof(data_size));
}

int  rpc_server_proto_response_write(rpc_response_t * rsp, int fd)
{

	char buffer[8] = { 0 };
	_rpc_server_proto_marshal_response_head(rsp, buffer);

	int error  = _rpc_protocol_write(fd, buffer, sizeof(buffer));
	if (error != RPC_OK)
		return error;
	
	if (rsp->data)
		return _rpc_protocol_write(fd, rsp->data,rsp->head.data_size);

	return RPC_OK;
}

void   rpc_server_proto_restore(rpc_server_protocol_t proto)
{
	if (proto->req.data)
		free(proto->req.data);

	memset(proto, 0, sizeof(struct rpc_server_protocol_));
}

void  rpc_server_proto_destroy(rpc_server_protocol_t proto)
{
	if (!proto)
		return;

	if (proto->req.data)
		free(proto->req.data);

	free(proto);
}


/*
response 序列化

_  _ | _ _| _ _ _ _ _ _ _ _| .......|
er    ack   data len         data
2byte 2byte 4byte            data len
*/

rpc_client_protocol_t   rpc_client_proto_new()
{
	return  (rpc_client_protocol_t)calloc(1, sizeof(struct rpc_client_protocol_));
}

static inline int  _rpc_client_proto_read_basic(rpc_client_protocol_t proto, int fd)
{
	size_t finished = 0;
	char * offset = (char *)&proto->basic + proto->recvd;
	
	int error = _rpc_protocol_read(fd, offset, 8 - proto->recvd, &finished);

	proto->recvd += finished;

	if (error != RPC_OK)
		return error;

	memcpy(&proto->rsp.head.er, proto->basic, 2);
	memcpy(&proto->rsp.head.ack, proto->basic + 2, 2);
	memcpy(&proto->rsp.head.data_size, proto->basic + 4, 4);
 
	//转换字节序
	proto->rsp.head.er = ntohs(proto->rsp.head.er);
	proto->rsp.head.ack = ntohs(proto->rsp.head.ack);
	proto->rsp.head.data_size = ntohl(proto->rsp.head.data_size);
 
	proto->recvd = 0;
	proto->basic_ok = true;
	return RPC_OK;
}

static inline int	 _rpc_client_proto_read_data(rpc_client_protocol_t proto, int fd)
{
	if (!proto->rsp.data  && proto->rsp.head.data_size > 0)
	{
		//兼容字符串
		proto->rsp.data = calloc(proto->rsp.head.data_size + 1, sizeof(char));
		if (!proto->rsp.data)
			return PRC_NO_MEMORY;
	}

	size_t finished = 0;
	char * offset = (char *)proto->rsp.data + proto->recvd;
	int error = _rpc_protocol_read(fd, offset, proto->rsp.head.data_size - proto->recvd, &finished);
	proto->recvd += finished;
	if (error != RPC_OK)
		return error;

	proto->data_ok = true;
	proto->recvd = 0;
	return RPC_OK;
}

void  rpc_client_proto_restore(rpc_client_protocol_t proto)
{
	memset(proto, 0, sizeof(struct rpc_client_protocol_));
}

int	 rpc_client_proto_read(rpc_client_protocol_t proto, int fd)
{
	if (!proto)
		return RPC_SERVER_ERROR;

	bool  finish[] = { proto->basic_ok, proto->data_ok };
	rpc_proto_client_read_flow flows[] = {
		_rpc_client_proto_read_basic,
		_rpc_client_proto_read_data
	};

	int i = 0, error = 0;
	for (; i < sizeof(flows) / sizeof(flows[0]); i++)
	{
		
		if (finish[i])
			continue;

		error = (*flows[i])(proto, fd);
		if (error != RPC_OK)
			return error;

		finish[i] = true;
	}
	return RPC_OK;
}
/*

request 序列化

|_  _ |       _       | _ _ _ _ _ _ _ _| _ _ ...   | .......|
req    func name len  data len         func name    data
2byte  1byte          4byte            16byte max   data len
*/

static inline  void _rpc_client_proto_marshal_request_head(rpc_request_t * req, char * buffer)
{
	unsigned short req_num = htons(req->head.req_num);
	unsigned int   data_size = htonl(req->head.data_size);
	
	memcpy(buffer, &req_num, sizeof(req_num));
	buffer[2] = strlen(req->head.func_name);
	memcpy(buffer + 3, &data_size, sizeof(data_size));
	memcpy(buffer + 7, req->head.func_name, strlen(req->head.func_name));
}

int   rpc_client_proto_request_write(rpc_request_t * req, int fd)
{
	char buffer[32] = { 0 };
	_rpc_client_proto_marshal_request_head(req, buffer);
	int error = _rpc_protocol_write(fd, buffer, 7 + strlen(req->head.func_name));
	if (error != RPC_OK)
		return error;


	if (req->data && req->head.data_size > 0)
		return _rpc_protocol_write(fd, req->data, req->head.data_size);

	return RPC_OK;
}

void rpc_client_proto_destroy(rpc_client_protocol_t proto)
{
	if (!proto)
		return;

	if (proto->rsp.data)
		free(proto->rsp.data);

	free(proto);
}