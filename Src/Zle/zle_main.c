/*
 * $Id: zle_main.c,v 3.1.0.11 1996/12/21 01:32:44 hzoli Exp $
 *
 * zle_main.c - main routines for line editor
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 1992-1996 Paul Falstad
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Paul Falstad or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Paul Falstad and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Paul Falstad and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Paul Falstad and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */

/*
 * Note on output from zle (PWS 1995/05/23):
 *
 * All input and output from the editor should be to/from the file descriptor
 * `SHTTY' and FILE `shout'.  (Normally, the former is used for input
 * operations, reading one key at a time, and the latter for output
 * operations, flushing after each refresh()).  Thus fprintf(shout, ...),
 * putc(..., shout), etc., should be used for output within zle.
 *
 * However, the functions printbind() and printbinding() can be invoked from
 * the builtin bindkey as well as zle, in which case output should be to
 * stdout.  For this purpose, the static variable FILE *bindout exists, which
 * is set to stdout in bin_bindkey() and shout in zleread().
 */

#define ZLEGLOBALS
#include "zle.h"

static int embindtab[256], eofchar, eofsent;
static int vibindtab[256];

static Key cky;

static char *keybuf = NULL;
static int keybuflen, keybufsz;
static long keytimeout;

/* set up terminal */

/**/
void
setterm(void)
{
    struct ttyinfo ti;

#if defined(CLOBBERS_TYPEAHEAD) && defined(FIONREAD)
    int val;

    ioctl(SHTTY, FIONREAD, (char *)&val);
    if (val)
	return;
#endif

/* sanitize the tty */
#ifdef HAS_TIO
#ifdef __EMX__
#ifndef IDEFAULT
#define IDEFAULT 0x8000
#endif
    shttyinfo.tio.c_lflag &= ~IDEFAULT;
#endif
    shttyinfo.tio.c_lflag |= ICANON | ECHO;
# ifdef FLUSHO
    shttyinfo.tio.c_lflag &= ~FLUSHO;
# endif
#else				/* not HAS_TIO */
    shttyinfo.sgttyb.sg_flags = (shttyinfo.sgttyb.sg_flags & ~CBREAK) | ECHO;
    shttyinfo.lmodes &= ~LFLUSHO;
#endif

    attachtty(mypgrp);
    ti = shttyinfo;
#ifdef HAS_TIO
    if (unset(FLOWCONTROL))
	ti.tio.c_iflag &= ~IXON;
    ti.tio.c_lflag &= ~(ICANON | ECHO
# ifdef FLUSHO
			| FLUSHO
# endif
	);
#ifdef __EMX__
#ifndef TAB3
    #define TAB3 0x1800
#endif
#endif
# ifdef TAB3
    ti.tio.c_oflag &= ~TAB3;
# else
#  ifdef OXTABS
    ti.tio.c_oflag &= ~OXTABS;
#  else
#ifndef __EMX__
    ti.tio.c_oflag &= ~XTABS;
#endif
#  endif
# endif
#ifndef __EMX__
    ti.tio.c_oflag |= ONLCR;
#endif
    ti.tio.c_cc[VQUIT] =
# ifdef VDISCARD
	ti.tio.c_cc[VDISCARD] =
# endif
# ifdef VSUSP
	ti.tio.c_cc[VSUSP] =
# endif
# ifdef VDSUSP
	ti.tio.c_cc[VDSUSP] =
# endif
# ifdef VSWTCH
	ti.tio.c_cc[VSWTCH] =
# endif
# ifdef VLNEXT
	ti.tio.c_cc[VLNEXT] =
# endif
	VDISABLEVAL;
# if defined(VSTART) && defined(VSTOP)
    if (unset(FLOWCONTROL))
	ti.tio.c_cc[VSTART] = ti.tio.c_cc[VSTOP] = VDISABLEVAL;
# endif
    eofchar = ti.tio.c_cc[VEOF];
    ti.tio.c_cc[VMIN] = 1;
    ti.tio.c_cc[VTIME] = 0;
    ti.tio.c_iflag |= (INLCR | ICRNL);
 /* this line exchanges \n and \r; it's changed back in getkey
	so that the net effect is no change at all inside the shell.
	This double swap is to allow typeahead in common cases, eg.

	% bindkey -s '^J' 'echo foo^M'
	% sleep 10
	echo foo<return>  <--- typed before sleep returns

	The shell sees \n instead of \r, since it was changed by the kernel
	while zsh wasn't looking. Then in getkey() \n is changed back to \r,
	and it sees "echo foo<accept line>", as expected. Without the double
	swap the shell would see "echo foo\n", which is translated to
	"echo fooecho foo<accept line>" because of the binding.
	Note that if you type <line-feed> during the sleep the shell just sees
	\n, which is translated to \r in getkey(), and you just get another
	prompt. For type-ahead to work in ALL cases you have to use
	stty inlcr.
	This workaround is due to Sven Wischnowsky <oberon@cs.tu-berlin.de>.

	Unfortunately it's IMPOSSIBLE to have a general solution if both
	<return> and <line-feed> are mapped to the same character. The shell
	could check if there is input and read it before setting it's own
	terminal modes but if we get a \n we don't know whether to keep it or
	change to \r :-(
	*/

#else				/* not HAS_TIO */
    ti.sgttyb.sg_flags = (ti.sgttyb.sg_flags | CBREAK) & ~ECHO
#ifndef __EMX__
      & ~XTABS;
#else
    ;
#endif
    ti.lmodes &= ~LFLUSHO;
    eofchar = ti.tchars.t_eofc;
    ti.tchars.t_quitc =
	ti.ltchars.t_suspc =
	ti.ltchars.t_flushc =
	ti.ltchars.t_dsuspc = ti.ltchars.t_lnextc = -1;
#endif

#if defined(TTY_NEEDS_DRAINING) && defined(TIOCOUTQ) && defined(HAVE_SELECT)
    if (baud) {			/**/
	int n = 0;

	while ((ioctl(SHTTY, TIOCOUTQ, (char *)&n) >= 0) && n) {
	    struct timeval tv;

	    tv.tv_sec = n / baud;
	    tv.tv_usec = ((n % baud) * 1000000) / baud;
	    select(0, NULL, NULL, NULL, &tv);
	}
    }
#endif

    settyinfo(&ti);
}

static char *kungetbuf;
static int kungetct, kungetsz;

/**/
void
ungetkey(int ch)
{
    if (kungetct == kungetsz)
	kungetbuf = realloc(kungetbuf, kungetsz *= 2);
    kungetbuf[kungetct++] = ch;
}

/**/
void
ungetkeys(char *s, int len)
{
    s += len;
    while (len--)
	ungetkey(*--s);
}

#if defined(pyr) && defined(HAVE_SELECT)
static int
breakread(int fd, char *buf, int n)
{
    fd_set f;

    FD_ZERO(&f);
    FD_SET(fd, &f);
    return (select(fd + 1, (SELECT_ARG_2_T) & f, NULL, NULL, NULL) == -1 ?
	    EOF : read(fd, buf, n));
}

# define read    breakread
#endif

/**/
int
getkey(int keytmout)
{
    char cc;
    unsigned int ret;
    long exp100ths;
    int die = 0, r, icnt = 0;
    int old_errno = errno;

#ifdef HAVE_SELECT
    fd_set foofd;

#else
# ifdef HAS_TIO
    struct ttyinfo ti;

# endif
#endif

    if (kungetct)
	ret = STOUC(kungetbuf[--kungetct]);
    else {
	if (keytmout) {
	    if (keytimeout > 500)
		exp100ths = 500;
	    else if (keytimeout > 0)
		exp100ths = keytimeout;
	    else
		exp100ths = 0;
#ifdef HAVE_SELECT
	    if (exp100ths) {
		struct timeval expire_tv;

		expire_tv.tv_sec = exp100ths / 100;
		expire_tv.tv_usec = (exp100ths % 100) * 10000L;
		FD_ZERO(&foofd);
		FD_SET(SHTTY, &foofd);
		if (select(SHTTY+1, (SELECT_ARG_2_T) & foofd,
			   NULL, NULL, &expire_tv) <= 0)
		    return EOF;
	    }
#else
# ifdef HAS_TIO
	    ti = shttyinfo;
	    ti.tio.c_lflag &= ~ICANON;
	    ti.tio.c_cc[VMIN] = 0;
	    ti.tio.c_cc[VTIME] = exp100ths / 10;
#  ifdef HAVE_TERMIOS_H
	    tcsetattr(SHTTY, TCSANOW, &ti.tio);
#  else
	    ioctl(SHTTY, TCSETA, &ti.tio);
#  endif
	    r = read(SHTTY, &cc, 1);
#  ifdef HAVE_TERMIOS_H
	    tcsetattr(SHTTY, TCSANOW, &shttyinfo.tio);
#  else
	    ioctl(SHTTY, TCSETA, &shttyinfo.tio);
#  endif
	    return (r <= 0) ? EOF : cc;
# endif
#endif
	}
	while ((r = read(SHTTY, &cc, 1)) != 1) {
	    if (r == 0) {
		/* The test for IGNOREEOF was added to make zsh ignore ^Ds
		   that were typed while commands are running.  Unfortuantely
		   this caused trouble under at least one system (SunOS 4.1).
		   Here shells that lost their xterm (e.g. if it was killed
		   with -9) didn't fail to read from the terminal but instead
		   happily continued to read EOFs, so that the above read
		   returned with 0, and, with IGNOREEOF set, this caused
		   an infinite loop.  The simple way around this was to add
		   the counter (icnt) so that this happens 20 times and than
		   the shell gives up (yes, this is a bit dirty...). */
		if (isset(IGNOREEOF) && icnt++ < 20)
		    continue;
		stopmsg = 1;
		zexit(1, 0);
	    }
	    icnt = 0;
	    if (errno == EINTR) {
		die = 0;
		if (!errflag && !retflag && !breaks)
		    continue;
		errflag = 0;
		errno = old_errno;
		return EOF;
	    } else if (errno == EWOULDBLOCK) {
		fcntl(0, F_SETFL, 0);
	    } else if (errno == EIO && !die) {
		ret = opts[MONITOR];
		opts[MONITOR] = 1;
		attachtty(mypgrp);
		refresh();	/* kludge! */
		opts[MONITOR] = ret;
		die = 1;
	    } else if (errno != 0) {
		zerr("error on TTY read: %e", NULL, errno);
		stopmsg = 1;
		zexit(1, 0);
	    }
	}
	if (cc == '\r')		/* undo the exchange of \n and \r determined by */
	    cc = '\n';		/* setterm() */
	else if (cc == '\n')
	    cc = '\r';

	ret = STOUC(cc);
    }
    if (vichgflag) {
	if (vichgbufptr == vichgbufsz)
	    vichgbuf = realloc(vichgbuf, vichgbufsz *= 2);
	vichgbuf[vichgbufptr++] = ret;
    }
    errno = old_errno;
    return ret;
}

/* Where to print out bindings:  either stdout, or the zle output shout */
static FILE *bindout;

/* Read a line.  It is returned metafied. */

/**/
unsigned char *
zleread(char *lp, char *rp)
{
    unsigned char *s;
    int old_errno = errno;
    int tmout = getiparam("TMOUT");

#ifdef HAVE_SELECT
    long costmult;
    struct timeval tv;
    fd_set foofd;

    baud = getiparam("BAUD");
    costmult = (baud) ? 3840000L / baud : 0;
    tv.tv_sec = 0;
#endif

    keytimeout = getiparam("KEYTIMEOUT");
    if (!shout) {
	if (SHTTY != -1)
	    init_shout();

	if (!shout)
	    return NULL;
	/* We could be smarter and default to a system read. */

	/* If we just got a new shout, make sure the terminal is set up. */
	if (!termok)
	    init_term();
    }

    fflush(shout);
    fflush(stderr);
    intr();
    insmode = unset(OVERSTRIKE);
    eofsent = 0;
    resetneeded = 0;
    lpmpt = lp;
    rpmpt = rp;
    PERMALLOC {
	histline = curhist;
#ifdef HAVE_SELECT
	FD_ZERO(&foofd);
#endif
	undoing = 1;
	line = (unsigned char *)zalloc((linesz = 256) + 2);
	virangeflag = vilinerange = lastcmd = done = cs = ll = mark = 0;
	curhistline = NULL;
	zmult = 1;
	vibufspec = 0;
	vibufappend = 0;
	gotmult = gotvibufspec = 0;
	bindtab = mainbindtab;
	keybindtab = mainkeybindtab;
	addedsuffix = complexpect = vichgflag = 0;
	viinsbegin = 0;
	statusline = NULL;
	bindout = shout;		/* always print bindings on terminal */
	if ((s = (unsigned char *)getlinknode(bufstack))) {
	    setline((char *)s);
	    zsfree((char *)s);
	    if (stackcs != -1) {
		cs = stackcs;
		stackcs = -1;
		if (cs > ll)
		    cs = ll;
	    }
	    if (stackhist != -1) {
		histline = stackhist;
		stackhist = -1;
	    }
	}
	initundo();
	if (isset(PROMPTCR))
	    putc('\r', shout);
	if (tmout)
	    alarm(tmout);
	zleactive = 1;
	resetneeded = 1;
	refresh();
	errflag = retflag = 0;
	while (!done && !errflag) {
	    struct zlecmd *zc;

	    statusline = NULL;
	    bindk = getkeycmd();
	    if (!ll && c == eofchar) {
		eofsent = 1;
		break;
	    }
	    if (bindk != -1) {
		int ce = complexpect;

		zc = ZLEGETCMD(bindk);
		if (!(lastcmd & ZLE_ARG)) {
		    zmult = 1;
		    vibufspec = 0;
		    gotmult = gotvibufspec = 0;
		}
		if ((lastcmd & ZLE_UNDO) != (zc->flags & ZLE_UNDO) && undoing)
		    addundo();
		if (bindk != z_sendstring) {
		    if (!(zc->flags & ZLE_MENUCMP))
			invalidatelist();
		    if (!(zc->flags & ZLE_MENUCMP) &&
			addedsuffix && !(zc->flags & ZLE_DELETE) &&
			!((zc->flags & ZLE_INSERT) && c != ' ' &&
			  (c != '/' || addedsuffix > 1 || line[cs-1] != c))) {
			backdel(addedsuffix);
		    }
		    if (!menucmp && !((zc->flags & ZLE_INSERT) && /*{*/
				      complexpect == 2 && c == '}'))
			addedsuffix = 0;
		}
		if (zc->func)
		    (*zc->func) ();
		/* for vi mode, make sure the cursor isn't somewhere illegal */
		if (bindtab == altbindtab && cs > findbol() &&
		    (cs == ll || line[cs] == '\n'))
		    cs--;
		if (ce == complexpect && ce && !menucmp)
		    complexpect = 0;
		if (bindk != z_sendstring)
		    lastcmd = zc->flags;
		if (!(lastcmd & ZLE_UNDO) && undoing)
		    addundo();
	    } else {
		errflag = 1;
		break;
	    }
#ifdef HAVE_SELECT
	    if (baud && !(lastcmd & ZLE_MENUCMP)) {
		FD_SET(SHTTY, &foofd);
		if ((tv.tv_usec = cost * costmult) > 500000)
		    tv.tv_usec = 500000;
		if (!kungetct && select(SHTTY+1, (SELECT_ARG_2_T) & foofd,
					NULL, NULL, &tv) <= 0)
		    refresh();
	    } else
#endif
		if (!kungetct)
		    refresh();
	}
	statusline = NULL;
	invalidatelist();
	trashzle();
	zleactive = 0;
	alarm(0);
    } LASTALLOC;
    zsfree(curhistline);
    free(lastline);		/* freeundo */
    if (eofsent) {
	free(line);
	line = NULL;
    } else {
	line[ll++] = '\n';
	line = (unsigned char *) metafy((char *) line, ll, META_REALLOC);
    }
    forget_edits();
    errno = old_errno;
    return line;
}

/**/
void
metafy_keybuf(void)
{
    if(keybuflen*2 + 1 > keybufsz)
	keybuf = (char *)realloc(keybuf, keybufsz = (keybufsz*2 + 2));
    metafy(keybuf, keybuflen, META_NOALLOC);
}

/**/
int
getkeycmd(void)
{
    int ret;
    static int hops = 0;

    cky = NULL;
    if (!keybuf)
	keybuf = (char *)zalloc(keybufsz = 50);

    if ((c = getkey(0)) < 0)
	return -1;
    keybuf[0] = c;
    keybuflen = 1;
    if ((ret = bindtab[c]) == z_prefix) {
	int lastlen = 0;
	Key ky;

	metafy_keybuf();
	ky = cky = (Key) keybindtab->getnode(keybindtab, keybuf);
	unmetafy(keybuf, NULL);

	if (cky->func == z_undefinedkey)
	    cky = NULL;
	else
	    lastlen = 1;
	while(ky->prefixct) {
	    if ((c = getkey(!!cky)) >= 0) {
		keybuf[keybuflen++] = c;
		metafy_keybuf();
		ky = (Key) keybindtab->getnode(keybindtab, keybuf);
		unmetafy(keybuf, NULL);
	    } else
		ky = NULL;
	    if (ky) {
		if (ky->func == z_undefinedkey)
		    continue;
		cky = ky;
		lastlen = keybuflen;
	    } else if (cky) {
		ungetkeys(keybuf + lastlen, keybuflen - lastlen);
		if(vichgflag)
		    vichgbufptr -= keybuflen - lastlen;
		c = keybuf[lastlen - 1];
		break;
	    } else
		return z_undefinedkey;
	}
	ret = cky->func;
    }
    if (ret == z_executenamedcmd && !statusline) {
	while(ret == z_executenamedcmd)
	    ret = executenamedcommand("execute: ");
	if(ret == -1)
	    ret = z_undefinedkey;
	else if(ret != z_executelastnamedcmd)
	    lastnamed = ret;
    }
    if (ret == z_executelastnamedcmd)
	ret = lastnamed;
    if (ret == z_sendstring) {
#define MAXHOPS 20
	if (++hops == MAXHOPS) {
	    zerr("string inserting another one too many times", NULL, 0);
	    hops = 0;
	    return -1;
	}
    } else
	hops = 0;
    if (ret == z_vidigitorbeginningofline)
	ret = (lastcmd & ZLE_DIGIT) ? z_digitargument : z_vibeginningofline;
    return ret;
}

/**/
void
ungetkeycmd(void)
{
    ungetkeys(keybuf, keybuflen);
}


/**/
void
sendstring(void)
{
    if(!cky) {
	metafy_keybuf();
	cky = (Key) keybindtab->getnode(keybindtab, keybuf);
	unmetafy(keybuf, NULL);
    }
    if(cky)
	ungetkeys(cky->str, cky->len);
    else
	feep();
}

/**/
Key
makefunckey(int fun)
{
    Key ky = (Key) zcalloc(sizeof *ky);

    ky->func = fun;
    return ky;
}

/* initialize the key bindings */

/**/
void
initkeybindings(void)
{
    char buf[2], *s;
    int i;

    stackhist = stackcs = -1;
    if (!kungetbuf)
	kungetbuf = (char *) zalloc(kungetsz = 32);
    zlecmdtot = ZLECMDCOUNT;
    lastnamed = z_undefinedkey;

    /* If VISUAL or EDITOR contain the string "vi" when    *
     * initializing the keymaps, then the main keymap will *
     * be bound to vi insert mode by default.              */
    if (((s = zgetenv("VISUAL")) && strstr(s, "vi")) ||
	((s = zgetenv("EDITOR")) && strstr(s, "vi"))) {
	mainbindtab = vibindtab;
	mainkeybindtab = vikeybindtab;
    } else {
	mainbindtab = embindtab;
	mainkeybindtab = emkeybindtab;
    }
    bindtab = mainbindtab;
    keybindtab = mainkeybindtab;

    /* vi insert mode and emacs mode:  *
     *   0-31   taken from the tables  *
     *  32-126  self-insert            *
     * 127      same as entry[8]       *
     * 128-255  self-insert            */
    for (i = 0; i < 32; i++) {
	vibindtab[i] = viinsbind[i];
	embindtab[i] = emacsbind[i];
    }
    for (i = 32; i < 256; i++) {
	vibindtab[i] = z_selfinsert;
	embindtab[i] = z_selfinsert;
    }
    vibindtab[127] = vibindtab[8];
    embindtab[127] = embindtab[8];

    /* vi command mode:              *
     *   0-127  taken from the table *
     * 128-255  undefined-key        */
    for (i = 0; i < 128; i++)
	altbindtab[i] = vicmdbind[i];
    for (i = 128; i < 256; i++)
	altbindtab[i] = z_undefinedkey;

    /* vi command mode: arrow keys */
    addbinding(altbindtab, altkeybindtab, "\33[C", 3, z_forwardchar);
    addbinding(altbindtab, altkeybindtab, "\33[D", 3, z_backwardchar);
    addbinding(altbindtab, altkeybindtab, "\33[A", 3, z_uplineorhistory);
    addbinding(altbindtab, altkeybindtab, "\33[B", 3, z_downlineorhistory);

    /* emacs mode: arrow keys */
    addbinding(embindtab, emkeybindtab, "\33[C", 3, z_forwardchar);
    addbinding(embindtab, emkeybindtab, "\33[D", 3, z_backwardchar);
    addbinding(embindtab, emkeybindtab, "\33[A", 3, z_uplineorhistory);
    addbinding(embindtab, emkeybindtab, "\33[B", 3, z_downlineorhistory);

    /* emacs mode: ^X sequences */
    addbinding(embindtab, emkeybindtab, "\30*", 2, z_expandword);
    addbinding(embindtab, emkeybindtab, "\30g", 2, z_listexpand);
    addbinding(embindtab, emkeybindtab, "\30G", 2, z_listexpand);
    addbinding(embindtab, emkeybindtab, "\30\16", 2, z_infernexthistory);
    addbinding(embindtab, emkeybindtab, "\30\13", 2, z_killbuffer);
    addbinding(embindtab, emkeybindtab, "\30\6", 2, z_vifindnextchar);
    addbinding(embindtab, emkeybindtab, "\30\17", 2, z_overwritemode);
    addbinding(embindtab, emkeybindtab, "\30\25", 2, z_undo);
    addbinding(embindtab, emkeybindtab, "\30\26", 2, z_vicmdmode);
    addbinding(embindtab, emkeybindtab, "\30\12", 2, z_vijoin);
    addbinding(embindtab, emkeybindtab, "\30\2", 2, z_vimatchbracket);
    addbinding(embindtab, emkeybindtab, "\30s", 2, z_historyincrementalsearchforward);
    addbinding(embindtab, emkeybindtab, "\30r", 2, z_historyincrementalsearchbackward);
    addbinding(embindtab, emkeybindtab, "\30u", 2, z_undo);
    addbinding(embindtab, emkeybindtab, "\30\30", 2, z_exchangepointandmark);

    /* emacs mode: ESC sequences, all taken from the meta binding table */
    buf[0] = '\33';
    for (i = 0; i < 128; i++)
	if (metabind[i] != z_undefinedkey) {
	    buf[1] = i;
	    addbinding(embindtab, emkeybindtab, buf, 2, metabind[i]);
	}
}

/**/
void
printbind(char const *s, int len)
{
    char const *e = s+len;

    putc('"', bindout);
    for(; s != e; s++) {
	if(*s == '"')
	    fputc('\\', bindout);
	fputs(nicechar(*s), bindout);
    }
    putc('"', bindout);
}

/**/
void
printmetabind(char const *s)
{
    putc('"', bindout);
    for(; *s; s++) {
	char c;
	if (*s == Meta)
	    c = *++s ^ 32;
	else
	    c = *s;
	if(c == '"')
	    fputc('\\', bindout);
	fputs(nicechar(c), bindout);
    }
    putc('"', bindout);
}

/**/
void
printbinding(HashNode hn, int printflags)
{
    Key k = (Key) hn;

    if (k->func == z_undefinedkey)
	return;
    printmetabind(k->nam);
    putc('\t', bindout);
    if (k->func == z_sendstring) {
	printbind(k->str, k->len);
	putc('\n', bindout);
    } else
	fprintf(bindout, "%s\n", ZLEGETCMD(k->func)->name);
}

/**/
int
zlefindfunc(char *name)
{
    struct zlecmd *zs;

    for (zs = zlecmds; zs < zlecmds + ZLECMDCOUNT; zs++)
	if (zs->name && !strcmp(name, zs->name))
	    return zs - zlecmds;
    for (zs = zlecmdadd; zs < zlecmdadd + (zlecmdtot-ZLECMDCOUNT); zs++)
	if (!(zs->flags & ZLE_DELETED) && !strcmp(name, zs->name))
	    return zs - zlecmdadd + ZLECMDCOUNT;

    return zlecmdtot;
}

/* This function is used to turn a single-character key binding into a  *
 * prefix entry.  If tab[i] is z_prefix, it is left alone, otherwise it *
 * is made into a prefix entry.  If a prefix entry is created, the old  *
 * binding is saved in the hash table as a multi-character binding.     */

/**/
void
makeprefix(int *tab, HashTable keytab, int i)
{
    char str[3];
    if(tab[i] == z_prefix)
	return;
    str[0] = i;
    metafy(str, 1, META_NOALLOC);
    keytab->addnode(keytab, ztrdup(str), makefunckey(tab[i]));
    tab[i] = z_prefix;
}

/* This function adds an entry to a multi-character binding table, *
 * and returns a pointer to the new node (which must then be given *
 * a function).  Prefix entries are added as required.             */

/**/
Key
addkeybindentry(int *tab, HashTable keytab, char *str, int len)
{
    Key ret = NULL;
    int p, added=0;
    char *buf = zalloc(len*2 + 1);

    makeprefix(tab, keytab, str[0]);
    for(p=len; p; p--) {
	Key ky;
	int thisadd;

	memcpy(buf, str, p);
	metafy(buf, p, META_NOALLOC);
	ky = (Key) keytab->getnode(keytab, buf);
	if((thisadd = !ky))
	    keytab->addnode(keytab, ztrdup(buf),
		ky = makefunckey(z_undefinedkey));
	if(added)
	    ky->prefixct++;
	added = thisadd;
	if(p == len)
	    ret = ky;
    }
    if(ret->func == z_sendstring) {
	free(ret->str);
	ret->str = NULL;
    }
    return ret;
}

/* This function is used to add key bindings to the tables, handling the *
 * graduation from the single-character binding table to the multi-      *
 * character hash table gracefully.                                      */

/**/
void
addbinding(int *tab, HashTable keytab, char *str, int len, int func)
{
    if(len == 1 && tab[str[0]] != z_prefix)
	tab[str[0]] = func;
    else {
	Key ky = addkeybindentry(tab, keytab, str, len);
	ky->func = func;
    }
}

/**/
int
bin_bindkey(char *name, char **argv, char *ops, int junc)
{
    int i, *tab;
    HashTable keytab;

    if (ops['v'] + ops['e'] + ops['a'] > 1 || (ops['r'] && ops['s'])) {
	zerrnam(name, "incompatible options", NULL, 0);
	return 1;
    }
    if (ops['v'] || ops['e'] || ops['d'] || ops['m']) {
	if (*argv) {
	    zerrnam(name, "too many arguments", NULL, 0);
	    return 1;
	}
	if (ops['d']) {
	    /* empty the hash tables for multi-character bindings */
	    emkeybindtab->emptytable(emkeybindtab);
	    vikeybindtab->emptytable(vikeybindtab);
	    altkeybindtab->emptytable(altkeybindtab);

	    /* reset all key bindings to initial setting */
	    initkeybindings();
	}
	if (ops['e']) {
	    mainbindtab = embindtab;
	    mainkeybindtab = emkeybindtab;
	} else if (ops['v']) {
	    mainbindtab = vibindtab;
	    mainkeybindtab = vikeybindtab;
	}
    }
    if(ops['a']) {
	tab = altbindtab;
	keytab = altkeybindtab;
    } else {
	tab = mainbindtab;
	keytab = mainkeybindtab;
    }
    if (ops['v'] || ops['e'] || ops['d'] || ops['m']) {
	if (ops['m'])
	    for (i = 128; i < 256; i++)
		if (tab[i] == z_selfinsert || tab[i] == z_undefinedkey)
		    tab[i] = metabind[i - 128];
	return 0;
    }

    /* print bindings to stdout */
    bindout = stdout;
    if (!*argv) {
	char buf[2];

	buf[1] = '\0';
	for (i = 0; i < 256; i++)
	    if(tab[i] != z_prefix && tab[i] != z_undefinedkey) {
		buf[0] = i;
		printbind(buf, 1);
		if (i < 254 && tab[i] == tab[i + 1] && tab[i] == tab[i + 2]) {
		    printf(" to ");
		    while (tab[i] == tab[i + 1])
			i++;
		    buf[0] = i;
		    printbind(buf, 1);
		}
		printf("\t%s\n", ZLEGETCMD(tab[i])->name);
	    }
	scanhashtable(keytab, 1, 0, 0, printbinding, 0);
	return 0;
    }
    while (*argv) {
	Key ky = NULL;
	char *s, *smeta;
	int func = 0, len;

	if (ops['u'] || ops['U']) {
	    /* unbind all references to given function */
	    func = zlefindfunc(*argv);
	    if (func == zlecmdtot) {
		zerr("undefined function: %s", *argv, 0);
		return 1;
	    }
	    unbindzlefunc(func, ops['U'] ? 0 : ops['a'] ? 6 : 5);
	    argv++;
	    continue;
	}

	s = getkeystring(*argv++, &len, 2, NULL);
	smeta = metafy(s, len, META_USEHEAP);

	if (!*argv || ops['r']) {
	    /* These are the cases in which we act on the one *
	     * specified binding.                             */
	    int intab = 0;

	    if (len == 1) {
		func = tab[STOUC(*s)];
		intab = func != z_prefix;
	    }
	    if(!intab)
		func = (ky = (Key) keytab->getnode(keytab, smeta)) ? ky->func
		    : z_undefinedkey;
	    if (func == z_undefinedkey) {
		zerrnam(name, "in-string is not bound", NULL, 0);
		zfree(s, len);
		return 1;
	    }
	    if (ops['r']) {
		if (intab)
		    tab[STOUC(*s)] = z_undefinedkey;
		else
		    delprefbinding(tab, keytab, ky, smeta);
		zfree(s, len);
		continue;
	    }
	    if (func == z_sendstring) {
		printbind(ky->str, ky->len);
		putchar('\n');
	    } else
		printf("%s\n", ZLEGETCMD(func)->name);
	    zfree(s, len);
	    return 0;
	}
	if (!ops['s']) {
	    i = zlefindfunc(*argv);
	    if (i == zlecmdtot) {
		zerr("undefined function: %s", *argv, 0);
		zfree(s, len);
		return 1;
	    }
	    func = i;
	} else
	    func = z_sendstring;

	if (len == 1 && !ops['s'] && tab[STOUC(*s)] != z_prefix)
	    tab[STOUC(*s)] = func;
	else {  
	    Key cur = addkeybindentry(tab, keytab, s, len);
	    if (cur->str) {
		zfree(cur->str, cur->len);
		cur->str = NULL;
	    }
	    cur->func = func;
	    if (ops['s']) {
		cur->str = getkeystring(*argv, &cur->len, 2, NULL);
		cur->str = (char *)realloc(cur->str, cur->len);
	    }
	}
	argv++;
	zfree(s, len);
    }
    return 0;
}

/* Delete a binding for a key sequence with prefixes */

/**/
void
delprefbinding(int *tab, HashTable keytab, Key ky, char *s)
{
    int len, pre;
    if (ky->func == z_sendstring) {
	zfree(ky->str, ky->len);
	ky->str = NULL;
    }
    if (ky->prefixct) {
	ky->func = z_undefinedkey;
	return;
    }
    len=strlen(s);
    pre = STOUC((s[0] == Meta) ? (s[1] ^ 32) : s[0]);
    do {
	free(keytab->removenode(keytab, s));
	s[--len] = 0;
	if(len && s[len-1] == Meta)
	    s[--len] = 0;
	if(!len) {
	    /* I think this should not happen if the hash table *
	     * is maintained correctly.                         */
	    tab[pre] = z_undefinedkey;
	    return;
	}
	ky = (Key) keytab->getnode(keytab, s);
	if(!--ky->prefixct && ky->func != z_sendstring &&
	    (!s[1] || (!s[2] && s[0] == Meta))) {
	    tab[pre] = ky->func;
	    free(keytab->removenode(keytab, s));
	    return;
	}
    } while(!ky->prefixct && ky->func == z_undefinedkey);
}

/**/
void
freekeynode(HashNode hn)
{
    Key k = (Key) hn;

    zsfree(k->nam);
    if (k->str)
	zfree(k->str, k->len);
    zfree(k, sizeof(struct key));
}

/* vared: edit (literally) a parameter value */

/**/
int
bin_vared(char *name, char **args, char *ops, int func)
{
    char *s;
    char *t;
    Param pm;
    int create = 0;
    char *p1 = NULL, *p2 = NULL;

    /* all options are handled as arguments */
    while (*args && **args == '-') {
	while (*++(*args))
	    switch (**args) {
	    case 'c':
		/* -c option -- allow creation of the parameter if it doesn't
		yet exist */
		create = 1;
		break;
	    case 'p':
		/* -p option -- set main prompt string */
		if ((*args)[1])
		    p1 = *args + 1, *args = "" - 1;
		else if (args[1])
		    p1 = *(++args), *args = "" - 1;
		else {
		    zwarnnam(name, "prompt string expected after -%c", NULL,
			     **args);
		    return 1;
		}
		break;
	    case 'r':
		/* -r option -- set right prompt string */
		if ((*args)[1])
		    p2 = *args + 1, *args = "" - 1;
		else if (args[1])
		    p2 = *(++args), *args = "" - 1;
		else {
		    zwarnnam(name, "prompt string expected after -%c", NULL,
			     **args);
		    return 1;
		}
		break;
	    case 'h':
		/* -h option -- enable history */
		ops['h'] = 1;
		break;
	    default:
		/* unrecognised option character */
		zwarnnam(name, "unknown option: %s", *args, 0);
		return 1;
	    }
	args++;
    }

    /* check we have a parameter name */
    if (!*args) {
	zwarnnam(name, "missing variable", NULL, 0);
	return 1;
    }
    /* handle non-existent parameter */
    if (!(s = getsparam(args[0]))) {
	if (create)
	    createparam(args[0], PM_SCALAR);
	else {
	    zwarnnam(name, "no such variable: %s", args[0], 0);
	    return 1;
	}
    }
    /* edit the parameter value */
    PERMALLOC {
	pushnode(bufstack, ztrdup(s));
    } LASTALLOC;
    in_vared = !ops['h'];
    t = (char *) zleread(p1, p2);
    in_vared = 0;
    if (!t || errflag) {
	/* error in editing */
	errflag = 0;
	return 1;
    }
    /* strip off trailing newline, if any */
    if (t[strlen(t) - 1] == '\n')
	t[strlen(t) - 1] = '\0';
    /* final assignment of parameter value */
    pm = (Param) paramtab->getnode(paramtab, args[0]);
    if (PM_TYPE(pm->flags) == PM_ARRAY) {
	PERMALLOC {
	    setaparam(args[0], spacesplit(t, 1));
	} LASTALLOC;
    } else
	setsparam(args[0], t);
    return 0;
}

extern int clearflag;

/**/
void
describekeybriefly(void)
{
    int cmd;
    int len;

    if (statusline)
	return;
    statusline = "Describe key briefly: _";
    statusll = strlen(statusline);
    refresh();
    cmd = getkeycmd();
    statusline = NULL;
    if (cmd < 0)
	return;
    trashzle();
    clearflag = (isset(USEZLE) && termok &&
		 (isset(ALWAYSLASTPROMPT) && zmult == 1)) ||
	(unset(ALWAYSLASTPROMPT) && zmult != 1);
    printbind(keybuf, len = keybuflen);
    fprintf(shout, " is ");
    if (cmd == z_sendstring) {
	if (!cky) {
	    metafy_keybuf();
	    cky = (Key) keybindtab->getnode(keybindtab, keybuf);
	    unmetafy(keybuf, NULL);
	}
	printbind(cky->str, cky->len);
    }
    else
	fprintf(shout, "%s", ZLEGETCMD(cmd)->name);
    if (clearflag)
	putc('\r', shout), tcmultout(TCUP, TCMULTUP, nlnct);
    else
	putc('\n', shout);
    showinglist = 0;
}

static int func, funcfound;
#define MAXFOUND 4

static void
printfuncbind(HashNode hn, int printflags)
{
    Key k = (Key) hn;

    if (k->func != func || funcfound >= MAXFOUND)
	return;
    if (!funcfound++)
	fprintf(shout, " on");
    putc(' ', shout);
    printmetabind(k->nam);
}

/**/
void
whereis(void)
{
    int i;

    if ((func = executenamedcommand("Where is: ")) == -1)
	return;
    funcfound = 0;
    trashzle();
    clearflag = (isset(USEZLE) && termok &&
		 (isset(ALWAYSLASTPROMPT) && zmult == 1)) ||
	(unset(ALWAYSLASTPROMPT) && zmult != 1);
    if (func == z_selfinsert || func == z_undefinedkey)
	fprintf(shout, "%s is on many keys", ZLEGETCMD(func)->name);
    else {
	fprintf(shout, "%s is", ZLEGETCMD(func)->name);
	for (i = 0; funcfound < MAXFOUND && i < 256; i++)
	    if (mainbindtab[i] == func) {
		char ch = i;
		if (!funcfound++)
		    fprintf(shout, " on");
		putc(' ', shout);
		printbind(&ch, 1);
	    }
	if (funcfound < MAXFOUND)
	    scanhashtable(mainkeybindtab, 1, 0, 0, printfuncbind, 0);
	if (!funcfound)
	    fprintf(shout, " not bound to any key");
    }
    if (clearflag)
	putc('\r', shout), tcmultout(TCUP, TCMULTUP, nlnct);
    else
	putc('\n', shout);
    showinglist = 0;
}

static LinkList bindremlist;

/**/
void
zerobinding(HashNode hn, int ifunc)
{
    if (((Key)hn)->func == ifunc)
	addlinknode(bindremlist, hn);
}

/*
 * Unbind the given zle function ifunc wherever it occurs.
 * With notall &3 == 0, do this for all binding tables.
 *             &3 == 1, do this only in the main binding table.
 *             &3 == 2, do this only in the alternate binding table.
 *             &4, don't mangle the last named command.
 * Note that the main and alternate binding tables share
 * the same table for sequences with prefixes.
 */

/**/
void
unbindzlefunc(int ifunc, int notall)
{
    int i, *tab = 0;
    HashTable keytab = NULL;
    /* N.B. we can't put the HashTable values in this table directly,  *
     * because ANSI C doesn't allow non-static aggregate initialisers. */
    static struct {
	int *tab;
	HashTable *keytabp;
    } list[] = {
	{ altbindtab, &altkeybindtab },
	{ embindtab,  &emkeybindtab  },
	{ vibindtab,  &vikeybindtab  },
	{ NULL, NULL }
    }, *ptr;

    switch(notall & 3) {
	case 1:  tab = mainbindtab; keytab = mainkeybindtab; break;
	case 2:  tab = altbindtab;  keytab = altkeybindtab;  break;
    }
    PERMALLOC {
	for (ptr = list; ptr->tab; ptr++) {
	    LinkNode ln;

	    if(!(notall & 3)) {
		tab = ptr->tab;
		keytab = *ptr->keytabp;
	    }

	    /*
	     * Search the key binding tables for the function.
	     * It's dangerous to modify a hash table while scanning,
	     * so put the bindings found in a list and
	     * delete the bindings one by one later.
	     */
	    bindremlist = newlinklist();
	    scanhashtable(keytab, 0, 0, 0, zerobinding, ifunc);
	    for (ln = firstnode(bindremlist); ln; incnode(ln)) {
		Key k = (Key) ln->dat;
		char *s = ztrdup(k->nam);
		int sl = strlen(s);
		delprefbinding(tab, keytab, k, s);
		zfree(s, sl);
	    }
	    freelinklist(bindremlist, NULL);

	    for (i = 0; i < 256; i++)
		if (tab[i] == ifunc)
		    tab[i] = z_undefinedkey;

	    if (notall & 3)
		break;
	}
	bindremlist = NULL;
    } LASTALLOC;

    if(!(notall & 4) && lastnamed == ifunc)
	lastnamed = z_undefinedkey;
}

/**/
void
trashzle(void)
{
    if (zleactive) {
	/* This refresh() is just to get the main editor display right and *
	 * get the cursor in the right place.  For that reason, we disable *
	 * list display (which would otherwise result in infinite          *
	 * recursion [at least, it would if refresh() didn't have its      *
	 * extra `inlist' check]).                                         */
	int sl = showinglist;
	showinglist = 0;
	refresh();
	showinglist = sl;
	moveto(nlnct, 0);
	if (clearflag && tccan(TCCLEAREOD)) {
	    tcout(TCCLEAREOD);
	    clearflag = 0;
	}
	if (postedit)
	    fprintf(shout, "%s", postedit);
	fflush(shout);
	resetneeded = 1;
	settyinfo(&shttyinfo);
    }
    if (errflag)
	kungetct = 0;
}

/********************************/
/* Compctl Hash Table Functions */
/********************************/

#ifndef MODULE
void printcompctlp _((HashNode hn, int printflags));
#endif

/**/
void
createcompctltable(void)
{
    compctltab = newhashtable(23);

    compctltab->hash        = hasher;
    compctltab->emptytable  = NULL;
    compctltab->filltable   = NULL;
    compctltab->addnode     = addhashnode;
    compctltab->getnode     = gethashnode2;
    compctltab->getnode2    = gethashnode2;
    compctltab->removenode  = removehashnode;
    compctltab->disablenode = NULL;
    compctltab->enablenode  = NULL;
    compctltab->freenode    = freecompctlp;
#ifdef MODULE
    compctltab->printnode   = NULL;
#else
    compctltab->printnode   = printcompctlp;
#endif
#ifdef ZSH_HASH_DEBUG
    compctltab->printinfo   = printhashtabinfo;
    compctltab->tablename   = ztrdup("compctltab");
#endif
}

/**/
void
freecompctl(Compctl cc)
{
    if (cc == &cc_default ||
 	cc == &cc_first ||
	cc == &cc_compos ||
	--cc->refc > 0)
	return;

    zsfree(cc->keyvar);
    zsfree(cc->glob);
    zsfree(cc->str);
    zsfree(cc->func);
    zsfree(cc->explain);
    zsfree(cc->prefix);
    zsfree(cc->suffix);
    zsfree(cc->hpat);
    zsfree(cc->subcmd);
    if (cc->cond)
	freecompcond(cc->cond);
    if (cc->ext) {
	Compctl n, m;

	n = cc->ext;
	do {
	    m = (Compctl) (n->next);
	    freecompctl(n);
	    n = m;
	}
	while (n);
    }
    if (cc->xor && cc->xor != &cc_default)
	freecompctl(cc->xor);
    zfree(cc, sizeof(struct compctl));
}

/**/
void
freecompctlp(HashNode hn)
{
    Compctlp ccp = (Compctlp) hn;

    zsfree(ccp->nam);
    freecompctl(ccp->cc);
    zfree(ccp, sizeof(struct compctlp));
}

/***********************************************************/
/* Emacs Multi-Character Key Bindings Hash Table Functions */
/***********************************************************/

/* size of the initial hashtable for multi-character *
 * emacs key bindings.                               */
#define INITIAL_EMKEYBINDTAB 67

/**/
void
createemkeybindtable(void)
{
    emkeybindtab = newhashtable(INITIAL_EMKEYBINDTAB);

    emkeybindtab->hash        = hasher;
    emkeybindtab->emptytable  = emptyemkeybindtable;
    emkeybindtab->filltable   = NULL;
    emkeybindtab->addnode     = addhashnode;
    emkeybindtab->getnode     = gethashnode2;
    emkeybindtab->getnode2    = gethashnode2;
    emkeybindtab->removenode  = removehashnode;
    emkeybindtab->disablenode = NULL;
    emkeybindtab->enablenode  = NULL;
    emkeybindtab->freenode    = freekeynode;

    /* need to combine printbinding and printfuncbinding for this */
    emkeybindtab->printnode   = NULL;
#ifdef ZSH_HASH_DEBUG
    emkeybindtab->printinfo   = printhashtabinfo;
    emkeybindtab->tablename   = ztrdup("emkeybindtab");
#endif
}

/**/
void
emptyemkeybindtable(HashTable ht)
{
    emptyhashtable(ht, INITIAL_EMKEYBINDTAB);
}

/********************************************************/
/* Vi Multi-Character Key Bindings Hash Table Functions */
/********************************************************/

/* size of the initial hash table for *
 * multi-character vi key bindings.   */
#define INITIAL_VIKEYBINDTAB 20

/**/
void
createvikeybindtable(void)
{
    vikeybindtab = newhashtable(INITIAL_VIKEYBINDTAB);

    vikeybindtab->hash        = hasher;
    vikeybindtab->emptytable  = emptyvikeybindtable;
    vikeybindtab->filltable   = NULL;
    vikeybindtab->addnode     = addhashnode;
    vikeybindtab->getnode     = gethashnode2;
    vikeybindtab->getnode2    = gethashnode2;
    vikeybindtab->removenode  = removehashnode;
    vikeybindtab->disablenode = NULL;
    vikeybindtab->enablenode  = NULL;
    vikeybindtab->freenode    = freekeynode;

    /* need to combine printbinding and printfuncbinding for this */
    vikeybindtab->printnode   = NULL;
#ifdef ZSH_HASH_DEBUG
    vikeybindtab->printinfo   = printhashtabinfo;
    vikeybindtab->tablename   = ztrdup("vikeybindtab");
#endif
}

/**/
void
emptyvikeybindtable(HashTable ht)
{
    emptyhashtable(ht, INITIAL_VIKEYBINDTAB);
}

/***************************************************************/
/* Alternate Multi-Character Key Bindings Hash Table Functions */
/***************************************************************/

/* size of the initial hash table for      *
 * alternate multi-character key bindings. */
#define INITIAL_ALTKEYBINDTAB 20

/**/
void
createaltkeybindtable(void)
{
    altkeybindtab = newhashtable(INITIAL_ALTKEYBINDTAB);

    altkeybindtab->hash        = hasher;
    altkeybindtab->emptytable  = emptyaltkeybindtable;
    altkeybindtab->filltable   = NULL;
    altkeybindtab->addnode     = addhashnode;
    altkeybindtab->getnode     = gethashnode2;
    altkeybindtab->getnode2    = gethashnode2;
    altkeybindtab->removenode  = removehashnode;
    altkeybindtab->disablenode = NULL;
    altkeybindtab->enablenode  = NULL;
    altkeybindtab->freenode    = freekeynode;

    /* need to combine printbinding and printfuncbinding for this */
    altkeybindtab->printnode   = NULL;
#ifdef ZSH_HASH_DEBUG
    altkeybindtab->printinfo   = printhashtabinfo;
    altkeybindtab->tablename   = ztrdup("altkeybindtab");
#endif
}

/**/
void
emptyaltkeybindtable(HashTable ht)
{
    emptyhashtable(ht, INITIAL_ALTKEYBINDTAB);
}

/**/
void
compctlsetup(void)
{
    cc_compos.mask = CC_COMMPATH;
    cc_default.refc = 10000;
    cc_default.mask = CC_FILES;
    cc_first.refc = 10000;
    cc_first.mask = 0;
}

/**/
int
boot_zle(Module m)
{
#ifdef MODULE
    if (load_zle_syms(m->handle))
	return -1;
#endif
    createcompctltable();   /* create hash table for compctls          */

    /* create hash tables for multi-character key bindings */
    createemkeybindtable();
    createvikeybindtable();
    createaltkeybindtable();

    initkeybindings();	    /* initialize key bindings */
    compctlsetup();
    addbuiltin("bindkey", 0, bin_bindkey, 0, -1, "asvemdruU");
    addbuiltin("vared", 0, bin_vared, 1, 7, NULL);
    return 0;
}
