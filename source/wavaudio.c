#include "common.h"
#include "audio.h"
#include "wavaudio.h"
#include "strm.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>

static short read_short(uint8_t *data) {
	short val = 0;
	val = *data;
	val = val + (*(data + 1) << 8);
	return val;
}

static int read_long(uint8_t *data) {
	int val = 0;
	val = *data;
	val = val + (*(data + 1) << 8);
	val = val + (*(data + 2) << 16);
	val = val + (*(data + 3) << 24);
	return val;
}

int static find_chunk(strm_t * strm, char *name) {
	int cb;
	uint8_t *data;
	do {
		cb = 8;
		data = strm_read(strm, &cb);
		if (data == 0 || cb < 8) {
			return -1;
		}
		int chunk_len = read_long(data + 4);
		if (memcmp(data, name, 4) == 0) {
			return chunk_len;
		}
		if (strm_seek(strm, chunk_len, SEEK_CUR) < 0) {
			break;
		}
	} while (1);

	return -1;
}


static int parse(audio_t *audio, strm_t * strm) {
	int cb;
	int chunk_len = find_chunk(strm, "RIFF");
	if (chunk_len < 0) {
		return -1;
	}

	cb = 4;
	uint8_t *data = strm_read(strm, &cb);
	if(memcmp(data,"WAVE", 4) != 0) {
		return -1;
	}

	chunk_len = find_chunk(strm, "fmt ");
	if (chunk_len < 0) {
		return -1;
	}

	cb = 16;
	data = strm_read(strm, &cb);
	if (data == 0 || cb < 16) {
		return -1;
	}

	short format = read_short(data);
	if (format != 1) {
		return -1;
	}
	audio->snd_channels = read_short(data + 2);
	audio->snd_speed = read_long(data + 4);
	audio->snd_samplebits = read_short(data + 14);
	audio->snd_sample_bytes = audio->snd_channels * (audio->snd_samplebits / 8);

	audio->wav_buffer_size = 4096;

	chunk_len = find_chunk(strm, "data");
	if (chunk_len < 0) {
		return -1;
	}

	return 0;
}

static int init_func(audio_t *audio) {
	if (audio == 0) {
		return -1;
	}
	wavaudio_t *wav = (wavaudio_t *)audio->impl;

	if (parse(audio, wav->strm)) {
		return -1;
	}
	return 0;
}

static int fill_func(audio_t *audio, uint8_t *buffer, int samples) {
	if (audio == 0) {
		return -1;
	}
	wavaudio_t *wav = (wavaudio_t *)audio->impl;
	int cb = samples * audio->snd_sample_bytes;
#ifdef WAVAUDIO_USE_THREAD
	int cbRead;
	int filled = 0;
	do {
		if (wav->out_bytesLeft > 0) {
			cbRead = cb;
			if (cbRead > wav->out_bytesLeft) {
				cbRead = wav->out_bytesLeft;
			}
			memcpy(buffer, wav->out_readPtr, cbRead);
			buffer += cbRead;
			wav->out_readPtr += cbRead;
			wav->out_bytesLeft -= cbRead;
			filled += cbRead;
			cb -= cbRead;
			continue;
		}

		LightEvent_Signal(&wav->request);
		LightEvent_Wait(&wav->response);
		LightEvent_Clear(&wav->response);
		
		if (wav->out_bytesLeft <= 0) {
			return -1;
		}

	} while (cb > 0);
#else
	strm_t *strm = wav->strm;
	int filled = strm_fill(strm, buffer, cb);
	if (filled < 0) {
		return -1;
	}
#endif
	return filled / audio->snd_sample_bytes;
}

static void wavaudio_destroy(wavaudio_t *wav) {
	if (wav) {
#ifdef WAVAUDIO_USE_THREAD
		wav->thread_quit = 1;
		printf("request join\n");
		LightEvent_Signal(&wav->request);
		threadJoin(wav->thread, U64_MAX);
		threadFree(wav->thread);
		printf("joined\n");
		if (wav->out_buffer) {
			memfree(wav->out_buffer);
		}
#endif
		if (wav->strm) {
			strm_destroy(wav->strm);
		}
		memfree(wav);
	}
}

static void close_func(audio_t *audio) {
	if (audio == 0) {
		return;
	}
	wavaudio_t *wav = (wavaudio_t *)audio->impl;
	wavaudio_destroy(wav);
}


static audio_api_t api = {
	init_func,
	fill_func,
	close_func
};

#ifdef WAVAUDIO_USE_THREAD

static int load_buffer(wavaudio_t *wav) {
	uint8_t *buffer = (wav->out_buffer + (wav->out_buffer_index*WAV_OUTBUF_SIZE));
	int length = strm_fill(wav->strm, buffer, WAV_OUTBUF_SIZE);
	if (length < 0) {
		return -1;
	}

	return length;
}

static void load_thread(void *user) {
	wavaudio_t *wav = (wavaudio_t *)user;
	int length;
	do {
		length = load_buffer(wav);
		//printf("length: %d\n", length);
		if (length < 0) {
			break;
		}
		LightEvent_Wait(&wav->request);
		LightEvent_Clear(&wav->request);
		//printf("released\n");

		if (wav->thread_quit != 0) {
			printf("quitting\n");
			break;
		}


		wav->out_readPtr = wav->out_buffer + (wav->out_buffer_index*WAV_OUTBUF_SIZE);
		wav->out_bytesLeft = length;
		wav->out_buffer_index = wav->out_buffer_index == 0 ? 1 : 0;

		LightEvent_Signal(&wav->response);

	} while (wav->thread_quit == 0);
	printf("done\n");
}
#endif

audio_t *wavaudio_create(strm_t *strm) {
	wavaudio_t *wav = (wavaudio_t *)memalloc(1, sizeof(*wav));
	if (wav == 0) {
		return 0;
	}
	wav->id = WAVAUDIO_ID;
	wav->strm = strm;

#ifdef WAVAUDIO_USE_THREAD
	wav->out_buffer = (uint8_t *)memalloc(1, WAV_OUTBUF_ALLOC);
	if (wav->out_buffer == 0) {
		return 0;
	}
#endif

	audio_t *audio = audio_create(api, wav);
	if (audio == 0) {
		//wavaudio_destroy(wav);
		return 0;
	}
#ifdef WAVAUDIO_USE_THREAD
	LightEvent_Init(&wav->request, RESET_STICKY);
	LightEvent_Init(&wav->response, RESET_STICKY);
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	wav->thread = threadCreate(load_thread, (void*)wav, (4 * 1024), prio - 1, -2, false);
	if (wav->thread == 0) {
		printf("holy fuckballs the thread failed to create\n");
		while (1);
	}
#endif
	return audio;
}