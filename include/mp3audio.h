#pragma once

#ifdef WIN32
#include <mp3dec.h>
#endif

#ifdef _3DS
#include <helix/mp3dec.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/thread.h>
#endif

#include <stdint.h>

#ifdef _3DS
#define MP3AUDIO_USE_THREAD
#endif

#define MP3AUDIO_ID 0xfade0006L

#define MP3_IN_BUFFER_SIZE 4096

#define MP3_DECODE_SIZE	(MAX_NCHAN * MAX_NGRAN * MAX_NSAMP * 2 * 2)
#define MP3_OUTBUF_SIZE	(MP3_DECODE_SIZE * 2)

#ifdef MP3AUDIO_USE_THREAD
#define MP3_OUTBUF_ALLOC (MP3_OUTBUF_SIZE * 2)
#else
#define MP3_OUTBUF_ALLOC (MP3_OUTBUF_SIZE * 2)
#endif

#define MP3_IN_BUFFER_ALLOC (MP3_IN_BUFFER_SIZE * 2)

typedef struct strm_s strm_t;
typedef struct audio_s audio_t;

typedef struct mp3audio_s {
	int id;

	MP3FrameInfo	frame_info;
	HMP3Decoder		decoder;

	uint8_t *in_readPtr;
	int in_bytesLeft;
	uint8_t *in_buffer;//[MP3_IN_BUFFER_SIZE*2];

	uint8_t *out_readPtr;
	int out_bytesLeft;
	uint8_t *out_buffer;// [MP3_OUTBUF_SIZE];

	strm_t *strm;
#ifdef MP3AUDIO_USE_THREAD
	int eos;
	int out_buffer_index;
	LightEvent request;
	LightEvent response;
	Thread thread;
	int thread_quit;
#endif
} mp3audio_t;

audio_t *mp3audio_create(strm_t *strm);
