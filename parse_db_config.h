#ifndef __PARSE_DB_CONFIG_H__
#define __PARSE_DB_CONFIG_H__

#include "linklist.h"
#include "main.h"

int get_dev_config(char *data, struct config *config);
void get_map_table(char *data, linkList *list);
int get_set_value_to_db(char *sql,char *row, char *colum, char *val);

#endif