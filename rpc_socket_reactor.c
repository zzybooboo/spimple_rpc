#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/epoll.h> 
#include <netinet/in.h>
#include <signal.h>
#include "rpc_socket_reactor.h"
#include "rpc_socket_reactor_event_list.h"


#define EPOLL_MAX_EVENT_SIZE 1024
#define RPC_READ_BUFFER_SIZE 8*1024

typedef struct rpc_socket_reactor_{
	bool                    run;
	int                     epfd;
	void *					param; 
	rpc_reactor_event_list  events;
	pthread_t  		*	    workder;
	rpc_reactor_handler_t	handlers;
};


rpc_socket_reactor_t   rpc_reactor_new(void * param, rpc_reactor_handler_t * handlers)
{
	rpc_socket_reactor_t reactor = (rpc_socket_reactor_t)calloc(1, sizeof(struct rpc_socket_reactor_));
	if (!reactor)
		return NULL;

	reactor->events = rpc_reactor_event_list_new();
	if (!reactor->events)
	{
		free(reactor);
		return NULL;
	}

	reactor->run = false;
	reactor->param = param;
	reactor->epfd = epoll_create(EPOLL_MAX_EVENT_SIZE);
	memcpy(&reactor->handlers, handlers, sizeof(rpc_reactor_handler_t));
	
	return reactor;
}

static inline int _rpc_reactor_accept(rpc_socket_reactor_t reactor, int listenfd)
{
	int accept_fd = 0;
	size_t client_size = 0;
	struct sockaddr_in clientaddr;
accept:
	while (reactor->run)
	{
		accept_fd = accept(listenfd, (struct sockaddr *)&clientaddr, &client_size);
		if (accept_fd < 0)
			break;

		if (!reactor->handlers.accept_func(reactor->param, accept_fd, &clientaddr))
		{
			close(accept_fd);
			continue;
		}

		struct epoll_event ev;
		ev.data.fd = accept_fd;
 
		ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
		epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, accept_fd, &ev);
	}

	if (accept_fd == -1)
	{
		// serious error return
		if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
			return -1;

		if (errno == EAGAIN)
			return 0;

		goto accept;
	}
}

static inline void _rpc_reactor_event_reset(rpc_socket_reactor_t reactor, int fd)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);

}

static inline void  _rpc_reactor_dispatch_read_event(rpc_socket_reactor_t reactor, int fd)
{
	if (!reactor->handlers.recv_func(reactor->param, fd))
		return;

	_rpc_reactor_event_reset(reactor, fd);
}

static inline  void * _rpc_reactor_thread_proc(void * args)
{
	signal(SIGFPE, SIG_IGN);
	rpc_socket_reactor_t reactor = (rpc_socket_reactor_t)args;
	while (reactor->run)
	{
		
		int fd = rpc_reactor_event_pop(reactor->events);
		if (-1 == fd)
			continue;
		
		_rpc_reactor_dispatch_read_event(reactor, fd);
	}
}

bool _rpc_reactor_worker_start(rpc_socket_reactor_t reactor)
{
	reactor->workder = (pthread_t *)calloc(5, sizeof(pthread_t));
	if (!reactor->workder)
		return false;

	int i = 0;
	for (; i < 5; i++)
	{
		pthread_create(&reactor->workder[i], NULL, _rpc_reactor_thread_proc, reactor);
	}
	return true;
}

void _rpc_reactor_worker_stop(rpc_socket_reactor_t reactor)
{
	if (!reactor->workder)
		return;
	int i = 0;
	for (; i < 5; i++)
	{
		void * status = NULL;
		pthread_join(reactor->workder[i], &status);
	}
}

void rpc_reactor_canncel_regsiter(rpc_socket_reactor_t reactor, int fd)
{
	if (!reactor)
		return;

	epoll_ctl(reactor->epfd, EPOLL_CTL_DEL, fd, NULL);
}

int  rpc_reactor_run(rpc_socket_reactor_t reactor, int listenfd)
{
	if (!reactor)
		return -1;

	if (reactor->run)
		return 0;

	reactor->run = true;
	if (!_rpc_reactor_worker_start(reactor))
	{
		reactor->run = false;
		return -1;
	}

	struct epoll_event listen_ev, events[EPOLL_MAX_EVENT_SIZE];
	listen_ev.data.fd = listenfd;
	listen_ev.events = EPOLLIN | EPOLLET;

	epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, listenfd, &listen_ev);
	while (reactor->run)
	{
		size_t index = 0;
		int count = epoll_wait(reactor->epfd, events, EPOLL_MAX_EVENT_SIZE, 1000);
		for (; index < count; index++)
		{
			if (events[index].data.fd == listenfd)
			{
				_rpc_reactor_accept(reactor, listenfd);
			}
			else
			{
				if (events[index].events&EPOLLIN)
				{
					rpc_reactor_event_push(reactor->events, events[index].data.fd);
				}
			}
		}
	}
	rpc_reactor_canncel_regsiter(reactor, listenfd);
	return 0;
}

void  rpc_reactor_stop(rpc_socket_reactor_t reactor)
{
	if (!reactor || !reactor->run)
		return;

	reactor->run = false;
	rpc_reactor_event_signal(reactor->events);
	_rpc_reactor_worker_stop(reactor);
}

void  rpc_reactor_destory(rpc_socket_reactor_t reactor)
{
	if (!reactor)
		return;

	rpc_reactor_stop(reactor);
	rpc_reactor_event_list_destory(reactor->events);
	reactor->events = NULL;
	close(reactor->epfd);
	free(reactor);
}
