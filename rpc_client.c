#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rpc.h"
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include "rpc_server.h"
#include "rpc_protocol.h"

#define RAND(smax) 1+(int)(smax*rand()/(RAND_MAX+1.0))

typedef struct rpc_client_
{
	rpc_socket_t sock;
	rpc_client_protocol_t proto;
};


int          rpc_client_create(rpc_client_t * client)
{
	
	rpc_client_t temp = (rpc_client_t)calloc(1, sizeof(struct rpc_client_));
	if (!temp)
		return PRC_NO_MEMORY;

	temp->sock = -1;
	int error = RPC_OK;
	temp->proto = rpc_client_proto_new();
	if (!temp->proto)
	{
		error = PRC_NO_MEMORY;
		goto clean;
	}
		
	*client = temp;
	return RPC_OK;
clean:
	rpc_client_destroy(temp);
	return error;
}

int          rpc_client_open(rpc_client_t client, const char * server_addr, int timeout)
{
	if (!client)
		return PRC_NO_MEMORY;

	if (!server_addr)
		return RPC_BAD_SOCKET;

	int error = rpc_unix_socket_create(&client->sock);
	if (error != RPC_OK)
		return error;

	return rpc_unix_socket_connect(client->sock, server_addr, timeout);;
}

void          rpc_client_destroy(rpc_client_t client)
{
	if (!client)
		return;

	rpc_unix_socket_close(client->sock);
	rpc_client_proto_destroy(client->proto);
	client->proto = NULL;
	client->sock = -1;
	free(client);
	
}


static inline void _rpc_client_request_init(rpc_request_t * req, const char * name,
	void * data, size_t len)
{
	memset(req, 0, sizeof(rpc_request_t));
	req->head.data_size = len;
	req->head.req_num = RAND(1000);
	strncpy(req->head.func_name, name, sizeof(req->head.func_name) - 1);
	req->data = data;
}

int  rpc_client_call_func(rpc_client_t client, const char * name, void * data,
	size_t len, void  ** ret, size_t * ret_len, int timeout)
{
	if (!client || !name)
		return PRC_NO_MEMORY;
	

	if (strlen(name) > MAX_RPC_NAME_LEN)
		return RPC_MAX_FUNCNAME;

	rpc_request_t req;
	_rpc_client_request_init(&req, name, data, len);

	int error = rpc_client_proto_request_write(&req, client->sock);
	
	if (error != RPC_OK)
		return error;

	while (true)
	{
		struct timeval tv;
		tv.tv_usec = 0;
		tv.tv_sec = timeout;

		fd_set r;
		FD_ZERO(&r);
		FD_SET(client->sock, &r);

		if (select(client->sock + 1, &r, NULL, NULL, &tv) < 0)
			return RPC_BROKEN_PIPE;


		if (FD_ISSET(client->sock, &r))
		{
			error = rpc_client_proto_read(client->proto, client->sock);
			if (error == PRC_RPOTOCOL_AGAIN)
				continue;

			break;
		}
		else
		{
			return RPC_TIME_OUT;
		}
	}
	
	if (client->proto->rsp.head.er != RPC_OK)
	{
		if (client->proto->rsp.data)
			free(client->proto->rsp.data);

		client->proto->rsp.data = NULL;
		rpc_client_proto_restore(client->proto);
		return client->proto->rsp.head.er;
	}

	if (client->proto->rsp.head.ack != (req.head.req_num + 1))
	{
		if (client->proto->rsp.data)
			free(client->proto->rsp.data);

		client->proto->rsp.data = NULL;
		rpc_client_proto_restore(client->proto);
		return RPC_PROTOCOL_ERROR;
	}

	if (client->proto->rsp.data && client->proto->rsp.head.data_size > 0)
	{
		*ret = client->proto->rsp.data;
		ret_len = client->proto->rsp.head.data_size;
	}
	rpc_client_proto_restore(client->proto);
	return RPC_OK;
}