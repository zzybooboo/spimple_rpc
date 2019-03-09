#include<unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "rpc_socket.h"
#include "rpc_transport.h"

#define TRANSPORT_DEFINE(arg) struct rpc_transport_ * transport = (struct rpc_transport_ *) param

static inline void _rpc_transport_close_client(struct rpc_transport_ *transport, int fd)
{
	pthread_rwlock_wrlock(&transport->lock);
	map_data_t key;
	ContainerInitIntData(key, fd);
	rpc_server_protocol_t proto = NULL;
	map_node_t * node = map_get(transport->client_map, &key);
	if (!node)
		goto clean;

	proto = (rpc_server_protocol_t)node->val.val.val_ptr;
	if (!proto)
		goto clean;


 
	//删除proto
	rpc_server_proto_destroy(proto);
	map_del(transport->client_map, node);
clean:
	close(fd);
	rpc_reactor_canncel_regsiter(transport->reactor, fd);

	pthread_rwlock_unlock(&transport->lock);
}

static inline  void _rpc_transport_clear_client(rpc_transport_t transport)
{
	pthread_rwlock_wrlock(&transport->lock);
	map_node_t * node = map_begin(transport->client_map);
	while (node)
	{
		map_node_t * next = map_next(node);
		rpc_server_protocol_t proto = (rpc_server_protocol_t)node->val.val.val_ptr;
		if (proto)
			rpc_server_proto_destroy(proto);
		
		close(node->key.val.val_int);
		rpc_reactor_canncel_regsiter(transport->reactor, node->key.val.val_int);
		map_del(transport->client_map, node);
		node = next;
	}
	pthread_rwlock_unlock(&transport->lock);
}

void             rpc_transport_stop(rpc_transport_t transport)
{
	if (!transport)
		return;

	rpc_reactor_stop(transport->reactor);
	_rpc_transport_clear_client(transport);
}

int             rpc_transport_run(rpc_transport_t transport, int listenfd)
{
	if (!transport)
		return PRC_NO_MEMORY;

	return rpc_reactor_run(transport->reactor, listenfd);
}


static inline void rpc_transport_clear_callback(rpc_transport_t transport)
{
	if (!transport->rpc_callback)
		return;

	map_node_t * node = map_begin(transport->rpc_callback);
	while (node)
	{
		map_node_t * next = map_next(node);
		map_del(transport->rpc_callback, node);
		node = next;
	}
}

void            rpc_transport_destory(rpc_transport_t transport)
{
	if (!transport)
		return;

	rpc_transport_stop(transport);
	rpc_transport_clear_callback(transport);

	if (transport->rpc_callback)
		map_destory(transport->rpc_callback);

	if (transport->client_map)
		map_destory(transport->client_map);

	pthread_rwlock_destroy(&transport->lock);
	transport->client_map = NULL;
	transport->rpc_callback = NULL;

	free(transport);
}


static inline int _rpc_transport_write(int fd, void * data, size_t len)
{
	if (!data || len <= 0)
		return RPC_OK;

	return rpc_unix_socket_send(fd, data, sizeof(rpc_response_head_t));	
}


static inline int _rpc_transport_response(rpc_request_t * req, int fd, void * data, size_t len)
{
	rpc_response_t rsp;
	memset(&rsp, 0, sizeof(rpc_response_t));
	rsp.data = data;
	rsp.head.er = RPC_OK;
	rsp.head.data_size = len;
	rsp.head.ack = req->head.req_num + 1;
	return rpc_server_proto_response_write(&rsp, fd);
}

static inline int _rpc_transport_response_error(int error, int fd)
{
	rpc_response_t rsp;
	memset(&rsp, 0, sizeof(rpc_response_t));
	rsp.head.er = 0;
	rsp.data = NULL;
	rsp.head.data_size = 0;
	rsp.head.ack = 0;

	rpc_server_proto_response_write(&rsp,fd);
}

static inline int _rpc_transport_excec_callback(struct rpc_transport_ * transport,
	int fd, rpc_request_t * req)
{
	map_data_t key;
	ContainerInitStringData(key, req->head.func_name);
	map_node_t * node = map_get(transport->rpc_callback, &key);
	if (!node)
		return PRC_UNKNOWN_CALLBACK;

	rpc_func func = (rpc_func)node->val.val.val_ptr;
	if (!func)
		return PRC_UNKNOWN_CALLBACK;
		

	void * rsp = NULL;
	size_t rsp_size = 0;
	(*func)(req->data, req->head.data_size, &rsp, &rsp_size);

	if (req->data)
		free(req->data);

	req->data = NULL;
	int error =  _rpc_transport_response(req, fd, rsp, rsp_size);
	if (rsp)
	{
		free(rsp);
	}
	return error;
}

rpc_server_protocol_t  _rpc_transport_protocol_get(struct rpc_transport_ * transport, int fd){
	pthread_rwlock_rdlock(&transport->lock);
	map_data_t key;
	ContainerInitIntData(key, fd);
	rpc_server_protocol_t proto = NULL;
	map_node_t * node = map_get(transport->client_map, &key);
	if (!node)
		goto unlock;

	proto = (rpc_server_protocol_t)node->val.val.val_ptr;
	if (!proto)
		goto  unlock;

unlock:
	pthread_rwlock_unlock(&transport->lock);
	return proto;
}



bool  _rpc_transport_on_recv(void * param, int fd)
{
	int error = 0;
	TRANSPORT_DEFINE(param);
	
	rpc_server_protocol_t proto = _rpc_transport_protocol_get(transport, fd);
	if (!proto)
	{
		error = RPC_SERVER_ERROR;
		goto response_error;
	}
	 
	error = rpc_server_proto_read(proto, fd); //没有完全读取
	if (error != RPC_OK && error == PRC_RPOTOCOL_AGAIN)
		return true;
		
	if (error == RPC_OK)
	{
		error = _rpc_transport_excec_callback(transport, fd, &proto->req); ///调用处理回调
		if (error != RPC_OK)
		{
			goto response_error;
		}
		rpc_server_proto_restore( proto);
		return true;
	}

response_error:
	////如果 不是网络连接出现问题，则回复错误代码
	if (error != RPC_BROKEN_PIPE)
		_rpc_transport_response_error(error, fd);

	_rpc_transport_close_client(transport,fd);
	return false;
}

bool  _rpc_transport_on_accept(void * param, int fd, void * addr)
{
	TRANSPORT_DEFINE(param);

	if (!rpc_socket_setopt(fd))
		return false;

	pthread_rwlock_wrlock(&transport->lock);
	
	bool success = false;
	map_data_t key,val;
	ContainerInitIntData(key, fd);
	rpc_server_protocol_t proto = NULL;
	map_node_t * node = map_get(transport->client_map, &key);
	if (node)
		goto unlock;

	proto = rpc_server_proto_new();
	if (!proto)
		goto unlock;

	ContainerInitPtrData(val, proto);
	if (!map_insert(transport->client_map, &key, &val))
	{
		rpc_server_proto_destroy(proto);
		goto unlock;
	}

	success = true;
unlock:
	if (proto && !success)
		rpc_server_proto_destroy(proto);

	pthread_rwlock_unlock(&transport->lock);
	return success;
}

static inline _rpc_init_rpc_reactor_handler(rpc_reactor_handler_t * handler)
{
	handler->recv_func = _rpc_transport_on_recv;
	handler->accept_func = _rpc_transport_on_accept;
}

int           rpc_transport_new(rpc_transport_t * trans)
{
	rpc_reactor_handler_t handler;
	rpc_transport_t transport = (rpc_transport_t)calloc(1, sizeof(struct rpc_transport_));
	if (!transport)
		return PRC_NO_MEMORY;

	int error = RPC_OK;

	//(&transport->mtx, NULL);
	pthread_rwlock_init(&transport->lock, NULL);
	// 客户端信息
	transport->client_map = map_new(type_int);
	if (!transport->client_map)
	{
		error = PRC_NO_MEMORY;
		goto clean;
	}
	// 回调
	transport->rpc_callback = map_new(type_string);
	if (!transport->rpc_callback)
	{
		error = PRC_NO_MEMORY;
		goto clean;
	}
	// 事件处理器
	_rpc_init_rpc_reactor_handler(&handler);
	transport->reactor = rpc_reactor_new(transport, &handler);
	if (!transport->reactor)
	{
		error = PRC_NO_MEMORY;
		goto clean;
	}

	*trans = transport;
	return RPC_OK;
clean:
	rpc_transport_destory(transport);
	return error;
}
