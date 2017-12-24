#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define STRM_ID 0xfade0001L

	typedef struct strm_s strm_t;

	typedef int(*strm_read_func)(strm_t *strm, uint8_t *buffer, int numBytes);
	typedef int(*strm_seek_func)(strm_t *strm, int offset, int origin);
	typedef int(*strm_tell_func)(strm_t *strm);
	typedef void(*strm_close_func)(strm_t *strm);


	typedef struct strm_api_s {
		strm_read_func read;
		strm_seek_func seek;
		strm_tell_func tell;
		strm_close_func close;
	} strm_api_t;

	typedef struct strm_s {
		int id;
		strm_read_func read;
		strm_seek_func seek;
		strm_tell_func tell;
		strm_close_func close;

		int bytesRemaining;
		uint8_t *readPtr;

		uint8_t *buffer;

		void *impl;
	} strm_t;


	strm_t *strm_create(strm_api_t api, void *impl);
	void strm_destroy(strm_t *strm);

	uint8_t* strm_read(strm_t *strm, int *numBytes);
	int strm_fill(strm_t *strm, uint8_t *buffer, int numBytes);
	int strm_seek(strm_t *strm, int offset, int origin);
	int strm_tell(strm_t *strm);

#ifdef __cplusplus
};
#endif