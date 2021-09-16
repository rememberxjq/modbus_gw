#ifndef __RADIS_DATA_OPT__
#define __RADIS_DATA_OPT__

#include <hiredis/hiredis.h>

int get_set_redis_data(enum redis_opt REDIS_OPT, char *key, char *set_value);
int parse_radis_data_to_struct(char *data_str, md_data_t* data_struct);

#endif