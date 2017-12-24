#pragma once

#include "audio.h"
#include <3ds/types.h>
#include <3ds/allocator/linear.h>
#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h>
#include <3ds/services/dsp.h>

#define	WAV_BUFFERS				128
#define	WAV_MASK				0x7F
//#define	WAV_BUFFER_SIZE			0x0400

typedef struct audiochannel_s {
	int id;
	int bytesPerSample;

	//playback buffers
	int bufferSize;
	void *buffer;
	int channel;
	int wav_buffer_size;
	ndspWaveBuf gWavebuf[WAV_BUFFERS];

	//playback position
	int snd_sent, snd_completed;

} audiochannel_t;



