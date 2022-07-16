/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdarg.h>
#include <stdio.h>
#include <xmp.h>
#include "common.h"

#if defined(AMIGA) || defined(__AMIGA__) || defined(__AROS__)
#ifdef __amigaos4__
#define __USE_INLINE__
#endif
#include <proto/exec.h>
#include <proto/dos.h>

#define MODE_NORMAL 0
#define MODE_RAW 1

#elif defined(__NDS__)
#include <nds.h>

static PrintConsole mainConsole, debugConsole;

#elif defined HAVE_TERMIOS_H
#include <termios.h>
#include <unistd.h>

static struct termios term;
#endif

int report(const char *fmt, ...)
{
#ifdef __NDS__
	va_list a;
	int n;

	consoleSelect(&mainConsole);
	va_start(a, fmt);
	n = viprintf(fmt, a);
	va_end(a);
	consoleSelect(&debugConsole);

	return n;
#else
	va_list a;
	int n;

	va_start(a, fmt);
	n = vfprintf(stderr, fmt, a);
	va_end(a);

	return n;
#endif
}

void init_tty(void)
{
#ifdef __NDS__
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&mainConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&debugConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
#endif
}

int set_tty(void)
{
#if defined(AMIGA) || defined(__AMIGA__) || defined(__AROS__)
	SetMode(Input(), MODE_RAW);

#elif defined HAVE_TERMIOS_H
	struct termios t;

	if (!isatty(STDIN_FILENO))
		return 0;
	if (tcgetattr(STDIN_FILENO, &term) < 0)
		return -1;

	t = term;
	t.c_lflag &= ~(ECHO | ICANON | TOSTOP);
	t.c_cc[VMIN] = t.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) < 0)
		return -1;
#endif

	return 0;
}

int reset_tty(void)
{
#if defined(AMIGA) || defined(__AMIGA__) || defined(__AROS__)
	SetMode(Input(), MODE_NORMAL);

#elif defined HAVE_TERMIOS_H
	if (!isatty(STDIN_FILENO))
		return 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
		fprintf(stderr, "can't reset terminal!\n");
		return -1;
	}
#endif

	return 0;
}

