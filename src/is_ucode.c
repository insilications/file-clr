/*
 * Copyright (c) Ian F. Darwin 1986-1995.
 * Software written by Ian F. Darwin and others;
 * maintained 1995-present by Christos Zoulas and others.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * based on is_tar.c
 *
 *
 */

#include "file.h"

#ifndef lint
FILE_RCSID("@(#)$File: is_ucode.c,v $")
#endif

#include "magic.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h>

struct header {
	uint32_t	header_version;
	uint32_t	update_version;
	uint16_t 	year;
	uint8_t		month;
	uint8_t		day;
	uint32_t	proc_sig;
	uint32_t	checksum;
	uint32_t	loader_version;
};

private int family, model, stepping, year, month, day;

private int decode_year(uint32_t _year)
{
	int y;
	uint32_t x;

	x = _year & 0xF;
	if (x > 9)
		return 0;
	y = x;
	_year = _year >> 4;

	x = _year & 0xF;
	if (x > 9)
		return 0;
	y += x * 10;
	_year = _year >> 4;

	x = _year & 0xF;
	if (x > 9)
		return 0;
	y += x * 100;
	_year = _year >> 4;

	x = _year & 0xF;
	if (x > 9)
		return 0;
	y += x * 1000;

	if (y < 2007 || x > 2020)
		return 0;
	return y;
}

private int decode_month(uint32_t _month)
{
	int y;
	uint32_t x;

	x = _month & 0xF;
	if (x > 9)
		return 0;
	y = x;
	_month = _month >> 4;

	x = _month & 0xF;
	if (x > 9)
		return 0;
	y += x * 10;

	if (y < 1 || x > 12)
		return 0;

	return y;
}


private int decode_day(uint32_t _day)
{
	int y;
	uint32_t x;

	x = _day & 0xF;
	if (x > 9)
		return 0;
	y = x;
	_day = _day >> 4;

	x = _day & 0xF;
	if (x > 9)
		return 0;
	y += x * 10;

	if (y < 1 || x > 31)
		return 0;

	return y;
}

private void decode_fms(uint32_t sig)
{

	family = (sig >> 8) & 0xf;

	if (family == 0xf) {
		family += (sig >> 20) & 0xff;
	}

	model = (sig >> 4) & 0xf;

	if (family >= 0x6) {
		model += ((sig >> 16) & 0xf) << 4;
	}

	stepping = sig & 0xf;
}

private int is_valid_header(struct header *h)
{
	if (h->header_version != 0x1)
		return 0;

	if (h->loader_version != 0x1)
		return 0;


	year = decode_year(h->year);
	month = decode_month(h->month);
	day = decode_day(h->day);

	if (year == 0 || month == 0 || day == 0)
		return 0;

	decode_fms(h->proc_sig);

	if (family != 6)
		return 0;

	return 1;
}


protected int
file_is_ucode(struct magic_set *ms, const struct buffer *b)
{
	char ucode_string[4096];
	struct header header;
	int mime = ms->flags & MAGIC_MIME;

	if ((ms->flags & (MAGIC_APPLE|MAGIC_EXTENSION)) != 0)
		return 0;

	if (b->flen < sizeof(struct header))
		return 0;

	memcpy(&header, b->fbuf, sizeof(struct header));

	if (!is_valid_header(&header))
		return 0;

	sprintf(ucode_string, "CPU microcode for f/m/s %i/%i/%i version 0x%02x (%04i/%02i/%02i)",
			family, model, stepping, header.update_version, year, month, day );

	if (file_printf(ms, "%s", mime ? "application/x-intel-ucode" :
	    ucode_string) == -1)
		return -1;
	return 1;
}
