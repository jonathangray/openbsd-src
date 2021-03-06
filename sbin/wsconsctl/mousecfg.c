/* $OpenBSD: mousecfg.c,v 1.2 2017/12/31 09:40:41 anton Exp $ */

/*
 * Copyright (c) 2017 Ulf Brosziewski
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Read/write wsmouse parameters for touchpad configuration.
 */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <dev/wscons/wsconsio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include "mousecfg.h"

#define BASE_FIRST		WSMOUSECFG_DX_SCALE
#define BASE_LAST		WSMOUSECFG_Y_INV
#define TP_FILTER_FIRST		WSMOUSECFG_DX_MAX
#define TP_FILTER_LAST		WSMOUSECFG_SMOOTHING
#define TP_FEATURES_FIRST	WSMOUSECFG_SOFTBUTTONS
#define TP_FEATURES_LAST	WSMOUSECFG_TAPPING
#define TP_SETUP_FIRST		WSMOUSECFG_LEFT_EDGE
#define TP_SETUP_LAST		WSMOUSECFG_TAP_LOCKTIME

#define BASESIZE (BASE_LAST - BASE_FIRST + 1)

#define BUFSIZE (BASESIZE \
    + (TP_FILTER_LAST - TP_FILTER_FIRST + 1) \
    + (TP_FEATURES_LAST - TP_FEATURES_FIRST + 1) \
    + (TP_SETUP_LAST - TP_SETUP_FIRST + 1))

static const int range[][2] = {
	{ BASE_FIRST, BASE_LAST },
	{ TP_FILTER_FIRST, TP_FILTER_LAST },
	{ TP_FEATURES_FIRST, TP_FEATURES_LAST },
	{ TP_SETUP_FIRST, TP_SETUP_LAST },
};

static const int touchpad_types[] = {
	WSMOUSE_TYPE_SYNAPTICS,		/* Synaptics touchpad */
	WSMOUSE_TYPE_ALPS,		/* ALPS touchpad */
	WSMOUSE_TYPE_ELANTECH,		/* Elantech touchpad */
	WSMOUSE_TYPE_SYNAP_SBTN,	/* Synaptics soft buttons */
};

struct wsmouse_parameters cfg_tapping = {
	(struct wsmouse_param[]) {
	    { WSMOUSECFG_TAPPING, 0 }, },
	1
};

struct wsmouse_parameters cfg_scaling = {
	(struct wsmouse_param[]) {
	    { WSMOUSECFG_DX_SCALE, 0 },
	    { WSMOUSECFG_DY_SCALE, 0 } },
	2
};

struct wsmouse_parameters cfg_swapsides = {
	(struct wsmouse_param[]) {
	    { WSMOUSECFG_SWAPSIDES, 0 }, },
	1
};

struct wsmouse_parameters cfg_disable = {
	(struct wsmouse_param[]) {
	    { WSMOUSECFG_DISABLE, 0 }, },
	1
};

struct wsmouse_parameters cfg_param = {
	(struct wsmouse_param[]) {
	    { -1, 0 },
	    { -1, 0 },
	    { -1, 0 },
	    { -1, 0 } },
	4
};

static int cfg_horiz_res;
static int cfg_vert_res;
static struct wsmouse_param cfg_buffer[BUFSIZE];


int
mousecfg_init(int dev_fd, const char **errstr)
{
	struct wsmouse_calibcoords coords;
	struct wsmouse_parameters parameters;
	struct wsmouse_param *param;
	enum wsmousecfg k;
	int i, err, type;

	*errstr = NULL;

	if ((err = ioctl(dev_fd, WSMOUSEIO_GTYPE, &type))) {
		*errstr = "WSMOUSEIO_GTYPE";
		return err;
	}
	for (i = 0; i < nitems(touchpad_types)
	    && type != touchpad_types[i]; i++) {}

	/*
	 * If the device is not a touchpad, return an error without
	 * setting the error string.  The caller shouldn't print a
	 * warning in this case.
	 */
	if (i == nitems(touchpad_types))
		return (-1);

	if ((err = ioctl(dev_fd, WSMOUSEIO_GCALIBCOORDS, &coords))) {
		*errstr = "WSMOUSEIO_GCALIBCOORDS";
		return err;
	}
	cfg_horiz_res = coords.resx;
	cfg_vert_res = coords.resy;

	param = cfg_buffer;
	for (i = 0; i < nitems(range); i++)
		for (k = range[i][0]; k <= range[i][1]; k++, param++) {
			param->key = k;
			param->value = 0;
		}

	/*
	 * Not all touchpad drivers configure wsmouse for compat mode yet.
	 * In those cases the first ioctl call may be successful but the
	 * second one will fail because it includes wstpad parameters:
	 */
	parameters.params = cfg_buffer;
	parameters.nparams = BASESIZE;
	if ((err = ioctl(dev_fd, WSMOUSEIO_GETPARAMS, &parameters))) {
		*errstr = "WSMOUSEIO_GETPARAMS";
		return (err);
	}
	parameters.params = cfg_buffer + BASESIZE;
	parameters.nparams = BUFSIZE - BASESIZE;
	if ((err = ioctl(dev_fd, WSMOUSEIO_GETPARAMS, &parameters))) {
		if (err != EINVAL)
			*errstr = "WSMOUSEIO_GETPARAMS";
		return (err);
	}

	return (0);
}

/* Map a key to its buffer index. */
static int
index_of(enum wsmousecfg key)
{
	int i, n;

	for (i = 0, n = 0; i < nitems(range); i++)
		if (key <= range[i][1] && key >= range[i][0])
			return (key - range[i][0] + n);
		else
			n += range[i][1] - range[i][0] + 1;

	return (-1);
}

int
mousecfg_get_field(struct wsmouse_parameters *field)
{
	int i, n;

	for (i = 0; i < field->nparams; i++) {
		if ((n = index_of(field->params[i].key)) >= 0)
			field->params[i].value = cfg_buffer[n].value;
		else
			return (-1);
	}
	return (0);
}

int
mousecfg_put_field(int fd, struct wsmouse_parameters *field)
{
	int i, n, d, err;

	d = 0;
	for (i = 0; i < field->nparams; i++)
		if ((n = index_of(field->params[i].key)) < 0)
			return (-1);
		else
			d |= (cfg_buffer[n].value != field->params[i].value);

	if (!d)
		return (0);

	/* Write and read back immediately, wsmouse may normalize values. */
	if ((err = ioctl(fd, WSMOUSEIO_SETPARAMS, field))
	    || (err = ioctl(fd, WSMOUSEIO_GETPARAMS, field)))
		return err;

	for (i = 0; i < field->nparams; i++)
		cfg_buffer[n].value = field->params[i].value;

	return (0);
}

static int
get_value(struct wsmouse_parameters *field, enum wsmousecfg key)
{
	int i;

	for (i = 0; i < field->nparams && key != field->params[i].key; i++) {}

	return (i < field->nparams ? field->params[i].value : 0);
}

static void
set_value(struct wsmouse_parameters *field, enum wsmousecfg key, int value)
{
	int i;

	for (i = 0; i < field->nparams && key != field->params[i].key; i++) {}

	field->params[i].value = (i < field->nparams ? value : 0);
}

/*
 * Read or write up to four raw parameter values.  In this case
 * reading is a 'put' operation that writes back a value from the
 * buffer.
 */
static int
read_param(struct wsmouse_parameters *field, char *val)
{
	int i, j, n;

	n = sscanf(val, "%d:%d,%d:%d,%d:%d,%d:%d",
		&field->params[0].key, &field->params[0].value,
		&field->params[1].key, &field->params[1].value,
		&field->params[2].key, &field->params[2].value,
		&field->params[3].key, &field->params[3].value);
	if (n > 0 && (n & 1) == 0) {
		n /= 2;
		for (i = 0; i < n; i++) {
			if (index_of(field->params[i].key) < 0)
				return (-1);
		}
		field->nparams = n;
		return (0);
	}
	n = sscanf(val, "%d,%d,%d,%d",
		&field->params[0].key, &field->params[1].key,
		&field->params[2].key, &field->params[3].key);
	if (n > 0) {
		for (i = 0; i < n; i++) {
			if ((j = index_of(field->params[i].key)) < 0)
				return (-1);
			field->params[i].value = cfg_buffer[j].value;
		}
		field->nparams = n;
		return (0);
	}
	return (-1);
}

void
mousecfg_pr_field(struct wsmouse_parameters *field)
{
	int i, value;
	float f;

	if (field == &cfg_param) {
		for (i = 0; i < field->nparams; i++)
			printf(i > 0 ? ",%d:%d" : "%d:%d",
			    field->params[i].key,
			    field->params[i].value);
		return;
	}

	if (field == &cfg_scaling) {
		value = get_value(field, WSMOUSECFG_DX_SCALE);
		f = (float) value / 4096;
		printf("%.3f", f);
		return;
	}

	for (i = 0; i < field->nparams; i++)
		printf(i > 0 ? ",%d" : "%d", field->params[i].value);
}

void
mousecfg_rd_field(struct wsmouse_parameters *field, char *val)
{
	int i, n;
	const char *s;
	float f;

	if (field == &cfg_param) {
		if (read_param(field, val))
			errx(1, "invalid input (param)");
		return;
	}

	if (field == &cfg_scaling) {
		if (sscanf(val, "%f", &f) == 1) {
			n = (int) (f * 4096);
			set_value(field, WSMOUSECFG_DX_SCALE, n);
			if (cfg_horiz_res && cfg_vert_res)
				n = n * cfg_horiz_res / cfg_vert_res;
			set_value(field, WSMOUSECFG_DY_SCALE, n);
		} else {
			errx(1, "invalid input (scaling)");
		}
		return;
	}

	s = val;
	for (i = 0; i < field->nparams; i++) {
		if (sscanf(s, (i > 0 ? ",%d" : "%d"), &n) != 1)
			break;
		field->params[i].value = abs(n);
		for (s++; *s != '\0' && *s != ','; s++) {}
	}
	if (i < field->nparams || *s != '\0')
		errx(1, "invalid input '%s'", val);
}
