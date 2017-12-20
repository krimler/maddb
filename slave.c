#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "cool_bus.h"
int main(void)
{
    BusMem *b = sm_init(M_SLAVE, 5678, 100*1024*1024);
    assert(NULL != b);
    int i;
    int j =  0;

    for(;;)
    {
        char *foo = NULL;
        j++;
        sm_rcv(b, NULL, 0, (void **)&foo);

        if(NULL == foo )
            continue;

        //assert(NULL != foo);
//        pr("%p\n", foo);
        printf("<%s> is data \n", foo);
    }

    pr("all done\n");
}

