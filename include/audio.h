#pragma once

#include <stdint.h>

#define AUDIO_ID 0xfade0003L

typedef struct audio_s audio_t;
typedef struct audiochannel_s audiochannel_t;

typedef int (*audio_init_func)(audio_t *audio);
typedef int (*audio_fill_func)(audio_t *audio, uint8_t *buffer, int samples);
typedef void(*audio_close_func)(audio_t *audio);


typedef struct audio_api_s {
	audio_init_func init;
	audio_fill_func fill;
	audio_close_func close;
} audio_api_t;

typedef struct audio_s {
	int id;

	audio_init_func init;
	audio_fill_func fill;
	audio_close_func close;

	//format
	int snd_channels;
	int snd_samplebits;
	int snd_speed;
	int snd_sample_bytes;

	audiochannel_t *chnl;

	int wav_buffer_size;

	void *impl;
} audio_t;


int audio_init();
void audio_exit();

audio_t *audio_create(audio_api_t api, void *impl);
void audio_destroy(audio_t *audio);

int audio_play(audio_t *audio);
