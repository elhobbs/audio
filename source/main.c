#include "common.h"
#include <stdio.h>
#include "filestrm.h"
#include "audio.h"
#include "wavaudio.h"
#include "mp3audio.h"
#include "oggaudio.h"

#ifdef WIN32
#include <Windows.h>

static HANDLE hStdin;
static DWORD fdwSaveOldMode;

VOID ErrorExit(LPSTR lpszMessage)
{
	fprintf(stderr, "%s\n", lpszMessage);

	// Restore input mode on exit.

	SetConsoleMode(hStdin, fdwSaveOldMode);

	ExitProcess(0);
}

void sys_init() {
	DWORD  fdwMode;

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
		ErrorExit("GetStdHandle");

	// Save the current input mode, to be restored on exit. 

	if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
		ErrorExit("GetConsoleMode");
	
	fdwMode = ENABLE_WINDOW_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode))
		ErrorExit("SetConsoleMode");
}

void sys_exit() {
	SetConsoleMode(hStdin, fdwSaveOldMode);
}

void frame_start() {
}

void frame_end() {
}

#endif

#ifdef _3DS
#include <3ds.h>

static int tick_row = 0;
#define TICKS_PER_MSEC 268111.856

static int tick_heights[400] = { 0 };

void display(u64 diff) {
	int i, j;
	int scale = 8;
	u8 *buffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, 0, 0);

	int height = (scale *(((double)diff) / TICKS_PER_MSEC));
	if (height > 240) {
		height = 240;
	}
	tick_heights[tick_row] = height;

	for (j = 0; j < 400; j++) {
		int b = 255, g = 0, r = 0;
		int ofs = ((400 + tick_row) - j) % 400;

		if (ofs < 32) {
			b = ofs * 8;
			g = 255 - ofs * 8;
		}
		height = tick_heights[j];
		u8 *row = buffer + j * 240 * 3;
		for (i = 0; i < height; i++) {
			*row++ = b;
			*row++ = g;
			*row++ = r;
		}
		for (; i < 240; i++) {
			*row++ = 0;
			*row++ = 0;
			*row++ = 0;
		}
		row = buffer + (j * 240 + (16 * scale)) * 3;
		*row++ = 0;
		*row++ = 0;
		*row++ = 255;
	}

	tick_row++;
	if (tick_row >= 400) {
		tick_row = 0;
	}
}

static u32 old_time_limit;

void sys_init() {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	osSetSpeedupEnable(true);

	APT_GetAppCpuTimeLimit(&old_time_limit);
	//printf("time limit: %d\n", (int)old_time_limit);

	Result cpuRes = APT_SetAppCpuTimeLimit(30);
	if (R_FAILED(cpuRes)) {
		printf("Failed to set syscore CPU time limit: %08lX", cpuRes);
	}
}

void sys_exit() {
	Result cpuRes = APT_SetAppCpuTimeLimit(old_time_limit);
	if (R_FAILED(cpuRes)) {
		printf("Failed to reset syscore CPU time limit: %08lX", cpuRes);
	}
	osSetSpeedupEnable(false);
	gfxExit();
}


void frame_start() {
	gspWaitForVBlank();
	hidScanInput();
}

void frame_end() {
	gfxFlushBuffers();
	gfxSwapBuffers();
}
#endif

#ifdef WIN32
int aptMainLoop() {
	return 1;
}
#endif

int main(int argc, char **argv) {
	int cb;
	int i = 0;
	int total = 0;
	int file_index = 0;
	int next_song = 0;

#ifdef WIN32
	char name[][100] = {
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test4.ogg",
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test4.wav",
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test4.mp3",
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test5.mp3",
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test6.mp3",
		"C:\\Users\\elhobbs\\AppData\\Roaming\\Citra\\sdmc\\test7.mp3"
	};
	//char name[] = "C:\\Data\\quake\\id1\\music\\02.mp3";
	DWORD cNumRead, j;
	INPUT_RECORD irInBuf[128]; 
#endif

#ifdef _3DS
	char name[][100] = {
		"/test4.ogg",
		"/test4.wav",
		"/test4.mp3",
		"/test5.mp3",
		"/test6.mp3",
		"/test7.mp3"
	};
	u64 last = svcGetSystemTick(), current;
#endif

	sys_init();

	audio_init();


#if 0
	void *alac_decode_init(strm_t *strm);

	strm_t *strm0 = filestrm_create("C:\\Users\\elhob\\Desktop\\1 Sullivan The Lost Chord, Seated one day at the organ.M4A");
	alac_decode_init(strm0);
#endif


	strm_t *strm = filestrm_create(name[0]);
	audio_t *audio = oggaudio_create(strm);

	while(aptMainLoop()){

		//if (audio == 0) {
		//	break;
		//}


		frame_start();

		//printf("frame: %d\n", i);

#ifdef _3DS
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
		if (kDown & KEY_A) {
			next_song = 1;
		}
		last = svcGetSystemTick();
#endif

#ifdef WIN32
		if (!PeekConsoleInput(
			hStdin,      // input buffer handle 
			irInBuf,     // buffer to read into 
			128,         // size of read buffer 
			&cNumRead)) // number of records read 
			ErrorExit("PeekConsoleInput");
		if (cNumRead > 0) {
			if (!ReadConsoleInput(
				hStdin,      // input buffer handle 
				irInBuf,     // buffer to read into 
				128,         // size of read buffer 
				&cNumRead)) // number of records read 
				ErrorExit("ReadConsoleInput");

			static int a_down = 0;
			for (j = 0; j < cNumRead; j++) {
				switch (irInBuf[j].EventType) {
				case KEY_EVENT:
						printf("key: %d %d\n", irInBuf[j].Event.KeyEvent.wVirtualKeyCode, irInBuf[j].Event.KeyEvent.bKeyDown);
					if (irInBuf[j].Event.KeyEvent.bKeyDown &&
						irInBuf[j].Event.KeyEvent.wVirtualKeyCode == 81) {
						goto done;
					}
					if (irInBuf[j].Event.KeyEvent.wVirtualKeyCode == 65) {
						if (irInBuf[j].Event.KeyEvent.bKeyDown && a_down == 0) {
							next_song = 1;
							a_down = 1;
						}
						if (irInBuf[j].Event.KeyEvent.bKeyDown == 0) {
							a_down = 0;
						}
					}
					break;
				default:
					break;
				}
			}
		}
#endif

		if (next_song) {
			file_index++;
			if (file_index >= 6) {
				file_index = 0;
			}
			audio_destroy(audio);
			printf("name: %s\n", name[file_index]);
			strm = filestrm_create(name[file_index]);
			//while (1);
			switch (file_index) {
			case 0:
				audio = oggaudio_create(strm);
				break;
			case 1:
				audio = wavaudio_create(strm);
				break;
			default:
				audio = mp3audio_create(strm);
				break;
			}
			next_song = 0;
		}

		cb = audio_play(audio);
		if (cb < 0) {
			audio_destroy(audio);
			audio = 0;
		}
#ifdef _3DS
		current = svcGetSystemTick();
		display(current - last);
#endif
		total += cb;
		i++;

		frame_end();
	}
#ifdef WIN32
	done:
#endif


	printf("total: %d\n", total);
	audio_destroy(audio);

	audio_exit();

	sys_exit();

	return 0;
}