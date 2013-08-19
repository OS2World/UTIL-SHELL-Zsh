/*
 * $Id: prompt.c,v 3.1.0.1 1996/12/05 03:59:45 hzoli Exp $
 *
 * prompt.c - construct zsh prompts
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

#include "zsh.h"

static char *bp;
static char *truncstr;
static int dontcount, lensb, trunclen;

/* stradd() adds a string to the prompt, in a *
 * visible representation, doing truncation.  */

/**/
void
stradd(char *d)
{
    /* dlen is the full length of the string we want to add */
    int dlen = nicestrlen(d);
    char *ps, *pd, *pc, *t;
    int tlen, maxlen;
    addbufspc(dlen);
    /* This loop puts the nice representation of the string into the prompt *
     * buffer.  It might be modified later.  Note that bp isn't changed.    */
    for(ps=d, pd=bp; *ps; ps++)
	for(pc=nicechar(STOUC(*ps)); *pc; pc++)
	    *pd++ = *pc;
    if(!trunclen || dlen <= trunclen) {
	/* No truncation is needed, so update bp and return, *
	 * leaving the full string in the prompt.            */
	bp += dlen;
	return;
    }
    /* We need to truncate.  t points to the truncation string -- which is *
     * inserted literally, without nice representation.  tlen is its       *
     * length, and maxlen is the amout of the main string that we want to  *
     * keep.  Note that if the truncation string is longer than the        *
     * truncation length (tlen > trunclen), the truncation string is used  *
     * in full.                                                            */
    t = truncstr + 1;
    tlen = strlen(t);
    maxlen = tlen < trunclen ? trunclen - tlen : 0;
    addbufspc(tlen);
    if(*truncstr == '>') {
	/* '>' means truncate at the right.  We just move past the first *
	 * maxlen characters of the string, and write the truncation     *
	 * string there.                                                 */
	bp += maxlen;
	while(*t)
	    pputc(*t++);
    } else {
	/* Truncation at the left: ps is initialised to the start of the *
	 * part of the string that we want to keep.  pc points to the    *
	 * end of the string.  The truncation string is added to the     *
	 * prompt, then the desired part of the string is copied into    *
	 * the right place.                                              */
	ps = bp + dlen - maxlen;
	pc = bp + dlen;
	while(*t)
	    pputc(*t++);
	while(ps < pc)
	    *bp++ = *ps++;
    }
}

/**/
int
putstr(int d)
{
    addbufspc(1);
    *bp++ = d;
    if (!dontcount)
	lensb++;
    return 0;
}

/* get a prompt string */

static char *buf, *bp1, *bl0, *fm, *pmpt;
static int bracepos, bufspc;

/**/
char *
putprompt(char *fmin, int *lenp, int *wp, int cnt)
{
    if (!fmin) {
	*lenp = 0;
	if (wp)
	    *wp = 0;
	return ztrdup("");
    }

    if (!termok && (unset(INTERACTIVE)))
        init_term();

    bracepos = -1;
    fm = fmin;
    lensb = 0;
    pmpt = (dontcount = !cnt) ? NULL : fm;
    bp = bl0 = buf = zalloc(bufspc = 256);
    bp1 = NULL;

    clearerr(stdin);

    trunclen = 0;
    putpromptchar(1, '\0');

    *lenp = bp - buf;
    if (wp) {
	*wp = bp - bl0 - lensb;
	if (pmpt != rpmpt) {
	    *wp %= columns;
	    if (*wp == columns - 1) {
		addbufspc(1);
		*wp = 0;
		*bp++ = ' ';
		++*lenp;
	    }
	    if (!*wp && *lenp) {
		addbufspc(1);
		(*wp)++;
	    	*bp++ = ' ';
		++*lenp;
	    }
	}
    }

    return buf;
}

/**/
int
putpromptchar(int doprint, int endchar)
{
    char buf3[PATH_MAX], *ss;
    int t0, arg, test, sep;
    struct tm *tm;
    time_t timet;
    Nameddir nd;

    if (isset(PROMPTSUBST)) {
	int olderr = errflag;

	HEAPALLOC {
	    fm = dupstring(fm);
	    if (!parsestr(fm))
		singsub(&fm);
	} LASTALLOC;
	/* Ignore errors in prompt substitution */
	errflag = olderr;
    }
    for (; *fm && *fm != endchar; fm++) {
	arg = 0;
	if (*fm == '%') {
	    if (idigit(*++fm)) {
		arg = zstrtol(fm, &fm, 10);
	    }
	    if (*fm == '(') {
		int tc;

		if (idigit(*++fm)) {
		    arg = zstrtol(fm, &fm, 10);
		}
		test = 0;
		ss = pwd;
		switch (tc = *fm) {
		case 'c':
		case '.':
		case '~':
		    if ((nd = finddir(ss))) {
			arg--;
			ss += strlen(nd->dir);
		    }
		case '/':
		case 'C':
		    for (; *ss; ss++)
			if (*ss == '/')
			    arg--;
		    if (arg <= 0)
			test = 1;
		    break;
		case 't':
		case 'T':
		case 'd':
		case 'D':
		case 'w':
		    timet = time(NULL);
		    tm = localtime(&timet);
		    switch (tc) {
		    case 't':
			test = (arg == tm->tm_min);
			break;
		    case 'T':
			test = (arg == tm->tm_hour);
			break;
		    case 'd':
			test = (arg == tm->tm_mday);
			break;
		    case 'D':
			test = (arg == tm->tm_mon);
			break;
		    case 'w':
			test = (arg == tm->tm_wday);
			break;
		    }
		    break;
		case '?':
		    if (lastval == arg)
			test = 1;
		    break;
		case '#':
		    if (geteuid() == arg)
			test = 1;
		    break;
		case 'g':
		    if (getegid() == arg)
			test = 1;
		    break;
		case 'L':
		    if (shlvl >= arg)
			test = 1;
		    break;
		case 'S':
		    if (time(NULL) - shtimer.tv_sec >= arg)
			test = 1;
		    break;
		case 'v':
		    if (arrlen(psvar) >= arg)
			test = 1;
		    break;
		case '_':
		    test = (cmdsp >= arg);
		    break;
		default:
		    test = -1;
		    break;
		}
		if (!*fm || !(sep = *++fm))
		    return 0;
		fm++;
		if (!putpromptchar(test == 1 && doprint, sep) || !*++fm ||
		    !putpromptchar(test == 0 && doprint, ')')) {
		    return 0;
		}
		continue;
	    }
	    if (!doprint)
		switch(*fm) {
		  case '[':
		    while(idigit(*++fm));
		    while(*++fm != ']');
		    continue;
		  case '<':
		    while(*++fm != '<');
		    continue;
		  case '>':
		    while(*++fm != '>');
		    continue;
		  case 'D':
		    if(fm[1]=='{')
			while(*++fm != '}');
		    continue;
		  default:
		    continue;
		}
	    switch (*fm) {
	    case '~':
		if ((nd = finddir(pwd))) {
		    sprintf(buf3, "~%s%s", nd->nam, pwd + strlen(nd->dir));
		    stradd(buf3);
		    break;
		}
	    case 'd':
	    case '/':
		stradd(pwd);
		break;
	    case 'c':
	    case '.':
		if ((nd = finddir(pwd)))
		    sprintf(buf3, "~%s%s", nd->nam, pwd + strlen(nd->dir));
		else
		    strcpy(buf3, pwd);
		if (!arg)
		    arg++;
		for (ss = buf3 + strlen(buf3); ss > buf3; ss--)
		    if (*ss == '/' && !--arg) {
			ss++;
			break;
		    }
		if (*ss == '/' && ss[1] && (ss != buf3))
		    ss++;
		stradd(ss);
		break;
	    case 'C':
		strcpy(buf3, pwd);
		if (!arg)
		    arg++;
		for (ss = buf3 + strlen(buf3); ss > buf3; ss--)
		    if (*ss == '/' && !--arg) {
			ss++;
			break;
		    }
		if (*ss == '/' && ss[1] && (ss != buf3))
		    ss++;
		stradd(ss);
		break;
	    case 'h':
	    case '!':
		addbufspc(DIGBUFSIZE);
		sprintf(bp, "%d", curhist);
		bp += strlen(bp);
		break;
	    case 'M':
		stradd(hostnam);
		break;
	    case 'm':
		if (!arg)
		    arg++;
		for (ss = hostnam; *ss; ss++)
		    if (*ss == '.' && !--arg)
			break;
		t0 = *ss;
		*ss = '\0';
		stradd(hostnam);
		*ss = t0;
		break;
	    case 'S':
		txtchangeset(TXTSTANDOUT, TXTNOSTANDOUT);
		txtset(TXTSTANDOUT);
		tsetcap(TCSTANDOUTBEG, 1);
		break;
	    case 's':
		txtchangeset(TXTNOSTANDOUT, TXTSTANDOUT);
		txtset(TXTDIRTY);
		txtunset(TXTSTANDOUT);
		tsetcap(TCSTANDOUTEND, 1);
		break;
	    case 'B':
		txtchangeset(TXTBOLDFACE, TXTNOBOLDFACE);
		txtset(TXTDIRTY);
		txtset(TXTBOLDFACE);
		tsetcap(TCBOLDFACEBEG, 1);
		break;
	    case 'b':
		txtchangeset(TXTNOBOLDFACE, TXTBOLDFACE);
		txtchangeset(TXTNOSTANDOUT, TXTSTANDOUT);
		txtchangeset(TXTNOUNDERLINE, TXTUNDERLINE);
		txtset(TXTDIRTY);
		txtunset(TXTBOLDFACE);
		tsetcap(TCALLATTRSOFF, 1);
		break;
	    case 'U':
		txtchangeset(TXTUNDERLINE, TXTNOUNDERLINE);
		txtset(TXTUNDERLINE);
		tsetcap(TCUNDERLINEBEG, 1);
		break;
	    case 'u':
		txtchangeset(TXTNOUNDERLINE, TXTUNDERLINE);
		txtset(TXTDIRTY);
		txtunset(TXTUNDERLINE);
		tsetcap(TCUNDERLINEEND, 1);
		break;
	    case '[':
                if (idigit(*++fm))
                    trunclen = zstrtol(fm, &fm, 10);
                else
                    trunclen = arg;
                if (trunclen) {
		    bp1 = bp;
		    while (*fm && *fm != ']') {
			if (*fm == '\\' && fm[1])
			    ++fm;
			addbufspc(1);
			*bp++ = *fm++;
		    }
		    addbufspc(2);
		    if (bp1 == bp)
			*bp++ = '<';
                    *bp = '\0';
		    zsfree(truncstr);
                    truncstr = ztrdup(bp = bp1);
		    bp1 = NULL;
                } else {
		    while (*fm && *fm != ']') {
			if (*fm == '\\' && fm[1])
			    fm++;
			fm++;
		    }
		}
		if(!*fm)
		    return 0;
		break;
	    case '<':
	    case '>':
		if((trunclen = arg)) {
		    bp1 = bp;
		    addbufspc(1);
		    *bp++ = *fm++;
		    while (*fm && *fm != *bp1) {
			if (*fm == '\\' && fm[1])
			    ++fm;
			addbufspc(1);
			*bp++ = *fm++;
		    }
		    addbufspc(1);
                    *bp = '\0';
		    zsfree(truncstr);
                    truncstr = ztrdup(bp = bp1);
		    bp1 = NULL;
		} else {
		    char ch = *fm++;
		    while(*fm && *fm != ch) {
			if (*fm == '\\' && fm[1])
			    fm++;
			fm++;
		    }
		}
		if(!*fm)
		    return 0;
		break;
	    case '{':
		if (!dontcount++)
		    bracepos = bp - buf;
		break;
	    case '}':
		if (!--dontcount && bracepos != -1) {
		    lensb += (bp - buf) - bracepos;
		    bracepos = -1;
		}
		break;
	    case 't':
	    case '@':
	    case 'T':
	    case '*':
	    case 'w':
	    case 'W':
	    case 'D':
		{
		    char *tmfmt, *dd;

		    switch (*fm) {
		    case 'T':
			tmfmt = "%k:%M";
			break;
		    case '*':
			tmfmt = "%k:%M:%S";
			break;
		    case 'w':
			tmfmt = "%a %e";
			break;
		    case 'W':
			tmfmt = "%m/%d/%y";
			break;
		    case 'D':
			tmfmt = "%y-%m-%d";
			if (fm[1] == '{') {
			    for (ss = fm + 2, dd = buf3; *ss && *ss != '}'; ++ss, ++dd)
				*dd = *((*ss == '\\' && ss[1]) ? ++ss : ss);
			    if (*ss == '}') {
				*dd = '\0';
				fm = ss;
				tmfmt = buf3;
			    }
			}
			break;
		    default:
			tmfmt = "%l:%M%p";
			break;
		    }
		    timet = time(NULL);
		    tm = localtime(&timet);
		    addbufspc(80);   /* 80 is arbitrary :-( */
		    ztrftime(bp, 79, tmfmt, tm);
		    if (*bp == ' ')
			chuck(bp);
		    bp += strlen(bp);
		    break;
		}
	    case 'n':
		stradd(get_username());
		break;
	    case 'l':
		if (*ttystrname) {
		    ss = (strncmp(ttystrname, "/dev/tty", 8) ?
			    ttystrname + 5 : ttystrname + 8);
		    stradd(ss);
		} else
		    stradd("()");
		break;
	    case '?':
		addbufspc(DIGBUFSIZE);
		sprintf(bp, "%ld", (long)lastval);
		bp += strlen(bp);
		break;
	    case '%':
		addbufspc(1);
		*bp++ = '%';
		break;
	    case ')':
		addbufspc(1);
		*bp++ = ')';
		break;
	    case '#':
		addbufspc(1);
		*bp++ = (geteuid())? '%' : '#';
		break;
	    case 'v':
		if (!arg)
		    arg = 1;
		if (arrlen(psvar) >= arg)
		    stradd(psvar[arg - 1]);
		break;
	    case 'E':
                tsetcap(TCCLEAREOL, 1);
		break;
	    case '_':
		if (cmdsp) {
		    if (arg > cmdsp || arg <= 0)
			arg = cmdsp;
		    for (t0 = cmdsp - arg; arg--; t0++) {
			stradd(cmdnames[cmdstack[t0]]);
			if (arg) {
			    addbufspc(1);
			    *bp++=' ';
			}
		    }
		}
		break;
	    case 'r':
		if (pmpt == sprompt) {
		    stradd(rstring);
		    break;
		}
	    case 'R':
		if (pmpt == sprompt) {
		    stradd(Rstring);
		    break;
		}
	    case '\0':
		return 0;
	    default:
		addbufspc(2);
		*bp++ = '%';
		pputc(*fm);
		break;
	    }
	} else if (doprint) {
	    addbufspc(1);
	    pputc(*fm == Meta ? *++fm ^ 32 : *fm);
	}
    }

    return *fm;
}

/**/
void
pputc(char c)
{
    if (!dontcount) {
	if (pmpt == rpmpt) {
	    if (c == '\t' || c == '\n')
		c = ' ';
	} else if (c == '\t') {
	    int t0 = 7 - (7 & (bp - bl0 - lensb));
	    lensb -= t0;
	} else if (c == '\n') {
	    *bp++ = c;
	    bl0 = bp;
	    lensb = 0;
	    return;
	}
    }
    *bp++ = c;
}

/**/
void
addbufspc(int need)
{
    if((bp - buf) + need > bufspc) {
	char *oldbuf = buf;
	if(need & 255)
	    need = (need | 255) + 1;
	buf = realloc(buf, bufspc += need);
	bp = buf + (bp - oldbuf);
	bl0 = buf + (bl0 - oldbuf);
	if(bp1)
	    bp1 = buf + (bp1 - oldbuf);
    }
}

/**/
void
tsetcap(int cap, int flag)
{
    if (termok && unset(SINGLELINEZLE) && tcstr[cap]) {
	switch(flag) {
	case -1:
	    tputs(tcstr[cap], 1, putraw);
	    break;
	case 0:
	    tputs(tcstr[cap], 1, putshout);
	    break;
	case 1:
	    if (!dontcount) {
		int  t0;

		if (cap == TCSTANDOUTBEG || cap == TCSTANDOUTEND)
		    t0 = tgetnum("sg");
		else if (cap == TCUNDERLINEBEG || cap == TCUNDERLINEEND)
		    t0 = tgetnum("ug");
		else
		    t0 = 0;
		if (t0 > 0)
		    lensb -= t0;
	    }
	    tputs(tcstr[cap], 1, putstr);
	    break;
	}

	if (txtisset(TXTDIRTY)) {
	    txtunset(TXTDIRTY);
	    if (txtisset(TXTBOLDFACE) && cap != TCBOLDFACEBEG)
		tsetcap(TCBOLDFACEBEG, flag);
	    if (txtisset(TXTSTANDOUT))
		tsetcap(TCSTANDOUTBEG, flag);
	    if (txtisset(TXTUNDERLINE))
		tsetcap(TCUNDERLINEBEG, flag);
	}
    }
}
