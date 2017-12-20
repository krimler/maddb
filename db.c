#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "m.h"
#include "cool_app.h"
#include "getb.h"
#include "alloc.h"
#include "om_mutex.h"
#define LAYOUT_START 1024
#define INIT_PATTERN 0xABCDABCD
#define MFENCE asm volatile ("mfence" ::: "memory")

static void plock_init(BusMem *bus)
{
    if(M_MASTER != bus->role)
    {
        bus->plock = (void *)bus->plock_space;
        /* as per pthread manual http://www.cs.ucsb.edu/~tyang/class/pthreads/pthread_mutex.txt
             only one process should init the mutex. */
        return;
    }

    bus->plock = Do_Lock_Init(bus->plock_space, MIN_LOCK_SIZE);
}

static void init_layout(Layout **layout, MRole role)
{
    if(M_MASTER != role)
    {
        sm_further_init(*layout, role);
        return;
    }

    struct Layout *lt = calloc(1, sizeof(Layout));

    pr("after calloc----------------------->\n");

    pr_layout(lt);

    sm_further_init(lt, role);

    pr("after sm_further_init ----------------------->\n");

    pr_layout(lt);

    MFENCE;

    memcpy(*layout, lt, sizeof(Layout));

    pr("after memcpy ----------------------->\n");

    pr_layout(*layout);

    free(lt);

    lt = NULL;
}
BusMem *sm_init(MRole role, int key, unsigned long long size)
{
    const int MASTER_CHUNK_LEN = (size * 90)/100;
    const int SLAVE_CHUNK_LEN = (size * 5)/100;
    BusMem *bs = calloc(1, sizeof(BusMem));
    assert(NULL != bs);
    bs->role = role;
    bs->lock = 0;
    Ms *ms = calloc(1, sizeof(Ms));
    assert(NULL != ms);
    ms->key = key;
    ms->sz = size;
    ms->pattern = 0x0;
    assert(0 == minit(ms));
    bs->ms = ms;
    Layout *layout = (Layout *)(ms->shm + LAYOUT_START);
    int lz = (int)sizeof(Layout);
    // printf ("The size of layout is %d\n", lz);
    assert (lz + 127 <= ms->sz);
    bs->layout = layout;
    init_layout(&layout, role);
    reset_locks(bs, role);
    plock_init(bs);
    bs->chunk  =  ms->shm + ( LAYOUT_START + (int)sizeof(Layout) + 1024);
    bs->chunk_len = ms->sz - ( LAYOUT_START + (int)sizeof(Layout) + 1024);
    bs->master_chunk =  bs->chunk;
    bs->master_chunk_len = MASTER_CHUNK_LEN;
    bs->slave_chunk  = bs->chunk + (MASTER_CHUNK_LEN + 1024);
    bs->slave_chunk_len = SLAVE_CHUNK_LEN;
    bs->our_mspace = 0;
    MFENCE;

    if(role == M_MASTER)
    {
        setMKGParams(&(bs->our_mspace), bs->master_chunk, bs->master_chunk_len);
    }

    else
    {
        setMKGParams(&(bs->our_mspace), bs->slave_chunk, bs->slave_chunk_len);
    }

    return bs;
}

void *db_alloc(DBMem  *db, size_t bytes)
{
    return getb(&(db->our_mspace), bytes);
}
void db_free(DBMem *db, void *mem)
{
    ungetb(db->our_mspace, mem);
}

char *db_strdup(DBMem *db, const char *s)
{
    int bytes = strlen(s);
    char *ptr = getb(&(db->our_mspace), bytes+1);
    assert(NULL != s);
    strcpy(ptr, s);
    return (ptr);
}

int db_set_atomic_op_bound(DBMem *db, int bound){
db->max_bound  = bound;
}
