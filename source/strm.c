#include "common.h"
#include "strm.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

strm_t *strm_create(strm_api_t api, void *impl) {
	strm_t *strm = (strm_t *)memalloc( 1, sizeof(*strm));
	uint8_t *buffer = (uint8_t *)memalloc( 1, 4096);
	if (strm) {
		strm->id = STRM_ID;
		strm->read = api.read;
		strm->seek = api.seek;
		strm->tell = api.tell;
		strm->close = api.close;
		strm->buffer = buffer;
		strm->impl = impl;
	}
	return strm;
}

void strm_destroy(strm_t *strm) {
	if (strm) {
		if (strm->close) {
			strm->close(strm);
		}
		if (strm->buffer) {
			memfree(strm->buffer);
		}
		memfree(strm);
	}
}

uint8_t* strm_read(strm_t *strm, int *numBytes) {
	if (strm == 0) {
		return 0;
	}
	int cb = *numBytes;
	if (cb < 0 || cb > 4096) {
		return 0;
	}
	do {
		if (strm->bytesRemaining >= cb) {
			uint8_t *buffer = strm->readPtr;
			strm->readPtr += cb;
			strm->bytesRemaining -= cb;
			return buffer;
		}
		if (strm->read) {
			int cbNeed = 4096;
			if (strm->bytesRemaining > 0) {
				memmove(strm->buffer, strm->readPtr, strm->bytesRemaining);
				cbNeed -= strm->bytesRemaining;
				strm->readPtr = strm->buffer + strm->bytesRemaining;
			}
			else {
				strm->readPtr = strm->buffer;
			}
			int cbRead = strm->read(strm, strm->readPtr, cbNeed);
			if (cbRead <= 0) {
				break;
			}
			strm->bytesRemaining += cbRead;
		}
		else {
			break;
		}
	} while (1);

	if (strm->bytesRemaining > 0) {
		uint8_t *buffer = strm->readPtr;
		*numBytes = strm->bytesRemaining;
		strm->readPtr  = strm->buffer;
		strm->bytesRemaining = 0;
		return buffer;
	}

	return 0;
}

int strm_fill(strm_t *strm, uint8_t *buffer, int numBytes) {
	if (strm == 0 || buffer == 0 || numBytes <= 0) {
		return -1;
	}
	int cbFilled = 0;
	int cbRead;
	if (strm->bytesRemaining > 0) {
		cbRead = numBytes;
		if (cbRead > strm->bytesRemaining) {
			cbRead = strm->bytesRemaining;
		}
		memcpy(buffer, strm->readPtr, cbRead);
		numBytes -= cbRead;
		strm->bytesRemaining -= cbRead;
		buffer += cbRead;
		cbFilled += cbRead;
	}
	if (numBytes) {
		do {
			if (strm->read) {
				cbRead = strm->read(strm, buffer, numBytes);
				if (cbRead <= 0) {
					break;
				}
				buffer += cbRead;
				numBytes -= cbRead;
				cbFilled += cbRead;
			}
			else {
				break;
			}
		} while (numBytes > 0);
	}

	return cbFilled;
}


int strm_seek(strm_t *strm, int offset, int origin) {
	if (strm && strm->seek) {
		switch (origin) {
		case SEEK_CUR:
			if (offset > 0 && offset < strm->bytesRemaining) {
				strm->readPtr += offset;
				strm->bytesRemaining -= offset;
				return 0;
			}
		default:
			strm->bytesRemaining = 0;
			break;
		}
		return strm->seek(strm, offset, origin);
	}
	return -1;
}
int strm_tell(strm_t *strm) {
	if (strm && strm->tell) {
		return strm->tell(strm) - strm->bytesRemaining;
	}
	return -1;
}
