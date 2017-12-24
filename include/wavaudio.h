#pragma once

#ifdef _3DS
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/thread.h>
#endif

#ifdef _3DS
#define WAVAUDIO_USE_THREAD
#endif

#define WAVAUDIO_ID 0xfade0004L

#define WAV_OUTBUF_SIZE 8192
#define WAV_OUTBUF_ALLOC (WAV_OUTBUF_SIZE*2)

typedef struct strm_s strm_t;
typedef struct audio_s audio_t;

typedef struct wavaudio_s {
	int id;
	strm_t *strm;
#ifdef WAVAUDIO_USE_THREAD
	uint8_t *out_readPtr;
	int out_bytesLeft;
	uint8_t *out_buffer;// [WAV_OUTBUF_ALLOC];
	int out_buffer_index;
	LightEvent request;
	LightEvent response;
	Thread thread;
	int thread_quit;
#endif
} wavaudio_t;

audio_t *wavaudio_create(strm_t *strm);
