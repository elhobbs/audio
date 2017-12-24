#pragma once
#ifdef WIN32
#include "win32/audiochannel_win32.h"
#endif
#ifdef _3DS
#include "3ds/audiochannel_3ds.h"
#endif

#define AUDIOCHANNEL_ID 0xfade0005L

audiochannel_t *audiochannel_create(audio_t *audio);
void audiochannel_destroy(audiochannel_t *chnl);
int audiochannel_samples_available(audiochannel_t *chnl);
uint8_t *audiochannel_buffer(audiochannel_t *chnl, int *samples);
int audiochannel_submit(audiochannel_t *chnl, int samples);

int audiochannel_chnl_init(audio_t *audio, audiochannel_t *chnl);
void audiochannel_chnl_close(audiochannel_t *chnl);

int audiochannel_init();
void audiochannel_exit();

