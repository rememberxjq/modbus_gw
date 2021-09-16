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
#include "parse_db_config.h"


#define LOG     0

int get_set_value_to_db(char *sql,char *row, char *colum, char *val);
void get_map_table(char *data, linkList *list);

int get_dev_config(char *data, struct config *config)
{
    cJSON *json = NULL;
    char *json_data = NULL;
    cJSON *node = NULL;
    cJSON *band = NULL;
    cJSON *dataBits = NULL;
    cJSON *parity = NULL;
    cJSON *stopBits = NULL;
    cJSON *port = NULL;
    cJSON *ip = NULL;

    if(data == NULL)
    {
        return -1;
    }
    json = cJSON_Parse(data);

// #if LOG
    json_data = cJSON_Print(json);
    printf("data: %s\n", json_data);
    free(json_data);
// #endif

    node = cJSON_GetObjectItem(json,"node");
    if(node != NULL)
    {
        strcpy(config->node, node->valuestring);
    }

    band = cJSON_GetObjectItem(json,"band");
    if(band != NULL)
    {
        config->band = band->valueint;
    }

    dataBits = cJSON_GetObjectItem(json,"dataBits");
    if(node != NULL)
    {
        config->dataBits = dataBits->valueint;
    }

    parity = cJSON_GetObjectItem(json,"parity");
    if(node != NULL)
    {
        strcpy(config->parity, parity->valuestring);
    }

    stopBits = cJSON_GetObjectItem(json,"stopBits");
    if(node != NULL)
    {
        config->stopBits = stopBits->valueint;
    }

    ip = cJSON_GetObjectItem(json,"ip");
    if(ip != NULL)
    {
        strcpy(config->ip, ip->valuestring);
    }

    port = cJSON_GetObjectItem(json,"port");
    if(node != NULL)
    {
        config->port = port->valueint;
    }

    cJSON_Delete(json);

    printf("config:\n node=%s\n band=%d\n dataBits=%d\n parity=%s\n stopBits=%d\n ip=%s\n port=%d\n", 
                    config->node, config->band, config->dataBits, 
                    config->parity, config->stopBits, config->ip, config->port);

    return 0;
}


void get_map_table(char *data, linkList *list)
{
    cJSON *json = NULL;
    char *json_data = NULL;
    cJSON *slaveAddr = NULL;
    cJSON *tmpdata = NULL;
    cJSON *tmpfunc = NULL;
    cJSON *deviceID = NULL;
    cJSON *dataID = NULL;
    cJSON *offset = NULL;
    cJSON *regAddr = NULL;
    char mdfun[][32] = {"func01", "func02", "func03", "func04"};
    cJSON *map = NULL;
    int i = 0;
    int addr = -1;
    linknodeData linknodeDataTmp;
    linknodeData *link_node_data= NULL;

    memset(&linknodeDataTmp, 0, sizeof(linknodeDataTmp));
    if(data == NULL)
    {
        return ;
    }
    json = cJSON_Parse(data);

#if LOG
    json_data = cJSON_Print(json);
    printf("data: %s\n", json_data);
    free(json_data);
#endif
    cJSON_ArrayForEach(map, json)
    {
        addr = -1;
        slaveAddr = cJSON_GetObjectItem(map,"slaveAddr");
        if(slaveAddr != NULL)
        {
            addr = slaveAddr->valueint;
            printf("slaveAddr: %d\n", slaveAddr->valueint);
        }

        for( i = 0; i < 4; i++)
        {
            tmpfunc = cJSON_GetObjectItemCaseSensitive(map, mdfun[i]);
            if(tmpfunc != NULL)
            {
#if LOG
                printf("-----------in function %s:-----------\n",mdfun[i]);
#endif
                cJSON_ArrayForEach(tmpdata, tmpfunc)
                {
                    if(addr != -1)
                    {
                        memset(&linknodeDataTmp, 0, sizeof(linknodeDataTmp));
                        linknodeDataTmp.map_forward.slave_addr = slaveAddr->valueint;
                    }
                    else
                    {
                        continue;
                    }

                    deviceID = cJSON_GetObjectItemCaseSensitive(tmpdata, "deviceID");
                    if(deviceID != NULL)
                    {
                        strcpy(linknodeDataTmp.map_forward.deviceid, deviceID->valuestring);
#if LOG
                        printf("mdfun=%d deviceID: %s\n", i, deviceID->valuestring);
#endif
                    }

                    dataID = cJSON_GetObjectItemCaseSensitive(tmpdata, "dataID");
                    if(dataID != NULL)
                    {
                        strcpy(linknodeDataTmp.map_forward.dataid, dataID->valuestring);
#if LOG
                        printf("mdfun=%d dataID: %s\n", i, dataID->valuestring);
#endif
                    }

                    offset = cJSON_GetObjectItemCaseSensitive(tmpdata, "offset");
                    if(offset != NULL)
                    {
                        linknodeDataTmp.map_forward.offset = offset->valueint;
#if LOG
                        printf("mdfun=%d offset: %d\n", i, offset->valueint);
#endif
                    }

                    regAddr = cJSON_GetObjectItemCaseSensitive(tmpdata, "regAddr");
                    if(regAddr != NULL)
                    {
                        strcpy(linknodeDataTmp.map_forward.regaddr, regAddr->valuestring);
#if LOG
                        printf("mdfun=%d regAddr: %s\n", i, regAddr->valuestring);
#endif
                    }
#if LOG
                    printf("\n");
#endif
                    link_node_data= NULL;
                    link_node_data = (linknodeData *)malloc(sizeof(linknodeData));
                    memcpy(link_node_data, &linknodeDataTmp, sizeof(linknodeDataTmp));
                    linklist_append_last(list, link_node_data);
                }
            }
        }
    }
    cJSON_Delete(json);
}


int get_set_value_to_db(char *sql,char *row, char *colum, char *val)
{
    sqlite3 *db=NULL;
    int len;
    char *zErrMsg =NULL;
    char **azResult=NULL;
    int nrow=0;
    int ncolumn = 0;
    int i =0;
    int j = 0;

    if(sql == NULL)
    {
        return ERR;
    }

    len = sqlite3_open("tb_forward",&db);
    
    if(len)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );

    if(val == NULL || colum == NULL)
    {
        sqlite3_close(db);
        return 0;
    }

#if LOG
    printf("nrow=%d ncolumn=%d\n",nrow,ncolumn);
    printf("the result is:\n");
#endif

    for (i = 1; i <= nrow; i++)
    {
        for(j = 0; j < ncolumn; j++)
        {
#if LOG
            printf("row:%d column:%d value:%s\n",i,j, azResult[i*ncolumn+j]);
#endif
            if(j == *colum)
            {
                strcpy(val, azResult[i*ncolumn+j]);
            }
        }
#if LOG
        printf("\n");
#endif
    }

    sqlite3_close(db);
    return 0;
}




