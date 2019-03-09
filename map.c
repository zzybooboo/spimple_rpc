#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(NDBUG)
#define ASSERT(p,format,...)  do{ if(p == false){ printf("[func:%s file:%s line:%d] " format "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__ );abort();}} while(0)
#else
#define ASSERT(p)

#endif


static int cmp_int(map_value_t * v1, map_value_t * v2)
{
	return (v1->val_int) - (v2->val_int);
}

static int cmp_string(map_value_t * v1, map_value_t * v2)
{
	return strcmp(v1->val_str, v2->val_str);
}

static int cmp_long(map_value_t * v1, map_value_t * v2)
{
	return (v1->val_long) - (v2->val_long);
}

static int cmp_int64(map_value_t * v1, map_value_t * v2)
{
	return (v1->val_int64) - (v2->val_int64);
}

static int cmp_float(map_value_t * v1, map_value_t * v2)
{
	return (v1->val_float) - (v2->val_float);
}

static int cmp_ptr(map_value_t * v1, map_value_t * v2)
{
	return ((char *)v1->val_ptr) - ((char *)v2->val_ptr);

}

typedef struct rb_node * (*rb_move_func)(const struct rb_node *);

typedef int(*cmp_func)(map_value_t *, map_value_t *);
const cmp_func cmp_func_array[] = { 
	cmp_string,
	cmp_int,
	cmp_long,
	cmp_int64,
	cmp_float,
	cmp_ptr 
};


map * map_new(data_type_t key_type)
{
	map * mp = (map*)calloc(sizeof(map), 1);
	if (!mp)
		return NULL;

	mp->size = 0;
	mp->tree = RB_ROOT;
	mp->key_type = key_type;
	return mp;
	
}


size_t map_size(map * mp)
{
	if (!mp)
		return -1;

	return mp->size;
}

map_node_t * map_get(map * mp, map_data_t* key)
{
	ASSERT(key->type == mp->key_type, "key type != mq.key_type");
	ASSERT(key->type < (sizeof(cmp_func_array) / sizeof(cmp_func_array[0])),"unkown data type");

	struct rb_node *node = mp->tree.rb_node;
	cmp_func cmp = cmp_func_array[key->type];
	while (node) {
		map_node_t *data = container_of(node, map_node_t, node);
		int result = cmp(&key->val, &data->key.val);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static inline map_node_t * map_node_new(map_data_t * key, map_data_t * val)
{
	map_node_t * node = (map_node_t *)calloc(sizeof(map_node_t), 1);
	if (!node)
		return NULL;

	if (key->type == type_string)
	{
		node->key.type = type_string;
		node->key.val.val_str = calloc(strlen(key->val.val_str) + 1, sizeof(char));
		if (!node->key.val.val_str)
		{
			free(node);
			return NULL;
		}
		strcpy(node->key.val.val_str, key->val.val_str);
	}
	else
	{
		memcpy(&node->key, key, sizeof(map_data_t));
	}

	memcpy(&node->val, val, sizeof(map_data_t));
	return node;
}

bool map_set(map * mp, map_data_t * key, map_data_t * val)
{
	ASSERT(key && val, "key or val must be not null");
	ASSERT(key->type == mp->key_type, "key type != mq.key_type");
	ASSERT(key->type < (sizeof(cmp_func_array) / sizeof(cmp_func_array[0])), "unkown data type:%d", key->type);

	map_node_t * data = map_get(mp, key);
	if (!data)
	{
		return false;
	}

	memcpy(&data->val, val, sizeof(map_data_t));
	return true;
}


bool map_insert(map * mp, map_data_t * key, map_data_t * val)
{
	ASSERT(key && val, "key or val must be not null");
	ASSERT(key->type == mp->key_type, "key type != mq.key_type");
	ASSERT(key->type < (sizeof(cmp_func_array) / sizeof(cmp_func_array[0])), "unkown data type:%d",key->type);

	struct rb_node **new_node = &(mp->tree.rb_node), *parent = NULL;
	cmp_func cmp = cmp_func_array[key->type];
	while (*new_node) {
		map_node_t *this = container_of(*new_node, map_node_t, node);
		int result = cmp(&key->val, &this->key.val);//strcmp(data->string, this->string);

		parent = *new_node;
		if (result < 0)
			new_node = &((*new_node)->rb_left);
		else if (result > 0)
			new_node = &((*new_node)->rb_right);
		else
			return 0;
	}

	map_node_t * data = map_node_new(key, val);
	if (!data){
		return 0;
	}

	rb_link_node(&data->node, parent, new_node);
	rb_insert_color(&data->node, &mp->tree);
	mp->size ++;
	return 1;
}

static inline map_node_t * map_move(map_node_t * cur, rb_move_func func)
{
	if (!cur)
		return NULL;

	struct rb_node * node = func(&cur->node);
	if (!node)
		return NULL;

	return rb_entry(node, map_node_t, node);
}

map_node_t * map_begin(map *mp)
{
	if (!mp)
		return NULL;

	return map_move(&mp->tree, rb_first);
}
map_node_t * map_last(map *mp)
{
	if (!mp)
		return NULL;

	return map_move(&mp->tree, rb_last);
}

map_node_t * map_next(map_node_t * cur)
{
	return map_move(cur,rb_next);
}

map_node_t * map_prev(map_node_t * cur)
{
	return map_move(cur, rb_prev);
}

void map_del(map * mp, map_node_t * node)
{
	rb_erase(&node->node, &mp->tree);
	if (node->key.type == type_string)
	{
		if (node->key.val.val_str)
			free(node->key.val.val_str);
		node->key.val.val_str = NULL;
	}
	free(node);
}

void map_clear(map * mp)
{
	if (!mp)
		return;

	map_node_t * data = map_begin(mp);
	while (data)
	{
		map_node_t * next = map_next(data);
		map_del(mp, next);
		data = next;
	}
	mp->size = 0;
}

void map_destory(map *mp)
{
	if (!mp)
		return;

	map_clear(mp);
	free(mp);
}