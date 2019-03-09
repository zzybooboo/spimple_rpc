#include "rpc_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static inline bool _rpc_socket_setnonblocking(rpc_socket_t sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
		return false;

	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
		return false;

	return true;
}

static inline bool _rpc_socket_setreuseaddr(rpc_socket_t sock)
{
	int opt =1;
	int error = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	return error != 0? false : true;
}

static inline bool _rpc_socket_nobroken_pipe(int sock)
{
 
	/*int  set = 1;
	int error = setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int));
	printf("%d\n", error);
	return error != 0 ? false : true;*/
	return true;
}

bool rpc_socket_setopt(int sock)
{
	return _rpc_socket_setnonblocking(sock) && _rpc_socket_setreuseaddr(sock) && _rpc_socket_nobroken_pipe(sock);
}

void    rpc_unix_socket_close(rpc_socket_t sock)
{
	if (-1 != sock)
		close(sock);
}


int     rpc_unix_socket_listen(rpc_socket_t sock)
{
	if (sock == -1)
		return RPC_BAD_SOCKET;

	if (listen(sock, 128) != 0)
		return RPC_SOCKET_OPEN;

	return RPC_OK;
}

int	 rpc_unix_socket_create(rpc_socket_t * sock)
{
	rpc_socket_t fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (-1 == fd)
		return RPC_BAD_SOCKET;

	if (!rpc_socket_setopt(fd))
	{
		close(fd);
		return RPC_BAD_SOCKET;
	}
	*sock = fd;
	return  RPC_OK;
}

int  rpc_unix_socket_bind(rpc_socket_t sock, const char * socket_path)
{
	if (!socket_path)
		return RPC_BAD_SOCKET;

	unlink(socket_path);
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_path);

	int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0)
		return RPC_SOCKET_BIND;

	return RPC_OK;
}


int  rpc_unix_socket_connect(rpc_socket_t sock, const char * addr, int timeout)
{
	if ( -1 == sock)
		return RPC_BAD_SOCKET;

	struct sockaddr_un adr;
	adr.sun_family = AF_UNIX;
	strcpy(adr.sun_path, addr);

	int ret = connect(sock, (struct sockaddr *)&adr, sizeof(adr));
	if (ret < 0)
	{
		if (errno != 115)
			return RPC_SOCKET_OPEN;

		struct timeval tv;
		tv.tv_usec = 0;
		tv.tv_sec = timeout;

		fd_set r, w;
		FD_ZERO(&r);
		FD_ZERO(&w);

		FD_SET(sock, &r);
		FD_SET(sock, &w);
	 
		if (select(sock + 1, &r, &w, NULL, &tv) < 0)
			return RPC_BROKEN_PIPE;

		if (FD_ISSET(sock, &r) && FD_ISSET(sock, &w))
			return RPC_SOCKET_OPEN;

		if (!FD_ISSET(sock, &r) && !FD_ISSET(sock, &w))
			return RPC_SOCKET_OPEN;
	}
	return RPC_OK;
}