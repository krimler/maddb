#include <stdint.h>
#ifndef __M__H_
#define __M__H_
#include <stdlib.h>
typedef enum MRole
{
    M_SLAVE,
    M_MASTER
} MRole;

typedef enum MStatus
{
    M_INIT,
    M_STARTED
} MStatus;
typedef struct Ms
{
    uint64_t sz ;
    uint32_t pattern;
    key_t key;

    /*internal*/
    char *shm;
    int shmid;
} Ms;

int minit(Ms  *ms);

/*int main()
{
    char c;
    char *s, *t;
    Ms ms;
    ms.key = 5678;
    ms.sz = 100*1024*1024;
    ms.pattern = 0xDEADBEEF;
    assert(0 == minit(&ms));
    s = ms.shm;
    s++;
    testWrite(s);
    testRead(s);
    exit(0);
}
*/

#endif //__M__H_
