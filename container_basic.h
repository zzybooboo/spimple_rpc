#ifndef CONTAINER_BASEIC_INCLUDE
#define CONTAINER_BASEIC_INCLUDE

#include<stdbool.h>
#include <sys/types.h>

typedef enum data_type_t
{
	type_string,
	type_int,
	type_long,
	type_int64,
	type_float,
	type_pointer
}data_type_t;


typedef union container_value_t
{
	char *		val_str;
	int			val_int;
	long		val_long;
	long long   val_int64;
	float		val_float;
	void *		val_ptr;
}container_value_t, map_value_t;

typedef struct container_data_t{
	data_type_t type;
	map_value_t val;
}container_data_t,map_data_t;


#define  ContainerInitStringData(data, value) do{ memset(&data,0,sizeof(container_data_t)); data.type=type_string;data.val.val_str = (char*) value;} while(0)
#define  ContainerInitIntData(data,value)     do{ memset(&data,0,sizeof(container_data_t)); data.type = type_int; data.val.val_int = value;} while(0)
#define  ContainerInitLongData(data,value)    do{ memset(&data,0,sizeof(container_data_t)); data.type = type_long; data.val.val_long = value;} while(0)
#define  ContainerInitInt64Data(data,value)   do{ memset(&data,0,sizeof(container_data_t)); data.type = type_int64; data.val.val_int64 =   value;} while(0)
#define  ContainerInitFloatData(data,value)   do{ memset(&data,0,sizeof(container_data_t)); data.type = type_float; data.val.val_float =  value;} while(0)
#define  ContainerInitPtrData(data,value)     do{ memset(&data,0,sizeof(container_data_t)); data.type = type_pointer; data.val.val_ptr = value;} while(0)

#endif