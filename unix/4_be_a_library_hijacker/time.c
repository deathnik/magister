#define _GNU_SOURCE
#include <sys/types.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

time_t time(time_t * t)
{
	time_t fixedTime= atoi(getenv("FIXED_TIME"));
    if(t!=NULL)
        *t=fixedTime;
    return fixedTime;
}
