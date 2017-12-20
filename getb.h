#include "alloc.h"
#include "internal.h"
/* works like calloc, but allocates froms hared memory. */
void  *getb(mspace1 *our_mspace,  size_t bytes);

/* works like free, but returns memory to shared memmory pool. */
void ungetb(mspace1 our_mspace, void *mem);

extern mspace1 tlms;
