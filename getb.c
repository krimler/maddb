#include "getb.h"

void  *getb(mspace *our_mspace, size_t bytes)
{
    if (*our_mspace == 0)
    {
        *our_mspace= create_mspace(0, 0);
        setMKGParamsFurther(bytes);
    }

    return mspace_calloc(*our_mspace, 1, bytes);
}

void  ungetb(mspace our_mspace, void *mem)
{
    mspace_free(our_mspace, mem);
}
