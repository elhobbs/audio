#pragma once

#include "strm.h"
#include <stdio.h>

#define FILESTRM_ID 0xfade0002L

typedef struct filestrm_s {
	int id;
	FILE *fp;
} filestrm_t;


strm_t *filestrm_create(char *filename);
void filestrm_destroy(filestrm_t *strm);
