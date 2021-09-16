#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>
#include "linklist.h"
#include "main.h"
#include "sqlite3.h"
#include "radis_data_opt.h"

#define LOG 0

int get_set_redis_data(enum redis_opt REDIS_OPT, char *key, char *value);

int get_set_redis_data(enum redis_opt REDIS_OPT, char *key, char *value)
{
    redisContext* conn = NULL;
    redisReply* reply = NULL;
    char form_set[32] = "SET %b %b";
    char form_get[32] = "GET %b";

    if(key == NULL)
    {
        printf("key value is NULL\n");
        return -1;
    }

    conn = redisConnect("127.0.0.1", 6379);
    if(conn->err)
    {
        printf("connection error:%s\n", conn->errstr);
    }

    if(REDIS_OPT)
    {
        reply = redisCommand(conn, form_set, key, strlen(key), value, strlen(value));
    }
    else
    {
        reply = redisCommand(conn, form_get, key, strlen(key));
    }

    if(reply == NULL || reply->str == NULL)
    {
        redisFree(conn);
        return -1;
    }

    if((strcmp(reply->str, "OK") == 0) &&(REDIS_OPT == SET))
    {
#if LOG
        printf("set value success\n");
#endif
    }
    else if((reply->str != NULL) && (reply != NULL))
    {
#if LOG
        printf("%s=%s\n", key, reply->str);
#endif
        strcpy(value, reply->str);
    }
    else
    {
        printf("not found value for %s\n", key);
        if(reply != NULL)
        {
            freeReplyObject(reply); 
        }
        redisFree(conn);
        return -1;
    }

    if(reply != NULL)
    {
        freeReplyObject(reply); 
    }
    redisFree(conn);
    return 0;
}

int parse_radis_data_to_struct(char *data_str, md_data_t* data_struct)
{
    cJSON *json = NULL;
    char *json_data = NULL;

    cJSON *name = NULL;
    cJSON *desc = NULL;
    cJSON *value = NULL;
    cJSON *type = NULL;
    cJSON *bytes = NULL;
    cJSON *rw = NULL;
    cJSON *dTime = NULL;
 
    if(data_str == NULL)
    {
        return -1;
    }
    json = cJSON_Parse(data_str);
#if LOG
    json_data = cJSON_Print(json);
    printf("data: %s\n", json_data);
    free(json_data);
#endif

    name = cJSON_GetObjectItem(json,"name");
    if(name != NULL)
    {
        strcpy(data_struct->name, name->valuestring);
#if LOG
        printf("name=%s\n", data_struct->name);
#endif
    }

    desc = cJSON_GetObjectItem(json,"desc");
    if(desc != NULL)
    {
        strcpy(data_struct->desc, desc->valuestring);
#if LOG
        printf("desc=%s\n", data_struct->desc);
#endif
    }

    value = cJSON_GetObjectItem(json,"value");
    if(value != NULL)
    {
        strcpy(data_struct->value, value->valuestring);
#if LOG
        printf("value=%s\n", data_struct->value);
#endif
    }

    type = cJSON_GetObjectItem(json,"type");
    if(type != NULL)
    {
        strcpy(data_struct->type, type->valuestring);
#if LOG
        printf("type=%s\n", data_struct->type);
#endif
    }

    bytes = cJSON_GetObjectItem(json,"bytes");
    if(bytes != NULL)
    {
        strcpy(data_struct->bytes, bytes->valuestring);
#if LOG
        printf("bytes=%s\n", data_struct->bytes);
#endif
    }

    rw = cJSON_GetObjectItem(json,"rw");
    if(rw != NULL)
    {
        strcpy(data_struct->rw, rw->valuestring);
#if LOG
        printf("rw=%s\n", data_struct->rw);
#endif
    }

    dTime = cJSON_GetObjectItem(json,"dTime");
    if(dTime != NULL)
    {
        strcpy(data_struct->dTime, dTime->valuestring);
#if LOG
        printf("dTime=%s\n", data_struct->dTime);
#endif
    }

    cJSON_Delete(json);
    return 0;
}
