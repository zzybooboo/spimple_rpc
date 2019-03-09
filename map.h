/*
* =============================================================================
*
*       Filename:  map.h
*
*    Description:  这是基于linux kernel 的 rbtree 编写的 map
				   key 和 value 支持多种数据类型
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
* 创建一个map， 需要销毁
* param [in] key_type key的数据类型
*/
map *        map_new(data_type_t key_type);

/**
* 获取map有多少个元素
* param [in] mp map指针
*/
size_t       map_size(map *mp);

/**
* 获取map元素的第一个元素
* param [in] mp map指针
*/
map_node_t * map_begin(map *mp);

/**
* 获取下一个元素
* param [in] cur 当前元素的指针
*/
map_node_t * map_next(map_node_t * cur);

/**
* 获取上一个元素
* param [in] cur 当前元素的指针
*/
map_node_t * map_prev(map_node_t * cur);

/**
* 获取map元素的最后一个元素
* param [in] mp map指针
*/
map_node_t * map_last(map *mp);

/**
* 查询
* param [in] mp  map指针
* param [in] key key
*/
map_node_t * map_get(map * mp, map_data_t* key);

/**
* 插入
* param [in] mp  map指针
* param [in] key key
* param [in] val value
*/
bool		 map_insert(map * mp, map_data_t * key, map_data_t * val);

/**
* 更新
* param [in] mp  map指针
* param [in] key key
* param [in] val value
*/
bool		 map_set(map * mp, map_data_t * key, map_data_t * val);

/**
* 删除
* param [in] mp  map指针
* param [in] node node
*/
void		 map_del(map * mp, map_node_t * node);

/**
* 清空
* param [in] mp  map指针
*/
void		 map_clear(map * mp);


/**
* 销毁
* param [in] mp  map指针
*/
void         map_destory(map *mp);


#endif