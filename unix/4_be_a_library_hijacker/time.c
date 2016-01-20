#define _GNU_SOURCE
#include <sys/types.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char * tmp;
time_t time(time_t * t)
{
	tmp = getenv("FIXED_TIME");
	time_t fixedTime = 0;
	if (tmp != NULL)
		fixedTime = atoi(tmp);
    if (t != NULL)
        *t=fixedTime;
    return fixedTime;
}
