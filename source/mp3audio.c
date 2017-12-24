#include "common.h"
#include "mp3audio.h"
#include "audio.h"
#include "strm.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>


static int fill_in_buffer(mp3audio_t *mp3) {
	if (mp3->in_bytesLeft <= (MP3_IN_BUFFER_SIZE/2)) {
		if (mp3->in_bytesLeft > 0) {
			memmove(mp3->in_buffer, mp3->in_readPtr, mp3->in_bytesLeft);
		}
		mp3->in_readPtr = mp3->in_buffer;
		int cbRead = MP3_IN_BUFFER_ALLOC - mp3->in_bytesLeft;
		int cb = strm_fill(mp3->strm, mp3->in_readPtr + mp3->in_bytesLeft, cbRead);
		if (cb <= 0) {
			return -1;
		}
		mp3->in_bytesLeft += cb;
	}
	return 0;
}

static int decode(mp3audio_t *mp3, int buffer_offset) {
	int offset, err;


	/* find start of next MP3 frame - assume EOF if no sync found */
	offset = MP3FindSyncWord(mp3->in_readPtr, mp3->in_bytesLeft);
	if (offset < 0) {
		return -1;
	}
	mp3->in_readPtr += offset;
	mp3->in_bytesLeft -= offset;

#ifdef MP3AUDIO_USE_THREAD
	err = MP3Decode(mp3->decoder, &mp3->in_readPtr, &mp3->in_bytesLeft, (short *)(mp3->out_buffer + (mp3->out_buffer_index*MP3_OUTBUF_SIZE) + buffer_offset), 0);
#else
	err = MP3Decode(mp3->decoder, &mp3->in_readPtr, &mp3->in_bytesLeft, (short *)mp3->out_buffer, 0);
#endif
	if (err) {
		/* error occurred */
		switch (err) {
		case ERR_MP3_INDATA_UNDERFLOW:
			//outOfData = 1;
			break;
		case ERR_MP3_MAINDATA_UNDERFLOW:
			/* do nothing - next call to decode will provide more mainData */
			return 0;
		case ERR_MP3_FREE_BITRATE_SYNC:
		default:
			return -1;
		}
		return 0;
	}
	/* no error */
	MP3GetLastFrameInfo(mp3->decoder, &mp3->frame_info);

	return mp3->frame_info.outputSamps * 2;
}

static int init_func(audio_t *audio) {
	if (audio == 0) {
		return -1;
	}
	mp3audio_t *mp3 = (mp3audio_t *)audio->impl;

	mp3->decoder = MP3InitDecoder();
	
	fill_in_buffer(mp3);

	//decode once to get format
	int cbDecoded = decode(mp3, 0);
	if(cbDecoded < 0) {
		return -1;
	}

	audio->snd_channels = mp3->frame_info.nChans;
	audio->snd_speed = mp3->frame_info.samprate;
	audio->snd_samplebits = mp3->frame_info.bitsPerSample;
	audio->snd_sample_bytes = (mp3->frame_info.bitsPerSample/8) * mp3->frame_info.nChans;

	audio->wav_buffer_size = cbDecoded;

	return 0;
}

static int fill_func(audio_t *audio, uint8_t *buffer, int samples) {
	if (audio == 0) {
		return -1;
	}
	mp3audio_t *mp3 = (mp3audio_t *)audio->impl;
	int cb = samples * audio->snd_sample_bytes;
	int filled = 0;

	do {
		int cbRead = cb;
		if (mp3->out_bytesLeft > 0) {
			if (cbRead > mp3->out_bytesLeft) {
				cbRead = mp3->out_bytesLeft;
			}
			memcpy(buffer, mp3->out_readPtr, cbRead);
			mp3->out_readPtr += cbRead;
			buffer += cbRead;
			mp3->out_bytesLeft -= cbRead;
			filled += cbRead;
			cb -= cbRead;
			continue;
		}
#ifdef MP3AUDIO_USE_THREAD
		LightEvent_Signal(&mp3->request);
		LightEvent_Wait(&mp3->response);
		LightEvent_Clear(&mp3->response);
#else
		int cbTotal = 0;
		int cbDecoded;
		do {
			fill_in_buffer(mp3);
			cbDecoded = decode(mp3, 0);
			if (cbDecoded < 0) {
				break;
			}
			//printf("decoded: %d\n", cbDecoded);
			cbTotal += cbDecoded;
			//svcSleepThread(20000ULL);
			break;
		} while ((MP3_OUTBUF_SIZE - cbTotal - MP3_DECODE_SIZE) > 0);

		mp3->out_readPtr = mp3->out_buffer;
		mp3->out_bytesLeft = cbTotal;
#endif
		if (mp3->out_bytesLeft <= 0) {
			return -1;
		}

	} while (cb > 0);


	return filled / audio->snd_sample_bytes;
}

static void mp3audio_destroy(mp3audio_t *mp3) {
	if (mp3) {
#ifdef MP3AUDIO_USE_THREAD
		mp3->thread_quit = 1;
		printf("request join\n");
		LightEvent_Signal(&mp3->request);
		threadJoin(mp3->thread, U64_MAX);
		threadFree(mp3->thread);
		printf("joined\n");
#endif
		if (mp3->decoder) {
			MP3FreeDecoder(mp3->decoder);
			mp3->decoder = 0;
		}
		if (mp3->in_buffer) {
			memfree(mp3->in_buffer);
		}
		if (mp3->out_buffer) {
			memfree(mp3->out_buffer);
		}
		if (mp3->strm) {
			strm_destroy(mp3->strm);
		}
		memfree(mp3);
	}
}

static void close_func(audio_t *audio) {
	if (audio == 0) {
		return;
	}
	mp3audio_t *mp3 = (mp3audio_t *)audio->impl;
	mp3audio_destroy(mp3);
}

static audio_api_t api = {
	init_func,
	fill_func,
	close_func
};

#ifdef MP3AUDIO_USE_THREAD
static void decode_thread(void *user) {
	mp3audio_t *mp3 = (mp3audio_t *)user;
	int cbDecoded;
	int cbTotal;
	int i;
	do {
		cbTotal = 0;
		i = 0;
		do {
			fill_in_buffer(mp3);
			cbDecoded = decode(mp3, 0);
			if (cbDecoded < 0) {
				break;
			}
			cbTotal += cbDecoded;
			i++;
			//printf("decoded: %d\n", cbDecoded);
			break;
		} while ((MP3_OUTBUF_SIZE - cbTotal - MP3_DECODE_SIZE) > 0);
		
		//printf("decoded: %d %d\n", i, cbTotal);

		//printf("hello world\n");
		LightEvent_Wait(&mp3->request);
		LightEvent_Clear(&mp3->request);
		//printf("released\n");

		if (mp3->thread_quit != 0) {
			printf("quitting\n");
			break;
		}


		mp3->out_readPtr = (uint8_t *)(mp3->out_buffer + (mp3->out_buffer_index*MP3_OUTBUF_SIZE));
		mp3->out_bytesLeft = cbTotal;// mp3->frame_info.outputSamps * 2;
		mp3->out_buffer_index = mp3->out_buffer_index == 0 ? 1 : 0;

		LightEvent_Signal(&mp3->response);

	} while (mp3->thread_quit == 0);
	printf("done\n");
}
#endif

audio_t *mp3audio_create(strm_t *strm) {
	if (strm == 0) {
		return 0;
	}
	mp3audio_t *mp3 = (mp3audio_t *)memalloc(1, sizeof(*mp3));
	if (mp3 == 0) {
		return 0;
	}
	mp3->id = MP3AUDIO_ID;
	mp3->strm = strm;
	mp3->out_buffer = (uint8_t *)memalloc(1, MP3_OUTBUF_ALLOC);
	mp3->in_buffer = (uint8_t *)memalloc(1, MP3_IN_BUFFER_ALLOC);
	if (mp3->out_buffer == 0 || mp3->in_buffer == 0) {
		mp3audio_destroy(mp3);
		return 0;
	}

	audio_t *audio = audio_create(api, mp3);
	if (audio == 0) {
		//mp3audio_destroy(mp3);
		return 0;
	}
#ifdef MP3AUDIO_USE_THREAD
	LightEvent_Init(&mp3->request, RESET_STICKY);
	LightEvent_Init(&mp3->response, RESET_STICKY);
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	mp3->thread = threadCreate(decode_thread, (void*)mp3, (4*1024), prio - 1, -2, false);
	if (mp3->thread == 0) {
		printf("holy fuckballs the thread failed to create\n");
		while (1);
	}
#endif
	return audio;
}
