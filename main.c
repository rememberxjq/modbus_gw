#include <stdio.h>  
#include <stdlib.h>
#include "sqlite3.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "main.h"
#include <modbus/modbus.h>
#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>
#include <time.h>
#include "linklist.h"
#include "radis_data_opt.h"
#include "parse_db_config.h"

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<assert.h>

#include <termios.h>
#include <limits.h> 
#include <asm/ioctls.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/epoll.h> 
#include <string.h>

/******************************** Global definition ************************/
linkList *list;
config_t config;
modbus_mapping_t *mb_mapping = NULL;
int md_map_ready = 0;
char md_type[32] = {0};
/******************************** Global definition ************************/

int reflash_node_of_table_data(linknode *pnode)
{
    time_t t;
    char key[MAX_REDIS_KEY_LEN] = {0};
    char value[MAX_REDIS_DATA_LEN] = {0};
    md_data_t tmp_data;
    unsigned int test_time = 0;
#if 0
    test_time=943891200;
#else
    time(&t);
    test_time = t;
#endif

    linknodeData *data = (linknodeData *)(((linknode *)pnode)->_data);

    sprintf(key, "%d-%04d-%s", test_time, atoi(data->map_forward.deviceid), data->map_forward.dataid);

    get_set_redis_data(GET, key, value);

    if(strlen(value) == 0)
    {
        return -1;
    }
    memset(&tmp_data, 0, sizeof(tmp_data));
    parse_radis_data_to_struct(value, &tmp_data);

    memcpy(&(data->map_forward.data), &tmp_data ,sizeof(md_data_t));

    /************************ set data to register *************************************/
    if((atoi(data->map_forward.regaddr)/10000) == 4)
    {
        mb_mapping->tab_registers[data->map_forward.offset] = atoi(data->map_forward.data.value);
        // printf("test register=%d offset=%d value=%d\n",atoi(data->map_forward.regaddr)/10000, data->map_forward.offset, atoi(data->map_forward.data.value));
    }
}

/* forword table Data refresh thread */
void *update_regedist_data_from_redis(void *arg)
{
    linknode *pnode = NULL;

    /* init modbus mapping */
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS,
                                MODBUS_MAX_WRITE_BITS,
                                MODBUS_MAX_READ_REGISTERS, 
                                MODBUS_MAX_WRITE_REGISTERS);

    while(1)
    {
        if (NULL == list || NULL == list->_phead || list->_count <= 0)
        {
            printf("%s list is empty !!!\n", __func__);
        }
        else
        {
            pnode = list->_phead;
            while (pnode != NULL)
            {
                reflash_node_of_table_data(pnode);
                pnode = pnode->_next;
            }
            pnode = NULL;
            md_map_ready = 1;
        }
        sleep(1);

    }
}

int SerialInit(char *device)
{
	struct termios stNew;
	struct termios stOld;
	int nFd = 0;

	nFd = open(device, O_RDWR|O_NOCTTY|O_NDELAY);

	if(-1 == nFd)
	{
		perror("Open Serial Port Error!\n");
		return -1;
	}

	if( (fcntl(nFd, F_SETFL, 0)) < 0 )
	{
		perror("Fcntl F_SETFL Error!\n");
		return -1;
	}

	if(tcgetattr(nFd, &stOld) != 0)
	{
		perror("tcgetattr error!\n");
		return -1;
	}
	stNew = stOld;
	cfmakeraw(&stNew);
	//set speed
	cfsetispeed(&stNew, B115200);//115200
	cfsetospeed(&stNew, B115200);
	//set databits
	stNew.c_cflag |= (CLOCAL|CREAD);
	stNew.c_cflag &= ~CSIZE;
	stNew.c_cflag |= CS8;
	//set parity
	stNew.c_cflag &= ~PARENB;
	stNew.c_iflag &= ~INPCK;
	//set stopbits
	stNew.c_cflag &= ~CSTOPB;
	stNew.c_cc[VTIME]=0;    
	stNew.c_cc[VMIN]=1; 
	tcflush(nFd,TCIFLUSH);
	if( tcsetattr(nFd,TCSANOW,&stNew) != 0 )
	{
		perror("tcsetattr Error!\n");
		return -1;
	}
    return nFd;
}

void modbus_slave_rtu(void)
{
    int s = -1;
    modbus_t *ctx = NULL;
    int rc;
    int use_backend;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    char cmd[128] = {0};
    int number = 0;
    char serial_name[64] = {0};
    FILE * fp;
    char parity = 0;

    // //if node name like "COM1" or "COM2"
    // sscanf(config.node, "%d", &number);
    // sprintf(cmd,"ls /dev |grep ttyUSB |head -n %d", number);
    // fp = popen(cmd, "r");
    // if(!fp)
    // {
    //     perror("popen");
    //     exit(EXIT_FAILURE);
    // }
    // fgets(serial_name, sizeof(serial_name), fp);
    // pclose(fp);

    // ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);
    if(strcmp(config.parity,"N") == 0)
    {
        parity = 'N';
    }
    else if(strcmp(config.parity,"O") == 0)
    {
        parity = 'O';
    }
    else if(strcmp(config.parity,"E") == 0)
    {
        parity = 'E';
    }
    
    ctx = modbus_new_rtu(config.node, config.band, parity, config.dataBits, config.stopBits);
    printf("test:node=%s band=%d parity=%c databits=%d stopbit=%d\n",config.node, config.band, parity, config.dataBits, config.stopBits);
    modbus_set_slave(ctx, 1);
    modbus_connect(ctx);
                                
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return;
    }
 
   while(1)
   {
        memset(query, 0, sizeof(query));
        rc = modbus_receive(ctx, query);

        if (rc > 0) {
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc  == -1) {
            continue;
        }
    }
 
    printf("Quit the loop: %s\n", modbus_strerror(errno));
 
    modbus_mapping_free(mb_mapping);
    if (s != -1) {
        close(s);
    }
    modbus_close(ctx);
    modbus_free(ctx);
}

void modbus_slave_tcp(void)
{
    int socket = -1;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    int rc;
    float temp;
    modbus_t *ctx = NULL;

    if(strlen(config.ip) == 0 || config.port == 0)
    {
        exit(-1);
    }
    printf("salve in tcpip:ip=%s  port=%d\n",config.ip, config.port);
    ctx = modbus_new_tcp(config.ip, config.port);
    socket = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &socket);
	while (1)
	{

        rc = modbus_receive(ctx, query);
		if (rc > 1)
		{
			modbus_reply(ctx, query, rc, mb_mapping);
		}
		else
		{
            continue;
		}
	}
}

void *modbus_slave_for_gw(void *arg)
{
    while(md_map_ready == 0)
    {
        sleep(1);
    }

    if(strcmp(md_type, "serial") == 0)
    {
        modbus_slave_rtu();
    }
    else if(strcmp(md_type, "net") == 0)
    {
        modbus_slave_tcp();
    }
    else
    {
        printf("unknow modbus type\n");
        exit(-1);
    }

    // modbus_slave(NULL);
}

int main(int argc, char **argv)
{
    char value[4096*500] = {0};
    char colum = 0;
    char row = 0;
    char *select_anything="select *from tb_forward";
    pthread_t reflash_tid, modbus_tid;
    int ret;

    row = 1;
    colum = 3; //dev config
    if(get_set_value_to_db(select_anything, &row, &colum,value) == -1)
    {
        return -1;
    }

    memset(&config, 0, sizeof(config));
    get_dev_config(value, &config);

    memset(value, 0, sizeof(value));
    row = 1;
    colum = 4; // table list
    get_set_value_to_db(select_anything, &row, &colum,value);
    list = create_linklist();
    get_map_table(value, list);
    print_linklist(list);

    memset(value, 0, sizeof(value));
    row = 1;
    colum = 2; // serial or net for modbus
    get_set_value_to_db(select_anything, &row, &colum,value);
    strcpy(md_type, value);

    ret = pthread_create(&reflash_tid, NULL, (void *)update_regedist_data_from_redis, NULL);
    if(ret != 0)
    {
        printf("create updata data thread failed\n");
        exit(1);
    }
    
    ret = pthread_create(&modbus_tid, NULL, (void *)modbus_slave_for_gw, NULL);
    if(ret != 0)
    {
        printf("create modbus gw failed\n");
        exit(1);
    }

    while(1)
    {
        sleep(1);
    }

    return 0;
}