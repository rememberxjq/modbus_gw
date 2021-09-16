#include "linklist.h"
#include <stdlib.h>


int linklist_insert_first(linkList *plist, void *pdata);
int linklist_append_last(linkList *plist, void *pdata);
int linklist_delete_first(linkList *plist);
int linklist_delete_last(linkList *plist);

static inline void free_linknode(linknode *pnode)
{
    if (NULL != pnode)
    {
        if (NULL != pnode->_data)
        {
            free(pnode->_data);
            pnode->_data = NULL;
        }
        free(pnode);
    }
}

static inline void print_linknodedata(linknode *pnode)
{
    linknodeData *data = (linknodeData *)(((linknode *)pnode)->_data);
    printf("dataid = %s\n", data->map_forward.dataid);
}

static linknode *create_linknode(void *data)
{
    linknode *pnode = (linknode *)malloc(sizeof(linknode));
    if (NULL == pnode)
    {
        return NULL;
    }
    pnode->_data = data;
    pnode->_next = NULL;
    
    return pnode;
}

linkList *create_linklist(void)
{
    linkList *plist = (linkList *)malloc(sizeof(linkList));
    if (NULL == plist)
    {
        return NULL;
    }
    plist->_phead = NULL;
    plist->_count = 0;
    
    return plist;
}

int destory_linklist(linkList *plist)
{
    if (NULL == plist)
    {
        printf("%s failed! dlink is null!\n", __func__);
        return -1;
    }
    
    linknode *pnode = plist->_phead;
    linknode *ptmp = NULL;
    while (pnode != NULL)
    {
        ptmp = pnode;
        pnode = pnode->_next;
        free_linknode(ptmp);
    }
    
    plist->_phead = NULL;
    plist->_count = 0;
    
    return 0;
}

int linklist_is_empty(linkList *plist)
{
    if (NULL == plist || plist->_count <= 0)
    {
        return 1;
    }
    
    return 0;
}

int linklist_size(linkList *plist)
{
    if (NULL == plist)
    {
        return 0;
    }
    
    return plist->_count;
}

static linknode *get_linknode(linkList *plist, int index)
{
    if (index < 0 || index >= plist->_count)
    {
        printf("%s failed! index out of bound!\n", __func__);
        return NULL;
    }
    
    int i = 0;
    linknode *pnode = plist->_phead;
    while ((i++) < index)
    {
        pnode = pnode->_next;
    }
    
    return pnode;
}

void *linklist_get(linkList *plist, int index)
{
    linknode *pnode = get_linknode(plist, index);
    if (NULL == pnode)
    {
        printf("%s failed!\n", __func__);
        return NULL;
    }
    
    return pnode->_data;
}

void *linklist_get_first(linkList *plist)
{
    if (NULL == plist || plist->_count <= 0)
    {
        return NULL;
    }
    
    return plist->_phead->_data;
}

void *linklist_get_last(linkList *plist)
{
    if (NULL == plist || plist->_count <= 0)
    {
        return NULL;
    }
    
    return get_linknode(plist, plist->_count - 1)->_data;
}

int linklist_insert(linkList *plist, int index, void *pdata)
{
    if (0 == index)
    {
        return linklist_insert_first(plist, pdata);
    }
    else if (index == plist->_count)
    {
        return linklist_append_last(plist, pdata);
    }
    else
    {
        if (NULL == plist)
        {
            return -1;
        }

        linknode *pindex = get_linknode(plist, index);
        if (NULL == pindex)
        {
            return -1;
        }
        linknode *preindex = get_linknode(plist, index - 1);
        if (NULL == preindex)
        {
            return -1;
        }
        
        linknode *pnode = create_linknode(pdata);
        if (NULL == pnode)
        {
            return -1;
        }
        pnode->_next = pindex;
        preindex->_next = pnode;
        
        plist->_count++;
        
        return 0;
    }
}

int linklist_insert_first(linkList *plist, void *pdata)
{
    if (NULL == plist)
    {
        return -1;
    }
    
    linknode *pnode = create_linknode(pdata);
    if (NULL == pnode)
    {
        return -1;
    }
    
    if (NULL == plist->_phead)
    {
        plist->_phead = pnode;
        plist->_count++;
    }
    else
    {
        pnode->_next = plist->_phead;
        plist->_phead = pnode;
        
        plist->_count++;
    }
    
    return 0;
}

int linklist_append_last(linkList *plist, void *pdata)
{
    if (NULL == plist)
    {
        return -1;
    }
    
    linknode *pnode = create_linknode(pdata);
    if (NULL == pnode)
    {
        return -1;
    }
    
    if (NULL == plist->_phead)
    {
        plist->_phead = pnode;
        plist->_count++;
    }
    else
    {
        linknode *plastnode = get_linknode(plist, plist->_count - 1);
        plastnode->_next = pnode;
        plist->_count++;
    }
    
    return 0;
}

int linklist_delete(linkList *plist, int index)
{
    if (NULL == plist || NULL == plist->_phead || plist->_count <= 0)
    {
        return -1;
    }
    
    if (index == 0)
    {
        return linklist_delete_first(plist);
    }
    else if (index == (plist->_count-1))
    {
        return linklist_delete_last(plist);
    }
    else
    {
        linknode *pindex = get_linknode(plist, index);
        if (NULL == pindex)
        {
            return -1;
        }
        linknode *preindex = get_linknode(plist, index - 1);
        if (NULL == preindex)
        {
            return -1;
        }
        
        preindex->_next = pindex->_next;
        
        free_linknode(pindex);
        plist->_count--;
    }
    
    return 0;
}

int linklist_delete_first(linkList *plist)
{
    if (NULL == plist || NULL == plist->_phead || plist->_count <= 0)
    {
        return -1;
    }
    
    linknode *phead = plist->_phead;
    plist->_phead = plist->_phead->_next;
    
    free_linknode(phead);
    plist->_count--;
    
    return 0;
}

int linklist_delete_last(linkList *plist)
{
    if (NULL == plist || NULL == plist->_phead || plist->_count <= 0)
    {
        return -1;
    }

    linknode *plastsecondnode = get_linknode(plist, plist->_count - 2);
    if (NULL != plastsecondnode)
    {
        linknode *plastnode = plastsecondnode->_next;
        plastsecondnode->_next = NULL;
        
        free_linknode(plastnode);
        plist->_count--;
    }
    else
    {
        linknode *plastnode = get_linknode(plist, plist->_count - 1);
        plist->_phead = NULL;
        plist->_count = 0;
        
        free_linknode(plastnode);
    }
    
    return 0;
}

void print_linklist(linkList *plist)
{
    if (NULL == plist || NULL == plist->_phead || plist->_count <= 0)
    {
        printf("%s list is empty !!!\n", __func__);
        return;
    }
    
    printf("{list : \n");
    
    linknode *pnode = plist->_phead;
    while (pnode != NULL)
    {
        print_linknodedata(pnode);
        pnode = pnode->_next;
    }
    
    printf("}\n");
}
