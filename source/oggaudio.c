#include "common.h"
#include "oggaudio.h"
#include "strm.h"
#include "audio.h"
#include <string.h>

static size_t ov_read_func(void *ptr, size_t size, size_t nmemb, void *datasource) {
	strm_t *strm = (strm_t *)datasource;
	if (strm == 0) {
		return -1;
	}
	int cbRequested = size * nmemb;
	int cbRead = strm_fill(strm, (uint8_t *)ptr, cbRequested);
	return cbRead;
}

static int ov_seek_func(void *datasource, ogg_int64_t offset, int whence) {
	return -1;
}

static int ov_close_func(void *datasource) {
	return -1;
}

static ov_callbacks rdr_callbacks = {
	ov_read_func,
	ov_seek_func,
	ov_close_func,
	0 //tell_func
};

static int decode(oggaudio_t *ogg, int buffer_offset) {
#ifdef OGGAUDIO_USE_THREAD
	int cbDecoded = ov_read(&ogg->vorbis, (ogg->out_buffer + (ogg->out_buffer_index*OGG_OUTBUF_SIZE) + buffer_offset), OGG_OUTBUF_SIZE, &ogg->current_section);
#else
	int cbDecoded = ov_read(&ogg->vorbis, (ogg->out_buffer + buffer_offset), OGG_OUTBUF_SIZE, &ogg->current_section);
#endif

	return cbDecoded;
}

static int init_func(audio_t *audio) {
	if (audio == 0) {
		return -1;
	}
	oggaudio_t *ogg = (oggaudio_t *)audio->impl;
	int ret = ov_open_callbacks(ogg->strm, &ogg->vorbis, NULL, 0, rdr_callbacks);
	if (ret < 0) {
		return -1;
	}
	vorbis_info *vi = ogg->vi = ov_info(&ogg->vorbis, -1);
	ogg->current_section = 0;

	audio->snd_channels = vi->channels;
	audio->snd_speed = vi->rate;
	audio->snd_samplebits = 16;
	audio->snd_sample_bytes = 2 * vi->channels;

	audio->wav_buffer_size = 4096;

	return 0;
}

static int fill_func(audio_t *audio, uint8_t *buffer, int samples) {
	if (audio == 0) {
		return -1;
	}
	oggaudio_t *ogg = (oggaudio_t *)audio->impl;
	int cb = samples * audio->snd_sample_bytes;
	int filled = 0;
	do {
		int cbRead = cb;
		if (ogg->out_bytesLeft > 0) {
			if (cbRead > ogg->out_bytesLeft) {
				cbRead = ogg->out_bytesLeft;
			}
			memcpy(buffer, ogg->out_readPtr, cbRead);
			ogg->out_readPtr += cbRead;
			buffer += cbRead;
			ogg->out_bytesLeft -= cbRead;
			filled += cbRead;
			cb -= cbRead;
			continue;
		}
#ifdef OGGAUDIO_USE_THREAD
		LightEvent_Signal(&ogg->request);
		LightEvent_Wait(&ogg->response);
		LightEvent_Clear(&ogg->response);
#else
		int cbTotal = 0;
		int cbDecoded;
		do {
			cbDecoded = decode(ogg, 0);
			if (cbDecoded < 0) {
				break;
			}
			//printf("decoded: %d\n", cbDecoded);
			cbTotal += cbDecoded;
			//svcSleepThread(20000ULL);
			break;
		} while ((OGG_OUTBUF_SIZE - cbTotal) > 0);

		ogg->out_readPtr = ogg->out_buffer;
		ogg->out_bytesLeft = cbTotal;
#endif
		if (ogg->out_bytesLeft <= 0) {
			return -1;
		}

	} while (cb > 0);


	return filled / audio->snd_sample_bytes;
}

static void oggaudio_destroy(oggaudio_t *ogg) {
	if (ogg) {
#ifdef OGGAUDIO_USE_THREAD
		ogg->thread_quit = 1;
		printf("request join\n");
		LightEvent_Signal(&ogg->request);
		threadJoin(ogg->thread, U64_MAX);
		threadFree(ogg->thread);
		printf("joined\n");
#endif
		ov_clear(&ogg->vorbis);

		if (ogg->out_buffer) {
			memfree(ogg->out_buffer);
		}
		if (ogg->strm) {
			strm_destroy(ogg->strm);
		}
		memfree(ogg);
	}
}

static void close_func(audio_t *audio) {
	if (audio == 0) {
		return;
	}
	oggaudio_t *ogg = (oggaudio_t *)audio->impl;
	oggaudio_destroy(ogg);
}


static audio_api_t api = {
	init_func,
	fill_func,
	close_func
};

#ifdef OGGAUDIO_USE_THREAD
static void decode_thread(void *user) {
	oggaudio_t *ogg = (oggaudio_t *)user;
	int cbDecoded;
	int cbTotal;
	int i;
	do {
		cbTotal = 0;
		i = 0;
		cbDecoded = 0;
		do {
			cbDecoded = decode(ogg, 0);
			if (cbDecoded < 0) {
				break;
			}
			cbTotal += cbDecoded;
			i++;
			//printf("decoded: %d\n", cbDecoded);
			break;
		} while ((OGG_OUTBUF_SIZE - cbTotal) > 0);

		//printf("decoded: %d %d\n", i, cbTotal);

		//printf("hello world\n");
		LightEvent_Wait(&ogg->request);
		LightEvent_Clear(&ogg->request);
		//printf("released\n");

		if (ogg->thread_quit != 0) {
			printf("quitting\n");
			break;
		}


		ogg->out_readPtr = (uint8_t *)(ogg->out_buffer + (ogg->out_buffer_index*OGG_OUTBUF_SIZE));
		ogg->out_bytesLeft = cbTotal;// mp3->frame_info.outputSamps * 2;
		ogg->out_buffer_index = ogg->out_buffer_index == 0 ? 1 : 0;

		LightEvent_Signal(&ogg->response);

	} while (ogg->thread_quit == 0);
	printf("done\n");
}
#endif

audio_t *oggaudio_create(strm_t *strm) {
	if (strm == 0) {
		return 0;
	}
	oggaudio_t *ogg = (oggaudio_t *)memalloc(1, sizeof(*ogg));
	if (ogg == 0) {
		return 0;
	}
	ogg->id = OGGAUDIO_ID;
	ogg->strm = strm;
	ogg->out_buffer = (uint8_t *)memalloc(1, OGG_OUTBUF_ALLOC);
	if (ogg->out_buffer == 0) {
		oggaudio_destroy(ogg);
		return 0;
	}

	audio_t *audio = audio_create(api, ogg);
	if (audio == 0) {
		//oggaudio_destroy(ogg);
		return 0;
	}
#ifdef OGGAUDIO_USE_THREAD
	LightEvent_Init(&ogg->request, RESET_STICKY);
	LightEvent_Init(&ogg->response, RESET_STICKY);
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	ogg->thread = threadCreate(decode_thread, (void*)ogg, (4 * 1024), prio - 1, -2, false);
	if (ogg->thread == 0) {
		printf("holy fuckballs the thread failed to create\n");
		while (1);
	}
#endif
	return audio;
}
