#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "m.h"
#include "cool_bus.h"
#include "getb.h"
#include "alloc.h"
#include "om_mutex.h"
#define LAYOUT_START 1024
#define INIT_PATTERN 0xABCDABCD
#define MFENCE asm volatile ("mfence" ::: "memory")
//https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
//http://attractivechaos.awardspace.com/kalloc.c
// http://preshing.com/20120515/memory-reordering-caught-in-the-act/
// https://www.google.co.in/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=asm+volatile+(%22mfence%22+:::+%22memory%22)

static inline void lock(volatile int *exclusion)
{
    while (__sync_lock_test_and_set(exclusion, 1)) while(*exclusion);
}

static inline void unlock(volatile int *exclusion)
{
    __sync_lock_release(exclusion);
}

#define INIT_Q(q) do {                               \
  int i;                                             \
  for(i=0; i<MAX_EL;  i++)                           \
    q->data[i] = 0;                                  \
    q->w[i]  = 0;                                    \
  } while(0)

#define pr

void pr_q(Q *q)
{
    pr("Q is pw<%d>, p_pw<%d>, cr<%d>, p_cr<%d>, lock<%d>, status<%d>\n", q->pw, q->p_pw, q->cr, q->p_cr, q->lock, q->status);
}
void pr_layout(Layout *lt)
{
    pr("Layout is: \n ");
    pr_q(&(lt->q1));
    pr_q(&(lt->q2));
}

void reset_locks(BusMem *bus, MRole role)
{
    //cleaning up locks
    Q *q = NULL;

    if(M_MASTER == role)
        q = &(bus->layout->q1);

    else
        q = &(bus->layout->q2);

    q->lock = 0;
}
void sm_further_init(Layout *layout, MRole role)
{
    int i;
    int is_master = (role == M_MASTER);

    if(1 == is_master)
    {
        Q *q =  &(layout->q1);
        pr("%s master\n", __FUNCTION__);
        MFENCE;
        q->status = M_INIT;
        INIT_Q(q);
        q->pw = q->p_pw = MAX_EL;
        layout->q2.cr = layout->q2.p_cr = MAX_EL;
        MFENCE;
        q->status = M_STARTED;
    }

    else
    {
        Q *q = &(layout->q2);
        pr("%s slave\n", __FUNCTION__);
        MFENCE;
        q->status = M_INIT;
        INIT_Q(q);
        q->pw = q->p_pw = MAX_EL;
        /*TODO Let us debate.*/
        layout->q1.cr = layout->q1.p_cr = MAX_EL;
        MFENCE;
        q->status = M_STARTED;
    }
}

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
    pr ("The size of layout is %d\n", lz);
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
int  sm_send(BusMem *bus, char **trans_id, int trans_id_len, void *data)
{
    assert(NULL != bus);
    uint64_t offset;
    int num;
    pr("LOCK m4\n");
    Do_Lock(bus->plock);

    if(M_MASTER == bus->role)
    {
        assert((char *)data >= (char *)bus->master_chunk);
        assert((char *)data < (bus->master_chunk + bus->master_chunk_len));
    }

    Q *selected_q=NULL;

    if(M_MASTER == bus->role)
    {
        pr("send to master bus\n");
        selected_q = &(bus->layout->q1);
        pr_q(selected_q);
    }

    else
    {
        selected_q = &(bus->layout->q2);
    }

    assert(NULL != selected_q);
    assert(M_STARTED == selected_q->status);
    /* convert data to relative offset */
    offset  =  ((char *)data - bus->chunk);

    if ((MAX_EL-1) > (selected_q->pw))
    {
        num = (selected_q->pw)+1;
    }

    else
    {
        num = 0;
    }

    if(0 != selected_q->w[num])
    {
        /* slow consumer, all blocks occupied, dont write. */
        pr("\n slow consumer, all blocks occupied, dont write.\n");
        Do_UnLock(bus->plock);
        return(1);
    }

    selected_q->pw = num;
    selected_q->data[num]= offset;
    selected_q->w[num] = 1;

pr("UNLOCK m\n");
Do_UnLock(bus->plock);
return 0;
}

int sm_rcv(BusMem *bus, char **trans_id, int trans_id_len, void **data)
{
    Q *q = NULL;
    assert(NULL != bus);
    uint64_t offset;
    int num;
    pr("LOCK m3\n");
    Do_Lock(bus->plock);

    if(M_MASTER == bus->role)
        q = &(bus->layout->q2);

    else
        q = &(bus->layout->q1);

    pr_q(q);

    if(M_STARTED != q->status || MAX_EL == q->pw || q->pw == q->cr) /* nothing is written till. */
    {
        pr("UNLOCK m2\n");
        Do_UnLock(bus->plock);
        return 1;
    }

    if(MAX_EL-1 > q->cr)
    {
        num = q->cr + 1;
    }

    else
    {
        num = 0;
    }

    if(1 != q->w[num])
    {
        /* slow producer, nothing to read. */
        Do_UnLock(bus->plock);
        return 1;
    }

    q->cr = num;
    offset = q->data[q->cr];
    /* convert relative offset to pointer */
    *data =  (void *)( bus->chunk + offset);
    pr("UNLOCK m1\n");
    Do_UnLock(bus->plock);
    return 0;
}

#if 0
int  sm_send(BusMem *bus, char **trans_id, int trans_id_len, void *data)
{
    assert(NULL != bus);
    uint64_t offset;

    if(M_MASTER == bus->role)
    {
        assert((char *)data >= (char *)bus->master_chunk);
        assert((char *)data < (bus->master_chunk + bus->master_chunk_len));
    }

    Q *selected_q=NULL;

    if(M_MASTER == bus->role)
    {
        pr("send to master bus\n");
        selected_q = &(bus->layout->q1);
        pr_q(selected_q);
    }

    else
    {
        selected_q = &(bus->layout->q2);
    }

    assert(NULL != selected_q);
    assert(M_STARTED == selected_q->status);
    /* convert data to relative offset */
    offset  =  ((char *)data - bus->chunk);
    lock(&(selected_q->lock)); /* Thread sync. */
    int old_pw = selected_q->pw;

    if ((MAX_EL-1) > (selected_q->pw))
    {
        selected_q->data[++(selected_q->pw)] = offset;
        MFENCE;
        selected_q->p_pw = old_pw;
    }

    else
    {
        selected_q->data[selected_q->pw = 0] = offset;
        MFENCE;
        selected_q->p_pw = 0;
    }

    unlock(&(selected_q->lock));
    return 0;
}

int sm_rcv(BusMem *bus, char **trans_id, int trans_id_len, void **data)
{
    Q *q = NULL;
    assert(NULL != bus);
    uint64_t offset;
    int p_pw;
    int pw;
    MFENCE;
    p_pw = q->p_w;
    MFENCE;
    pw = q->pw;

    if(M_MASTER == bus->role)
        q = &(bus->layout->q2);

    else
        q = &(bus->layout->q1);

    pr_q(q);

    if(M_STARTED != q->status)
    {
        return 1;
    }

    lock(&(q->lock)); /* Thread sync. */
    int old_cr = q->cr;

    if(MAX_EL == p_pw) /* nothing is written till. */
    {
        unlock(&(q->lock));
        return 1;
    }

    MFENCE;

    if( p_pw == q->cr && pw > p_pw)
    {
        offset = q->data
                 return 1;
    }

    if(MAX_EL-1 > q->cr)
    {
        offset  = q->data[++(q->cr)];
        MFENCE;
        q->p_cr =  old_cr;
    }

    else
    {
        offset   = q->data[q->cr=0];
        MFENCE;
        q->p_cr = 0;
    }

    unlock(&(q->lock));
    /* convert relative offset to pointer */
    *data =  (void *)( bus->chunk + offset);
    return 0;
}
#endif
void *sm_alloc(BusMem *bus, size_t bytes)
{
    return getb(&(bus->our_mspace), bytes);
}
void sm_free(BusMem *bus, void *mem)
{
    ungetb(bus->our_mspace, mem);
}

char *sm_strdup(BusMem *bus, const char *s)
{
    int bytes = strlen(s);
    char *ptr = getb(&(bus->our_mspace), bytes+1);
    assert(NULL != s);
    strcpy(ptr, s);
    return (ptr);
}

int sm_set_atomic_op_bound(BusMem *bus, int bound){
bus->max_bound  = bound;
}
