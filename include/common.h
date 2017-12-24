#pragma once
#include <malloc.h>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

int __waitforit(char *str);

#define waitforit() __waitforit(__FILE__":"STRINGIZE(__LINE__))


void __verifyid(int id1, int id2, char *file);

#define verifyid(_x,_y) __verifyid((_x),(_y),__FILE__":"STRINGIZE(__LINE__))


#ifdef _3DS
#undef WIN32
#endif

#ifdef __INTELLISENSE__
#define __attribute__(...)
#endif

#define DEBUG_MEMORY 1

#if DEBUG_MEMORY
void *__memalloc(size_t count, size_t size);
void __memfree(void *p);
#define memalloc __memalloc
#define memfree __memfree
#else
#define memalloc calloc
#define memfree free
#endif
