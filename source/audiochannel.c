#include "common.h"
#include "audiochannel.h"
#include <malloc.h>

audiochannel_t *audiochannel_create(audio_t *audio) {
	audiochannel_t *chnl = (audiochannel_t *)memalloc(1, sizeof(*chnl));
	if (chnl == 0) {
		return 0;
	}
	chnl->id = AUDIOCHANNEL_ID;
	if (audiochannel_chnl_init(audio, chnl) < 0) {
		audiochannel_destroy(chnl);
		return 0;
	}
	return chnl;
}

void audiochannel_destroy(audiochannel_t *chnl) {
	if (chnl == 0) {
		return;
	}

	audiochannel_chnl_close(chnl);

	memfree(chnl);
}
