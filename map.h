/*
* =============================================================================
*
*       Filename:  map.h
*
*    Description:  ���ǻ���linux kernel �� rbtree ��д�� map
				   key �� value ֧�ֶ�����������
*
*        Created:  2018-12-26
*
*         Author:  zw
*        Company:  topsec
*
* =============================================================================
*/

#ifndef MAP_INCLUDE
#define MAP_INCLUDE

#include "rbtree.h"
#include <stdbool.h>

#include "container_basic.h"


typedef struct map_node_t{
	struct rb_node node;
	map_data_t  key;
	map_data_t  val;
}map_node_t;


typedef struct map {
	struct rb_root tree;
	size_t       size;
	data_type_t  key_type;
}map;


/**
* ����һ��map�� ��Ҫ����
* param [in] key_type key����������
*/
map *        map_new(data_type_t key_type);

/**
* ��ȡmap�ж��ٸ�Ԫ��
* param [in] mp mapָ��
*/
size_t       map_size(map *mp);

/**
* ��ȡmapԪ�صĵ�һ��Ԫ��
* param [in] mp mapָ��
*/
map_node_t * map_begin(map *mp);

/**
* ��ȡ��һ��Ԫ��
* param [in] cur ��ǰԪ�ص�ָ��
*/
map_node_t * map_next(map_node_t * cur);

/**
* ��ȡ��һ��Ԫ��
* param [in] cur ��ǰԪ�ص�ָ��
*/
map_node_t * map_prev(map_node_t * cur);

/**
* ��ȡmapԪ�ص����һ��Ԫ��
* param [in] mp mapָ��
*/
map_node_t * map_last(map *mp);

/**
* ��ѯ
* param [in] mp  mapָ��
* param [in] key key
*/
map_node_t * map_get(map * mp, map_data_t* key);

/**
* ����
* param [in] mp  mapָ��
* param [in] key key
* param [in] val value
*/
bool		 map_insert(map * mp, map_data_t * key, map_data_t * val);

/**
* ����
* param [in] mp  mapָ��
* param [in] key key
* param [in] val value
*/
bool		 map_set(map * mp, map_data_t * key, map_data_t * val);

/**
* ɾ��
* param [in] mp  mapָ��
* param [in] node node
*/
void		 map_del(map * mp, map_node_t * node);

/**
* ���
* param [in] mp  mapָ��
*/
void		 map_clear(map * mp);


/**
* ����
* param [in] mp  mapָ��
*/
void         map_destory(map *mp);


#endif