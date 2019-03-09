#include "rpc.h"
//#include <map.h>
#include "rpc_server.h"
#include "rpc_transport.h"
#include "rpc_socket_reactor.h"

struct rpc_server
{
	rpc_socket_t			sock;
	rpc_transport_t         transport;
};


int rpc_server_create(rpc_server_t * server)
{
	
	rpc_server_t  server_temp = (rpc_server_t)calloc(1, sizeof(rpc_server_t));
	if (!server_temp)
		return PRC_NO_MEMORY;

	server_temp->sock = -1;
	int error = rpc_transport_new(&server_temp->transport);
	if (RPC_OK != error)
		goto clean;
	
	*server = server_temp;
	return RPC_OK;
clean:
	rpc_server_destroy(server_temp);
	return error;
}

int  rpc_server_regiser_func(rpc_server_t server, const char * name, rpc_func func)
{
	if (!func)
		return RPC_BAD_CALLBACK;

	if (!server || !server->transport || !name)
		return PRC_NO_MEMORY;

	if (strlen(name) >= MAX_RPC_NAME_LEN)
		return MAX_RPC_NAME_LEN;

	map_data_t key, val;
	ContainerInitStringData(key, name);

	map_node_t * node = map_get(server->transport->rpc_callback, &key);
	if (node){
		return RPC_CALLBACK_EXIST;
	}

	ContainerInitPtrData(val, func);
	return map_insert(server->transport->rpc_callback, &key, &val) == 1 ? RPC_OK:RPC_CALLBACK_EXIST;
}

int   rpc_server_open(rpc_server_t server, const char * socket_path)
{
	if (!server || !socket_path)
		return PRC_NO_MEMORY;

	if (map_size(server->transport->rpc_callback) <= 0)
		return RPC_NO_CALLBACKS;

	int error = rpc_unix_socket_create(&server->sock);
	if (error != RPC_OK)
		return error;

	error = rpc_unix_socket_bind(server->sock, socket_path);
	if (error != RPC_OK)
		return error;

	error = rpc_unix_socket_listen(server->sock);
	if (error != RPC_OK)
		return error;

	error = rpc_transport_run(server->transport, server->sock);
	if (error != 0)
		return error;
	
	return RPC_OK;
}

void  rpc_server_close(rpc_server_t server)
{
	if (!server)
		return; 

	rpc_transport_stop(server->transport);
	rpc_unix_socket_close(server->sock);
	server->sock = -1;
}


void         rpc_server_destroy(rpc_server_t server)
{
	if (!server)
		return;

	rpc_server_close(server);
	rpc_transport_destory(server->transport);
	free(server);
}
