#include "common.h"
#include "filestrm.h"
#include <malloc.h>

static int read_func(strm_t *strm, uint8_t *buffer, int numBytes) {
	filestrm_t *file = (filestrm_t *)strm->impl;
	int cbRead, cb = 0;
	do {
		cbRead = fread(buffer + cb, 1, numBytes, file->fp);
		if (cbRead <= 0) {
			break;
		}
		numBytes -= cbRead;
		cb += cbRead;
	} while (numBytes > 0);

	return cb;
}

static int seek_func(strm_t *strm, int offset, int origin) {
	filestrm_t *file = (filestrm_t *)strm->impl;
	if (fseek(file->fp, offset, origin) < 0) {
		return -1;
	}
	return 0;
}

static int tell_func(strm_t *strm) {
	filestrm_t *file = (filestrm_t *)strm->impl;
	return ftell(file->fp);
}

static void close_func(strm_t *strm) {
	filestrm_t *file = (filestrm_t *)strm->impl;
	filestrm_destroy(file);
}

static strm_api_t api = {
	read_func,
	seek_func,
	tell_func,
	close_func
};

strm_t *filestrm_create(char *filename) {
	FILE *fp = fopen(filename, "rb");
	filestrm_t *file = (filestrm_t *)memalloc(1, sizeof(*file));
	strm_t *strm = strm_create(api, file);

	if (fp == 0 || file == 0 || strm == 0) {
		return 0;
	}

	file->id = FILESTRM_ID;
	file->fp = fp;

	return strm;
}

void filestrm_destroy(filestrm_t *file) {
	if (file == 0) {
		return;
	}
	if (file->fp) {
		fclose(file->fp);
	}
	memfree(file);
}
