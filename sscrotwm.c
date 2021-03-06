/*
 * Copyright (c) 2012 Jacob Courtneay <jacob@sporkexec.com>
 * Copyright (c) 2009-2012 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2009-2011 Ryan McBride <mcbride@countersiege.com>
 * Copyright (c) 2009 Darrin Chandler <dwchandler@stilyagin.com>
 * Copyright (c) 2009 Pierre-Yves Ritschard <pyr@spootnik.org>
 * Copyright (c) 2010 Tuukka Kataja <stuge@xor.fi>
 * Copyright (c) 2011 Jason L. Wright <jason@thought.net>
 * Copyright (c) 2011-2012 Reginald Kennedy <rk@rejii.com>
 * Copyright (c) 2011-2012 Lawrence Teo <lteo@lteo.net>
 * Copyright (c) 2011-2012 Tiago Cunha <tcunha@gmx.com>
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
 * Much code and ideas taken from dwm under the following license:
 * MIT/X Consortium License
 *
 * 2006-2008 Anselm R Garbe <garbeam at gmail dot com>
 * 2006-2007 Sander van Dijk <a dot h dot vandijk at gmail dot com>
 * 2006-2007 Jukka Salmi <jukka at salmi dot ch>
 * 2007 Premysl Hruby <dfenze at gmail dot com>
 * 2007 Szabolcs Nagy <nszabolcs at gmail dot com>
 * 2007 Christof Musik <christof at sendfax dot de>
 * 2007-2008 Enno Gottox Boland <gottox at s01 dot de>
 * 2007-2008 Peter Hartlich <sgkkr at hartlich dot com>
 * 2008 Martin Hurton <martin dot hurton at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <util.h>
#include <pwd.h>
#include <paths.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/select.h>
#if defined(__linux__)
#include "tree.h"
#elif defined(__OpenBSD__)
#include <sys/tree.h>
#elif defined(__FreeBSD__)
#include <sys/tree.h>
#else
#include "tree.h"
#endif

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#ifdef __OSX__
#include <osx.h>
#endif

#include "version.h"

#ifdef SSCROTWM_BUILDSTR
static const char	*buildstr = SSCROTWM_BUILDSTR;
#else
static const char	*buildstr = SSCROTWM_VERSION;
#endif

#if RANDR_MAJOR < 1
#  error XRandR versions less than 1.0 are not supported
#endif

#if RANDR_MAJOR >= 1
#if RANDR_MINOR >= 2
#define SWM_XRR_HAS_CRTC
#endif
#endif

/*#define SWM_DEBUG*/
#ifdef SWM_DEBUG
#define DPRINTF(x...)		do { if (swm_debug) fprintf(stderr, x); } while (0)
#define DNPRINTF(n,x...)	do { if (swm_debug & n) fprintf(stderr, x); } while (0)
#define SWM_D_MISC		0x0001
#define SWM_D_EVENT		0x0002
#define SWM_D_WS		0x0004
#define SWM_D_FOCUS		0x0008
#define SWM_D_MOVE		0x0010
#define SWM_D_STACK		0x0020
#define SWM_D_MOUSE		0x0040
#define SWM_D_PROP		0x0080
#define SWM_D_CLASS		0x0100
#define SWM_D_KEY		0x0200
#define SWM_D_QUIRK		0x0400
#define SWM_D_SPAWN		0x0800
#define SWM_D_EVENTQ		0x1000
#define SWM_D_CONF		0x2000

u_int32_t		swm_debug = 0
			    | SWM_D_MISC
			    | SWM_D_EVENT
			    | SWM_D_WS
			    | SWM_D_FOCUS
			    | SWM_D_MOVE
			    | SWM_D_STACK
			    | SWM_D_MOUSE
			    | SWM_D_PROP
			    | SWM_D_CLASS
			    | SWM_D_KEY
			    | SWM_D_QUIRK
			    | SWM_D_SPAWN
			    | SWM_D_EVENTQ
			    | SWM_D_CONF
			    ;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define LENGTH(x)		(sizeof x / sizeof x[0])
#define MODKEY			Mod1Mask
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define BUTTONMASK		(ButtonPressMask|ButtonReleaseMask)
#define MOUSEMASK		(BUTTONMASK|PointerMotionMask)
#define SWM_PROPLEN		(16)
#define SWM_FUNCNAME_LEN	(32)
#define SWM_KEYS_LEN		(255)
#define SWM_QUIRK_LEN		(64)
#define X(r)			(r)->g.x
#define Y(r)			(r)->g.y
#define WIDTH(r)		(r)->g.w
#define HEIGHT(r)		(r)->g.h
#define SH_MIN(w)		(w)->sh_mask & PMinSize
#define SH_MIN_W(w)		(w)->sh.min_width
#define SH_MIN_H(w)		(w)->sh.min_height
#define SH_MAX(w)		(w)->sh_mask & PMaxSize
#define SH_MAX_W(w)		(w)->sh.max_width
#define SH_MAX_H(w)		(w)->sh.max_height
#define SH_INC(w)		(w)->sh_mask & PResizeInc
#define SH_INC_W(w)		(w)->sh.width_inc
#define SH_INC_H(w)		(w)->sh.height_inc
#define WINID(w)		((w) ? (w)->id : 0)
#define YESNO(x)		((x) ? "yes" : "no")

char			**start_argv;
Atom			astate;
Atom			aprot;
Atom			adelete;
Atom			takefocus;
Atom			a_wmname;
Atom			a_netwmname;
Atom			a_utf8_string;
Atom			a_string;
volatile sig_atomic_t   running = 1;
volatile sig_atomic_t   restart_wm = 0;
int			outputs = 0;
int			(*xerrorxlib)(Display *, XErrorEvent *);
int			other_wm;
int			xrandr_support;
int			xrandr_eventbase;
unsigned int		numlockmask = 0;
Display			*display;


/* configurable vars */
unsigned int		mod_key = MODKEY;
double			dialog_ratio = 0.6;
int			disable_border = 0;
int			border_width = 1;

struct passwd		*pwd;

/* layout manager data */
struct swm_geometry {
	int			x;
	int			y;
	int			w;
	int			h;
};

struct swm_screen;
struct workspace;

/* virtual "screens" */
struct swm_region {
	TAILQ_ENTRY(swm_region)	entry;
	struct swm_geometry	g;
	struct workspace	*ws;	/* current workspace on this region */
	struct workspace	*ws_prior; /* prior workspace on this region */
	struct swm_screen	*s;	/* screen idx */
};
TAILQ_HEAD(swm_region_list, swm_region);

struct ws_win {
	TAILQ_ENTRY(ws_win)	entry;
	Window			id;
	Window			transient;
	struct ws_win		*child_trans;	/* transient child window */
	struct swm_geometry	g;		/* current geometry */
	struct swm_geometry	g_float;	/* geometry when floating */
	struct swm_geometry	rg_float;	/* region geom when floating */
	int			g_floatvalid;	/* g_float geometry validity */
	int			floatmaxed;	/* whether maxed by max_stack */
	int			floating;
	int			manual;
	unsigned int		ewmh_flags;
	int			can_delete;
	int			take_focus;
	unsigned long		quirks;
	struct workspace	*ws;	/* always valid */
	struct swm_screen	*s;	/* always valid, never changes */
	XWindowAttributes	wa;
	XSizeHints		sh;
	long			sh_mask;
	XClassHint		ch;
	XWMHints		*hints;
};
TAILQ_HEAD(ws_win_list, ws_win);

/* pid goo */
struct pid_e {
	TAILQ_ENTRY(pid_e)	entry;
	long			pid;
	int			ws;
};
TAILQ_HEAD(pid_list, pid_e);
struct pid_list			pidlist = TAILQ_HEAD_INITIALIZER(pidlist);

/* layout handlers */
void	stack(void);
void	vertical_config(struct workspace *, int);
void	vertical_stack(struct workspace *, struct swm_geometry *);
void	horizontal_config(struct workspace *, int);
void	horizontal_stack(struct workspace *, struct swm_geometry *);
void	max_stack(struct workspace *, struct swm_geometry *);

struct ws_win *find_window(Window);

void	grabbuttons(struct ws_win *, int);
void	new_region(struct swm_screen *, int, int, int, int);
void	unmanage_window(struct ws_win *);
long	getstate(Window);

int	conf_load(char *);

struct layout {
	void		(*l_stack)(struct workspace *, struct swm_geometry *);
	void		(*l_config)(struct workspace *, int);
	u_int32_t	flags;
#define SWM_L_FOCUSPREV		(1<<0)
#define SWM_L_MAPONFOCUS	(1<<1)
} layouts[] =  {
	/* stack,		configure */
	{ vertical_stack,	vertical_config,	0 },
	{ horizontal_stack,	horizontal_config,	0 },
	{ max_stack,		NULL, SWM_L_MAPONFOCUS | SWM_L_FOCUSPREV },
	{ NULL,			NULL,			0 },
};

/* position of max_stack mode in the layouts array, index into layouts! */
#define SWM_V_STACK		(0)
#define SWM_H_STACK		(1)
#define SWM_MAX_STACK		(2)

#define SWM_H_SLICE		(32)
#define SWM_V_SLICE		(32)

/* define work spaces */
struct workspace {
	int			idx;		/* workspace index */
	char			*name;		/* workspace name */
	int			always_raise;	/* raise windows on focus */
	struct layout		*cur_layout;	/* current layout handlers */
	struct ws_win		*focus;		/* may be NULL */
	struct ws_win		*focus_prev;	/* may be NULL */
	struct swm_region	*r;		/* may be NULL */
	struct swm_region	*old_r;		/* may be NULL */
	struct ws_win_list	winlist;	/* list of windows in ws */
	struct ws_win_list	unmanagedlist;	/* list of dead windows in ws */

	/* stacker state */
	struct {
				int horizontal_msize;
				int horizontal_mwin;
				int horizontal_stacks;
				int horizontal_flip;
				int vertical_msize;
				int vertical_mwin;
				int vertical_stacks;
				int vertical_flip;
	} l_state;
};

enum	{ SWM_S_COLOR_FOCUS, SWM_S_COLOR_UNFOCUS, SWM_S_COLOR_MAX };

/* physical screen mapping */
#define SWM_WS_MAX		(10)
struct swm_screen {
	int			idx;	/* screen index */
	struct swm_region_list	rl;	/* list of regions on this screen */
	struct swm_region_list	orl;	/* list of old regions */
	Window			root;
	struct workspace	ws[SWM_WS_MAX];

	/* colors */
	struct {
		unsigned long	color;
		char		*name;
	} c[SWM_S_COLOR_MAX];
};
struct swm_screen	*screens;

/* args to functions */
union arg {
	int			id;
#define SWM_ARG_ID_FOCUSNEXT	(0)
#define SWM_ARG_ID_FOCUSPREV	(1)
#define SWM_ARG_ID_FOCUSMAIN	(2)
#define SWM_ARG_ID_FOCUSCUR	(4)
#define SWM_ARG_ID_SWAPNEXT	(10)
#define SWM_ARG_ID_SWAPPREV	(11)
#define SWM_ARG_ID_SWAPMAIN	(12)
#define SWM_ARG_ID_MOVELAST	(13)
#define SWM_ARG_ID_MASTERSHRINK (20)
#define SWM_ARG_ID_MASTERGROW	(21)
#define SWM_ARG_ID_MASTERADD	(22)
#define SWM_ARG_ID_MASTERDEL	(23)
#define SWM_ARG_ID_FLIPLAYOUT	(24)
#define SWM_ARG_ID_STACKRESET	(30)
#define SWM_ARG_ID_STACKINIT	(31)
#define SWM_ARG_ID_CYCLEWS_UP	(40)
#define SWM_ARG_ID_CYCLEWS_DOWN	(41)
#define SWM_ARG_ID_CYCLESC_UP	(42)
#define SWM_ARG_ID_CYCLESC_DOWN	(43)
#define SWM_ARG_ID_CYCLEWS_UP_ALL	(44)
#define SWM_ARG_ID_CYCLEWS_DOWN_ALL	(45)
#define SWM_ARG_ID_STACKINC	(50)
#define SWM_ARG_ID_STACKDEC	(51)
#define SWM_ARG_ID_KILLWINDOW	(80)
#define SWM_ARG_ID_DELETEWINDOW	(81)
#define SWM_ARG_ID_WIDTHGROW	(90)
#define SWM_ARG_ID_WIDTHSHRINK	(91)
#define SWM_ARG_ID_HEIGHTGROW	(92)
#define SWM_ARG_ID_HEIGHTSHRINK	(93)
#define SWM_ARG_ID_MOVEUP	(100)
#define SWM_ARG_ID_MOVEDOWN	(101)
#define SWM_ARG_ID_MOVELEFT	(102)
#define SWM_ARG_ID_MOVERIGHT	(103)
	char			**argv;
};

void	focus(struct swm_region *, union arg *);
void	focus_magic(struct ws_win *);

/* quirks */
struct quirk {
	TAILQ_ENTRY(quirk)	entry;
	char			*class;
	char			*name;
	unsigned long		quirk;
#define SWM_Q_FLOAT		(1<<0)	/* float this window */
#define SWM_Q_TRANSSZ		(1<<1)	/* transiend window size too small */
#define SWM_Q_ANYWHERE		(1<<2)	/* don't position this window */
#define SWM_Q_FULLSCREEN	(1<<3)	/* remove border */
#define SWM_Q_FOCUSPREV		(1<<4)	/* focus on caller */
};
TAILQ_HEAD(quirk_list, quirk);
struct quirk_list		quirks = TAILQ_HEAD_INITIALIZER(quirks);

/*
 * Supported EWMH hints should be added to
 * both the enum and the ewmh array
 */
enum { _NET_ACTIVE_WINDOW, _NET_MOVERESIZE_WINDOW, _NET_CLOSE_WINDOW,
    _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_DOCK,
    _NET_WM_WINDOW_TYPE_TOOLBAR, _NET_WM_WINDOW_TYPE_UTILITY,
    _NET_WM_WINDOW_TYPE_SPLASH, _NET_WM_WINDOW_TYPE_DIALOG,
    _NET_WM_WINDOW_TYPE_NORMAL, _NET_WM_STATE,
    _NET_WM_STATE_SKIP_TASKBAR, _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_ABOVE, _SWM_WM_STATE_MANUAL,
    _NET_WM_STATE_FULLSCREEN, _NET_WM_ALLOWED_ACTIONS, _NET_WM_ACTION_MOVE,
    _NET_WM_ACTION_RESIZE, _NET_WM_ACTION_CLOSE,
    SWM_EWMH_HINT_MAX };

struct ewmh_hint {
	char	*name;
	Atom	 atom;
} ewmh[SWM_EWMH_HINT_MAX] =	{
    /* must be in same order as in the enum */
    {"_NET_ACTIVE_WINDOW", None},
    {"_NET_MOVERESIZE_WINDOW", None},
    {"_NET_CLOSE_WINDOW", None},
    {"_NET_WM_WINDOW_TYPE", None},
    {"_NET_WM_WINDOW_TYPE_DOCK", None},
    {"_NET_WM_WINDOW_TYPE_TOOLBAR", None},
    {"_NET_WM_WINDOW_TYPE_UTILITY", None},
    {"_NET_WM_WINDOW_TYPE_SPLASH", None},
    {"_NET_WM_WINDOW_TYPE_DIALOG", None},
    {"_NET_WM_WINDOW_TYPE_NORMAL", None},
    {"_NET_WM_STATE", None},
    {"_NET_WM_STATE_SKIP_TASKBAR", None},
    {"_NET_WM_STATE_SKIP_PAGER", None},
    {"_NET_WM_STATE_ABOVE", None},
    {"_SWM_WM_STATE_MANUAL", None},
    {"_NET_WM_STATE_FULLSCREEN", None},
    {"_NET_WM_ALLOWED_ACTIONS", None},
    {"_NET_WM_ACTION_MOVE", None},
    {"_NET_WM_ACTION_RESIZE", None},
    {"_NET_WM_ACTION_CLOSE", None},
};

void		 store_float_geom(struct ws_win *, struct swm_region *);
int		 floating_toggle_win(struct ws_win *);

int
get_property(Window id, Atom atom, long count, Atom type, unsigned long *nitems,
    unsigned long *nbytes, unsigned char **data)
{
	int			format, status;
	unsigned long		*nbytes_ret, *nitems_ret;
	unsigned long		nbytes_tmp, nitems_tmp;
	Atom			real;

	nbytes_ret = nbytes != NULL ? nbytes : &nbytes_tmp;
	nitems_ret = nitems != NULL ? nitems : &nitems_tmp;

	status = XGetWindowProperty(display, id, atom, 0L, count, False, type,
	    &real, &format, nitems_ret, nbytes_ret, data);

	if (status != Success)
		return False;
	if (real != type)
		return False;

	return True;
}

void
setup_ewmh(void)
{
	int			i,j;
	Atom			sup_list;

	sup_list = XInternAtom(display, "_NET_SUPPORTED", False);

	for (i = 0; i < LENGTH(ewmh); i++)
		ewmh[i].atom = XInternAtom(display, ewmh[i].name, False);

	for (i = 0; i < ScreenCount(display); i++) {
		/* Support check window will be created by workaround(). */

		/* Report supported atoms */
		XDeleteProperty(display, screens[i].root, sup_list);
		for (j = 0; j < LENGTH(ewmh); j++)
			XChangeProperty(display, screens[i].root,
			    sup_list, XA_ATOM, 32,
			    PropModeAppend, (unsigned char *)&ewmh[j].atom,1);
	}
}

void
teardown_ewmh(void)
{
	int			i, success;
	unsigned char		*data = NULL;
	unsigned long		n;
	Atom			sup_check, sup_list;
	Window			id;

	sup_check = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	sup_list = XInternAtom(display, "_NET_SUPPORTED", False);

	for (i = 0; i < ScreenCount(display); i++) {
		/* Get the support check window and destroy it */
		success = get_property(screens[i].root, sup_check, 1, XA_WINDOW,
		    &n, NULL, &data);

		if (success) {
			id = data[0];
			XDestroyWindow(display, id);
			XDeleteProperty(display, screens[i].root, sup_check);
			XDeleteProperty(display, screens[i].root, sup_list);
		}

		XFree(data);
	}
}

void
ewmh_autoquirk(struct ws_win *win)
{
	int			success, i;
	unsigned long		*data = NULL, n;
	Atom			type;

	success = get_property(win->id, ewmh[_NET_WM_WINDOW_TYPE].atom, (~0L),
	    XA_ATOM, &n, NULL, (void *)&data);

	if (!success) {
		XFree(data);
		return;
	}

	for (i = 0; i < n; i++) {
		type = data[i];
		if (type == ewmh[_NET_WM_WINDOW_TYPE_NORMAL].atom)
			break;
		if (type == ewmh[_NET_WM_WINDOW_TYPE_DOCK].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_TOOLBAR].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_UTILITY].atom) {
			win->floating = 1;
			win->quirks = SWM_Q_FLOAT | SWM_Q_ANYWHERE;
			break;
		}
		if (type == ewmh[_NET_WM_WINDOW_TYPE_SPLASH].atom ||
		    type == ewmh[_NET_WM_WINDOW_TYPE_DIALOG].atom) {
			win->floating = 1;
			win->quirks = SWM_Q_FLOAT;
			break;
		}
	}

	XFree(data);
}

#define SWM_EWMH_ACTION_COUNT_MAX	(6)
#define EWMH_F_FULLSCREEN		(1<<0)
#define EWMH_F_ABOVE			(1<<1)
#define EWMH_F_SKIP_PAGER		(1<<2)
#define EWMH_F_SKIP_TASKBAR		(1<<3)
#define SWM_F_MANUAL			(1<<4)

int
ewmh_set_win_fullscreen(struct ws_win *win, int fs)
{
	struct swm_geometry	rg;

	if (!win->ws->r)
		return 0;

	if (!win->floating)
		return 0;

	DNPRINTF(SWM_D_MISC, "ewmh_set_win_fullscreen: window: 0x%lx, "
	    "fullscreen %s\n", win->id, YESNO(fs));

	rg = win->ws->r->g;

	if (fs) {
		store_float_geom(win, win->ws->r);

		win->g = rg;
	} else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			X(win) = win->g_float.x - win->rg_float.x + rg.x;
			Y(win) = win->g_float.y - win->rg_float.y + rg.y;
			WIDTH(win) = win->g_float.w;
			HEIGHT(win) = win->g_float.h;
		}
	}

	return 1;
}

void
ewmh_update_actions(struct ws_win *win)
{
	Atom			actions[SWM_EWMH_ACTION_COUNT_MAX];
	int			n = 0;

	if (win == NULL)
		return;

	actions[n++] = ewmh[_NET_WM_ACTION_CLOSE].atom;

	if (win->floating) {
		actions[n++] = ewmh[_NET_WM_ACTION_MOVE].atom;
		actions[n++] = ewmh[_NET_WM_ACTION_RESIZE].atom;
	}

	XChangeProperty(display, win->id, ewmh[_NET_WM_ALLOWED_ACTIONS].atom,
	    XA_ATOM, 32, PropModeReplace, (unsigned char *)actions, n);
}

#define _NET_WM_STATE_REMOVE	0    /* remove/unset property */
#define _NET_WM_STATE_ADD	1    /* add/set property */
#define _NET_WM_STATE_TOGGLE	2    /* toggle property */

void
ewmh_update_win_state(struct ws_win *win, long state, long action)
{
	unsigned int		mask = 0;
	unsigned int		changed = 0;
	unsigned int		orig_flags;

	if (win == NULL)
		return;

	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		mask = EWMH_F_FULLSCREEN;
	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		mask = EWMH_F_ABOVE;
	if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		mask = SWM_F_MANUAL;
	if (state == ewmh[_NET_WM_STATE_SKIP_PAGER].atom)
		mask = EWMH_F_SKIP_PAGER;
	if (state == ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom)
		mask = EWMH_F_SKIP_TASKBAR;


	orig_flags = win->ewmh_flags;

	switch (action) {
	case _NET_WM_STATE_REMOVE:
		win->ewmh_flags &= ~mask;
		break;
	case _NET_WM_STATE_ADD:
		win->ewmh_flags |= mask;
		break;
	case _NET_WM_STATE_TOGGLE:
		win->ewmh_flags ^= mask;
		break;
	}

	changed = (win->ewmh_flags & mask) ^ (orig_flags & mask) ? 1 : 0;

	if (state == ewmh[_NET_WM_STATE_ABOVE].atom)
		if (changed)
			if (!floating_toggle_win(win))
				win->ewmh_flags = orig_flags; /* revert */
	if (state == ewmh[_SWM_WM_STATE_MANUAL].atom)
		if (changed)
			win->manual = (win->ewmh_flags & SWM_F_MANUAL) != 0;
	if (state == ewmh[_NET_WM_STATE_FULLSCREEN].atom)
		if (changed)
			if (!ewmh_set_win_fullscreen(win,
			    win->ewmh_flags & EWMH_F_FULLSCREEN))
				win->ewmh_flags = orig_flags; /* revert */

	XDeleteProperty(display, win->id, ewmh[_NET_WM_STATE].atom);

	if (win->ewmh_flags & EWMH_F_FULLSCREEN)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_FULLSCREEN].atom, 1);
	if (win->ewmh_flags & EWMH_F_SKIP_PAGER)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_SKIP_PAGER].atom, 1);
	if (win->ewmh_flags & EWMH_F_SKIP_TASKBAR)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_SKIP_TASKBAR].atom, 1);
	if (win->ewmh_flags & EWMH_F_ABOVE)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_NET_WM_STATE_ABOVE].atom, 1);
	if (win->ewmh_flags & SWM_F_MANUAL)
		XChangeProperty(display, win->id, ewmh[_NET_WM_STATE].atom,
		    XA_ATOM, 32, PropModeAppend,
		    (unsigned char *)&ewmh[_SWM_WM_STATE_MANUAL].atom, 1);
}

void
ewmh_get_win_state(struct ws_win *win)
{
	int			success, i;
	unsigned long		n;
	Atom			*states;

	if (win == NULL)
		return;

	win->ewmh_flags = 0;
	if (win->floating)
		win->ewmh_flags |= EWMH_F_ABOVE;
	if (win->manual)
		win->ewmh_flags |= SWM_F_MANUAL;

	success = get_property(win->id, ewmh[_NET_WM_STATE].atom,
	    (~0L), XA_ATOM, &n, NULL, (void *)&states);

	if (!success)
		return;

	for (i = 0; i < n; i++)
		ewmh_update_win_state(win, states[i], _NET_WM_STATE_ADD);

	XFree(states);
}

/* events */
#ifdef SWM_DEBUG
char *
geteventname(XEvent *e)
{
	char			*name = NULL;

	switch (e->type) {
	case KeyPress:
		name = "KeyPress";
		break;
	case KeyRelease:
		name = "KeyRelease";
		break;
	case ButtonPress:
		name = "ButtonPress";
		break;
	case ButtonRelease:
		name = "ButtonRelease";
		break;
	case MotionNotify:
		name = "MotionNotify";
		break;
	case EnterNotify:
		name = "EnterNotify";
		break;
	case LeaveNotify:
		name = "LeaveNotify";
		break;
	case FocusIn:
		name = "FocusIn";
		break;
	case FocusOut:
		name = "FocusOut";
		break;
	case KeymapNotify:
		name = "KeymapNotify";
		break;
	case Expose:
		name = "Expose";
		break;
	case GraphicsExpose:
		name = "GraphicsExpose";
		break;
	case NoExpose:
		name = "NoExpose";
		break;
	case VisibilityNotify:
		name = "VisibilityNotify";
		break;
	case CreateNotify:
		name = "CreateNotify";
		break;
	case DestroyNotify:
		name = "DestroyNotify";
		break;
	case UnmapNotify:
		name = "UnmapNotify";
		break;
	case MapNotify:
		name = "MapNotify";
		break;
	case MapRequest:
		name = "MapRequest";
		break;
	case ReparentNotify:
		name = "ReparentNotify";
		break;
	case ConfigureNotify:
		name = "ConfigureNotify";
		break;
	case ConfigureRequest:
		name = "ConfigureRequest";
		break;
	case GravityNotify:
		name = "GravityNotify";
		break;
	case ResizeRequest:
		name = "ResizeRequest";
		break;
	case CirculateNotify:
		name = "CirculateNotify";
		break;
	case CirculateRequest:
		name = "CirculateRequest";
		break;
	case PropertyNotify:
		name = "PropertyNotify";
		break;
	case SelectionClear:
		name = "SelectionClear";
		break;
	case SelectionRequest:
		name = "SelectionRequest";
		break;
	case SelectionNotify:
		name = "SelectionNotify";
		break;
	case ColormapNotify:
		name = "ColormapNotify";
		break;
	case ClientMessage:
		name = "ClientMessage";
		break;
	case MappingNotify:
		name = "MappingNotify";
		break;
	default:
		name = "Unknown";
	}

	return name;
}

char *
xrandr_geteventname(XEvent *e)
{
	char			*name = NULL;

	switch(e->type - xrandr_eventbase) {
	case RRScreenChangeNotify:
		name = "RRScreenChangeNotify";
		break;
	default:
		name = "Unknown";
	}

	return name;
}

void
dumpwins(struct swm_region *r, union arg *args)
{
	struct ws_win		*win;
	unsigned int		state;
	XWindowAttributes	wa;

	if (r->ws == NULL) {
		warnx("dumpwins: invalid workspace");
		return;
	}

	warnx("=== managed window list ws %02d ===", r->ws->idx);

	TAILQ_FOREACH(win, &r->ws->winlist, entry) {
		state = getstate(win->id);
		if (!XGetWindowAttributes(display, win->id, &wa))
			warnx("window: 0x%lx, failed XGetWindowAttributes",
			    win->id);
		warnx("window: 0x%lx, map_state: %d, state: %d, "
		    "transient: 0x%lx", win->id, wa.map_state, state,
		    win->transient);
	}

	warnx("===== unmanaged window list =====");
	TAILQ_FOREACH(win, &r->ws->unmanagedlist, entry) {
		state = getstate(win->id);
		if (!XGetWindowAttributes(display, win->id, &wa))
			warnx("window: 0x%lx, failed XGetWindowAttributes",
			    win->id);
		warnx("window: 0x%lx, map_state: %d, state: %d, "
		    "transient: 0x%lx", win->id, wa.map_state, state,
		    win->transient);
	}

	warnx("=================================");
}
#else
void
dumpwins(struct swm_region *r, union arg *args)
{
}
#endif /* SWM_DEBUG */

void			expose(XEvent *);
void			keypress(XEvent *);
void			buttonpress(XEvent *);
void			configurerequest(XEvent *);
void			configurenotify(XEvent *);
void			destroynotify(XEvent *);
void			mapnotify(XEvent *);
void			mappingnotify(XEvent *);
void			maprequest(XEvent *);
void			propertynotify(XEvent *);
void			unmapnotify(XEvent *);
void			visibilitynotify(XEvent *);
void			clientmessage(XEvent *);

void			(*handler[LASTEvent])(XEvent *) = {
				[Expose] = expose,
				[KeyPress] = keypress,
				[ButtonPress] = buttonpress,
				[ConfigureRequest] = configurerequest,
				[ConfigureNotify] = configurenotify,
				[DestroyNotify] = destroynotify,
				[MapNotify] = mapnotify,
				[MappingNotify] = mappingnotify,
				[MapRequest] = maprequest,
				[PropertyNotify] = propertynotify,
				[UnmapNotify] = unmapnotify,
				[VisibilityNotify] = visibilitynotify,
				[ClientMessage] = clientmessage,
};

void
sighdlr(int sig)
{
	int			saved_errno, status;
	pid_t			pid;

	saved_errno = errno;

	switch (sig) {
	case SIGCHLD:
		while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) != 0) {
			if (pid == -1) {
				if (errno == EINTR)
					continue;
#ifdef SWM_DEBUG
				if (errno != ECHILD)
					warn("sighdlr: waitpid");
#endif /* SWM_DEBUG */
				break;
			}

#ifdef SWM_DEBUG
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) != 0)
					warnx("sighdlr: child exit status: %d",
					    WEXITSTATUS(status));
			} else
				warnx("sighdlr: child is terminated "
				    "abnormally");
#endif /* SWM_DEBUG */
		}
		break;

	case SIGHUP:
		restart_wm = 1;
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		running = 0;
		break;
	}

	errno = saved_errno;
}

struct pid_e *
find_pid(long pid)
{
	struct pid_e		*p = NULL;

	DNPRINTF(SWM_D_MISC, "find_pid: %lu\n", pid);

	if (pid == 0)
		return (NULL);

	TAILQ_FOREACH(p, &pidlist, entry) {
		if (p->pid == pid)
			return (p);
	}

	return (NULL);
}

unsigned long
name_to_color(char *colorname)
{
	Colormap		cmap;
	Status			status;
	XColor			screen_def, exact_def;
	unsigned long		result = 0;
	char			cname[32] = "#";

	cmap = DefaultColormap(display, screens[0].idx);
	status = XAllocNamedColor(display, cmap, colorname,
	    &screen_def, &exact_def);
	if (!status) {
		strncpy(cname, colorname + 2, sizeof cname - 1);
		status = XAllocNamedColor(display, cmap, cname, &screen_def,
		    &exact_def);
	}
	if (status)
		result = screen_def.pixel;
	else
		warnx("color '%s' not found", colorname);

	return (result);
}

void
setscreencolor(char *val, int i, int c)
{
	if (i > 0 && i <= ScreenCount(display)) {
		screens[i - 1].c[c].color = name_to_color(val);
		free(screens[i - 1].c[c].name);
		if ((screens[i - 1].c[c].name = strdup(val)) == NULL)
			err(1, "strdup");
	} else if (i == -1) {
		for (i = 0; i < ScreenCount(display); i++) {
			screens[i].c[c].color = name_to_color(val);
			free(screens[i].c[c].name);
			if ((screens[i].c[c].name = strdup(val)) == NULL)
				err(1, "strdup");
		}
	} else
		errx(1, "invalid screen index: %d out of bounds (maximum %d)",
		    i, ScreenCount(display));
}

void
custom_region(char *val)
{
	unsigned int			sidx, x, y, w, h;

	if (sscanf(val, "screen[%u]:%ux%u+%u+%u", &sidx, &w, &h, &x, &y) != 5)
		errx(1, "invalid custom region, "
		    "should be 'screen[<n>]:<n>x<n>+<n>+<n>");
	if (sidx < 1 || sidx > ScreenCount(display))
		errx(1, "invalid screen index: %d out of bounds (maximum %d)",
		    sidx, ScreenCount(display));
	sidx--;

	if (w < 1 || h < 1)
		errx(1, "region %ux%u+%u+%u too small", w, h, x, y);

	if (x > DisplayWidth(display, sidx) ||
	    y > DisplayHeight(display, sidx) ||
	    w + x > DisplayWidth(display, sidx) ||
	    h + y > DisplayHeight(display, sidx)) {
		warnx("ignoring region %ux%u+%u+%u - not within screen "
		    "boundaries (%ux%u)", w, h, x, y,
		    DisplayWidth(display, sidx), DisplayHeight(display, sidx));
		return;
	}

	new_region(&screens[sidx], x, y, w, h);
}

void
drain_enter_notify(void)
{
	int			i = 0;
	XEvent			cne;

	while (XCheckMaskEvent(display, EnterWindowMask, &cne))
		i++;

	DNPRINTF(SWM_D_MISC, "drain_enter_notify: drained: %d\n", i);
}

void
set_win_state(struct ws_win *win, long state)
{
	long			data[] = {state, None};

	DNPRINTF(SWM_D_EVENT, "set_win_state: window: 0x%lx\n", win->id);

	if (win == NULL)
		return;

	XChangeProperty(display, win->id, astate, astate, 32, PropModeReplace,
	    (unsigned char *)data, 2);
}

long
getstate(Window w)
{
	long			result = -1;
	unsigned char		*p = NULL;
	unsigned long		n;

	if (!get_property(w, astate, 2L, astate, &n, NULL, &p))
		return (-1);
	if (n != 0)
		result = *((long *)p);
	XFree(p);
	return (result);
}

void
client_msg(struct ws_win *win, Atom a)
{
	XClientMessageEvent	cm;

	if (win == NULL)
		return;

	bzero(&cm, sizeof cm);
	cm.type = ClientMessage;
	cm.window = win->id;
	cm.message_type = aprot;
	cm.format = 32;
	cm.data.l[0] = a;
	cm.data.l[1] = CurrentTime;
	XSendEvent(display, win->id, False, 0L, (XEvent *)&cm);
}

/* synthetic response to a ConfigureRequest when not making a change */
void
config_win(struct ws_win *win, XConfigureRequestEvent  *ev)
{
	XConfigureEvent		ce;

	if (win == NULL)
		return;

	/* send notification of unchanged state. */
	ce.type = ConfigureNotify;
	ce.x = X(win);
	ce.y = Y(win);
	ce.width = WIDTH(win);
	ce.height = HEIGHT(win);
	ce.override_redirect = False;

	if (ev == NULL) {
		/* EWMH */
		ce.display = display;
		ce.event = win->id;
		ce.window = win->id;
		ce.border_width = border_width;
		ce.above = None;
	} else {
		/* normal */
		ce.display = ev->display;
		ce.event = ev->window;
		ce.window = ev->window;

		/* make response appear more WM_SIZE_HINTS-compliant */
		if (win->sh_mask)
			DNPRINTF(SWM_D_MISC, "config_win: hints: window: 0x%lx,"
			    " sh_mask: %ld, min: %d x %d, max: %d x %d, inc: "
			    "%d x %d\n", win->id, win->sh_mask, SH_MIN_W(win),
			    SH_MIN_H(win), SH_MAX_W(win), SH_MAX_H(win),
			    SH_INC_W(win), SH_INC_H(win));

		/* min size */
		if (SH_MIN(win)) {
			/* the hint may be set... to 0! */
			if (SH_MIN_W(win) > 0 && ce.width < SH_MIN_W(win))
				ce.width = SH_MIN_W(win);
			if (SH_MIN_H(win) > 0 && ce.height < SH_MIN_H(win))
				ce.height = SH_MIN_H(win);
		}

		/* max size */
		if (SH_MAX(win)) {
			/* may also be advertized as 0 */
			if (SH_MAX_W(win) > 0 && ce.width > SH_MAX_W(win))
				ce.width = SH_MAX_W(win);
			if (SH_MAX_H(win) > 0 && ce.height > SH_MAX_H(win))
				ce.height = SH_MAX_H(win);
		}

		/* resize increment. */
		if (SH_INC(win)) {
			if (SH_INC_W(win) > 1 && ce.width > SH_INC_W(win))
				ce.width -= (ce.width - SH_MIN_W(win)) %
				    SH_INC_W(win);
			if (SH_INC_H(win) > 1 && ce.height > SH_INC_H(win))
				ce.height -= (ce.height - SH_MIN_H(win)) %
				    SH_INC_H(win);
		}

		/* adjust x and y for requested border_width. */
		ce.x += border_width - ev->border_width;
		ce.y += border_width - ev->border_width;
		ce.border_width = ev->border_width;
		ce.above = ev->above;
	}

	DNPRINTF(SWM_D_MISC, "config_win: ewmh: %s, window: 0x%lx, (x,y) w x h: "
	    "(%d,%d) %d x %d, border: %d\n", YESNO(ev == NULL), win->id, ce.x,
	    ce.y, ce.width, ce.height, ce.border_width);

	XSendEvent(display, win->id, False, StructureNotifyMask, (XEvent *)&ce);
}

int
count_win(struct workspace *ws, int count_transient)
{
	struct ws_win		*win;
	int			count = 0;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (count_transient == 0 && win->floating)
			continue;
		if (count_transient == 0 && win->transient)
			continue;
		count++;
	}
	DNPRINTF(SWM_D_MISC, "count_win: %d\n", count);

	return (count);
}

void
quit(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "quit\n");
	running = 0;
}

void
unmap_window(struct ws_win *win)
{
	if (win == NULL)
		return;

	/* don't unmap again */
	if (getstate(win->id) == IconicState)
		return;

	set_win_state(win, IconicState);

	XUnmapWindow(display, win->id);
	XSetWindowBorder(display, win->id,
	    win->s->c[SWM_S_COLOR_UNFOCUS].color);
}

void
unmap_all(void)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unmap_window(win);
}

void
restart(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "restart: %s\n", start_argv[0]);

	/* disable alarm because the following code may not be interrupted */
	alarm(0);
	if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
		err(1, "can't disable alarm");

	unmap_all();
	XCloseDisplay(display);
	execvp(start_argv[0], start_argv);
	warn("execvp failed");
	quit(NULL, NULL);
}

struct swm_region *
root_to_region(Window root)
{
	struct swm_region	*r = NULL;
	Window			rr, cr;
	int			i, x, y, wx, wy;
	unsigned int		mask;

	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == root)
			break;

	if (XQueryPointer(display, screens[i].root,
	    &rr, &cr, &x, &y, &wx, &wy, &mask) != False) {
		/* choose a region based on pointer location */
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			if (x >= X(r) && x <= X(r) + WIDTH(r) &&
			    y >= Y(r) && y <= Y(r) + HEIGHT(r))
				break;
	}

	if (r == NULL)
		r = TAILQ_FIRST(&screens[i].rl);

	return (r);
}

struct ws_win *
find_unmanaged_window(Window id)
{
	struct ws_win		*win;
	int			i, j;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].unmanagedlist,
			    entry)
				if (id == win->id)
					return (win);
	return (NULL);
}

struct ws_win *
find_window(Window id)
{
	struct ws_win		*win;
	Window			wrr, wpr, *wcr = NULL;
	int			i, j;
	unsigned int		nc;

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (id == win->id)
					return (win);

	/* if we were looking for the parent return that window instead */
	if (XQueryTree(display, id, &wrr, &wpr, &wcr, &nc) == 0)
		return (NULL);
	if (wcr)
		XFree(wcr);

	/* ignore not found and root */
	if (wpr == 0 || wrr == wpr)
		return (NULL);

	/* look for parent */
	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				if (wpr == win->id)
					return (win);

	return (NULL);
}

void
spawn(int ws_idx, union arg *args, int close_fd)
{
	int			fd;
	char			*ret = NULL;

	DNPRINTF(SWM_D_MISC, "spawn: %s\n", args->argv[0]);

	if (display)
		close(ConnectionNumber(display));

	if (asprintf(&ret, "%d", ws_idx) == -1) {
		warn("spawn: asprintf SWM_WS");
		_exit(1);
	}
	setenv("_SWM_WS", ret, 1);
	free(ret);
	ret = NULL;

	if (asprintf(&ret, "%d", getpid()) == -1) {
		warn("spawn: asprintf _SWM_PID");
		_exit(1);
	}
	setenv("_SWM_PID", ret, 1);
	free(ret);
	ret = NULL;

	if (setsid() == -1) {
		warn("spawn: setsid");
		_exit(1);
	}

	if (close_fd) {
		/*
		 * close stdin and stdout
		 * leave stderr open to record errors
		*/
		if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) == -1) {
			warn("spawn: open");
			_exit(1);
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		if (fd > 2)
			close(fd);
	}

	execvp(args->argv[0], args->argv);

	warn("spawn: execvp");
	_exit(1);
}

void
kill_refs(struct ws_win *win)
{
	int			i, x;
	struct swm_region	*r;
	struct workspace	*ws;

	if (win == NULL)
		return;

	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < SWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				if (win == ws->focus)
					ws->focus = NULL;
				if (win == ws->focus_prev)
					ws->focus_prev = NULL;
			}
}

int
validate_win(struct ws_win *testwin)
{
	struct ws_win		*win;
	struct workspace	*ws;
	struct swm_region	*r;
	int			i, x;

	if (testwin == NULL)
		return (0);

	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < SWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				TAILQ_FOREACH(win, &ws->winlist, entry)
					if (win == testwin)
						return (0);
			}
	return (1);
}

int
validate_ws(struct workspace *testws)
{
	struct swm_region	*r;
	struct workspace	*ws;
	int			i, x;

	/* validate all ws */
	for (i = 0; i < ScreenCount(display); i++)
		TAILQ_FOREACH(r, &screens[i].rl, entry)
			for (x = 0; x < SWM_WS_MAX; x++) {
				ws = &r->s->ws[x];
				if (ws == testws)
					return (0);
			}
	return (1);
}

void
unfocus_win(struct ws_win *win)
{
	XEvent			cne;
	Window			none = None;

	DNPRINTF(SWM_D_FOCUS, "unfocus_win: window: 0x%lx\n", WINID(win));

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (win->ws->r == NULL)
		return;

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (win->ws->focus == win) {
		win->ws->focus = NULL;
		win->ws->focus_prev = win;
	}

	if (validate_win(win->ws->focus)) {
		kill_refs(win->ws->focus);
		win->ws->focus = NULL;
	}
	if (validate_win(win->ws->focus_prev)) {
		kill_refs(win->ws->focus_prev);
		win->ws->focus_prev = NULL;
	}

	/* drain all previous unfocus events */
	while (XCheckTypedEvent(display, FocusOut, &cne) == True)
		;

	grabbuttons(win, 0);
	XSetWindowBorder(display, win->id,
	    win->ws->r->s->c[SWM_S_COLOR_UNFOCUS].color);

	XChangeProperty(display, win->s->root,
	    ewmh[_NET_ACTIVE_WINDOW].atom, XA_WINDOW, 32,
	    PropModeReplace, (unsigned char *)&none,1);
}

void
unfocus_all(void)
{
	struct ws_win		*win;
	int			i, j;

	DNPRINTF(SWM_D_FOCUS, "unfocus_all\n");

	for (i = 0; i < ScreenCount(display); i++)
		for (j = 0; j < SWM_WS_MAX; j++)
			TAILQ_FOREACH(win, &screens[i].ws[j].winlist, entry)
				unfocus_win(win);
}

void
focus_win(struct ws_win *win)
{
	XEvent			cne;
	Window			cur_focus;
	int			rr;
	struct ws_win		*cfw = NULL;


	DNPRINTF(SWM_D_FOCUS, "focus_win: window: 0x%lx\n", WINID(win));

	if (win == NULL)
		return;
	if (win->ws == NULL)
		return;

	if (validate_ws(win->ws))
		return; /* XXX this gets hit with thunderbird, needs fixing */

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	if (validate_win(win)) {
		kill_refs(win);
		return;
	}

	XGetInputFocus(display, &cur_focus, &rr);
	if ((cfw = find_window(cur_focus)) != NULL)
		unfocus_win(cfw);
	else {
		/* use larger hammer since the window was killed somehow */
		TAILQ_FOREACH(cfw, &win->ws->winlist, entry)
			if (cfw->ws && cfw->ws->r && cfw->ws->r->s)
				XSetWindowBorder(display, cfw->id,
				    cfw->ws->r->s->c[SWM_S_COLOR_UNFOCUS].color);
	}

	win->ws->focus = win;

	if (win->ws->r != NULL) {
		/* drain all previous focus events */
		while (XCheckTypedEvent(display, FocusIn, &cne) == True);

		XSetInputFocus(display, win->id, RevertToParent, CurrentTime);
		grabbuttons(win, 1);
		XSetWindowBorder(display, win->id,
		    win->ws->r->s->c[SWM_S_COLOR_FOCUS].color);
		if (win->ws->cur_layout->flags & SWM_L_MAPONFOCUS ||
		    win->ws->always_raise)
			XMapRaised(display, win->id);

		XChangeProperty(display, win->s->root,
		    ewmh[_NET_ACTIVE_WINDOW].atom, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win->id,1);
	}
}

void
switchws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id, unmap_old = 0;
	struct swm_region	*this_r, *other_r;
	struct ws_win		*win;
	struct workspace	*new_ws, *old_ws;
	union arg		a;

	if (!(r && r->s))
		return;

	this_r = r;
	old_ws = this_r->ws;
	new_ws = &this_r->s->ws[wsid];

	DNPRINTF(SWM_D_WS, "switchws: screen[%d]:%dx%d+%d+%d: %d -> %d\n",
	    r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), old_ws->idx, wsid);

	if (new_ws == NULL || old_ws == NULL)
		return;
	if (new_ws == old_ws)
		return;

	other_r = new_ws->r;
	if (other_r == NULL) {
		/* the other workspace is hidden, hide this one */
		old_ws->r = NULL;
		unmap_old = 1;
	} else {
		/* the other ws is visible in another region, exchange them */
		other_r->ws_prior = new_ws;
		other_r->ws = old_ws;
		old_ws->r = other_r;
	}
	this_r->ws_prior = old_ws;
	this_r->ws = new_ws;
	new_ws->r = this_r;

	/* this is needed so that we can click on a window after a restart */
	unfocus_all();

	stack();
	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(new_ws->r, &a);

	/* unmap old windows */
	if (unmap_old)
		TAILQ_FOREACH(win, &old_ws->winlist, entry)
			unmap_window(win);
}

void
cyclews(struct swm_region *r, union arg *args)
{
	union			arg a;
	struct swm_screen	*s = r->s;
	int			cycle_all = 0;
	int diff;

	DNPRINTF(SWM_D_WS, "cyclews: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	switch (args->id) {
	case SWM_ARG_ID_CYCLEWS_UP_ALL:
		cycle_all = 1;
		/* FALLTHROUGH */
	case SWM_ARG_ID_CYCLEWS_UP:
		diff = 1;
		break;
	case SWM_ARG_ID_CYCLEWS_DOWN_ALL:
		cycle_all = 1;
		/* FALLTHROUGH */
	case SWM_ARG_ID_CYCLEWS_DOWN:
		diff = -1;
		break;
	default:
		return;
	};

	diff += SWM_WS_MAX;
	a.id = r->ws->idx;
	do {
		/* +1 or -1 a.id within [0..SWM_WS_MAX-1] */
		a.id = (a.id + diff) % SWM_WS_MAX;

		/* Skip empty workspaces */
		if (!cycle_all && TAILQ_EMPTY(&s->ws[a.id].winlist))
			continue;

		/* Skip workspaces already visible on other regions */
		if (s->ws[a.id].r != NULL)
			continue;

		switchws(r, &a);
	} while (a.id != r->ws->idx);
}

void
priorws(struct swm_region *r, union arg *args)
{
	union arg		a;

	DNPRINTF(SWM_D_WS, "priorws: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	if (r->ws_prior == NULL)
		return;

	a.id = r->ws_prior->idx;
	switchws(r, &a);
}

void
cyclescr(struct swm_region *r, union arg *args)
{
	struct swm_region	*rr = NULL;
	union arg		a;
	int			i, x, y;

	/* do nothing if we don't have more than one screen */
	if (!(ScreenCount(display) > 1 || outputs > 1))
		return;

	i = r->s->idx;
	switch (args->id) {
	case SWM_ARG_ID_CYCLESC_UP:
		rr = TAILQ_NEXT(r, entry);
		if (rr == NULL)
			rr = TAILQ_FIRST(&screens[i].rl);
		break;
	case SWM_ARG_ID_CYCLESC_DOWN:
		rr = TAILQ_PREV(r, swm_region_list, entry);
		if (rr == NULL)
			rr = TAILQ_LAST(&screens[i].rl, swm_region_list);
		break;
	default:
		return;
	};
	if (rr == NULL)
		return;

	/* move mouse to region */
	x = X(rr) + 1;
	y = Y(rr) + 1;
	XWarpPointer(display, None, rr->s[i].root, 0, 0, 0, 0, x, y);

	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(rr, &a);

	if (rr->ws->focus) {
		/* move to focus window */
		x = X(rr->ws->focus) + 1;
		y = Y(rr->ws->focus) + 1;
		XWarpPointer(display, None, rr->s[i].root, 0, 0, 0, 0, x, y);
	}
}

void
sort_windows(struct ws_win_list *wl)
{
	struct ws_win		*win, *parent, *nxt;

	if (wl == NULL)
		return;

	for (win = TAILQ_FIRST(wl); win != TAILQ_END(wl); win = nxt) {
		nxt = TAILQ_NEXT(win, entry);
		if (win->transient) {
			parent = find_window(win->transient);
			if (parent == NULL) {
				warnx("not possible bug");
				continue;
			}
			TAILQ_REMOVE(wl, win, entry);
			TAILQ_INSERT_AFTER(wl, parent, win, entry);
		}
	}

}

void
swapwin(struct swm_region *r, union arg *args)
{
	struct ws_win		*target, *source;
	struct ws_win		*cur_focus;
	struct ws_win_list	*wl;


	DNPRINTF(SWM_D_WS, "swapwin: id: %d, screen[%d]:%dx%d+%d+%d, ws: %d\n",
	    args->id, r->s->idx, WIDTH(r), HEIGHT(r), X(r), Y(r), r->ws->idx);

	cur_focus = r->ws->focus;
	if (cur_focus == NULL)
		return;

	source = cur_focus;
	wl = &source->ws->winlist;

	switch (args->id) {
	case SWM_ARG_ID_SWAPPREV:
		if (source->transient)
			source = find_window(source->transient);
		target = TAILQ_PREV(source, ws_win_list, entry);
		if (target && target->transient)
			target = find_window(target->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_TAIL(wl, source, entry);
		else
			TAILQ_INSERT_BEFORE(target, source, entry);
		break;
	case SWM_ARG_ID_SWAPNEXT:
		target = TAILQ_NEXT(source, entry);
		/* move the parent and let the sort handle the move */
		if (source->transient)
			source = find_window(source->transient);
		TAILQ_REMOVE(wl, source, entry);
		if (target == NULL)
			TAILQ_INSERT_HEAD(wl, source, entry);
		else
			TAILQ_INSERT_AFTER(wl, target, source, entry);
		break;
	case SWM_ARG_ID_SWAPMAIN:
		target = TAILQ_FIRST(wl);
		if (target == source) {
			if (source->ws->focus_prev != NULL &&
			    source->ws->focus_prev != target)

				source = source->ws->focus_prev;
			else
				return;
		}
		if (target == NULL || source == NULL)
			return;
		source->ws->focus_prev = target;
		TAILQ_REMOVE(wl, target, entry);
		TAILQ_INSERT_BEFORE(source, target, entry);
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_HEAD(wl, source, entry);
		break;
	case SWM_ARG_ID_MOVELAST:
		TAILQ_REMOVE(wl, source, entry);
		TAILQ_INSERT_TAIL(wl, source, entry);
		break;
	default:
		DNPRINTF(SWM_D_MOVE, "swapwin: invalid id: %d\n", args->id);
		return;
	}

	sort_windows(wl);

	stack();
}

void
focus_prev(struct ws_win *win)
{
	struct ws_win		*winfocus = NULL;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	DNPRINTF(SWM_D_FOCUS, "focus_prev: window: 0x%lx\n", WINID(win));

	if (!(win && win->ws))
		return;

	ws = win->ws;
	wl = &ws->winlist;
	cur_focus = ws->focus;

	/* pickle, just focus on whatever */
	if (cur_focus == NULL) {
		/* use prev_focus if valid */
		if (ws->focus_prev && ws->focus_prev != cur_focus &&
		    find_window(WINID(ws->focus_prev)))
			winfocus = ws->focus_prev;
		if (winfocus == NULL)
			winfocus = TAILQ_FIRST(wl);
		goto done;
	}

	/* if transient focus on parent */
	if (cur_focus->transient) {
		winfocus = find_window(cur_focus->transient);
		goto done;
	}

	/* if in max_stack try harder */
	if ((win->quirks & SWM_Q_FOCUSPREV) ||
	    (ws->cur_layout->flags & SWM_L_FOCUSPREV)) {
		if (cur_focus != ws->focus_prev)
			winfocus = ws->focus_prev;
		else
			winfocus = TAILQ_PREV(win, ws_win_list, entry);
		if (winfocus)
			goto done;
	}

	if (cur_focus == win)
		winfocus = TAILQ_PREV(win, ws_win_list, entry);
	if (winfocus == NULL)
		winfocus = TAILQ_LAST(wl, ws_win_list);
	if (winfocus == NULL || winfocus == win)
		winfocus = TAILQ_NEXT(cur_focus, entry);

done:
	focus_magic(winfocus);
}

void
focus(struct swm_region *r, union arg *args)
{
	struct ws_win		*winfocus = NULL, *head;
	struct ws_win		*cur_focus = NULL;
	struct ws_win_list	*wl = NULL;
	struct workspace	*ws = NULL;

	if (!(r && r->ws))
		return;

	DNPRINTF(SWM_D_FOCUS, "focus: id: %d\n", args->id);

	/* treat FOCUS_CUR special */
	if (args->id == SWM_ARG_ID_FOCUSCUR) {
		if (r->ws->focus)
			winfocus = r->ws->focus;
		else if (r->ws->focus_prev)
			winfocus = r->ws->focus_prev;
		else
			TAILQ_FOREACH(winfocus, &r->ws->winlist, entry)
				break;

		focus_magic(winfocus);
		return;
	}

	if ((cur_focus = r->ws->focus) == NULL)
		return;
	ws = r->ws;
	wl = &ws->winlist;
	if (TAILQ_EMPTY(wl))
		return;

	switch (args->id) {
	case SWM_ARG_ID_FOCUSPREV:
		head = TAILQ_PREV(cur_focus, ws_win_list, entry);
		if (head == NULL)
			head = TAILQ_LAST(wl, ws_win_list);
		winfocus = head;
		if (WINID(winfocus) == cur_focus->transient) {
			head = TAILQ_PREV(winfocus, ws_win_list, entry);
			if (head == NULL)
				head = TAILQ_LAST(wl, ws_win_list);
			winfocus = head;
		}

		break;

	case SWM_ARG_ID_FOCUSNEXT:
		head = TAILQ_NEXT(cur_focus, entry);
		if (head == NULL)
			head = TAILQ_FIRST(wl);
		winfocus = head;

		break;

	case SWM_ARG_ID_FOCUSMAIN:
		winfocus = TAILQ_FIRST(wl);
		if (winfocus == cur_focus)
			winfocus = cur_focus->ws->focus_prev;
		break;

	default:
		return;
	}

	focus_magic(winfocus);
}

void
cycle_layout(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;
	union arg		a;

	DNPRINTF(SWM_D_EVENT, "cycle_layout: workspace: %d\n", ws->idx);

	ws->cur_layout++;
	if (ws->cur_layout->l_stack == NULL)
		ws->cur_layout = &layouts[0];

	stack();
	a.id = SWM_ARG_ID_FOCUSCUR;
	focus(r, &a);
}

void
stack_config(struct swm_region *r, union arg *args)
{
	struct workspace	*ws = r->ws;

	DNPRINTF(SWM_D_STACK, "stack_config: id: %d workspace: %d\n",
	    args->id, ws->idx);

	if (ws->cur_layout->l_config != NULL)
		ws->cur_layout->l_config(ws, args->id);

	if (args->id != SWM_ARG_ID_STACKINIT)
		stack();
}

void
stack(void) {
	struct swm_geometry	g;
	struct swm_region	*r;
	int			i;
#ifdef SWM_DEBUG
	int j;
#endif

	DNPRINTF(SWM_D_STACK, "stack: begin\n");

	for (i = 0; i < ScreenCount(display); i++) {
#ifdef SWM_DEBUG
		j = 0;
#endif
		TAILQ_FOREACH(r, &screens[i].rl, entry) {
			DNPRINTF(SWM_D_STACK, "stack: workspace: %d "
			    "(screen: %d, region: %d)\n", r->ws->idx, i, j++);

			/* use screen geometry */
			g = r->g;
			g.w -= 2 * border_width;
			g.h -= 2 * border_width;
			r->ws->cur_layout->l_stack(r->ws, &g);
			/* save r so we can track region changes */
			r->ws->old_r = r;
		}
	}

	DNPRINTF(SWM_D_STACK, "stack: end\n");
}

void
store_float_geom(struct ws_win *win, struct swm_region *r)
{
	/* retain window geom and region geom */
	win->g_float = win->g;
	win->rg_float = r->g;
	win->g_floatvalid = 1;
}

void
stack_floater(struct ws_win *win, struct swm_region *r)
{
	unsigned int		mask;
	XWindowChanges		wc;

	if (win == NULL)
		return;

	bzero(&wc, sizeof wc);
	mask = CWX | CWY | CWBorderWidth | CWWidth | CWHeight;

	/*
	 * to allow windows to change their size (e.g. mplayer fs) only retrieve
	 * geom on ws switches or return from max mode
	 */
	if (win->floatmaxed || (r != r->ws->old_r && win->g_floatvalid
	    && !(win->ewmh_flags & EWMH_F_FULLSCREEN))) {
		/*
		 * use stored g and rg to set relative position and size
		 * as in old region or before max stack mode
		 */
		X(win) = win->g_float.x - win->rg_float.x + X(r);
		Y(win) = win->g_float.y - win->rg_float.y + Y(r);
		WIDTH(win) = win->g_float.w;
		HEIGHT(win) = win->g_float.h;
		win->g_floatvalid = 0;
	}

	win->floatmaxed = 0;

	if ((win->quirks & SWM_Q_FULLSCREEN) && (WIDTH(win) >= WIDTH(r)) &&
	    (HEIGHT(win) >= HEIGHT(r)))
		wc.border_width = 0;
	else
		wc.border_width = border_width;
	if (win->transient && (win->quirks & SWM_Q_TRANSSZ)) {
		WIDTH(win) = (double)WIDTH(r) * dialog_ratio;
		HEIGHT(win) = (double)HEIGHT(r) * dialog_ratio;
	}

	if (!win->manual) {
		/*
		 * floaters and transients are auto-centred unless moved
		 * or resized
		 */
		X(win) = X(r) + (WIDTH(r) - WIDTH(win)) /  2 - wc.border_width;
		Y(win) = Y(r) + (HEIGHT(r) - HEIGHT(win)) / 2 - wc.border_width;
	}

	/* win can be outside r if new r smaller than old r */
	/* Ensure horizontal and vertical border inside r */
	if (X(win) > X(r)+WIDTH(r) - wc.border_width)
		X(win) = X(r)+WIDTH(r) - wc.border_width;
	if (X(win) < X(r) - WIDTH(win) - 2*wc.border_width + 1)
		X(win) = X(r) - WIDTH(win) - 2*wc.border_width + 1;
	if (Y(win) > Y(r)+HEIGHT(r) - wc.border_width)
		Y(win) = Y(r)+HEIGHT(r) - wc.border_width;
	if (Y(win) < Y(r) - HEIGHT(win) - 2*wc.border_width + 1)
		Y(win) = Y(r) - HEIGHT(win) - 2*wc.border_width + 1;

	wc.x = X(win);
	wc.y = Y(win);
	wc.width = WIDTH(win);
	wc.height = HEIGHT(win);

	/*
	 * Retain floater and transient geometry for correct positioning
	 * when ws changes region
	 */
	if (!(win->ewmh_flags & EWMH_F_FULLSCREEN))
		store_float_geom(win, r);

	DNPRINTF(SWM_D_MISC, "stack_floater: window: %lu, (x,y) w x h: (%d,%d) "
	    "%d x %d\n", win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
}

#define SWAPXY(g)	do {				\
	int tmp;					\
	tmp = (g)->y; (g)->y = (g)->x; (g)->x = tmp;	\
	tmp = (g)->h; (g)->h = (g)->w; (g)->w = tmp;	\
} while (0)
void
stack_master(struct workspace *ws, struct swm_geometry *g, int rot, int flip)
{
	XWindowChanges		wc;
	XWindowAttributes	wa;
	struct swm_geometry	win_g, r_g = *g;
	struct ws_win		*win, *fs_win = 0;
	int			i, j, s, stacks;
	int			w_inc = 1, h_inc, w_base = 1, h_base;
	int			hrh, extra = 0, h_slice, last_h = 0;
	int			split, colno, winno, mwin, msize, mscale;
	int			remain, missing, v_slice, reconfigure;
	unsigned int		mask;

	DNPRINTF(SWM_D_STACK, "stack_master: workspace: %d, rot: %s, "
	    "flip: %s\n", ws->idx, YESNO(rot), YESNO(flip));

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry)
		if (win->transient == 0 && win->floating == 0)
			break;

	if (win == NULL)
		goto notiles;

	if (rot) {
		w_inc = win->sh.width_inc;
		w_base = win->sh.base_width;
		mwin = ws->l_state.horizontal_mwin;
		mscale = ws->l_state.horizontal_msize;
		stacks = ws->l_state.horizontal_stacks;
		SWAPXY(&r_g);
	} else {
		w_inc = win->sh.height_inc;
		w_base = win->sh.base_height;
		mwin = ws->l_state.vertical_mwin;
		mscale = ws->l_state.vertical_msize;
		stacks = ws->l_state.vertical_stacks;
	}
	win_g = r_g;

	if (stacks > winno - mwin)
		stacks = winno - mwin;
	if (stacks < 1)
		stacks = 1;

	h_slice = r_g.h / SWM_H_SLICE;
	if (mwin && winno > mwin) {
		v_slice = r_g.w / SWM_V_SLICE;

		split = mwin;
		colno = split;
		win_g.w = v_slice * mscale;

		if (w_inc > 1 && w_inc < v_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.w - w_base) % w_inc;
			win_g.w -= remain;
		}

		msize = win_g.w;
		if (flip)
			win_g.x += r_g.w - msize;
	} else {
		msize = -2;
		colno = split = winno / stacks;
		win_g.w = ((r_g.w - (stacks * 2 * border_width) +
		    2 * border_width) / stacks);
	}
	hrh = r_g.h / colno;
	extra = r_g.h - (colno * hrh);
	win_g.h = hrh - 2 * border_width;

	/*  stack all the tiled windows */
	i = j = 0, s = stacks;
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient != 0 || win->floating != 0)
			continue;

		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		if (split && i == split) {
			colno = (winno - mwin) / stacks;
			if (s <= (winno - mwin) % stacks)
				colno++;
			split = split + colno;
			hrh = (r_g.h / colno);
			extra = r_g.h - (colno * hrh);
			if (flip)
				win_g.x = r_g.x;
			else
				win_g.x += win_g.w + 2 * border_width;
			win_g.w = (r_g.w - msize -
			    (stacks * 2 * border_width)) / stacks;
			if (s == 1)
				win_g.w += (r_g.w - msize -
				    (stacks * 2 * border_width)) % stacks;
			s--;
			j = 0;
		}
		win_g.h = hrh - 2 * border_width;
		if (rot) {
			h_inc = win->sh.width_inc;
			h_base = win->sh.base_width;
		} else {
			h_inc =	win->sh.height_inc;
			h_base = win->sh.base_height;
		}
		if (j == colno - 1) {
			win_g.h = hrh + extra;
		} else if (h_inc > 1 && h_inc < h_slice) {
			/* adjust for window's requested size increment */
			remain = (win_g.h - h_base) % h_inc;
			missing = h_inc - remain;

			if (missing <= extra || j == 0) {
				extra -= missing;
				win_g.h += missing;
			} else {
				win_g.h -= remain;
				extra += remain;
			}
		}

		if (j == 0)
			win_g.y = r_g.y;
		else
			win_g.y += last_h + 2 * border_width;

		bzero(&wc, sizeof wc);
		if (disable_border && winno == 1){
			wc.border_width = 0;
			win_g.w += 2 * border_width;
			win_g.h += 2 * border_width;
		} else
			wc.border_width = border_width;
		reconfigure = 0;
		if (rot) {
			if (X(win) != win_g.y || Y(win) != win_g.x ||
			    WIDTH(win) != win_g.h || HEIGHT(win) != win_g.w) {
				reconfigure = 1;
				X(win) = wc.x = win_g.y;
				Y(win) = wc.y = win_g.x;
				WIDTH(win) = wc.width = win_g.h;
				HEIGHT(win) = wc.height = win_g.w;
			}
		} else {
			if (X(win) != win_g.x || Y(win) != win_g.y ||
			    WIDTH(win) != win_g.w || HEIGHT(win) != win_g.h) {
				reconfigure = 1;
				X(win) = wc.x = win_g.x;
				Y(win) = wc.y = win_g.y;
				WIDTH(win) = wc.width = win_g.w;
				HEIGHT(win) = wc.height = win_g.h;
			}
		}
		if (reconfigure) {
			mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
			XConfigureWindow(display, win->id, mask, &wc);
		}

		if (XGetWindowAttributes(display, win->id, &wa))
			if (wa.map_state == IsUnmapped)
				XMapRaised(display, win->id);

		last_h = win_g.h;
		i++;
		j++;
	}

notiles:
	/* now, stack all the floaters and transients */
	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient == 0 && win->floating == 0)
			continue;
		if (win->ewmh_flags & EWMH_F_FULLSCREEN) {
			fs_win = win;
			continue;
		}

		stack_floater(win, ws->r);
		XMapRaised(display, win->id);
	}

	if (fs_win) {
		stack_floater(fs_win, ws->r);
		XMapRaised(display, fs_win->id);
	}
}

void
vertical_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "vertical_config: id: %d, workspace: %d\n",
	    id, ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.vertical_msize = SWM_V_SLICE / 2;
		ws->l_state.vertical_mwin = 1;
		ws->l_state.vertical_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.vertical_msize > 1)
			ws->l_state.vertical_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.vertical_msize < SWM_V_SLICE - 1)
			ws->l_state.vertical_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.vertical_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.vertical_mwin > 0)
			ws->l_state.vertical_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.vertical_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.vertical_stacks > 1)
			ws->l_state.vertical_stacks--;
		break;
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.vertical_flip = !ws->l_state.vertical_flip;
		break;
	default:
		return;
	}
}

void
vertical_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "vertical_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 0, ws->l_state.vertical_flip);
}

void
horizontal_config(struct workspace *ws, int id)
{
	DNPRINTF(SWM_D_STACK, "horizontal_config: workspace: %d\n", ws->idx);

	switch (id) {
	case SWM_ARG_ID_STACKRESET:
	case SWM_ARG_ID_STACKINIT:
		ws->l_state.horizontal_mwin = 1;
		ws->l_state.horizontal_msize = SWM_H_SLICE / 2;
		ws->l_state.horizontal_stacks = 1;
		break;
	case SWM_ARG_ID_MASTERSHRINK:
		if (ws->l_state.horizontal_msize > 1)
			ws->l_state.horizontal_msize--;
		break;
	case SWM_ARG_ID_MASTERGROW:
		if (ws->l_state.horizontal_msize < SWM_H_SLICE - 1)
			ws->l_state.horizontal_msize++;
		break;
	case SWM_ARG_ID_MASTERADD:
		ws->l_state.horizontal_mwin++;
		break;
	case SWM_ARG_ID_MASTERDEL:
		if (ws->l_state.horizontal_mwin > 0)
			ws->l_state.horizontal_mwin--;
		break;
	case SWM_ARG_ID_STACKINC:
		ws->l_state.horizontal_stacks++;
		break;
	case SWM_ARG_ID_STACKDEC:
		if (ws->l_state.horizontal_stacks > 1)
			ws->l_state.horizontal_stacks--;
		break;
	case SWM_ARG_ID_FLIPLAYOUT:
		ws->l_state.horizontal_flip = !ws->l_state.horizontal_flip;
		break;
	default:
		return;
	}
}

void
horizontal_stack(struct workspace *ws, struct swm_geometry *g)
{
	DNPRINTF(SWM_D_STACK, "horizontal_stack: workspace: %d\n", ws->idx);

	stack_master(ws, g, 1, ws->l_state.horizontal_flip);
}

/* fullscreen view */
void
max_stack(struct workspace *ws, struct swm_geometry *g)
{
	XWindowChanges		wc;
	struct swm_geometry	gg = *g;
	struct ws_win		*win, *wintrans = NULL, *parent = NULL;
	unsigned int		mask;
	int			winno;

	DNPRINTF(SWM_D_STACK, "max_stack: workspace: %d\n", ws->idx);

	if (ws == NULL)
		return;

	winno = count_win(ws, 0);
	if (winno == 0 && count_win(ws, 1) == 0)
		return;

	TAILQ_FOREACH(win, &ws->winlist, entry) {
		if (win->transient) {
			wintrans = win;
			parent = find_window(win->transient);
			continue;
		}

		if (win->floating && win->floatmaxed == 0 ) {
			/*
			 * retain geometry for retrieval on exit from
			 * max_stack mode
			 */
			store_float_geom(win, ws->r);
			win->floatmaxed = 1;
		}

		/* only reconfigure if necessary */
		if (X(win) != gg.x || Y(win) != gg.y || WIDTH(win) != gg.w ||
		    HEIGHT(win) != gg.h) {
			bzero(&wc, sizeof wc);
			X(win) = wc.x = gg.x;
			Y(win) = wc.y = gg.y;
			wc.border_width = 0;
			WIDTH(win) = wc.width = gg.w + 2 * border_width;
			HEIGHT(win) = wc.height = gg.h +
				2 * border_width;
			mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
			XConfigureWindow(display, win->id, mask, &wc);
		}
		/* unmap only if we don't have multi screen */
		if (win != ws->focus)
			if (!(ScreenCount(display) > 1 || outputs > 1))
				unmap_window(win);
	}

	/* put the last transient on top */
	if (wintrans) {
		if (parent)
			XMapRaised(display, parent->id);
		stack_floater(wintrans, ws->r);
		focus_magic(wintrans);
	}
}

void
send_to_ws(struct swm_region *r, union arg *args)
{
	int			wsid = args->id;
	struct ws_win		*win = NULL, *parent;
	struct workspace	*ws, *nws;
	Atom			ws_idx_atom = 0;
	unsigned char		ws_idx_str[SWM_PROPLEN];
	union arg		a;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;
	if (win == NULL)
		return;
	if (win->ws->idx == wsid)
		return;

	DNPRINTF(SWM_D_MOVE, "send_to_ws: window: 0x%lx\n", win->id);

	ws = win->ws;
	nws = &win->s->ws[wsid];

	a.id = SWM_ARG_ID_FOCUSPREV;
	focus(r, &a);
	if (win->transient) {
		parent = find_window(win->transient);
		if (parent) {
			unmap_window(parent);
			TAILQ_REMOVE(&ws->winlist, parent, entry);
			TAILQ_INSERT_TAIL(&nws->winlist, parent, entry);
			parent->ws = nws;
		}
	}
	unmap_window(win);
	TAILQ_REMOVE(&ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&nws->winlist, win, entry);
	if (TAILQ_EMPTY(&ws->winlist))
		r->ws->focus = NULL;
	win->ws = nws;

	/* Try to update the window's workspace property */
	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);
	if (ws_idx_atom &&
	    snprintf((char *)ws_idx_str, SWM_PROPLEN, "%d", nws->idx) <
	        SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "send_to_ws: set property: _SWM_WS: %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, strlen((char *)ws_idx_str));
	}

	stack();
}

void
raise_toggle(struct swm_region *r, union arg *args)
{
	if (r == NULL || r->ws == NULL)
		return;

	r->ws->always_raise = !r->ws->always_raise;

	/* bring floaters back to top */
	if (r->ws->always_raise == 0)
		stack();
}

void
wkill(struct swm_region *r, union arg *args)
{
	DNPRINTF(SWM_D_MISC, "wkill: id: %d\n", args->id);

	if (r->ws->focus == NULL)
		return;

	if (args->id == SWM_ARG_ID_KILLWINDOW)
		XKillClient(display, r->ws->focus->id);
	else
		if (r->ws->focus->can_delete)
			client_msg(r->ws->focus, adelete);
}


int
floating_toggle_win(struct ws_win *win)
{
	struct swm_region	*r;

	if (win == NULL)
		return 0;

	if (!win->ws->r)
		return 0;

	r = win->ws->r;

	/* reject floating toggles in max stack mode */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK])
		return 0;

	if (win->floating) {
		if (!win->floatmaxed) {
			/* retain position for refloat */
			store_float_geom(win, r);
		}
		win->floating = 0;
	} else {
		if (win->g_floatvalid) {
			/* refloat at last floating relative position */
			X(win) = win->g_float.x - win->rg_float.x + X(r);
			Y(win) = win->g_float.y - win->rg_float.y + Y(r);
			WIDTH(win) = win->g_float.w;
			HEIGHT(win) = win->g_float.h;
		}
		win->floating = 1;
	}

	ewmh_update_actions(win);

	return 1;
}

void
floating_toggle(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = r->ws->focus;
	union arg		a;

	if (win == NULL)
		return;

	ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
	    _NET_WM_STATE_TOGGLE);

	stack();
	if (win == win->ws->focus) {
		a.id = SWM_ARG_ID_FOCUSCUR;
		focus(win->ws->r, &a);
	}
}

void
update_window(struct ws_win *win)
{
	unsigned int		mask;
	XWindowChanges		wc;

	bzero(&wc, sizeof wc);
	mask = CWBorderWidth | CWWidth | CWHeight | CWX | CWY;
	wc.border_width = border_width;
	wc.x = X(win);
	wc.y = Y(win);
	wc.width = WIDTH(win);
	wc.height = HEIGHT(win);

	DNPRINTF(SWM_D_MISC, "update_window: window: 0x%lx, (x,y) w x h: "
	    "(%d,%d) %d x %d\n", win->id, wc.x, wc.y, wc.width, wc.height);

	XConfigureWindow(display, win->id, mask, &wc);
}

#define SWM_RESIZE_STEPS	(50)

void
resize(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	struct swm_region	*r = NULL;
	int			resize_step = 0;
	Window			rr, cr;
	int			x, y, wx, wy;
	unsigned int		mask;
	struct swm_geometry	g;
	int			dx, dy;

	if (win == NULL)
		return;
	r = win->ws->r;

	DNPRINTF(SWM_D_MOUSE, "resize: window: 0x%lx, floating: %s, "
	    "transient: 0x%lx\n", win->id, YESNO(win->floating),
	    win->transient);

	if (!(win->transient != 0 || win->floating != 0))
		return;

	/* reject resizes in max mode for floaters (transient ok) */
	if (win->floatmaxed)
		return;

	win->manual = 1;
	ewmh_update_win_state(win, ewmh[_SWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);

	stack();

	switch (args->id) {
	case SWM_ARG_ID_WIDTHSHRINK:
		WIDTH(win) -= SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_WIDTHGROW:
		WIDTH(win) += SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_HEIGHTSHRINK:
		HEIGHT(win) -= SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	case SWM_ARG_ID_HEIGHTGROW:
		HEIGHT(win) += SWM_RESIZE_STEPS;
		resize_step = 1;
		break;
	default:
		break;
	}
	if (resize_step) {
		if(HEIGHT(win) < 1)
			HEIGHT(win) = 1;
		if(WIDTH(win) < 1)
			WIDTH(win) = 1;
		update_window(win);
		store_float_geom(win,r);
		return;
	}

	/* get cursor offset from window root */
	if (!XQueryPointer(display, win->id, &rr, &cr, &x, &y, &wx, &wy, &mask))
	    return;

	g = win->g;

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
		XFreeCursor(display, None);
		return;
	}

	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* cursor offset/delta from start of the operation */
			dx = ev.xmotion.x_root - x;
			dy = ev.xmotion.y_root - y;

			/* vertical */
			if (g.h + dy < 1)
				dy = 1 - g.h;
			Y(win) = g.y;
			HEIGHT(win) = g.h + dy;

			/* horizontal */
			if (g.w + dx < 1)
				dx = 1 - g.w;
			X(win) = g.x;
			WIDTH(win) = g.w + dx;

			/* not free, don't sync more than 120 times / second */
			if ((ev.xmotion.time - time) > (1000 / 120) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				update_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		update_window(win);
	}
	store_float_geom(win,r);

	XUngrabPointer(display, CurrentTime);
	XFreeCursor(display, None);

	/* drain events */
	drain_enter_notify();
}

void
resize_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	resize(win, args);
}

#define SWM_MOVE_STEPS	(50)

void
move(struct ws_win *win, union arg *args)
{
	XEvent			ev;
	Time			time = 0;
	int			move_step = 0;
	struct swm_region	*r = NULL;

	Window			rr, cr;
	int			x, y, wx, wy;
	unsigned int		mask;

	if (win == NULL)
		return;
	r = win->ws->r;

	DNPRINTF(SWM_D_MOUSE, "move: window: 0x%lx, floating: %s, transient: "
	    "0x%lx\n", win->id, YESNO(win->floating), win->transient);

	/* in max_stack mode should only move transients */
	if (win->ws->cur_layout == &layouts[SWM_MAX_STACK] && !win->transient)
		return;

	win->manual = 1;
	if (win->floating == 0 && !win->transient) {
		store_float_geom(win,r);
		ewmh_update_win_state(win, ewmh[_NET_WM_STATE_ABOVE].atom,
		    _NET_WM_STATE_ADD);
	}
	ewmh_update_win_state(win, ewmh[_SWM_WM_STATE_MANUAL].atom,
	    _NET_WM_STATE_ADD);

	stack();

	move_step = 0;
	switch (args->id) {
	case SWM_ARG_ID_MOVELEFT:
		X(win) -= (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVERIGHT:
		X(win) += (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVEUP:
		Y(win) -= (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	case SWM_ARG_ID_MOVEDOWN:
		Y(win) += (SWM_MOVE_STEPS - border_width);
		move_step = 1;
		break;
	default:
		break;
	}
	if (move_step) {
		update_window(win);
		store_float_geom(win, r);
		stack();
		return;
	}

	if (XGrabPointer(display, win->id, False, MOUSEMASK, GrabModeAsync,
	    GrabModeAsync, None, None,
	    CurrentTime) != GrabSuccess)
		return;

	/* get cursor offset from window root */
	if (!XQueryPointer(display, win->id, &rr, &cr, &x, &y, &wx, &wy, &mask))
	    return;

	do {
		XMaskEvent(display, MOUSEMASK | ExposureMask |
		    SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			X(win) = ev.xmotion.x_root - wx - border_width;
			Y(win) = ev.xmotion.y_root - wy - border_width;

			/* not free, don't sync more than 120 times / second */
			if ((ev.xmotion.time - time) > (1000 / 120) ) {
				time = ev.xmotion.time;
				XSync(display, False);
				update_window(win);
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	if (time) {
		XSync(display, False);
		update_window(win);
	}
	store_float_geom(win,r);
	XUngrabPointer(display, CurrentTime);

	/* drain events */
	drain_enter_notify();
}

void
move_step(struct swm_region *r, union arg *args)
{
	struct ws_win		*win = NULL;

	if (r && r->ws && r->ws->focus)
		win = r->ws->focus;
	else
		return;

	if (!(win->transient != 0 || win->floating != 0))
		return;

	move(win, args);
}


/* user/key callable function IDs */
enum keyfuncid {
	kf_cycle_layout,
	kf_flip_layout,
	kf_stack_reset,
	kf_master_shrink,
	kf_master_grow,
	kf_master_add,
	kf_master_del,
	kf_stack_inc,
	kf_stack_dec,
	kf_swap_main,
	kf_focus_next,
	kf_focus_prev,
	kf_swap_next,
	kf_swap_prev,
	kf_quit,
	kf_restart,
	kf_focus_main,
	kf_ws_1,
	kf_ws_2,
	kf_ws_3,
	kf_ws_4,
	kf_ws_5,
	kf_ws_6,
	kf_ws_7,
	kf_ws_8,
	kf_ws_9,
	kf_ws_10,
	kf_ws_next,
	kf_ws_prev,
	kf_ws_next_all,
	kf_ws_prev_all,
	kf_ws_prior,
	kf_screen_next,
	kf_screen_prev,
	kf_mvws_1,
	kf_mvws_2,
	kf_mvws_3,
	kf_mvws_4,
	kf_mvws_5,
	kf_mvws_6,
	kf_mvws_7,
	kf_mvws_8,
	kf_mvws_9,
	kf_mvws_10,
	kf_wind_kill,
	kf_wind_del,
	kf_float_toggle,
	kf_spawn_custom,
	kf_raise_toggle,
	kf_width_shrink,
	kf_width_grow,
	kf_height_shrink,
	kf_height_grow,
	kf_move_left,
	kf_move_right,
	kf_move_up,
	kf_move_down,
	kf_name_workspace,
	kf_dumpwins, /* MUST BE LAST */
	kf_invalid
};

/* key definitions */
void dummykeyfunc(struct swm_region *r, union arg *args) {};
struct keyfunc {
	char			name[SWM_FUNCNAME_LEN];
	void			(*func)(struct swm_region *r, union arg *);
	union arg		args;
} keyfuncs[kf_invalid + 1] = {
	/* name			function	argument */
	{ "cycle_layout",	cycle_layout,	{0} },
	{ "flip_layout",	stack_config,	{.id = SWM_ARG_ID_FLIPLAYOUT} },
	{ "stack_reset",	stack_config,	{.id = SWM_ARG_ID_STACKRESET} },
	{ "master_shrink",	stack_config,	{.id = SWM_ARG_ID_MASTERSHRINK} },
	{ "master_grow",	stack_config,	{.id = SWM_ARG_ID_MASTERGROW} },
	{ "master_add",		stack_config,	{.id = SWM_ARG_ID_MASTERADD} },
	{ "master_del",		stack_config,	{.id = SWM_ARG_ID_MASTERDEL} },
	{ "stack_inc",		stack_config,	{.id = SWM_ARG_ID_STACKINC} },
	{ "stack_dec",		stack_config,	{.id = SWM_ARG_ID_STACKDEC} },
	{ "swap_main",		swapwin,	{.id = SWM_ARG_ID_SWAPMAIN} },
	{ "focus_next",		focus,		{.id = SWM_ARG_ID_FOCUSNEXT} },
	{ "focus_prev",		focus,		{.id = SWM_ARG_ID_FOCUSPREV} },
	{ "swap_next",		swapwin,	{.id = SWM_ARG_ID_SWAPNEXT} },
	{ "swap_prev",		swapwin,	{.id = SWM_ARG_ID_SWAPPREV} },
	{ "quit",		quit,		{0} },
	{ "restart",		restart,	{0} },
	{ "focus_main",		focus,		{.id = SWM_ARG_ID_FOCUSMAIN} },
	{ "ws_1",		switchws,	{.id = 0} },
	{ "ws_2",		switchws,	{.id = 1} },
	{ "ws_3",		switchws,	{.id = 2} },
	{ "ws_4",		switchws,	{.id = 3} },
	{ "ws_5",		switchws,	{.id = 4} },
	{ "ws_6",		switchws,	{.id = 5} },
	{ "ws_7",		switchws,	{.id = 6} },
	{ "ws_8",		switchws,	{.id = 7} },
	{ "ws_9",		switchws,	{.id = 8} },
	{ "ws_10",		switchws,	{.id = 9} },
	{ "ws_next",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP} },
	{ "ws_prev",		cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN} },
	{ "ws_next_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_UP_ALL} },
	{ "ws_prev_all",	cyclews,	{.id = SWM_ARG_ID_CYCLEWS_DOWN_ALL} },
	{ "ws_prior",		priorws,	{0} },
	{ "screen_next",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_UP} },
	{ "screen_prev",	cyclescr,	{.id = SWM_ARG_ID_CYCLESC_DOWN} },
	{ "mvws_1",		send_to_ws,	{.id = 0} },
	{ "mvws_2",		send_to_ws,	{.id = 1} },
	{ "mvws_3",		send_to_ws,	{.id = 2} },
	{ "mvws_4",		send_to_ws,	{.id = 3} },
	{ "mvws_5",		send_to_ws,	{.id = 4} },
	{ "mvws_6",		send_to_ws,	{.id = 5} },
	{ "mvws_7",		send_to_ws,	{.id = 6} },
	{ "mvws_8",		send_to_ws,	{.id = 7} },
	{ "mvws_9",		send_to_ws,	{.id = 8} },
	{ "mvws_10",		send_to_ws,	{.id = 9} },
	{ "wind_kill",		wkill,		{.id = SWM_ARG_ID_KILLWINDOW} },
	{ "wind_del",		wkill,		{.id = SWM_ARG_ID_DELETEWINDOW} },
	{ "float_toggle",	floating_toggle,	{0} },
	{ "spawn_custom",	dummykeyfunc,	{0} },
	{ "raise_toggle",	raise_toggle,	{0} },
	{ "width_shrink",	resize_step,	{.id = SWM_ARG_ID_WIDTHSHRINK} },
	{ "width_grow",		resize_step,	{.id = SWM_ARG_ID_WIDTHGROW} },
	{ "height_shrink",	resize_step,	{.id = SWM_ARG_ID_HEIGHTSHRINK} },
	{ "height_grow",	resize_step,	{.id = SWM_ARG_ID_HEIGHTGROW} },
	{ "move_left",		move_step,	{.id = SWM_ARG_ID_MOVELEFT} },
	{ "move_right",		move_step,	{.id = SWM_ARG_ID_MOVERIGHT} },
	{ "move_up",		move_step,	{.id = SWM_ARG_ID_MOVEUP} },
	{ "move_down",		move_step,	{.id = SWM_ARG_ID_MOVEDOWN} },
	{ "dumpwins",		dumpwins,	{0} }, /* MUST BE LAST */
	{ "invalid key func",	NULL,		{0} },
};
struct key {
	RB_ENTRY(key)		entry;
	unsigned int		mod;
	KeySym			keysym;
	enum keyfuncid		funcid;
	char			*spawn_name;
};
RB_HEAD(key_list, key);

int
key_cmp(struct key *kp1, struct key *kp2)
{
	if (kp1->keysym < kp2->keysym)
		return (-1);
	if (kp1->keysym > kp2->keysym)
		return (1);

	if (kp1->mod < kp2->mod)
		return (-1);
	if (kp1->mod > kp2->mod)
		return (1);

	return (0);
}

RB_GENERATE_STATIC(key_list, key, entry, key_cmp);
struct key_list			keys;

/* mouse */
enum { client_click, root_click };
struct button {
	unsigned int		action;
	unsigned int		mask;
	unsigned int		button;
	void			(*func)(struct ws_win *, union arg *);
	union arg		args;
} buttons[] = {
	  /* action	key		mouse button	func	args */
	{ client_click,	MODKEY,		Button3,	resize,	{0} },
	{ client_click,	MODKEY,		Button1,	move,	{0} },
};

void
update_modkey(unsigned int mod)
{
	int			i;
	struct key		*kp;

	mod_key = mod;
	RB_FOREACH(kp, key_list, &keys)
		if (kp->mod & ShiftMask)
			kp->mod = mod | ShiftMask;
		else
			kp->mod = mod;

	for (i = 0; i < LENGTH(buttons); i++)
		if (buttons[i].mask & ShiftMask)
			buttons[i].mask = mod | ShiftMask;
		else
			buttons[i].mask = mod;
}

/* spawn */
struct spawn_prog {
	TAILQ_ENTRY(spawn_prog)	entry;
	char			*name;
	int			argc;
	char			**argv;
};
TAILQ_HEAD(spawn_list, spawn_prog);
struct spawn_list		spawns = TAILQ_HEAD_INITIALIZER(spawns);

int
spawn_expand(struct swm_region *r, union arg *args, char *spawn_name,
    char ***ret_args)
{
	struct spawn_prog	*prog = NULL;
	int			i;
	char			*ap, **real_args;

	DNPRINTF(SWM_D_SPAWN, "spawn_expand: %s\n", spawn_name);

	/* find program */
	TAILQ_FOREACH(prog, &spawns, entry) {
		if (!strcasecmp(spawn_name, prog->name))
			break;
	}
	if (prog == NULL) {
		warnx("spawn_custom: program %s not found", spawn_name);
		return (-1);
	}

	/* make room for expanded args */
	if ((real_args = calloc(prog->argc + 1, sizeof(char *))) == NULL)
		err(1, "spawn_custom: calloc real_args");

	/* expand spawn_args into real_args */
	for (i = 0; i < prog->argc; i++) {
		ap = prog->argv[i];
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: raw arg: %s\n", ap);
		if (!strcasecmp(ap, "$color_focus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_FOCUS].name))
			    == NULL)
				err(1, "spawn_custom color focus");
		} else if (!strcasecmp(ap, "$color_unfocus")) {
			if ((real_args[i] =
			    strdup(r->s->c[SWM_S_COLOR_UNFOCUS].name))
			    == NULL)
				err(1, "spawn_custom color unfocus");
		} else {
			/* no match --> copy as is */
			if ((real_args[i] = strdup(ap)) == NULL)
				err(1, "spawn_custom strdup(ap)");
		}
		DNPRINTF(SWM_D_SPAWN, "spawn_custom: cooked arg: %s\n",
		    real_args[i]);
	}

#ifdef SWM_DEBUG
	DNPRINTF(SWM_D_SPAWN, "spawn_custom: result: ");
	for (i = 0; i < prog->argc; i++)
		DNPRINTF(SWM_D_SPAWN, "\"%s\" ", real_args[i]);
	DNPRINTF(SWM_D_SPAWN, "\n");
#endif
	*ret_args = real_args;
	return (prog->argc);
}

void
spawn_custom(struct swm_region *r, union arg *args, char *spawn_name)
{
	union arg		a;
	char			**real_args;
	int			spawn_argc, i;

	if ((spawn_argc = spawn_expand(r, args, spawn_name, &real_args)) < 0)
		return;
	a.argv = real_args;
	if (fork() == 0)
		spawn(r->ws->idx, &a, 1);

	for (i = 0; i < spawn_argc; i++)
		free(real_args[i]);
	free(real_args);
}

void
spawn_insert(char *name, char *args)
{
	char			*arg, *cp, *ptr;
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "spawn_insert: %s\n", name);

	if ((sp = calloc(1, sizeof *sp)) == NULL)
		err(1, "spawn_insert: malloc");
	if ((sp->name = strdup(name)) == NULL)
		err(1, "spawn_insert: strdup");

	/* convert the arguments to an argument list */
	if ((ptr = cp = strdup(args)) == NULL)
		err(1, "spawn_insert: strdup");
	while ((arg = strsep(&ptr, " \t")) != NULL) {
		/* empty field; skip it */
		if (*arg == '\0')
			continue;

		sp->argc++;
		if ((sp->argv = realloc(sp->argv, sp->argc *
		    sizeof *sp->argv)) == NULL)
			err(1, "spawn_insert: realloc");
		if ((sp->argv[sp->argc - 1] = strdup(arg)) == NULL)
			err(1, "spawn_insert: strdup");
	}
	free(cp);

	TAILQ_INSERT_TAIL(&spawns, sp, entry);
	DNPRINTF(SWM_D_SPAWN, "spawn_insert: leave\n");
}

void
spawn_remove(struct spawn_prog *sp)
{
	int			i;

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: %s\n", sp->name);

	TAILQ_REMOVE(&spawns, sp, entry);
	for (i = 0; i < sp->argc; i++)
		free(sp->argv[i]);
	free(sp->argv);
	free(sp->name);
	free(sp);

	DNPRINTF(SWM_D_SPAWN, "spawn_remove: leave\n");
}

void
spawn_replace(struct spawn_prog *sp, char *name, char *args)
{
	DNPRINTF(SWM_D_SPAWN, "spawn_replace: %s [%s]\n", sp->name, name);

	spawn_remove(sp);
	spawn_insert(name, args);

	DNPRINTF(SWM_D_SPAWN, "spawn_replace: leave\n");
}

void
setspawn(char *name, char *args)
{
	struct spawn_prog	*sp;

	DNPRINTF(SWM_D_SPAWN, "setspawn: %s\n", name);

	if (name == NULL)
		return;

	TAILQ_FOREACH(sp, &spawns, entry) {
		if (!strcmp(sp->name, name)) {
			if (*args == '\0')
				spawn_remove(sp);
			else
				spawn_replace(sp, name, args);
			DNPRINTF(SWM_D_SPAWN, "setspawn: leave\n");
			return;
		}
	}
	if (*args == '\0') {
		warnx("error: setspawn: cannot find program: %s", name);
		return;
	}

	spawn_insert(name, args);
	DNPRINTF(SWM_D_SPAWN, "setspawn: leave\n");
}

int
setconfspawn(char *selector, char *value, int flags)
{
	DNPRINTF(SWM_D_SPAWN, "setconfspawn: [%s] [%s]\n", selector, value);

	setspawn(selector, value);

	DNPRINTF(SWM_D_SPAWN, "setconfspawn: done\n");
	return (0);
}

/* key bindings */
#define SWM_MODNAME_SIZE	32
#define	SWM_KEY_WS		"\n+ \t"
int
parsekeys(char *keystr, unsigned int currmod, unsigned int *mod, KeySym *ks)
{
	char			*cp, *name;
	KeySym			uks;
	DNPRINTF(SWM_D_KEY, "parsekeys: enter [%s]\n", keystr);
	if (mod == NULL || ks == NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no mod or key vars\n");
		return (1);
	}
	if (keystr == NULL || strlen(keystr) == 0) {
		DNPRINTF(SWM_D_KEY, "parsekeys: no keystr\n");
		return (1);
	}
	cp = keystr;
	*ks = NoSymbol;
	*mod = 0;
	while ((name = strsep(&cp, SWM_KEY_WS)) != NULL) {
		DNPRINTF(SWM_D_KEY, "parsekeys: key [%s]\n", name);
		if (cp)
			cp += (long)strspn(cp, SWM_KEY_WS);
		if (strncasecmp(name, "MOD", SWM_MODNAME_SIZE) == 0)
			*mod |= currmod;
		else if (!strncasecmp(name, "Mod1", SWM_MODNAME_SIZE))
			*mod |= Mod1Mask;
		else if (!strncasecmp(name, "Mod2", SWM_MODNAME_SIZE))
			*mod += Mod2Mask;
		else if (!strncmp(name, "Mod3", SWM_MODNAME_SIZE))
			*mod |= Mod3Mask;
		else if (!strncmp(name, "Mod4", SWM_MODNAME_SIZE))
			*mod |= Mod4Mask;
		else if (strncasecmp(name, "SHIFT", SWM_MODNAME_SIZE) == 0)
			*mod |= ShiftMask;
		else if (strncasecmp(name, "CONTROL", SWM_MODNAME_SIZE) == 0)
			*mod |= ControlMask;
		else {
			*ks = XStringToKeysym(name);
			XConvertCase(*ks, ks, &uks);
			if (ks == NoSymbol) {
				DNPRINTF(SWM_D_KEY,
				    "parsekeys: invalid key %s\n",
				    name);
				return (1);
			}
		}
	}
	DNPRINTF(SWM_D_KEY, "parsekeys: leave ok\n");
	return (0);
}

char *
strdupsafe(char *str)
{
	if (str == NULL)
		return (NULL);
	else
		return (strdup(str));
}

void
key_insert(unsigned int mod, KeySym ks, enum keyfuncid kfid, char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "key_insert: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = malloc(sizeof *kp)) == NULL)
		err(1, "key_insert: malloc");

	kp->mod = mod;
	kp->keysym = ks;
	kp->funcid = kfid;
	kp->spawn_name = strdupsafe(spawn_name);
	RB_INSERT(key_list, &keys, kp);

	DNPRINTF(SWM_D_KEY, "key_insert: leave\n");
}

struct key *
key_lookup(unsigned int mod, KeySym ks)
{
	struct key		kp;

	kp.keysym = ks;
	kp.mod = mod;

	return (RB_FIND(key_list, &keys, &kp));
}

void
key_remove(struct key *kp)
{
	DNPRINTF(SWM_D_KEY, "key_remove: %s\n", keyfuncs[kp->funcid].name);

	RB_REMOVE(key_list, &keys, kp);
	free(kp->spawn_name);
	free(kp);

	DNPRINTF(SWM_D_KEY, "key_remove: leave\n");
}

void
key_replace(struct key *kp, unsigned int mod, KeySym ks, enum keyfuncid kfid,
    char *spawn_name)
{
	DNPRINTF(SWM_D_KEY, "key_replace: %s [%s]\n", keyfuncs[kp->funcid].name,
	    spawn_name);

	key_remove(kp);
	key_insert(mod, ks, kfid, spawn_name);

	DNPRINTF(SWM_D_KEY, "key_replace: leave\n");
}

void
setkeybinding(unsigned int mod, KeySym ks, enum keyfuncid kfid,
    char *spawn_name)
{
	struct key		*kp;

	DNPRINTF(SWM_D_KEY, "setkeybinding: enter %s [%s]\n",
	    keyfuncs[kfid].name, spawn_name);

	if ((kp = key_lookup(mod, ks)) != NULL) {
		if (kfid == kf_invalid)
			key_remove(kp);
		else
			key_replace(kp, mod, ks, kfid, spawn_name);
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}
	if (kfid == kf_invalid) {
		warnx("error: setkeybinding: cannot find mod/key combination");
		DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
		return;
	}

	key_insert(mod, ks, kfid, spawn_name);
	DNPRINTF(SWM_D_KEY, "setkeybinding: leave\n");
}

int
setconfbinding(char *selector, char *value, int flags)
{
	enum keyfuncid		kfid;
	unsigned int		mod;
	KeySym			ks;
	struct spawn_prog	*sp;
	DNPRINTF(SWM_D_KEY, "setconfbinding: enter\n");
	if (selector == NULL) {
		DNPRINTF(SWM_D_KEY, "setconfbinding: unbind %s\n", value);
		if (parsekeys(value, mod_key, &mod, &ks) == 0) {
			kfid = kf_invalid;
			setkeybinding(mod, ks, kfid, NULL);
			return (0);
		} else
			return (1);
	}
	/* search by key function name */
	for (kfid = 0; kfid < kf_invalid; (kfid)++) {
		if (strncasecmp(selector, keyfuncs[kfid].name,
		    SWM_FUNCNAME_LEN) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kfid, NULL);
				return (0);
			} else
				return (1);
		}
	}
	/* search by custom spawn name */
	TAILQ_FOREACH(sp, &spawns, entry) {
		if (strcasecmp(selector, sp->name) == 0) {
			DNPRINTF(SWM_D_KEY, "setconfbinding: %s: match\n",
			    selector);
			if (parsekeys(value, mod_key, &mod, &ks) == 0) {
				setkeybinding(mod, ks, kf_spawn_custom,
				    sp->name);
				return (0);
			} else
				return (1);
		}
	}
	DNPRINTF(SWM_D_KEY, "setconfbinding: no match\n");
	return (1);
}

void
updatenumlockmask(void)
{
	unsigned int		i, j;
	XModifierKeymap		*modmap;

	DNPRINTF(SWM_D_MISC, "updatenumlockmask\n");
	numlockmask = 0;
	modmap = XGetModifierMapping(display);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			    == XKeysymToKeycode(display, XK_Num_Lock))
				numlockmask = (1 << i);

	XFreeModifiermap(modmap);
}

void
grabkeys(void)
{
	unsigned int		j, k;
	KeyCode			code;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask | LockMask };
	struct key		*kp;

	DNPRINTF(SWM_D_MISC, "grabkeys\n");
	updatenumlockmask();

	for (k = 0; k < ScreenCount(display); k++) {
		if (TAILQ_EMPTY(&screens[k].rl))
			continue;
		XUngrabKey(display, AnyKey, AnyModifier, screens[k].root);
		RB_FOREACH(kp, key_list, &keys) {
			if ((code = XKeysymToKeycode(display, kp->keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(display, code,
					    kp->mod | modifiers[j],
					    screens[k].root, True,
					    GrabModeAsync, GrabModeAsync);
		}
	}
}

void
grabbuttons(struct ws_win *win, int focused)
{
	unsigned int		i, j;
	unsigned int		modifiers[] =
	    { 0, LockMask, numlockmask, numlockmask|LockMask };

	updatenumlockmask();
	XUngrabButton(display, AnyButton, AnyModifier, win->id);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].action == client_click)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(display, buttons[i].button,
					    buttons[i].mask | modifiers[j],
					    win->id, False, BUTTONMASK,
					    GrabModeAsync, GrabModeSync, None,
					    None);
	} else
		XGrabButton(display, AnyButton, AnyModifier, win->id, False,
		    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
}

const char *quirkname[] = {
	"NONE",		/* config string for "no value" */
	"FLOAT",
	"TRANSSZ",
	"ANYWHERE",
	"FULLSCREEN",
	"FOCUSPREV",
};

/* SWM_Q_WS: retain '|' for back compat for now (2009-08-11) */
#define	SWM_Q_WS		"\n|+ \t"
int
parsequirks(char *qstr, unsigned long *quirk)
{
	char			*cp, *name;
	int			i;

	if (quirk == NULL)
		return (1);

	cp = qstr;
	*quirk = 0;
	while ((name = strsep(&cp, SWM_Q_WS)) != NULL) {
		if (cp)
			cp += (long)strspn(cp, SWM_Q_WS);
		for (i = 0; i < LENGTH(quirkname); i++) {
			if (!strncasecmp(name, quirkname[i], SWM_QUIRK_LEN)) {
				DNPRINTF(SWM_D_QUIRK,
				    "parsequirks: %s\n", name);
				if (i == 0) {
					*quirk = 0;
					return (0);
				}
				*quirk |= 1 << (i-1);
				break;
			}
		}
		if (i >= LENGTH(quirkname)) {
			DNPRINTF(SWM_D_QUIRK,
			    "parsequirks: invalid quirk [%s]\n", name);
			return (1);
		}
	}
	return (0);
}

void
quirk_insert(const char *class, const char *name, unsigned long quirk)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "quirk_insert: %s:%s [%lu]\n", class, name,
	    quirk);

	if ((qp = malloc(sizeof *qp)) == NULL)
		err(1, "quirk_insert: malloc");
	if ((qp->class = strdup(class)) == NULL)
		err(1, "quirk_insert: strdup");
	if ((qp->name = strdup(name)) == NULL)
		err(1, "quirk_insert: strdup");

	qp->quirk = quirk;
	TAILQ_INSERT_TAIL(&quirks, qp, entry);

	DNPRINTF(SWM_D_QUIRK, "quirk_insert: leave\n");
}

void
quirk_remove(struct quirk *qp)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_remove: %s:%s [%lu]\n", qp->class,
	    qp->name, qp->quirk);

	TAILQ_REMOVE(&quirks, qp, entry);
	free(qp->class);
	free(qp->name);
	free(qp);

	DNPRINTF(SWM_D_QUIRK, "quirk_remove: leave\n");
}

void
quirk_replace(struct quirk *qp, const char *class, const char *name,
    unsigned long quirk)
{
	DNPRINTF(SWM_D_QUIRK, "quirk_replace: %s:%s [%lu]\n", qp->class,
	    qp->name, qp->quirk);

	quirk_remove(qp);
	quirk_insert(class, name, quirk);

	DNPRINTF(SWM_D_QUIRK, "quirk_replace: leave\n");
}

void
setquirk(const char *class, const char *name, unsigned long quirk)
{
	struct quirk		*qp;

	DNPRINTF(SWM_D_QUIRK, "setquirk: enter %s:%s [%lu]\n", class, name,
	   quirk);

	TAILQ_FOREACH(qp, &quirks, entry) {
		if (!strcmp(qp->class, class) && !strcmp(qp->name, name)) {
			if (!quirk)
				quirk_remove(qp);
			else
				quirk_replace(qp, class, name, quirk);
			DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
			return;
		}
	}
	if (!quirk) {
		warnx("error: setquirk: cannot find class/name combination");
		return;
	}

	quirk_insert(class, name, quirk);
	DNPRINTF(SWM_D_QUIRK, "setquirk: leave\n");
}

int
setconfquirk(char *selector, char *value, int flags)
{
	char			*cp, *class, *name;
	int			retval;
	unsigned long		quirks;
	if (selector == NULL)
		return (0);
	if ((cp = strchr(selector, ':')) == NULL)
		return (0);
	*cp = '\0';
	class = selector;
	name = cp + 1;
	if ((retval = parsequirks(value, &quirks)) == 0)
		setquirk(class, name, quirks);
	return (retval);
}

enum	{
	  SWM_S_DISABLE_BORDER, SWM_S_BORDER_WIDTH,
	  SWM_S_DIALOG_RATIO
	};

int
setconfvalue(char *selector, char *value, int flags)
{
	switch (flags) {
	case SWM_S_DISABLE_BORDER:
		disable_border = atoi(value);
		break;
	case SWM_S_BORDER_WIDTH:
		border_width = atoi(value);
		break;
	case SWM_S_DIALOG_RATIO:
		dialog_ratio = atof(value);
		if (dialog_ratio > 1.0 || dialog_ratio <= .3)
			dialog_ratio = .6;
		break;
	default:
		return (1);
	}
	return (0);
}

int
setconfmodkey(char *selector, char *value, int flags)
{
	if (!strncasecmp(value, "Mod1", strlen("Mod1")))
		update_modkey(Mod1Mask);
	else if (!strncasecmp(value, "Mod2", strlen("Mod2")))
		update_modkey(Mod2Mask);
	else if (!strncasecmp(value, "Mod3", strlen("Mod3")))
		update_modkey(Mod3Mask);
	else if (!strncasecmp(value, "Mod4", strlen("Mod4")))
		update_modkey(Mod4Mask);
	else
		return (1);
	return (0);
}

int
setconfcolor(char *selector, char *value, int flags)
{
	setscreencolor(value, ((selector == NULL)?-1:atoi(selector)), flags);
	return (0);
}

int
setconfregion(char *selector, char *value, int flags)
{
	custom_region(value);
	return (0);
}

int
setautorun(char *selector, char *value, int flags)
{
	int			ws_id;
	char			s[1024];
	char			*ap, *sp = s;
	union arg		a;
	int			argc = 0;
	long			pid;
	struct pid_e		*p;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%1023c", &ws_id, s) != 2)
		errx(1, "invalid autorun entry, should be 'ws[<idx>]:command'");
	ws_id--;
	if (ws_id < 0 || ws_id >= SWM_WS_MAX)
		errx(1, "autorun: invalid workspace %d", ws_id + 1);

	/*
	 * This is a little intricate
	 *
	 * If the pid already exists we simply reuse it because it means it was
	 * used before AND not claimed by manage_window.  We get away with
	 * altering it in the parent after INSERT because this can not be a race
	 */
	a.argv = NULL;
	while ((ap = strsep(&sp, " \t")) != NULL) {
		if (*ap == '\0')
			continue;
		DNPRINTF(SWM_D_SPAWN, "setautorun: arg [%s]\n", ap);
		argc++;
		if ((a.argv = realloc(a.argv, argc * sizeof(char *))) == NULL)
			err(1, "setautorun: realloc");
		a.argv[argc - 1] = ap;
	}

	if ((a.argv = realloc(a.argv, (argc + 1) * sizeof(char *))) == NULL)
		err(1, "setautorun: realloc");
	a.argv[argc] = NULL;

	if ((pid = fork()) == 0) {
		spawn(ws_id, &a, 1);
		/* NOTREACHED */
		_exit(1);
	}
	free(a.argv);

	/* parent */
	p = find_pid(pid);
	if (p == NULL) {
		p = calloc(1, sizeof *p);
		if (p == NULL)
			return (1);
		TAILQ_INSERT_TAIL(&pidlist, p, entry);
	}

	p->pid = pid;
	p->ws = ws_id;

	return (0);
}

int
setlayout(char *selector, char *value, int flags)
{
	int			ws_id, i, x, mg, ma, si, raise;
	int			st = SWM_V_STACK;
	char			s[1024];
	struct workspace	*ws;

	if (getenv("SWM_STARTED"))
		return (0);

	bzero(s, sizeof s);
	if (sscanf(value, "ws[%d]:%d:%d:%d:%d:%1023c",
	    &ws_id, &mg, &ma, &si, &raise, s) != 6)
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");
	ws_id--;
	if (ws_id < 0 || ws_id >= SWM_WS_MAX)
		errx(1, "layout: invalid workspace %d", ws_id + 1);

	if (!strcasecmp(s, "vertical"))
		st = SWM_V_STACK;
	else if (!strcasecmp(s, "horizontal"))
		st = SWM_H_STACK;
	else if (!strcasecmp(s, "fullscreen"))
		st = SWM_MAX_STACK;
	else
		errx(1, "invalid layout entry, should be 'ws[<idx>]:"
		    "<master_grow>:<master_add>:<stack_inc>:<always_raise>:"
		    "<type>'");

	for (i = 0; i < ScreenCount(display); i++) {
		ws = (struct workspace *)&screens[i].ws;
		ws[ws_id].cur_layout = &layouts[st];

		ws[ws_id].always_raise = raise;
		if (st == SWM_MAX_STACK)
			continue;

		/* master grow */
		for (x = 0; x < abs(mg); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    mg >= 0 ?  SWM_ARG_ID_MASTERGROW :
			    SWM_ARG_ID_MASTERSHRINK);
			stack();
		}
		/* master add */
		for (x = 0; x < abs(ma); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    ma >= 0 ?  SWM_ARG_ID_MASTERADD :
			    SWM_ARG_ID_MASTERDEL);
			stack();
		}
		/* stack inc */
		for (x = 0; x < abs(si); x++) {
			ws[ws_id].cur_layout->l_config(&ws[ws_id],
			    si >= 0 ?  SWM_ARG_ID_STACKINC :
			    SWM_ARG_ID_STACKDEC);
			stack();
		}
	}

	return (0);
}

/* config options */
struct config_option {
	char			*optname;
	int			(*func)(char*, char*, int);
	int			funcflags;
};
struct config_option configopt[] = {
	{ "bind",			setconfbinding,	0 },
	{ "color_focus",		setconfcolor,	SWM_S_COLOR_FOCUS },
	{ "color_unfocus",		setconfcolor,	SWM_S_COLOR_UNFOCUS },
	{ "dialog_ratio",		setconfvalue,	SWM_S_DIALOG_RATIO },
	{ "modkey",			setconfmodkey,	0 },
	{ "program",			setconfspawn,	0 },
	{ "quirk",			setconfquirk,	0 },
	{ "region",			setconfregion,	0 },
	{ "disable_border",		setconfvalue,	SWM_S_DISABLE_BORDER },
	{ "border_width",		setconfvalue,	SWM_S_BORDER_WIDTH },
	{ "autorun",			setautorun,	0 },
	{ "layout",			setlayout,	0 },
};


int
conf_load(char *filename)
{
	FILE			*config;
	char			*line, *cp, *optsub, *optval;
	size_t			linelen, lineno = 0;
	int			wordlen, i, optind;
	struct config_option	*opt;

	DNPRINTF(SWM_D_CONF, "conf_load: begin\n");

	if (filename == NULL)
		errx(1, "conf_load: no filename");
	if ((config = fopen(filename, "r")) == NULL)
		err(1, "conf_load: fopen: %s", filename);

	while (!feof(config)) {
		if ((line = fparseln(config, &linelen, &lineno, NULL, 0))
		    == NULL) {
			if (ferror(config))
				err(1, "%s", filename);
			else
				continue;
		}
		cp = line;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		if (cp[0] == '\0') {
			/* empty line */
			free(line);
			continue;
		}
		/* get config option */
		wordlen = strcspn(cp, "=[ \t\n");
		if (wordlen == 0) {
			warnx("%s: line %zd: no option found",
			    filename, lineno);
			goto out;
		}
		optind = -1;
		for (i = 0; i < LENGTH(configopt); i++) {
			opt = &configopt[i];
			if (!strncasecmp(cp, opt->optname, wordlen) &&
			    strlen(opt->optname) == wordlen) {
				optind = i;
				break;
			}
		}
		if (optind == -1) {
			warnx("%s: line %zd: unknown option %.*s",
			    filename, lineno, wordlen, cp);
			goto out;
		}
		cp += wordlen;
		cp += strspn(cp, " \t\n"); /* eat whitespace */
		/* get [selector] if any */
		optsub = NULL;
		if (*cp == '[') {
			cp++;
			wordlen = strcspn(cp, "]");
			if (*cp != ']') {
				if (wordlen == 0) {
					warnx("%s: line %zd: syntax error",
					    filename, lineno);
					goto out;
				}

				if (asprintf(&optsub, "%.*s", wordlen, cp) ==
				    -1) {
					warnx("%s: line %zd: unable to allocate"
					    "memory for selector", filename,
					    lineno);
					goto out;
				}
			}
			cp += wordlen;
			cp += strspn(cp, "] \t\n"); /* eat trailing */
		}
		cp += strspn(cp, "= \t\n"); /* eat trailing */
		/* get RHS value */
		optval = strdup(cp);
		/* call function to deal with it all */
		if (configopt[optind].func(optsub, optval,
		    configopt[optind].funcflags) != 0)
			errx(1, "%s: line %zd: invalid data for %s",
			    filename, lineno, configopt[optind].optname);
		free(optval);
		free(optsub);
		free(line);
	}

	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load: end\n");

	return (0);

out:
	free(line);
	fclose(config);
	DNPRINTF(SWM_D_CONF, "conf_load: end with error.\n");

	return (1);
}

void
set_child_transient(struct ws_win *win, Window *trans)
{
	struct ws_win		*parent, *w;
	XWMHints		*wmh = NULL;
	struct swm_region	*r;
	struct workspace	*ws;

	parent = find_window(win->transient);
	if (parent)
		parent->child_trans = win;
	else {
		DNPRINTF(SWM_D_MISC, "set_child_transient: parent doesn't exist"
		    " for 0x%lx trans 0x%lx\n", win->id, win->transient);

		if (win->hints == NULL) {
			warnx("no hints for 0x%lx", win->id);
			return;
		}

		r = root_to_region(win->wa.root);
		ws = r->ws;
		/* parent doen't exist in our window list */
		TAILQ_FOREACH(w, &ws->winlist, entry) {
			if (wmh)
				XFree(wmh);

			if ((wmh = XGetWMHints(display, w->id)) == NULL) {
				warnx("can't get hints for 0x%lx", w->id);
				continue;
			}

			if (win->hints->window_group != wmh->window_group)
				continue;

			w->child_trans = win;
			win->transient = w->id;
			*trans = w->id;
			DNPRINTF(SWM_D_MISC, "set_child_transient: asjusting "
			    "transient to 0x%lx\n", win->transient);
			break;
		}
	}

	if (wmh)
		XFree(wmh);
}

long
window_get_pid(Window win)
{
	Atom			actual_type_return;
	int			actual_format_return = 0;
	unsigned long		nitems_return = 0;
	unsigned long		bytes_after_return = 0;
	long			*pid = NULL;
	long			ret = 0;
	unsigned char		*prop = NULL;

	if (XGetWindowProperty(display, win,
	    XInternAtom(display, "_NET_WM_PID", False), 0, 1, False,
	    XA_CARDINAL, &actual_type_return, &actual_format_return,
	    &nitems_return, &bytes_after_return,
	    (unsigned char**)(void*)&pid) != Success)
		goto tryharder;
	if (actual_type_return != XA_CARDINAL)
		goto tryharder;
	if (pid == NULL)
		goto tryharder;

	ret = *pid;
	XFree(pid);

	return (ret);

tryharder:
	if (XGetWindowProperty(display, win,
	    XInternAtom(display, "_SWM_PID", False), 0, SWM_PROPLEN, False,
	    XA_STRING, &actual_type_return, &actual_format_return,
	    &nitems_return, &bytes_after_return, &prop) != Success)
		return (0);
	if (actual_type_return != XA_STRING)
		return (0);
	if (prop == NULL)
		return (0);

	ret = strtol((const char *)prop, (char**)NULL, 10);
	XFree(prop);

	return (ret);
}

struct ws_win *
manage_window(Window id)
{
	Window			trans = 0;
	struct workspace	*ws;
	struct ws_win		*win, *ww;
	int			format, i, ws_idx, n, border_me = 0;
	unsigned long		nitems, bytes;
	Atom			ws_idx_atom = 0, type;
	Atom			*prot = NULL, *pp;
	unsigned char		ws_idx_str[SWM_PROPLEN], *prop = NULL;
	struct swm_region	*r;
	long			mask = 0;
	XWindowChanges		wc;
	struct pid_e		*p;
	struct quirk		*qp;

	if ((win = find_window(id)) != NULL)
		return (win);	/* already being managed */

	/* see if we are on the unmanaged list */
	if ((win = find_unmanaged_window(id)) != NULL) {
		DNPRINTF(SWM_D_MISC, "manage_window: previously unmanaged "
		    "window: 0x%lx\n", win->id);
		TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);
		if (win->transient) {
			set_child_transient(win, &trans);
		} if (trans && (ww = find_window(trans)))
			TAILQ_INSERT_AFTER(&win->ws->winlist, ww, win, entry);
		else
			TAILQ_INSERT_TAIL(&win->ws->winlist, win, entry);
		ewmh_update_actions(win);
		return (win);
	}

	if ((win = calloc(1, sizeof(struct ws_win))) == NULL)
		err(1, "manage_window: calloc: failed to allocate memory for "
		    "new window");

	win->id = id;

	/* see if we need to override the workspace */
	p = find_pid(window_get_pid(id));

	/* Get all the window data in one shot */
	ws_idx_atom = XInternAtom(display, "_SWM_WS", False);
	if (ws_idx_atom) {
		XGetWindowProperty(display, id, ws_idx_atom, 0, SWM_PROPLEN,
		    False, XA_STRING, &type, &format, &nitems, &bytes, &prop);
	}
	XGetWindowAttributes(display, id, &win->wa);
	XGetWMNormalHints(display, id, &win->sh, &win->sh_mask);
	win->hints = XGetWMHints(display, id);
	XGetTransientForHint(display, id, &trans);
	if (trans) {
		win->transient = trans;
		set_child_transient(win, &trans);
		DNPRINTF(SWM_D_MISC, "manage_window: window: 0x%lx, "
		    "transient: 0x%lx\n", win->id, win->transient);
	}

	/* get supported protocols */
	if (XGetWMProtocols(display, id, &prot, &n)) {
		for (i = 0, pp = prot; i < n; i++, pp++) {
			if (*pp == takefocus)
				win->take_focus = 1;
			if (*pp == adelete)
				win->can_delete = 1;
		}
		if (prot)
			XFree(prot);
	}

	/*
	 * Figure out where to put the window. If it was previously assigned to
	 * a workspace (either by spawn() or manually moving), and isn't
	 * transient, * put it in the same workspace
	 */
	r = root_to_region(win->wa.root);
	if (p) {
		ws = &r->s->ws[p->ws];
		TAILQ_REMOVE(&pidlist, p, entry);
		free(p);
		p = NULL;
	} else if (prop && win->transient == 0) {
		DNPRINTF(SWM_D_PROP, "manage_window: get _SWM_WS: %s\n", prop);
		errno = 0;
		ws_idx = strtol((const char *)prop, (char**)NULL, 10);
		if (ws_idx < 0 || errno) {
			ws_idx = 0;
			DNPRINTF(SWM_D_EVENT, "manage_window: window: %s", prop);
		}
		ws = &r->s->ws[ws_idx];
	} else {
		ws = r->ws;
		/* this should launch transients in the same ws as parent */
		if (id && trans)
			if ((ww = find_window(trans)) != NULL)
				if (ws->r) {
					ws = ww->ws;
					if (ww->ws->r)
						r = ww->ws->r;
					else
						warnx("manage_window: fix this "
						    "bug mcbride");
					border_me = 1;
				}
	}

	/* set up the window layout */
	win->id = id;
	win->ws = ws;
	win->s = r->s;	/* this never changes */
	if (trans && (ww = find_window(trans)))
		TAILQ_INSERT_AFTER(&ws->winlist, ww, win, entry);
	else
		TAILQ_INSERT_TAIL(&ws->winlist, win, entry);

	WIDTH(win) = win->wa.width;
	HEIGHT(win) = win->wa.height;
	X(win) = win->wa.x;
	Y(win) = win->wa.y;
	win->g_floatvalid = 0;
	win->floatmaxed = 0;
	win->ewmh_flags = 0;

	/* Set window properties so we can remember this after reincarnation */
	if (ws_idx_atom && prop == NULL &&
	    snprintf((char *)ws_idx_str, SWM_PROPLEN, "%d", ws->idx) <
	        SWM_PROPLEN) {
		DNPRINTF(SWM_D_PROP, "manage_window: set _SWM_WS: %s\n",
		    ws_idx_str);
		XChangeProperty(display, win->id, ws_idx_atom, XA_STRING, 8,
		    PropModeReplace, ws_idx_str, strlen((char *)ws_idx_str));
	}
	if (prop)
		XFree(prop);

	ewmh_autoquirk(win);

	if (XGetClassHint(display, win->id, &win->ch)) {
		DNPRINTF(SWM_D_CLASS, "manage_window: class: %s, name: %s\n",
		    win->ch.res_class, win->ch.res_name);

		TAILQ_FOREACH(qp, &quirks, entry) {
			if (!strcmp(win->ch.res_class, qp->class) &&
			    !strcmp(win->ch.res_name, qp->name)) {
				DNPRINTF(SWM_D_CLASS, "manage_window: found: "
				    "class: %s, name: %s\n", win->ch.res_class,
				    win->ch.res_name);
				if (qp->quirk & SWM_Q_FLOAT) {
					win->floating = 1;
					border_me = 1;
				}
				win->quirks = qp->quirk;
			}
		}
	}

	/* alter window position if quirky */
	if (win->quirks & SWM_Q_ANYWHERE) {
		win->manual = 1; /* don't center the quirky windows */
		bzero(&wc, sizeof wc);
		mask = 0;
		if (WIDTH(win) + X(win) > WIDTH(r)) {
			X(win) = wc.x = WIDTH(r) - WIDTH(win) - 2;
			mask |= CWX;
		}
		border_me = 1;
	}

	ewmh_get_win_state(win);
	ewmh_update_actions(win);
	ewmh_update_win_state(win, None, _NET_WM_STATE_REMOVE);

	/* border me */
	if (border_me) {
		bzero(&wc, sizeof wc);
		wc.border_width = border_width;
		mask |= CWBorderWidth;
		XConfigureWindow(display, win->id, mask, &wc);
	}

	XSelectInput(display, id, EnterWindowMask | FocusChangeMask |
	    PropertyChangeMask | StructureNotifyMask);

	/* floaters need to be mapped if they are in the current workspace */
	if ((win->floating || win->transient) && (ws->idx == r->ws->idx))
		XMapRaised(display, win->id);

	return (win);
}

void
free_window(struct ws_win *win)
{
	DNPRINTF(SWM_D_MISC, "free_window: window: 0x%lx\n", win->id);

	if (win == NULL)
		return;

	/* needed for restart wm */
	set_win_state(win, WithdrawnState);

	TAILQ_REMOVE(&win->ws->unmanagedlist, win, entry);

	if (win->ch.res_class)
		XFree(win->ch.res_class);
	if (win->ch.res_name)
		XFree(win->ch.res_name);

	kill_refs(win);

	/* paint memory */
	memset(win, 0xff, sizeof *win);	/* XXX kill later */

	free(win);
}

void
unmanage_window(struct ws_win *win)
{
	struct ws_win		*parent;

	if (win == NULL)
		return;

	DNPRINTF(SWM_D_MISC, "unmanage_window: window: 0x%lx\n", win->id);

	if (win->transient) {
		parent = find_window(win->transient);
		if (parent)
			parent->child_trans = NULL;
	}

	/* focus on root just in case */
	XSetInputFocus(display, PointerRoot, PointerRoot, CurrentTime);

	focus_prev(win);

	if (win->hints) {
		XFree(win->hints);
		win->hints = NULL;
	}

	TAILQ_REMOVE(&win->ws->winlist, win, entry);
	TAILQ_INSERT_TAIL(&win->ws->unmanagedlist, win, entry);

	kill_refs(win);
}

void
focus_magic(struct ws_win *win)
{
	DNPRINTF(SWM_D_FOCUS, "focus_magic: window: 0x%lx\n", WINID(win));

	if (win == NULL) {
		return;
	}

	if (win->child_trans) {
		/* make sure transient hasn't disappeared */
		if (validate_win(win->child_trans) == 0) {
			focus_win(win->child_trans);
			if (win->child_trans->take_focus)
				client_msg(win->child_trans, takefocus);
		} else {
			win->child_trans = NULL;
			focus_win(win);
			if (win->take_focus)
				client_msg(win, takefocus);
		}
	} else {
		/* regular focus */
		focus_win(win);
		if (win->take_focus)
			client_msg(win, takefocus);
	}
}

void
expose(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "expose: window: 0x%lx\n", e->xexpose.window);
}

void
keypress(XEvent *e)
{
	KeySym			keysym;
	XKeyEvent		*ev = &e->xkey;
	struct key		*kp;
	struct swm_region	*r;

	keysym = XKeycodeToKeysym(display, (KeyCode)ev->keycode, 0);
	if ((kp = key_lookup(CLEANMASK(ev->state), keysym)) == NULL)
		return;
	if (keyfuncs[kp->funcid].func == NULL)
		return;

	r = root_to_region(ev->root);
	if (kp->funcid == kf_spawn_custom)
		spawn_custom(r, &(keyfuncs[kp->funcid].args), kp->spawn_name);
	else
		keyfuncs[kp->funcid].func(r, &(keyfuncs[kp->funcid].args));
}

void
buttonpress(XEvent *e)
{
	struct ws_win		*win;
	int			i, action;
	XButtonPressedEvent	*ev = &e->xbutton;

	if ((win = find_window(ev->window)) == NULL)
		return;

	focus_magic(win);
	action = client_click;

	for (i = 0; i < LENGTH(buttons); i++)
		if (action == buttons[i].action && buttons[i].func &&
		    buttons[i].button == ev->button &&
		    CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(win, &buttons[i].args);
}

void
configurerequest(XEvent *e)
{
	XConfigureRequestEvent	*ev = &e->xconfigurerequest;
	struct ws_win		*win;
	int			new = 0;
	XWindowChanges		wc;

	if ((win = find_window(ev->window)) == NULL)
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			new = 1;

	DNPRINTF(SWM_D_EVENT, "configurerequest: window: 0x%lx, new: %s\n",
	    ev->window, YESNO(new));

	if (new) {
		bzero(&wc, sizeof wc);
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(display, ev->window, ev->value_mask, &wc);
	} else {
		config_win(win, ev);
	}
}

void
configurenotify(XEvent *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "configurenotify: window: 0x%lx\n",
	    e->xconfigure.window);

	win = find_window(e->xconfigure.window);
	if (win) {
		XGetWMNormalHints(display, win->id, &win->sh, &win->sh_mask);
	}
}

void
destroynotify(XEvent *e)
{
	struct ws_win		*win;
	XDestroyWindowEvent	*ev = &e->xdestroywindow;

	DNPRINTF(SWM_D_EVENT, "destroynotify: window: 0x%lx\n", ev->window);

	if ((win = find_window(ev->window)) == NULL) {
		if ((win = find_unmanaged_window(ev->window)) == NULL)
			return;
		free_window(win);
		return;
	}

	/* make sure we focus on something */
	win->floating = 0;

	unmanage_window(win);
	stack();
	free_window(win);
}

/* lets us use one switch statement for arbitrary mode/detail combinations */
#define MERGE_MEMBERS(a,b)	(((a & 0xffff) << 16) | (b & 0xffff))

void
mapnotify(XEvent *e)
{
	struct ws_win		*win;
	XMapEvent		*ev = &e->xmap;

	DNPRINTF(SWM_D_EVENT, "mapnotify: window: 0x%lx\n", ev->window);

	win = manage_window(ev->window);
	if (win)
		set_win_state(win, NormalState);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent		*ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	struct ws_win		*win;
	struct swm_region	*r;
	XWindowAttributes	wa;
	XMapRequestEvent	*ev = &e->xmaprequest;

	DNPRINTF(SWM_D_EVENT, "maprequest: window: 0x%lx\n",
	    e->xmaprequest.window);

	if (!XGetWindowAttributes(display, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;

	win = manage_window(e->xmaprequest.window);
	if (win == NULL)
		return; /* can't happen */

	stack();

	/* make new win focused */
	r = root_to_region(win->wa.root);
	if (win->ws == r->ws)
		focus_magic(win);
}

void
propertynotify(XEvent *e)
{
	struct ws_win		*win;
	XPropertyEvent		*ev = &e->xproperty;
#ifdef SWM_DEBUG
	char			*name;
	name = XGetAtomName(display, ev->atom);
	DNPRINTF(SWM_D_EVENT, "propertynotify: window: 0x%lx, atom: %s\n",
	    ev->window, name);
	XFree(name);
#endif

	win = find_window(ev->window);
	if (win == NULL)
		return;
}

void
unmapnotify(XEvent *e)
{
	struct ws_win		*win;

	DNPRINTF(SWM_D_EVENT, "unmapnotify: window: 0x%lx\n", e->xunmap.window);

	/* determine if we need to help unmanage this window */
	win = find_window(e->xunmap.window);
	if (win == NULL)
		return;

	if (getstate(e->xunmap.window) == NormalState) {
		unmanage_window(win);
		stack();

		/* giant hack for apps that don't destroy transient windows */
		/* eat a bunch of events to prevent remanaging the window */
		XEvent			cne;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    EnterWindowMask, &cne))
			;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    StructureNotifyMask, &cne))
			;
		while (XCheckWindowEvent(display, e->xunmap.window,
		    SubstructureNotifyMask, &cne))
			;
		/* resend unmap because we ated it */
		XUnmapWindow(display, e->xunmap.window);
	}
}

void
visibilitynotify(XEvent *e)
{
	DNPRINTF(SWM_D_EVENT, "visibilitynotify: window: 0x%lx\n",
	    e->xvisibility.window);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *ev;
	struct ws_win *win;

	ev = &e->xclient;

	win = find_window(ev->window);
	if (win == NULL)
		return;

	DNPRINTF(SWM_D_EVENT, "clientmessage: window: 0x%lx, type: %ld\n",
	    ev->window, ev->message_type);

	if (ev->message_type == ewmh[_NET_ACTIVE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_ACTIVE_WINDOW\n");
		focus_win(win);
	}
	if (ev->message_type == ewmh[_NET_CLOSE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_CLOSE_WINDOW\n");
		if (win->can_delete)
			client_msg(win, adelete);
		else
			XKillClient(display, win->id);
	}
	if (ev->message_type == ewmh[_NET_MOVERESIZE_WINDOW].atom) {
		DNPRINTF(SWM_D_EVENT,
		    "clientmessage: _NET_MOVERESIZE_WINDOW\n");
		if (win->floating) {
			if (ev->data.l[0] & (1<<8)) /* x */
				X(win) = ev->data.l[1];
			if (ev->data.l[0] & (1<<9)) /* y */
				Y(win) = ev->data.l[2];
			if (ev->data.l[0] & (1<<10)) /* width */
				WIDTH(win) = ev->data.l[3];
			if (ev->data.l[0] & (1<<11)) /* height */
				HEIGHT(win) = ev->data.l[4];

			update_window(win);
		}
		else {
			/* TODO: Change stack sizes */
			/* notify no change was made. */
			config_win(win, NULL);
		}
	}
	if (ev->message_type == ewmh[_NET_WM_STATE].atom) {
		DNPRINTF(SWM_D_EVENT, "clientmessage: _NET_WM_STATE\n");
		ewmh_update_win_state(win, ev->data.l[1], ev->data.l[0]);
		if (ev->data.l[2])
			ewmh_update_win_state(win, ev->data.l[2],
			    ev->data.l[0]);

		stack();
	}
}

int
xerror_start(Display *d, XErrorEvent *ee)
{
	other_wm = 1;
	return (-1);
}

int
xerror(Display *d, XErrorEvent *ee)
{
	/* warnx("error: %p %p", display, ee); */
	return (-1);
}

int
active_wm(void)
{
	other_wm = 0;
	xerrorxlib = XSetErrorHandler(xerror_start);

	/* this causes an error if some other window manager is running */
	XSelectInput(display, DefaultRootWindow(display),
	    SubstructureRedirectMask);
	XSync(display, False);
	if (other_wm)
		return (1);

	XSetErrorHandler(xerror);
	XSync(display, False);
	return (0);
}

void
new_region(struct swm_screen *s, int x, int y, int w, int h)
{
	struct swm_region	*r, *n;
	struct workspace	*ws = NULL;
	int			i;

	DNPRINTF(SWM_D_MISC, "new region: screen[%d]:%dx%d+%d+%d\n",
	     s->idx, w, h, x, y);

	/* remove any conflicting regions */
	n = TAILQ_FIRST(&s->rl);
	while (n) {
		r = n;
		n = TAILQ_NEXT(r, entry);
		if (X(r) < (x + w) &&
		    (X(r) + WIDTH(r)) > x &&
		    Y(r) < (y + h) &&
		    (Y(r) + HEIGHT(r)) > y) {
			if (r->ws->r != NULL)
				r->ws->old_r = r->ws->r;
			r->ws->r = NULL;
			TAILQ_REMOVE(&s->rl, r, entry);
			TAILQ_INSERT_TAIL(&s->orl, r, entry);
		}
	}

	/* search old regions for one to reuse */

	/* size + location match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (X(r) == x && Y(r) == y &&
		    HEIGHT(r) == h && WIDTH(r) == w)
			break;

	/* size match */
	TAILQ_FOREACH(r, &s->orl, entry)
		if (HEIGHT(r) == h && WIDTH(r) == w)
			break;

	if (r != NULL) {
		TAILQ_REMOVE(&s->orl, r, entry);
		/* try to use old region's workspace */
		if (r->ws->r == NULL)
			ws = r->ws;
	} else
		if ((r = calloc(1, sizeof(struct swm_region))) == NULL)
			err(1, "new_region: calloc: failed to allocate memory "
			    "for screen");

	/* if we don't have a workspace already, find one */
	if (ws == NULL) {
		for (i = 0; i < SWM_WS_MAX; i++)
			if (s->ws[i].r == NULL) {
				ws = &s->ws[i];
				break;
			}
	}

	if (ws == NULL)
		errx(1, "new_region: no free workspaces");

	X(r) = x;
	Y(r) = y;
	WIDTH(r) = w;
	HEIGHT(r) = h;
	r->s = s;
	r->ws = ws;
	r->ws_prior = NULL;
	ws->r = r;
	outputs++;
	TAILQ_INSERT_TAIL(&s->rl, r, entry);
}

void
scan_xrandr(int i)
{
#ifdef SWM_XRR_HAS_CRTC
	XRRCrtcInfo		*ci;
	XRRScreenResources	*sr;
	int			c;
	int			ncrtc = 0;
#endif /* SWM_XRR_HAS_CRTC */
	struct swm_region	*r;


	if (i >= ScreenCount(display))
		errx(1, "scan_xrandr: invalid screen");

	/* remove any old regions */
	while ((r = TAILQ_FIRST(&screens[i].rl)) != NULL) {
		r->ws->old_r = r->ws->r = NULL;
		TAILQ_REMOVE(&screens[i].rl, r, entry);
		TAILQ_INSERT_TAIL(&screens[i].orl, r, entry);
	}
	outputs = 0;

	/* map virtual screens onto physical screens */
#ifdef SWM_XRR_HAS_CRTC
	if (xrandr_support) {
		sr = XRRGetScreenResources(display, screens[i].root);
		if (sr == NULL)
			new_region(&screens[i], 0, 0,
			    DisplayWidth(display, i),
			    DisplayHeight(display, i));
		else
			ncrtc = sr->ncrtc;

		for (c = 0, ci = NULL; c < ncrtc; c++) {
			ci = XRRGetCrtcInfo(display, sr, sr->crtcs[c]);
			if (ci->noutput == 0)
				continue;

			if (ci != NULL && ci->mode == None)
				new_region(&screens[i], 0, 0,
				    DisplayWidth(display, i),
				    DisplayHeight(display, i));
			else
				new_region(&screens[i],
				    ci->x, ci->y, ci->width, ci->height);
		}
		if (ci)
			XRRFreeCrtcInfo(ci);
		XRRFreeScreenResources(sr);
	} else
#endif /* SWM_XRR_HAS_CRTC */
	{
		new_region(&screens[i], 0, 0, DisplayWidth(display, i),
		    DisplayHeight(display, i));
	}
}

void
screenchange(XEvent *e) {
	XRRScreenChangeNotifyEvent	*xe = (XRRScreenChangeNotifyEvent *)e;
	int				i;

	DNPRINTF(SWM_D_EVENT, "screenchange: root: 0x%lx\n", xe->root);

	if (!XRRUpdateConfiguration(e))
		return;

	/* silly event doesn't include the screen index */
	for (i = 0; i < ScreenCount(display); i++)
		if (screens[i].root == xe->root)
			break;
	if (i >= ScreenCount(display))
		errx(1, "screenchange: screen not found");

	/* brute force for now, just re-enumerate the regions */
	scan_xrandr(i);

	stack();
}

void
grab_windows(void)
{
	Window			d1, d2, *wins = NULL;
	XWindowAttributes	wa;
	unsigned int		no;
	int			i, j;
	long			state, manage;

	for (i = 0; i < ScreenCount(display); i++) {
		if (!XQueryTree(display, screens[i].root, &d1, &d2, &wins, &no))
			continue;

		/* attach windows to a region */
		/* normal windows */
		for (j = 0; j < no; j++) {
			if (!XGetWindowAttributes(display, wins[j], &wa) ||
			    wa.override_redirect ||
			    XGetTransientForHint(display, wins[j], &d1))
				continue;

			state = getstate(wins[j]);
			manage = state == IconicState;
			if (wa.map_state == IsViewable || manage)
				manage_window(wins[j]);
		}
		/* transient windows */
		for (j = 0; j < no; j++) {
			if (!XGetWindowAttributes(display, wins[j], &wa) ||
			    wa.override_redirect)
				continue;

			state = getstate(wins[j]);
			manage = state == IconicState;
			if (XGetTransientForHint(display, wins[j], &d1) &&
			    manage)
				manage_window(wins[j]);
		}
		if (wins) {
			XFree(wins);
			wins = NULL;
		}
	}
}

void
setup_screens(void)
{
	int			i, j, k;
	int			errorbase, major, minor;
	struct workspace	*ws;

	if ((screens = calloc(ScreenCount(display),
	     sizeof(struct swm_screen))) == NULL)
		err(1, "setup_screens: calloc: failed to allocate memory for "
		    "screens");

	/* initial Xrandr setup */
	xrandr_support = XRRQueryExtension(display,
	    &xrandr_eventbase, &errorbase);
	if (xrandr_support)
		if (XRRQueryVersion(display, &major, &minor) && major < 1)
			xrandr_support = 0;

	/* map physical screens */
	for (i = 0; i < ScreenCount(display); i++) {
		DNPRINTF(SWM_D_WS, "setup_screens: init screen: %d\n", i);
		screens[i].idx = i;
		TAILQ_INIT(&screens[i].rl);
		TAILQ_INIT(&screens[i].orl);
		screens[i].root = RootWindow(display, i);

		/* set default colors */
		setscreencolor("red", i + 1, SWM_S_COLOR_FOCUS);
		setscreencolor("rgb:88/88/88", i + 1, SWM_S_COLOR_UNFOCUS);

		/* init all workspaces */
		/* XXX these should be dynamically allocated too */
		for (j = 0; j < SWM_WS_MAX; j++) {
			ws = &screens[i].ws[j];
			ws->idx = j;
			ws->name = NULL;
			ws->focus = NULL;
			ws->r = NULL;
			ws->old_r = NULL;
			TAILQ_INIT(&ws->winlist);
			TAILQ_INIT(&ws->unmanagedlist);

			for (k = 0; layouts[k].l_stack != NULL; k++)
				if (layouts[k].l_config != NULL)
					layouts[k].l_config(ws,
					    SWM_ARG_ID_STACKINIT);
			ws->cur_layout = &layouts[0];
		}

		scan_xrandr(i);

		if (xrandr_support)
			XRRSelectInput(display, screens[i].root,
			    RRScreenChangeNotifyMask);
	}
}

void
workaround(void)
{
	int			i;
	Atom			netwmcheck, netwmname, utf8_string;
	Window			root, win;

	/* work around sun jdk bugs, code from wmname */
	netwmcheck = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
	netwmname = XInternAtom(display, "_NET_WM_NAME", False);
	utf8_string = XInternAtom(display, "UTF8_STRING", False);
	for (i = 0; i < ScreenCount(display); i++) {
		root = screens[i].root;
		win = XCreateSimpleWindow(display,root, 0, 0, 1, 1, 0,
		    screens[i].c[SWM_S_COLOR_UNFOCUS].color,
		    screens[i].c[SWM_S_COLOR_UNFOCUS].color);

		XChangeProperty(display, root, netwmcheck, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win,1);
		XChangeProperty(display, win, netwmcheck, XA_WINDOW, 32,
		    PropModeReplace, (unsigned char *)&win,1);
		XChangeProperty(display, win, netwmname, utf8_string, 8,
		    PropModeReplace, (unsigned char*)"LG3D", strlen("LG3D"));
	}
}

#define SWM_CONF_FILE		"sscrotwm.conf"

int
main(int argc, char *argv[])
{
	struct swm_region	*rr;
	struct ws_win		*winfocus = NULL;
	struct timeval		tv;
	union arg		a;
	char			conf[PATH_MAX], *cfile = NULL;
	struct stat		sb;
	XEvent			e;
	int			xfd, i;
	fd_set			rd;
	struct sigaction	sact;

	start_argv = argv;
	warnx("Welcome to sscrotwm V%s Build: %s", SSCROTWM_VERSION, buildstr);
	if (!setlocale(LC_CTYPE, "") || !setlocale(LC_TIME, "") ||
	    !XSupportsLocale())
		warnx("no locale support");

	if (!X_HAVE_UTF8_STRING)
		warnx("no UTF-8 support");

	if (!(display = XOpenDisplay(0)))
		errx(1, "can not open display");

	if (active_wm())
		errx(1, "other wm running");

	/* handle some signals */
	bzero(&sact, sizeof(sact));
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = sighdlr;
	sigaction(SIGINT, &sact, NULL);
	sigaction(SIGQUIT, &sact, NULL);
	sigaction(SIGTERM, &sact, NULL);
	sigaction(SIGHUP, &sact, NULL);

	sact.sa_handler = sighdlr;
	sact.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sact, NULL);

	astate = XInternAtom(display, "WM_STATE", False);
	aprot = XInternAtom(display, "WM_PROTOCOLS", False);
	adelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
	takefocus = XInternAtom(display, "WM_TAKE_FOCUS", False);
	a_wmname = XInternAtom(display, "WM_NAME", False);
	a_netwmname = XInternAtom(display, "_NET_WM_NAME", False);
	a_utf8_string = XInternAtom(display, "UTF8_STRING", False);
	a_string = XInternAtom(display, "STRING", False);

	/* look for local and global conf file */
	pwd = getpwuid(getuid());
	if (pwd == NULL)
		errx(1, "invalid user: %d", getuid());

	setup_screens();

	#ifdef SWM_DEBUG
		setkeybinding(MODKEY|ShiftMask,	XK_d, kf_dumpwins, NULL);
	#endif

	/* load config */
	for (i = 0; ; i++) {
		conf[0] = '\0';
		switch (i) {
		case 0:
			/* ~ */
			snprintf(conf, sizeof conf, "%s/.%s",
			    pwd->pw_dir, SWM_CONF_FILE);
			break;
		case 1:
			/* global */
			snprintf(conf, sizeof conf, "/etc/%s",
			    SWM_CONF_FILE);
			break;
		default:
			errx(i, "No config file found");
			break;
		}

		if (strlen(conf) && stat(conf, &sb) != -1 && S_ISREG(sb.st_mode)) {
			cfile = conf;
			break;
		}
	}

	/* load conf */
	conf_load(cfile);

	setup_ewmh();
	/* set some values to work around bad programs */
	workaround();
	/* grab existing windows */
	grab_windows();

	if (getenv("SWM_STARTED") == NULL)
		setenv("SWM_STARTED", "YES", 1);

	unfocus_all();
	grabkeys();
	stack();

	xfd = ConnectionNumber(display);
	while (running) {
		while (XPending(display)) {
			XNextEvent(display, &e);
			if (running == 0)
				goto done;
			if (e.type < LASTEvent) {
				DNPRINTF(SWM_D_EVENTQ ,"XEvent: handled: %s, "
				    "window: 0x%lx, type: %s (%d), %d remaining"
				    "\n", YESNO(handler[e.type]),
				    e.xany.window, geteventname(&e),
				    e.type, QLength(display));

				if (handler[e.type])
					handler[e.type](&e);
			} else {
				DNPRINTF(SWM_D_EVENTQ, "XRandr Event: window: "
				    "0x%lx, type: %s (%d)\n", e.xany.window,
				    xrandr_geteventname(&e), e.type);

				switch (e.type - xrandr_eventbase) {
				case RRScreenChangeNotify:
					screenchange(&e);
					break;
				default:
					break;
				}
			}
		}

		/* if we are being restarted go focus on first window */
		if (winfocus) {
			rr = winfocus->ws->r;
			if (rr == NULL) {
				/* not a visible window */
				winfocus = NULL;
				continue;
			}
			/* move pointer to first screen if multi screen */
			if (ScreenCount(display) > 1 || outputs > 1)
				XWarpPointer(display, None, rr->s[0].root,
				    0, 0, 0, 0, X(rr), Y(rr));

			a.id = SWM_ARG_ID_FOCUSCUR;
			focus(rr, &a);
			winfocus = NULL;
			continue;
		}

		FD_ZERO(&rd);
		FD_SET(xfd, &rd);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (select(xfd + 1, &rd, NULL, NULL, &tv) == -1)
			if (errno != EINTR)
				DNPRINTF(SWM_D_MISC, "select failed");
		if (restart_wm == 1)
			restart(NULL, NULL);
		if (running == 0)
			goto done;
	}
done:
	teardown_ewmh();

	XCloseDisplay(display);

	return (0);
}
