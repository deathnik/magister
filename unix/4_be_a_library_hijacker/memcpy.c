#define _GNU_SOURCE
#include <sys/types.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include "string.h"


void *memcpy(void * dst, const void * src, size_t n){
  fputs("memcpy: our reloaded version called\n", stdout);
  if ( (src <= dst and dst < src + n) or
  	   (src <= dst + n -1 and dst + n - 1 < src + n) ){
  	fputs("memcpy: src == dst, falling to memmove", stdout);
  	return memmove(dst,src,n);
  }
  void *(*original_memcpy)(void * dst, const void * src, size_t n);
  original_memcpy = dlsym(RTLD_NEXT, "memcpy");
  return (*original_memcpy)(dst,src,n);
}