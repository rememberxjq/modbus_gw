#ifndef Linklist_h
#define Linklist_h

#include <stdio.h>

/*******************************************/
typedef struct md_data
{
    char    name[128];
    char    desc[128];
    char    value[128];
    char    type[128];
    char    bytes[128];
    char    rw[128];
    char    dTime[128];
}md_data_t;

typedef struct md_data_map
{
    int         slave_addr;
    int         mdfun[32];
    char        deviceid[32];
    char        dataid[128];
    int         offset;
    char        regaddr[32];
    md_data_t   data;
}md_data_map_t;
/*****************************************/

typedef struct _linknodeData
{
    md_data_map_t map_forward;
}linknodeData;

typedef struct _linknode
{
    struct _linknode *_next;
    void *_data;
}linknode;

typedef struct _linkList
{
    linknode *_phead;
    int _count;
}linkList;


linkList *create_linklist(void);
int destory_linklist(linkList *plist);
int linklist_is_empty(linkList *plist);
int linklist_size(linkList *plist);
void *linklist_get(linkList *plist, int index);
void *linklist_get_first(linkList *plist);
void *linklist_get_last(linkList *plist);
int linklist_insert(linkList *plist, int index, void *pdata);
int linklist_insert_first(linkList *plist, void *pdata);
int linklist_append_last(linkList *plist, void *pdata);
int linklist_delete(linkList *plist, int index);
int linklist_delete_first(linkList *plist);
int linklist_delete_last(linkList *plist);
void print_linklist(linkList *plist);
#endif /* Linklist_h */