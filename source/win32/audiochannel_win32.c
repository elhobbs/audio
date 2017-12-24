#include "audiochannel.h"
#include <malloc.h>

int audiochannel_chnl_init(audio_t *audio, audiochannel_t *chnl) {
	if (audio == 0 || chnl == 0) {
		return -1;
	}
	PaStreamParameters outputParameters;

	/* -- setup input  -- */
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	outputParameters.channelCount = audio->snd_channels;
	outputParameters.sampleFormat = audio->snd_samplebits == 16 ? paInt16 : paInt8;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	/* -- setup stream -- */
	PaError err = Pa_OpenStream(
		&chnl->stream,
		0,
		&outputParameters,
		audio->snd_speed,
		(1152),
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		NULL, /* no callback, use blocking API */
		NULL); /* no callback, so no callback userData */
	if (err != paNoError) goto error;

	err = Pa_StartStream(chnl->stream);
	if (err != paNoError) goto error;

	chnl->bytesPerSample = audio->snd_sample_bytes;
	chnl->wav_buffer_size = audio->wav_buffer_size;

	return 0;
error:
	return -1;
}

int audiochannel_samples_available(audiochannel_t *chnl) {
	if (chnl == 0 || chnl->stream == 0) {
		return -1;
	}

	return Pa_GetStreamWriteAvailable(chnl->stream);
}

uint8_t *audiochannel_buffer(audiochannel_t *chnl, int *samples) {
	if (chnl == 0 || samples == 0) {
		return 0;
	}
	*samples = AUDIOCHANNEL_BUFFER_SIZE / chnl->bytesPerSample;
	return chnl->buffer;
}

int audiochannel_submit(audiochannel_t *chnl, int samples) {
	PaError err = Pa_WriteStream(chnl->stream, chnl->buffer, samples);
	return samples;
}

void audiochannel_chnl_close(audiochannel_t *chnl) {
	if (chnl->stream) {
		Pa_CloseStream(chnl->stream);
	}
}

int audiochannel_init() {
	/* -- initialize PortAudio -- */
	PaError err = Pa_Initialize();
	if (err != paNoError) goto error;
	return 0;
error:
	return -1;

}
void audiochannel_exit() {
	Pa_Terminate();
}
