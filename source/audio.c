#include "common.h"
#include "audio.h"
#include "strm.h"
#include <malloc.h>
#include <stdio.h>
#include "audiochannel.h"

audio_t *audio_create(audio_api_t api, void *impl) {
	if (impl == 0) {
		return 0;
	}
	audio_t *audio = (audio_t *)memalloc(1, sizeof(*audio));
	if (audio == 0) {
		return 0;
	}
	audio->id = AUDIO_ID;

	audio->init = api.init;
	audio->fill = api.fill;
	audio->close = api.close;

	audio->impl = impl;

	if (audio->init) {
		if (audio->init(audio) < 0) {
			goto error;
		}
	}

	audio->chnl = audiochannel_create(audio);
	if (audio->chnl == 0) {
		goto error;
	}

	return audio;
error:
	audio_destroy(audio);
	return 0;
}

void audio_destroy(audio_t *audio) {
	if (audio) {
		if (audio->close) {
			audio->close(audio);
		}
		audio->impl = 0;
		if (audio->chnl) {
			audiochannel_destroy(audio->chnl);
		}
		memfree(audio);
	}
}

int audio_play(audio_t *audio) {
	if (audio == 0) {
		return 0;
	}
	int samplesPlayed = 0;
	audiochannel_t *chnl = audio->chnl;
	int samples = audiochannel_samples_available(chnl);

	if (samples == 0) {
		return 0;
	}
	//printf("samples: %d\n", samples);
	waitforit();

	do {
		int send = 0;
		uint8_t *buffer = audiochannel_buffer(chnl, &send);
		waitforit();
		send = audio->fill(audio, buffer, send);
		//printf("send: %d\n", send);
		waitforit();
		if (send < 0) {
			return -1;
		}
		send = audiochannel_submit(chnl, send);
		//printf("send: %d\n", send);
		waitforit();
		if (send < 0) {
			break;
		}
		//data += (cb * bytes);
		samples -= send;
		samplesPlayed += send;
		waitforit();
	} while (samples > 0);

	return samplesPlayed;
}


int audio_init() {
	if (audiochannel_init() < 0) {
		return -1;
	}
	return 0;
}

void audio_exit() {
	audiochannel_exit();
}
