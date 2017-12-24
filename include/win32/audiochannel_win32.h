#pragma once
#include <portaudio.h>
#include "audio.h"

#define AUDIOCHANNEL_BUFFER_SIZE 4096

typedef struct audiochannel_s {
	int id;
	int bytesPerSample;
	int wav_buffer_size;
	uint8_t buffer[AUDIOCHANNEL_BUFFER_SIZE];
	PaStream *stream;
} audiochannel_t;

