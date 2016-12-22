#include <stdio.h>

#include <unistd.h>


int num_cpu(void){
    return sysconf(_SC_NPROCESSORS_ONLN);
}

