#ifndef __COOL_DB_H
#define  __COOL_DB_H
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <alloc.h>
#include "om_mutex.h"
#include "utlist.h"
#include "uthash.h"
#include "m.h"
#define MAX_EL 1000
#define pr

typedef struct Key {
  void *k;
}Key;
typedef struct Item
{
    Key *key;
    int pw; /* producer write */
    int cr; /* consumer read*/
    int status;
    void *value;
    UT_hash_handle hh;         /* makes this structure hashable */

} Item ; //__attribute__ ((aligned (64)));

typedef struct Layout {
  Item *item;
  struct element *prev;
  struct element *next; /* needed for singly- or doubly-linked lists */
};
typedef void *mspace;

typedef struct DBMem
{
    Ms *ms;
    struct Layout *layout;
    char *chunk;
    int chunk_len;
    MRole role;
    mspace our_mspace;
    char *master_chunk; /* does allocation of individiaul keys and values. */
    int master_chunk_len;
    char *slave_chunk; /* does allocation of individual hash table items that hold pointers to key and values for hash tables. */
    int slave_chunk_len;
    volatile char plock_space [MIN_LOCK_SIZE];
    volatile void *plock;
    int max_bound;
} DBMem;

typedef enum Free_Strategy {
  MANAGED_BY_APP,
  MANAGED_BY_LIB_VIA_LIFO,
}Free_Strategy;
  
typedef struct DBAttr {
  MRole role;
  Free_Strategy strategy; /* relevant to master only */
};

typedef seconds int;
DBMem *cdb_init(MRole role, int key, unsigned long long size);

void *cdb_get(DBMem *bus, void *key );

int cdb_set(DBMem *bus, char* key, void*value, seconds timeout); 

void *db_alloc(DBMem *bus, size_t bytes);
void db_free(DBMem *bus, void *mem);
char *db_strdup(DBMem *bus, const char *s);
int cdb_set_atomic_op_bound(DBMem *bus, int bound);
#endif //__COOL_DB_H
