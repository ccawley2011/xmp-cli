/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <nds.h>
#include <stdio.h>
#include "sound.h"

#define MAXBUFFERS	32			/* max number of buffers */
#define BUFFERSIZE	120			/* buffer size in ms */

/* frame rate = (50 * bpm / 125) Hz */
/* frame size = (sampling rate * channels * size) / frame rate */
#define OUT_MAXLEN 0x8000

typedef struct {
	void *data;
	int count;
	int pos;
} Buffer;

static Buffer buffers[MAXBUFFERS];
static volatile int active;
static volatile int currentbuffer;
static int nextbuffer;				/* next buffer to be mixed */
static int num_buffers;

static int format;
static int freq;

static void timerCallback()
{
	Buffer *buffer = &buffers[currentbuffer];

	if (!active)
		return;

	buffer->pos++;

	if (buffer->pos == buffer->count) {
		active = 0;
		currentbuffer++;
		currentbuffer %= num_buffers;
	}
}

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	int i;

	num_buffers = 10;

	parm_init(parm);
	chkparm1("buffers", num_buffers = strtoul(token, NULL, 0));
	parm_end();

	if (num_buffers > MAXBUFFERS)
		num_buffers = MAXBUFFERS;

	soundEnable();

	/* TODO: Support stereo output */
	options->format = (options->format | XMP_FORMAT_MONO) & ~XMP_FORMAT_UNSIGNED;
	format = (options->format & XMP_FORMAT_8BIT) ? SoundFormat_8Bit : SoundFormat_16Bit;
	freq = options->rate;

	for (i = 0; i < num_buffers; i++) {
		buffers[i].data = malloc(OUT_MAXLEN);
		if (!buffers[i].data) {
			return -1;
		}
		buffers[i].count = 0;
		buffers[i].pos = 0;
	}

	currentbuffer = nextbuffer = 0;
	active = 0;

	timerStart(2, ClockDivider_1, TIMER_FREQ(freq), timerCallback);

	return 0;
}

static void play(void *b, int len)
{
	memcpy(buffers[nextbuffer].data, b, len);
	buffers[nextbuffer].count = len;
	buffers[nextbuffer].pos = 0;
	if (format == SoundFormat_16Bit)
		buffers[nextbuffer].count >>= 1;

	while ((nextbuffer + 1) % num_buffers == currentbuffer)
		delay_ms(100);

	nextbuffer++;
	nextbuffer %= num_buffers;

	if (!active) {
		soundPlaySample(buffers[currentbuffer].data, format, buffers[currentbuffer].count, freq, 100, 64, false, 0);
		active = 1;
	}
}

static void deinit(void)
{
	int i;

	timerStop(2);
	soundDisable();

	for (i = 0; i < num_buffers; i++) {
		free(buffers[i].data);
	}
}

static void flush(void)
{
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *const help[] = {
	"buffers=val", "Number of buffers (default 10)",
	NULL
};

struct sound_driver sound_nds = {
	"nds",
	"Nintendo DS sound output",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

