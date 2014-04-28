/*
This file is part of Snappy Driver Installer.

Snappy Driver Installer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Snappy Driver Installer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Snappy Driver Installer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"

#define _7Z_CPP

//{ Global variables
int trap_mode=0;

const char *heaps[NUM_HEAPS]= // heap_t
{
    "ID_HASH_ITEMS",      // 0
    "ID_HASH_STR",        // 1

    "ID_STATE_DEVICES",   // 2
    "ID_STATE_DRIVERS",   // 3
    "ID_STATE_TEXT",      // 4

    "ID_DRIVERPACK_inffile",      // 5
    "ID_DRIVERPACK_manufacturer", // 6
    "ID_DRIVERPACK_desc_list",    // 7
    "ID_DRIVERPACK_HWID_list",    // 8
    "ID_DRIVERPACK_text",         // 9
    "ID_DRIVERPACK",              // 10

    "ID_PARSE",                   // 11
    "ID_MATCHER",                 // 12
    "ID_MANAGER",                 // 13
};

const int heapsz[NUM_HEAPS]=
{
    1024, // auto
    1024, // auto

    1024*32*2,
    1024*128*2,
    1024*128,

    1024*4,
    1024*4,
    1024*8,
    1024*8,
    1024*32,
    1024*32*4,  // must not resize

    1024*8,
    1024*8,
    1024*8,
};

const char *hashes[NUM_HASHES]= // hash_t
{
    "ID_MEMCPYZ_DUP", // 0

    "ID_STRINGS",     // 1
    "ID_SECTIONS",    // 2
    "ID_INDEXES",     // 3
    "ID_DUP_LIST",    // 4
    "ID_INF_LIST",    // 5
    "ID_CAT_LIST",    // 6
};
//}

//{ Heap
void heap_refresh(heap_t *t)
{
    void *p;
    p=malloc(t->allocated);
    memcpy(p,t->base,t->used);
    memset(t->base,0,t->used);
    free(t->base);
    t->base=p;
    *t->membck=t->base;
}

void heap_expand(heap_t *t,int sz)
{
    if(t->used+sz>t->allocated)
    {
        //printf("Expand[%-25s] +%d, (%d/%d) -> ",heaps[t->id],sz,t->used,t->allocated);
        while(t->used+sz>t->allocated)
            t->allocated*=2;
        //printf("(%d/%d)\n",t->used+sz,t->allocated);

        heap_refresh(t);
/*        t->base=realloc(t->base,t->allocated);
        if(!t->base)
        {
            heap_refresh(t);
            return;
            //t->base=realloc_wrap(t->base,t->used,t->allocated);
        }
        *t->membck=t->base;*/
    }
}

void heap_init(heap_t *t,int id,void **mem,int sz, int itemsize)
{
    t->id=id;
    if(!sz)sz=heapsz[id];
    //log("initsize %s: %d\n",heaps[t->id],sz);
    t->base=malloc(sz);
    if(!t->base)log_err("No mem %d\n",sz);
    t->membck=mem;
    t->used=0;
    t->allocated=sz;
    t->itemsize=itemsize;
    t->items=0;
    *t->membck=t->base;

    t->dup=0;
}

void heap_free(heap_t *t)
{
    if(t->base)free(t->base);
    //log("heap[%s] wasted %d bytes of RAM\n",heaps[t->id],t->allocated-t->used);
    if(t->dup)
    {
        hash_free(t->dup);
        free(t->dup);
    }
    t->base=0;
    *t->membck=0;
}

void heap_reset(heap_t *t,int sz)
{
    if(t->dup)
    {
        hash_clear(t->dup,1);
    }
    t->used=sz;
    t->items=0;
}

int heap_alloc(heap_t *t,int sz)
{
    int r=t->used;

    heap_expand(t,sz);
    t->used+=sz;
    return r;
}

int heap_allocitem_i(heap_t *t)
{
    int r=t->used;

    heap_expand(t,t->itemsize);
    memset((char *)t->base+t->used,0,t->itemsize);
    t->used+=t->itemsize;
    t->items++;
    return r/t->itemsize;
}

void *heap_allocitem_ptr(heap_t *t)
{
    int r=t->used;

    heap_expand(t,t->itemsize);
    memset((char *)t->base+t->used,0,t->itemsize);
    t->used+=t->itemsize;
    t->items++;
    return (char *)t->base+r;
}

void heap_freelastitem(heap_t *t)
{
    t->items--;
    t->used-=t->itemsize;
}

int heap_memcpy(heap_t *t,const void *mem,int sz)
{
    int r=t->used;

    heap_expand(t,sz);
    memcpy((char *)t->base+t->used,mem,sz);
    t->used+=sz;
    return r;
}

int heap_strcpy(heap_t *t,const char *s)
{
    int r=t->used;
    int sz=strlen(s)+1;

    heap_expand(t,sz);
    memcpy((char *)t->base+t->used,s,sz);
    t->used+=sz;
    return r;
}

int heap_memcpyz(heap_t *t,const void *mem,int sz)
{
    int r=t->used;

    heap_expand(t,sz+1);
    memcpy((char *)t->base+t->used,mem,sz);
    t->used+=sz+1;
    *(char *)((int)t->base+r+sz)=0;
    return r;
}

int heap_memcpyz_dup(heap_t *t,const void *mem,int sz)
{
    int r=t->used;

    heap_expand(t,sz+1);

    if(!t->dup)
    {
        t->dup=(hashtable_t *)malloc(sizeof(hashtable_t));
        hash_init(t->dup,ID_MEMCPYZ_DUP,4096*2,0);
    }
    if(t->dup)
    {
        int y,flag;
        y=hash_find(t->dup,(char*)mem,sz,&flag);
        if(flag)return y;
    }

    char *u=(char *)t->base+t->used;
    memcpy(u,mem,sz);
    t->used+=sz+1;
    *(char *)((int)t->base+r+sz)=0;

    if(t->dup)hash_add(t->dup,u,sz,r,HASH_MODE_INTACT);
    return r;
}

int heap_strtolowerz(heap_t *t,const char *s,int sz)
{
    int r=t->used;

    heap_expand(t,sz+1);
    memcpy((char *)t->base+t->used,s,sz);
    strtolower((char *)t->base+t->used,sz);
    *(char *)((int)t->base+r+sz)=0;
    t->used+=sz+1;
    return r;
}

char *heap_save(heap_t *t,char *p)
{
    memcpy(p,&t->used,sizeof(t->used));p+=sizeof(t->used);
    memcpy(p,&t->items,sizeof(t->items));p+=sizeof(t->items);
    memcpy(p,t->base,t->used);p+=t->used;
    return p;
}

char *heap_load(heap_t *t,char *p)
{
    int sz;

    memcpy(&sz,p,sizeof(t->used));p+=sizeof(t->used);
    memcpy(&t->items,p,sizeof(t->items));p+=sizeof(t->items);
    t->used=0;
    heap_alloc(t,sz);
    memcpy(t->base,p,t->used);p+=t->used;
    return p;
}

//}

//{ Strings
void strtoupper(char *s,int len)
{
    while(len--)
    {
        *s=toupper(*s);
        s++;
    }
}

void strtolower(char *s,int len)
{
    if(len)
    while(len--)
    {
        *s=tolower(*s);
        s++;
    }
}

char *strtolower_alloc(const char *s)
{
    char *d,*p;

    p=d=(char *)malloc(strlen(s)+1);
    do
    {
        *p=tolower(*s);
        s++;p++;
    }
    while(*s);
    *p=0;
    return d;
}

char *memcpy_alloc(const char *s,int sz)
{
    char *d;

    d=(char *)malloc(sz+1);
    memcpy(d,s,sz);
    d[sz]=0;
    return d;
}
//}

//{ 7-zip
void registerall()
{
#ifdef _7Z_CPP
    registercrc();
    register7z();
    registerBCJ();
    registerBCJ2();
    registerBranch();
    registerCopy();
    registerLZMA();
    registerLZMA2();
//  registerPPMD();
    registerDelta();
    registerByteSwap();
#endif
}
//}

//{ Hash
unsigned hash_getcode(const char *s,int sz)
{
    int h=5381;
    int ch;

    while(sz--)
    {
        ch=*s++;
        h=((h<<5)+h)^ch;
    }
    return h;
}

inline char *getstr(hashtable_t *t,hashitem_t *cur)
{
    return (t->flags&HASH_FLAG_STRS_ARE_INTS)?(char *)&cur->string1:t->strs+cur->string1;
}

void hash_init(hashtable_t *t,int id,int size,int hflags)
{
    memset(t,0,sizeof(hashtable_t));
    t->id=id;
    t->size=size;
    t->flags=hflags;

    heap_init(&t->items_handle,ID_HASH_ITEMS,(void **)&t->items,t->size*sizeof(hashitem_t)*2,sizeof(hashitem_t));
    heap_alloc(&t->items_handle,t->size*sizeof(hashitem_t));
    t->items_handle.items=t->size;
    memset(t->items,0,t->size*sizeof(hashitem_t));

    heap_init(&t->strs_handle,ID_HASH_STR,(void **)&t->strs,4096,1);
    heap_alloc(&t->strs_handle,1);
}

void hash_clear(hashtable_t *t,int zero)
{
    hashitem_t *cur;
    int i=0;

    while(i<t->size)
    {
        cur=&t->items[i++];
        while(1)
        {
            if(t->flags&HASH_FLAG_KEYS_ARE_POINTERS)
            {
                //if(t->id==ID_STRINGS)printf("'%s'\n",cur->key);
                free((void *)cur->key);
            }

            if(cur->next<=0)break;
            cur=&t->items[cur->next];
        }
    }
    if(zero)memset(t->items,0,t->size*sizeof(hashitem_t));
    heap_reset(&t->items_handle,t->size*sizeof(hashitem_t));
    heap_reset(&t->strs_handle,1);

    t->search_all=t->search_miss=0;
    t->cmp_all=t->cmp_miss=0;
    t->max_deep=t->used_1=0;
}

void hash_free(hashtable_t *t)
{
    hash_clear(t,0);
    heap_free(&t->items_handle);
    heap_free(&t->strs_handle);
}

void hash_stats(hashtable_t *t)
{
    log("Hash stats\n");
    log("  Space\n");
    log("    Size: %4d/%4d\t\t[%3.0f%%]\n",t->used_1,t->size,(double)t->used_1/t->size*100);
    log("    Deep: %4d\n",t->max_deep);
    log("    Itms: %d[%d]/%d\n",t->items_handle.used,t->items_handle.items,t->items_handle.allocated);
    log("    Strs: %d[%d]/%d\n",t->strs_handle.used,t->strs_handle.items,t->strs_handle.allocated);
    log("  Perfomance\n");
    log("    Hits: %4d/%4d\t\t[%3.0f%%]\n",t->search_all-t->search_miss,t->search_all,
        (double)(t->search_all-t->search_miss)/t->search_all*100);
    log("    Cmps: %4d/%4d\t\t(%3.0f%%)\n\n",t->cmp_all-t->cmp_miss,t->cmp_all,
        (t->cmp_all-t->cmp_miss)/(double)t->cmp_all*100);
}

char *hash_save(hashtable_t *t,char *p)
{
    memcpy(p,&t->size,sizeof(int));p+=sizeof(int);
    memcpy(p,&t->items_handle.used,sizeof(int));p+=sizeof(int);
    memcpy(p,&t->items_handle.items,sizeof(int));p+=sizeof(int);
    memcpy(p,t->items,t->items_handle.used);p+=t->items_handle.used;
    return p;
}

char *hash_load(hashtable_t *t,char *p)
{
    memcpy(&t->size,p,sizeof(int));p+=sizeof(int);
    hash_init(t,ID_INDEXES,t->size,HASH_FLAG_STRS_ARE_INTS);
    p=heap_load(&t->items_handle,p);
    return p;
}

/*
next
     -1: used,next is free
      0: free,next is free
  1..x : used,next is used
*/

void hash_add(hashtable_t *t,const char *s,int s_sz,int key,int mode)
{
    hashitem_t *cur;
    char buf[BUFLEN];
    int curi;
    int previ=-1;
    int cur_deep=0;
    //if(key==0)log("*** zero added (%s)%.*s***\n",hashes[t->id],s_sz,s);
    if(t->flags&HASH_FLAG_STR_TO_LOWER)
    {
        memcpy(buf,s,s_sz);
        strtolower(buf,s_sz);
        buf[s_sz]=0;
        s=buf;
    }
    curi=hash_getcode(s,s_sz)%t->size;
    cur=&t->items[curi];

    if(cur->next!=0)
    do
    {
        cur_deep++;
        cur=&t->items[curi];
        if(s_sz==cur->strlen&&memcmp(s,getstr(t,cur),s_sz)==0)
        switch(mode)
        {
            case HASH_MODE_INTACT:
                 return;

            case HASH_MODE_REPLACE:
                cur->key=key;
                return;

            case HASH_MODE_ADD:
                break;

             default:
                log("hash_add(): invalid mode\n");
        }
        previ=curi;
    }
    while((curi=cur->next)>0);

    if(cur_deep>t->max_deep)t->max_deep=cur_deep;
    if(cur->next==0)t->used_1++;

    if(cur->next==-1)
    {
        curi=heap_allocitem_i(&t->items_handle);
        cur=&t->items[curi];
    }

    if(t->flags&HASH_FLAG_STRS_ARE_INTS)
    {
        memcpy(&cur->string1,s,sizeof(int));
    }else
    cur->string1=(t->flags&HASH_FLAG_STR_TO_LOWER)?
            heap_strtolowerz(&t->strs_handle,s,s_sz)
            :
            heap_memcpyz(&t->strs_handle,s,s_sz);

    cur->strlen=s_sz;
    cur->key=key;
    cur->next=-1;
    if(previ>=0)(&t->items[previ])->next=curi;
}

int hash_find(hashtable_t *t,const char *s,int sz,int *isfound)
{
    hashitem_t *cur;
    char buf[BUFLEN];
    char *os=s;
    int curi;

    if((t->flags&HASH_FLAG_STR_TO_LOWER))
    {
        memcpy(buf,s,sz);
        strtolower(buf,sz);
        buf[sz]=0;
        s=buf;
    }

    t->search_all++;
    if(!t->size)
    {
        t->search_miss++;
        *isfound=0;
        return 0;
    }
    curi=hash_getcode(s,sz)%t->size;
    cur=&t->items[curi];

    if(cur->next<0)
    {
        t->cmp_all++;
        if(sz==cur->strlen&&memcmp(s,getstr(t,cur),sz)==0)
        {
            t->finddata.findnext=cur->next;
            t->finddata.findstr=os;
            t->finddata.findstrlen=sz;
            *isfound=1;
            return cur->key;
        }
            else t->cmp_miss++;
    }

    if(cur->next==0)
    {
        t->search_miss++;
        *isfound=0;
        return 0;
    }

    do
    {
        cur=&t->items[curi];
        t->cmp_all++;
        if(sz==cur->strlen&&memcmp(s,getstr(t,cur),sz)==0)
        {
            t->finddata.findnext=cur->next;
            t->finddata.findstr=os;
            t->finddata.findstrlen=sz;
            *isfound=1;
            return cur->key;
        }
            else t->cmp_miss++;
    }
    while((curi=cur->next)>0);

    t->search_miss++;
    *isfound=0;
    return 0;
}

int hash_find_str(hashtable_t *t,const char *s)
{
    int b;
    return hash_find(t,s,strlen(s),&b);
}

int hash_findnext_rec(hashtable_t *t,const char *s)
{
    t->finddata.findstr=s;
    t->finddata.findstrlen=strlen(s);
    return hash_findnext(t);
}

int hash_findnext_b(hashtable_t *t,int *isfound)
{
    hashitem_t *cur;
    int curi=t->finddata.findnext,sz;

    *isfound=0;
    if(curi<=0)return 0;

    t->search_all++;
    cur=&t->items[curi];
    sz=t->finddata.findstrlen;

    do
    {
        cur=&t->items[curi];
        t->cmp_all++;
        if(sz==cur->strlen&&memcmp(t->finddata.findstr,getstr(t,cur),sz)==0)
        {
            t->finddata.findnext=cur->next;
            *isfound=1;
            return cur->key;
        }
            else t->cmp_miss++;
    }
    while((curi=cur->next)>0);

    t->search_miss++;
    return 0;
}

int hash_findnext(hashtable_t *t)
{
    int b;
    return hash_findnext_b(t,&b);
}
//}
