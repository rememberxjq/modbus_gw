#ifndef __MAIN_H__
#define __MAIN_H__

#define  MAX_REDIS_DATA_LEN 1024
#define  MAX_REDIS_KEY_LEN 256



enum STATUS
{
    ERR=-1,
    SUCCESS,
};

typedef struct config
{
    int     id;
    char    name[64];
    char    type[12];
    char    node[64];
    int band;
    int dataBits;
    char    parity[12];
    int stopBits;
    int port;
    char    ip[32];
}config_t;

typedef struct data
{
    char    idp[128];
    char    name[128];
    char    desc[256];
    char    value[128];
    char    type[128];
    char    byte[32];
    char    rw[32];
    char    dTime[128];
}data_t;

enum redis_opt
{
    GET=0,
    SET
};

#endif