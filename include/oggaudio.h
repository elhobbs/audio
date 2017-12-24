#pragma once

#ifdef WIN32
#include <ivorbiscodec.h>
#include <ivorbisfile.h>
#endif

#ifdef _3DS
#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/thread.h>
#endif

#include <stdint.h>

#ifdef _3DS
#define OGGAUDIO_USE_THREAD
#endif

#define OGGAUDIO_ID 0xfade0007L

#define OGG_IN_BUFFER_SIZE 4096

#define OGG_OUTBUF_SIZE	4096

#ifdef OGGAUDIO_USE_THREAD
#define OGG_OUTBUF_ALLOC (OGG_OUTBUF_SIZE * 2)
#else
#define OGG_OUTBUF_ALLOC (OGG_OUTBUF_SIZE * 2)
#endif

#define OGG_IN_BUFFER_ALLOC (OGG_IN_BUFFER_SIZE * 2)

typedef struct strm_s strm_t;
typedef struct audio_s audio_t;

typedef struct oggaudio_s {
	int id;

	OggVorbis_File	vorbis;
	vorbis_info		*vi;
	int current_section;

	uint8_t *out_readPtr;
	int out_bytesLeft;
	uint8_t *out_buffer;// [MP3_OUTBUF_SIZE];

	strm_t *strm;
#ifdef OGGAUDIO_USE_THREAD
	int out_buffer_index;
	LightEvent request;
	LightEvent response;
	Thread thread;
	int thread_quit;
#endif
} oggaudio_t;

audio_t *oggaudio_create(strm_t *strm);
