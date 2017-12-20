#ifndef __COOL_APP_H
#define  __COOL_APP_H
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include "om_mutex.h"
#include "m.h"
#include "alloc.h"
#include "internal.h"
#define MAX_EL 1000
#define pr
typedef struct Q
{
    int pw; /* producer write */
    int p_pw; /* previous write. */
    int cr; /* consumer read*/
    int p_cr; /* previous read. */
    volatile int lock;
    int status;
    uint64_t data[MAX_EL]; /* stores relative offset */
    int w[MAX_EL];

} Q ; //__attribute__ ((aligned (64)));

typedef struct Layout
{
    struct Q q1; /* master is writer; slave is reader. */
    struct Q q2; /* master is reader; slave is writer. */
} Layout;


typedef struct BusMem
{
    Ms *ms;
    struct Layout *layout;
    char *chunk;
    int chunk_len;
    MRole role;
    mspace1 our_mspace;
    char *master_chunk;
    int master_chunk_len;
    char *slave_chunk;
    int slave_chunk_len;
    volatile int lock;
    volatile char plock_space [MIN_LOCK_SIZE];
    volatile void *plock;
    int max_bound;
} BusMem;

BusMem *sm_init(MRole role, int key, unsigned long long size);

void sm_set_bus_inventory(BusMem **buses);
BusMem *sm_get_next_working_bus(BusMem **buses);

int sm_set_atomic_op_bound(BusMem *bus, int bound);
int get_transaction_id(BusMem *bus, char **trans_id, int trans_id_len);

int  sm_send(BusMem *bus, char **trans_id, int trans_id_len, void *data);
int sm_rcv(BusMem *bus, char **trans_id, int trans_id_len, void **data);

void *sm_alloc(BusMem *bus, size_t bytes);
void sm_free(BusMem *bus, void *mem);
char *sm_strdup(BusMem *bus, const char *s);

#endif //__COOL_APP_H
