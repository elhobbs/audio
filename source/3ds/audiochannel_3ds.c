#include "common.h"
#include "audiochannel.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <3ds/svc.h>

static int ndsp_initialized = 0;
static float gMix[12];


int audiochannel_samples_available(audiochannel_t *chnl) {
	if (chnl == 0) {
		return -1;
	}

	while (1)
	{
		if (chnl->snd_completed == chnl->snd_sent)
		{
			//printf("Sound overrun\n");
			break;
		}

		if (chnl->gWavebuf[chnl->snd_completed & WAV_MASK].status != NDSP_WBUF_DONE) {
			break;
		}

		chnl->snd_completed++;	// this buffer has been played
	}

	int buffers = 80 - (chnl->snd_sent - chnl->snd_completed);
	if (buffers < 0) {
		return 0;
	}
	int samples = buffers * chnl->wav_buffer_size / chnl->bytesPerSample;

	return samples;
}

uint8_t *audiochannel_buffer(audiochannel_t *chnl, int *samples) {
	if (chnl == 0 || samples == 0) {
		return 0;
	}
	*samples = chnl->wav_buffer_size / chnl->bytesPerSample;
	ndspWaveBuf *wbuf = chnl->gWavebuf + (chnl->snd_sent&WAV_MASK);

	return wbuf->data_adpcm;
}

int audiochannel_submit(audiochannel_t *chnl, int samples) {
	ndspWaveBuf *wbuf = chnl->gWavebuf + (chnl->snd_sent&WAV_MASK);
	DSP_FlushDataCache(wbuf->data_vaddr, chnl->wav_buffer_size);
	
	ndspChnWaveBufAdd(chnl->channel, wbuf);
	chnl->snd_sent++;
	//printf("submit: %d\n", chnl->snd_sent);

	return samples;
}

static int next_channel = 0;
static int pickChannel() {
	//TODO find better channel
	int channel = next_channel++;
	if (next_channel > 23) {
		next_channel = 0;
	}
	return channel;
}

static int getFormat(audio_t *audio) {
	int format = 0;

	switch (audio->snd_channels) {
	case 1:
	case 2:
		format |= NDSP_CHANNELS(audio->snd_channels);
		break;
	}
	switch (audio->snd_samplebits) {
	case 8:
		format |= NDSP_ENCODING(NDSP_ENCODING_PCM8);
		break;
	case 16:
		format |= NDSP_ENCODING(NDSP_ENCODING_PCM16);
		break;
	}

	return format;
}

int audiochannel_chnl_init(audio_t *audio, audiochannel_t *chnl) {
	if (audio == 0 || chnl == 0) {
		return -1;
	}
	int bufferSize = WAV_BUFFERS*chnl->wav_buffer_size;
	chnl->buffer = linearAlloc(bufferSize);
	if (chnl->buffer) {
		chnl->bufferSize = bufferSize;
		memset(chnl->gWavebuf, 0, sizeof(chnl->gWavebuf));
		DSP_FlushDataCache(chnl->buffer, chnl->bufferSize);
	}


	chnl->bytesPerSample = audio->snd_sample_bytes;

	int format = getFormat(audio);
	int channel = pickChannel();

	ndspChnSetInterp(channel, NDSP_INTERP_NONE);
	ndspChnSetRate(channel, (float)audio->snd_speed);
	ndspChnSetFormat(channel, format);
	ndspChnSetMix(channel, gMix);

	chnl->channel = channel;
	printf("start: %d %d %d %d\n", format, audio->snd_channels, audio->snd_speed, audio->snd_samplebits);
	//while (1);

	chnl->bytesPerSample = audio->snd_sample_bytes;
	chnl->wav_buffer_size = audio->wav_buffer_size;

	int samples = chnl->wav_buffer_size / chnl->bytesPerSample;
	int i;
	for (i = 0; i<WAV_BUFFERS; i++)
	{
		chnl->gWavebuf[i].nsamples = samples;
		chnl->gWavebuf[i].looping = 0;
		chnl->gWavebuf[i].status = NDSP_WBUF_FREE;
		chnl->gWavebuf[i].data_pcm8 = ((int8_t *)(chnl->buffer)) + i*chnl->wav_buffer_size;
	}


	return 0;
}


void audiochannel_chnl_close(audiochannel_t *chnl) {
	if (chnl == 0) {
		return;
	}
	svcSleepThread(20000ULL);
	ndspChnWaveBufClear(chnl->channel);
	do {
		svcSleepThread(20000ULL);
		if (!ndspChnIsPlaying(chnl->channel)) {
			break;
		}
		printf("I need some more time\n");
	} while (1);

	if (chnl->buffer) {
		linearFree(chnl->buffer);
		chnl->buffer = 0;
	}
}

int audiochannel_init() {
	if (ndsp_initialized != 0) {
		return 0;
	}
	ndsp_initialized++;
	ndspInit();
	gMix[0] = 1.0f;
	gMix[1] = 1.0f;

	return 0;
}

void audiochannel_exit() {
	if (ndsp_initialized == 0) {
		return;
	}
	ndsp_initialized--;
	ndspExit();
}
