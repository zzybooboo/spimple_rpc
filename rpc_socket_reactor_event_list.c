#include <stdlib.h>
#include "rpc_socket_reactor_event_list.h"


rpc_reactor_event_list  rpc_reactor_event_list_new()
{
	rpc_reactor_event_list  plist = (rpc_reactor_event_list)calloc(1, sizeof(rpc_reactor_event_list_));
	if (!plist)
		return NULL;

	INIT_LIST_HEAD(&plist->root);

	pthread_mutex_init(&plist->mtx, NULL);
	pthread_cond_init(&plist->cond, NULL);

	return plist;
}

void rpc_reactor_event_signal(rpc_reactor_event_list plist)
{
	if (!plist)
		return;
	pthread_cond_broadcast(&plist->cond);
}

void _rpc_reactor_event_free_event(rpc_reactor_event_list plist, rpc_reactor_event_t ev)
{
	list_del(&ev->node);
	free(ev);
}

void _rpc_reactor_event_free_clean(rpc_reactor_event_list plist)
{
	pthread_mutex_lock(&plist->mtx);

	while (!list_empty(&plist->root))
	{
		rpc_reactor_event_t ev = list_first_entry(&plist->root, struct rpc_reactor_event_, node);

		_rpc_reactor_event_free_event/(plist,ev);
	}

	pthread_mutex_unlock(&plist->mtx);
}
void rpc_reactor_event_list_destory(rpc_reactor_event_list plist)
{
	if (!plist)
		return;

	_rpc_reactor_event_free_clean(plist);

	rpc_reactor_event_signal(plist);
	pthread_mutex_destroy(&plist->mtx);
	pthread_cond_destroy(&plist->cond);
	free(plist);
}

bool  rpc_reactor_event_push(rpc_reactor_event_list plist, int fd)
{
	if (!plist)
		return false;

	rpc_reactor_event_t  evt = (rpc_reactor_event_t)calloc(1, sizeof(struct rpc_reactor_event_));
	if (!evt)
		return false;

	evt->fd = fd;
	pthread_mutex_lock(&plist->mtx);

	list_add(&evt->node, &plist->root);
 
	pthread_mutex_unlock(&plist->mtx);
	pthread_cond_signal(&plist->cond);
}

int  rpc_reactor_event_pop(rpc_reactor_event_list plist)
{
	if (!plist)
		return -1; 

	int fd = -1;
	pthread_mutex_lock(&plist->mtx);
	if (list_empty(&plist->root))
	{
		pthread_cond_wait(&plist->cond, &plist->mtx);
		goto unlock;
	}

	rpc_reactor_event_t ev = list_first_entry(&plist->root, struct rpc_reactor_event_, node);
	fd = ev->fd;
	_rpc_reactor_event_free_event(plist, ev);
	 
unlock:
	pthread_mutex_unlock(&plist->mtx);
	return fd;
}