#include "common.h"
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>

#ifdef _3DS
#include <3ds/svc.h>
#include <3ds/services/hid.h>
#endif

int __waitforit(char *str) {

#if 0
	printf("%s ...", str);
	do {
		hidScanInput();
	} while ((hidKeysHeld() & KEY_A) == 0);
	do {
		hidScanInput();
	} while ((hidKeysHeld() & KEY_A) != 0);
	printf("done.\n");
#endif
	return 0;
}

void __verifyid(int id1, int id2, char *file) {
	if (id1 != id2) {
		printf("mismatched id: %08x != %08x\nfile: %s\n", id1, id2, file);
		while (1);
	}
}

size_t mem_allocated = 0;
size_t mem_freed = 0;

void *__memalloc(size_t count, size_t size) {
	size_t cb = count * size;
	uint32_t *p =  (uint32_t *)calloc(count + 4, size);
	*p++ = cb;
	mem_allocated += cb;
	return p;
}

void __memfree(void *p) {
	uint32_t *ptr = (uint32_t *)p;
	uint32_t cb;
	ptr--;
	cb = *ptr;
	mem_freed += cb;
	free(ptr);
}
