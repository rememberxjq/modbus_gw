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

#define MODBUS_DEV_NAME "/dev/ttyUSB1"

#define BAUDRATE B115200 ///Baud rate : 115200

linkList *list;

#define TEST 1

#if TEST
#define SUB_DATA(string,deviceID,dataID,offset,regAddr)             \
{                                                                   \
    sprintf(string,"{\"deviceID\": \"%d\",                          \
                    \"dataID\": \"%s\",                             \
                    \"offset\": %d,                                 \
                    \"regAddr\": \"%s\" }",                         \
                    deviceID,                                       \
                    dataID,                                         \
                    offset,                                         \
                    regAddr);                                       \
}                                                                   \

char *map_config="[{\"slaveAddr\": 1, \
		\"func01\": [], \
		\"func02\": [], \
		\"func03\": [{ \
				\"deviceID\": 1, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a1\", \
				\"offset\": 0, \
				\"regAddr\": 400001 \
			}, \
			{ \
				\"deviceID\": 1, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a2\", \
				\"offset\": 2, \
				\"regAddr\": 400003 \
			} \
		], \
		\"func04\": [{ \
				\"deviceID\": 2, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a3\", \
				\"offset\": 0, \
				\"regAddr\": 300001 \
			}, \
			{ \
				\"deviceID\": 2, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a4\", \
				\"offset\": 2, \
				\"regAddr\": 300003 \
			} \
		] \
	}, \
    {\"slaveAddr\": 2, \
		\"func01\": [], \
		\"func02\": [], \
		\"func03\": [{ \
				\"deviceID\": 2, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a5\", \
				\"offset\": 0, \
				\"regAddr\": 410001 \
			}, \
			{ \
				\"deviceID\": 2, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a6\", \
				\"offset\": 2, \
				\"regAddr\": 410003 \
			} \
		], \
		\"func04\": [{ \
				\"deviceID\": 2, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a7\", \
				\"offset\": 0, \
				\"regAddr\": 310001 \
			}, \
			{ \
				\"deviceID\": 3, \
				\"dataID\": \"806ed33e-8664-45d7-a775-8e3ed17c40a8\", \
				\"offset\": 2, \
				\"regAddr\": 310003 \
			} \
		] \
	}]";

char *dev_config = "{\"node\":\"/dev/ttyUSB0\", \
                \"band\":115200, \
                \"dataBits\":8, \
                \"parity\":\"N\", \
                \"stopBits\":1, \
                \"port\":502, \
                \"ip\":\"192.168.204.129\", \
            }";

char *create_table = " CREATE TABLE tb_forward(\
    ID INTEDER PRIMARY KEY,\
    name VARCHAR(64),\
    type VARCHAR(12),\
    params TEXT,\
    maps TEXT\
    );" ;

int SerialInit(char *device);

void make_test_table(char * test_table)
{
    int addrnum = 2;
    int funnum = 4;
    int subnum = 25;
    char tmp_buf[1024] = {0};
    char tmp_id[1024] = {0};
    char regAddr[256] = {0};
    int i  = 0, j = 0 ,k = 0;

    strcpy(test_table, "[{");
    for(i = 1; i <= addrnum; i++)
    {
        memset(tmp_buf, 0, sizeof(tmp_buf));
        sprintf(tmp_buf, "\"slaveAddr\": %d,", i);
        strcat(test_table, tmp_buf);
        for(j = 1; j <= funnum; j++)
        {
            memset(tmp_buf, 0, sizeof(tmp_buf));
            sprintf(tmp_buf, "\"func0%d\": [", j);
            strcat(test_table, tmp_buf);
            for(k = 1; k <= subnum; k++)
            {
                memset(tmp_id, 0, sizeof(tmp_id));
                sprintf(tmp_id, "806ed33e-8664-45d7-a775-8e3ed17c4a%02x", ((((i-1)*(funnum*subnum))) + ((j-1)*subnum)  + (k - 1)));
                memset(regAddr, 0, sizeof(regAddr));
                sprintf(regAddr, "%d",40000+(((i-1)*(funnum*subnum)) + ((j-1)*subnum)  + (k - 1)));
                memset(tmp_buf, 0, sizeof(tmp_buf));
                SUB_DATA(tmp_buf, i, tmp_id, ((((i-1)*(funnum*subnum))) + ((j-1)*subnum)  + (k - 1)), regAddr);
 
                // printf("offset=%d  regAddr=%s\n",((((i-1)*(funnum*subnum))) + ((j-1)*subnum)+ (k - 1)), regAddr);
                strcat(test_table, tmp_buf);
                if(k != subnum)
                {
                    strcat(test_table, ",");
                }
            }

            strcat(test_table, "]");

            if(j != funnum)
            {
                strcat(test_table, ",");
            }
        }
        if(i == 1)
        {
            strcat(test_table, "},{");
        }
    }
    strcat(test_table, "}]");


    // printf("=====================================================================\n");
    // printf("test=%s\n", test_table);
}


void test_write_data_to_redis(void)
{
    int i = 0;
    unsigned int time=943891200;
    char test_data_id_base[128] = "806ed33e-8664-45d7-a775-8e3ed17c4a";
    char key[MAX_REDIS_KEY_LEN] = {0};

    char test_data[MAX_REDIS_DATA_LEN] = {0};
    
    for(i = 0; i < 200; i++)
    {
        memset(key, 0, sizeof(key));
        memset(test_data, 0, sizeof(test_data));
        if(i < 100)
        {
            sprintf(key,"%d-%04d-%s%02x",time, 1,test_data_id_base,i);
        }
        else
        {
            sprintf(key,"%d-%04d-%s%02x",time, 2,test_data_id_base,i);
        }

        sprintf(test_data,     "{\"id\":\"522cab58-2e1f-47a3-a056-bfae437b0d%02x\","
                                "\"name\": \"testname\","
                                "\"desc\": \"testdesc\","
                                "\"value\": \"%d\","
                                "\"type\": \"float32\","
                                "\"bytes\":\"4\","
                                "\"rw\":\"r\","
                                "\"dTime\":\"2000-00-00 00:00:00\"}",
                                i,
                                i);
        printf("val=%s\n",test_data);
        if( i == 0)
        {
            printf("test key=%s\n", key);
        }

        get_set_redis_data(SET, key, test_data);
    }
}

void create_test_data()
{
    char create_data[4096*500] = {0};
    char type[12] = {0};
    char name[64] = {0};
    char test_table[4096*500]={0};
    make_test_table(test_table);

    get_set_value_to_db(create_table, NULL, NULL, NULL);
    strcpy(type, "serial");
    strcpy(name, "testdevice");

    sprintf(create_data, "INSERT INTO tb_forward VALUES(1,'%s','%s','%s','%s');",name, type, dev_config, test_table);
    get_set_value_to_db(create_data, NULL, NULL, NULL);

    /* create test redis data */
    test_write_data_to_redis();
}

#endif


int reflash_node_of_table_data(linknode *pnode)
{
    time_t t;
    char key[MAX_REDIS_KEY_LEN] = {0};
    char value[MAX_REDIS_DATA_LEN] = {0};
    md_data_t tmp_data;
    unsigned int test_time=943891200;

    time(&t);
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
}

/* forword table Data refresh thread */
void *update_regedist_data_from_redis(void *arg)
{
    linknode *pnode = NULL;
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

        }
        sleep(1);
    }
}

void modbus_slave(void)
{
    int s = -1;
    modbus_t *ctx = NULL;
    modbus_mapping_t *mb_mapping = NULL;
    int rc;
    int use_backend;
    ctx = modbus_new_rtu(MODBUS_DEV_NAME, 115200, 'N', 8, 1);
    modbus_set_slave(ctx, 1);
    modbus_connect(ctx);
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS,
                                    MODBUS_MAX_WRITE_BITS,
                                    MODBUS_MAX_READ_REGISTERS, 
                                    MODBUS_MAX_WRITE_REGISTERS);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return (void *)(-1);
    }

     uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
     int i = 0 , j=0, k=0;
 
   mb_mapping->tab_registers[0] = 1;
   mb_mapping->tab_registers[1] = 2;
   mb_mapping->tab_registers[2] = 3;
   mb_mapping->tab_registers[3] = 4;
   mb_mapping->tab_registers[4] = 5;
   mb_mapping->tab_registers[5] = 6;

   while( 1 )
   {
       memset(query, 0, sizeof(query));

        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            printf("test buff=\n");
            for(j = 0; j< sizeof(query); j++)
            {
                printf("%02hhx",query[j]);
            }
            printf("test end\n");
            printf("\n");
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc  == -1) {
            /* Connection closed by the client or error */
            i++;
            if(i > 10)
            {
                break;
            }
            printf("test buff=\n");
            for(j = 0; j< sizeof(query); j++)
            {
                printf("%02x",query[j]);
            }
            printf("\n");
            printf("err %d times err=%s\n", i, modbus_strerror(errno));
            continue;
        }
    }
 
    printf("Quit the loop: %s\n", modbus_strerror(errno));
 
    modbus_mapping_free(mb_mapping);
    if (s != -1) {
        close(s);
    }
    /* For RTU, skipped by TCP (no TCP connect) */
    modbus_close(ctx);
    modbus_free(ctx);
}

void serial_read(void *arg)
{
	int i;
	int fd = 0;
    int nRet = 0;
	struct termios strctNewTermios;

	unsigned char testbuf[MODBUS_TCP_MAX_ADU_LENGTH] = {0};

	fd = SerialInit(MODBUS_DEV_NAME);
	if(fd == -1)
	{
		perror("SerialInit Error!\n");
        return -1;
	}

	printf("success\n");

	tcflush(fd, TCIFLUSH);
	tcflush(fd, TCOFLUSH);
	tcflush(fd, TCIOFLUSH);
	tcsetattr(0, TCSANOW, &strctNewTermios);

    while(1)
    {   
        nRet = read(fd, testbuf, 64);
        if(-1 == nRet)
        {
            perror("Read Data Error!\n");
            break;
        }

        if(0 < nRet)
        {
			printf("Recv Data:\n");
			for(int i = 0; i < 64; i++)
			{
				printf("%02hhx",testbuf[i]);
				printf(" ");
			}
			printf("\n");
            printf("recv=%d\n",nRet);
            memset(testbuf, 0, sizeof(testbuf));
        }
        close(fd);
    }

    close(fd);
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
	cfsetispeed(&stNew, BAUDRATE);//115200
	cfsetospeed(&stNew, BAUDRATE);
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



int main(int argc, char **argv)
{
    char value[4096*500] = {0};
    char colum = 0;
    char row = 0;

    char *select_anything="select *from tb_forward";

    create_test_data();
    printf("================create success====================\n");

    row = 1;
    colum = 3;
    get_set_value_to_db(select_anything, &row, &colum,value);
    config_t config;
    memset(&config, 0, sizeof(config));
    get_dev_config(value, &config);


    memset(value, 0, sizeof(value));
    row = 1;
    colum = 4;
    get_set_value_to_db(select_anything, &row, &colum,value);
    list = create_linklist();
    get_map_table(value, list);
    print_linklist(list);


    test_write_data_to_redis();

    pthread_t reflash_tid, modbus_tid;
    int ret;
    ret = pthread_create(&reflash_tid, NULL, (void *)update_regedist_data_from_redis, NULL);
    if(ret != 0)
    {
        printf("create updata data thread failed\n");
        exit(1);
    }
    
    // ret = pthread_create(&modbus_tid, NULL, (void *)modbus_slave_for_gw, NULL);
    // if(ret != 0)
    // {
    //     printf("create modbus gw failed\n");
    //     exit(1);
    // }

    while(1)
    {
        sleep(1);
    }

    return 0;
}