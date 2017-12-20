#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "cool_bus.h"
int main(void)
{
    BusMem *b = sm_init(M_MASTER, 5678, 100*1024*1024);
    assert(NULL != b);
    int i;
    char *ch = "madhav";

    for(i=0; i<100; i++)
    {
        char *foo;
        printf("doo alloc\n");
        foo =  sm_alloc(b, 1024);
        assert(NULL != foo);
        sprintf(foo, "%s %d", ch, i);
        printf("do_send\n"); 
        if(1 == sm_send(b, NULL, 0, foo)) {
          printf("err occured\n");
          return 1;
        }
        pr("%p has <%s>\n", foo, foo);
        sm_free(b, foo);
    }

//    printf("master all done\n");
//    sleep(15);
}

