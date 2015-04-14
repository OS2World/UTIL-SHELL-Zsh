/*
 * $Id: zle_misc.c,v 3.1.0.4 1996/12/05 03:59:45 hzoli Exp $
 *
 * zle_misc.c - miscellaneous editor routines
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

#include "zle.h"

/**/
void
selfinsert(void)
{
    int ncs = cs + zmult;

    if (complexpect && isset(AUTOPARAMKEYS)) {
	if (complexpect == 2 && /*{*/ c == '}') {
	    if (!menucmp || line[cs-1] == '/' || addedsuffix) {
		int i = 0;
		spaceinline(1);
		do {
		    line[cs-i] = line[cs-i-1];
		} while (++i < addedsuffix);
		line[cs-i] = c;
		cs++;
		return;
	    }
	} else if (c == ':' || c == '[' || (complexpect == 2 &&
		    (c == '#' || c == '%' || c == '-' ||
		     c == '?' || c == '+' || c == '='))) {
	    if (addedsuffix > 1)
		backdel(addedsuffix - 1);

	    if (!menucmp || line[cs-1] == '/' || addedsuffix) {
		line[cs - 1] = c;
		return;
	    }
	}
    }
    if (zmult < 0) {
	zmult = -zmult;
	ncs = cs;
    }
    if (insmode || ll == cs)
	spaceinline(zmult);
    else if (zmult + cs > ll)
	spaceinline(ll - (zmult + cs));
    while (zmult--)
	line[cs++] = c;
    cs = ncs;
}

/**/
void
selfinsertunmeta(void)
{
    c &= 0x7f;
    if (c == '\r')
	c = '\n';
    selfinsert();
}

/**/
void
deletechar(void)
{
    if (zmult < 0) {
	zmult = -zmult;
	backwarddeletechar();
	return;
    }
    if (cs + zmult <= ll) {
	cs += zmult;
	backdel(zmult);
    } else
	feep();
}

/**/
void
backwarddeletechar(void)
{
    if (zmult < 0) {
	zmult = -zmult;
	deletechar();
	return;
    }
    if (zmult > cs)
	zmult = cs;
    backdel(zmult);
}

/**/
void
killwholeline(void)
{
    int i, fg;

    if (zmult < 0)
	return;
    while (zmult--) {
	if ((fg = (cs && cs == ll)))
	    cs--;
	while (cs && line[cs - 1] != '\n')
	    cs--;
	for (i = cs; i != ll && line[i] != '\n'; i++);
	forekill(i - cs + (i != ll), fg);
    }
}

/**/
void
killbuffer(void)
{
    cs = 0;
    forekill(ll, 0);
}

/**/
void
backwardkillline(void)
{
    int i = 0;

    if (zmult < 0) {
	zmult = -zmult;
	killline();
	return;
    }
    while (zmult--) {
	if (cs && line[cs - 1] == '\n')
	    cs--, i++;
	else
	    while (cs && line[cs - 1] != '\n')
		cs--, i++;
    }
    forekill(i, 1);
}

/**/
void
gosmacstransposechars(void)
{
    int cc;

    if (cs < 2 || line[cs - 1] == '\n' || line[cs - 2] == '\n') {
	if (cs == ll || line[cs] == '\n' ||
	    ((cs + 1 == ll || line[cs + 1] == '\n') &&
	     (!cs || line[cs - 1] == '\n'))) {
	    feep();
	    return;
	}
	cs += (cs == 0 || line[cs - 1] == '\n') ? 2 : 1;
    }
    cc = line[cs - 2];
    line[cs - 2] = line[cs - 1];
    line[cs - 1] = cc;
}

/**/
void
transposechars(void)
{
    int cc, ct;
    int neg = zmult < 0;

    if (neg)
	zmult = -zmult;
    while (zmult--) {
	if (!(ct = cs) || line[cs - 1] == '\n') {
	    if (ll == cs || line[cs] == '\n') {
		feep();
		return;
	    }
	    if (!neg)
		cs++;
	    ct++;
	}
	if (neg) {
	    if (cs && line[cs - 1] != '\n') {
		cs--;
		if (ct > 1 && line[ct - 2] != '\n')
		    ct--;
	    }
	} else {
	    if (cs != ll && line[cs] != '\n')
		cs++;
	}
	if (ct == ll || line[ct] == '\n')
	    ct--;
	if (ct < 1 || line[ct - 1] == '\n') {
	    feep();
	    return;
	}
	cc = line[ct - 1];
	line[ct - 1] = line[ct];
	line[ct] = cc;
    }
}

/**/
void
poundinsert(void)
{
    cs = 0;
    vifirstnonblank();
    if (line[cs] != '#') {
	spaceinline(1);
	line[cs] = '#';
	cs = findeol();
	while(cs != ll) {
	    cs++;
	    vifirstnonblank();
	    spaceinline(1);
	    line[cs] = '#';
	    cs = findeol();
	}
    } else {
	foredel(1);
	cs = findeol();
	while(cs != ll) {
	    cs++;
	    vifirstnonblank();
	    if(line[cs] == '#')
		foredel(1);
	    cs = findeol();
	}
    }
    done = 1;
}

/**/
void
acceptline(void)
{
    done = 1;
}

/**/
void
acceptandhold(void)
{
    pushnode(bufstack, metafy((char *)line, ll, META_DUP));
    stackcs = cs;
    done = 1;
}

/**/
void
killline(void)
{
    int i = 0;

    if (zmult < 0) {
	zmult = -zmult;
	backwardkillline();
	return;
    }
    while (zmult--) {
	if (line[cs] == '\n')
	    cs++, i++;
	else
	    while (cs != ll && line[cs] != '\n')
		cs++, i++;
    }
    backkill(i, 0);
}

/**/
void
killregion(void)
{
    if (mark > ll)
	mark = ll;
    if (mark > cs)
	forekill(mark - cs, 0);
    else
	backkill(cs - mark, 1);
}

/**/
void
copyregionaskill(void)
{
    if (mark > ll)
	mark = ll;
    if (mark > cs)
	cut(cs, mark - cs, 0);
    else
	cut(mark, cs - mark, 1);
}

static int kct, yankb, yanke;

/**/
void
yank(void)
{
    Cutbuffer buf = &cutbuf;

    if (zmult < 0)
	return;
    if (gotvibufspec)
	buf = &vibuf[vibufspec];
    if (!buf->buf) {
	feep();
	return;
    }
    mark = cs;
    yankb = cs;
    while (zmult--) {
	kct = kringnum;
	spaceinline(buf->len);
	memcpy((char *)line + cs, buf->buf, buf->len);
	cs += buf->len;
	yanke = cs;
    }
}

/**/
void
yankpop(void)
{
    int cc;

    if (!(lastcmd & ZLE_YANK) || !kring[kct].buf) {
	feep();
	return;
    }
    cs = yankb;
    foredel(yanke - yankb);
    cc = kring[kct].len;
    spaceinline(cc);
    memcpy((char *)line + cs, kring[kct].buf, cc);
    cs += cc;
    yanke = cs;
    kct = (kct + KRINGCT - 1) % KRINGCT;
}

/**/
void
overwritemode(void)
{
    insmode ^= 1;
}

/**/
void
undefinedkey(void)
{
    feep();
}

/**/
void
quotedinsert(void)
{
#ifndef HAS_TIO
    struct sgttyb sob;

    sob = shttyinfo.sgttyb;
    sob.sg_flags = (sob.sg_flags | RAW) & ~ECHO;
    ioctl(SHTTY, TIOCSETN, &sob);
#endif
    c = getkey(0);
#ifndef HAS_TIO
    setterm();
#endif
    if (c < 0)
	feep();
    else
	selfinsert();
}

/**/
void
digitargument(void)
{
    int sign = (zmult < 0) ? -1 : 1;

    if (!(lastcmd & ZLE_DIGIT))
	zmult = 0;
    zmult = zmult * 10 + sign * (c & 0xf);
    gotmult = 1;
}

/**/
void
negargument(void)
{
    if (lastcmd & (ZLE_NEGARG | ZLE_DIGIT))
	feep();
    zmult = -1;
    gotmult = 1;
}

/**/
void
universalargument(void)
{
    if (!(lastcmd & ZLE_ARG))
	zmult = 4;
    else
	zmult *= 4;
    gotmult = 1;
}

/**/
void
copyprevword(void)
{
    int len, t0;

    for (t0 = cs - 1; t0 >= 0; t0--)
	if (iword(line[t0]))
	    break;
    for (; t0 >= 0; t0--)
	if (!iword(line[t0]))
	    break;
    if (t0)
	t0++;
    len = cs - t0;
    spaceinline(len);
    memcpy((char *)&line[cs], (char *)&line[t0], len);
    cs += len;
}

/**/
void
sendbreak(void)
{
    errflag = 1;
}

/**/
void
undo(void)
{
    char *s;
    struct undoent *ue;

    ue = undos + undoct;
    if (!ue->change) {
	feep();
	return;
    }
    line[ll] = '\0';
    s = zalloc(ue->suff + 1);
    memcpy(s, (char *)&line[ll - ue->suff], ue->suff);
    sizeline(ll = ue->pref + ue->suff + ue->len);
    memcpy((char *)line + ue->pref, ue->change, ue->len);
    memcpy((char *)line + ue->pref + ue->len, s, ue->suff);
    zfree(s, ue->suff + 1);
    ue->change = NULL;
    undoct = (undoct + UNDOCT - 1) % UNDOCT;
    cs = ue->cs;
}

/**/
void
quoteregion(void)
{
    char *str;
    size_t len;

    if (mark > ll)
	mark = ll;
    if (mark < cs) {
	int tmp = mark;
	mark = cs;
	cs = tmp;
    }
    str = (char *)hcalloc(len = mark - cs);
    memcpy(str, (char *)&line[cs], len);
    foredel(len);
    str = makequote(str, &len);
    spaceinline(len);
    memcpy((char *)&line[cs], str, len);
    mark = cs;
    cs += len;
}

/**/
void
quoteline(void)
{
    char *str;
    size_t len = ll;

    str = makequote((char *)line, &len);
    sizeline(len);
    memcpy(line, str, len);
    cs = ll = len;
}

/**/
char *
makequote(char *str, size_t *len)
{
    int qtct = 0;
    char *l, *ol;
    char *end = str + *len;

    for (l = str; l < end; l++)
	if (*l == '\'')
	    qtct++;
    *len += 2 + qtct*3;
    l = ol = (char *)halloc(*len);
    *l++ = '\'';
    for (; str < end; str++)
	if (*str == '\'') {
	    *l++ = '\'';
	    *l++ = '\\';
	    *l++ = '\'';
	    *l++ = '\'';
	} else
	    *l++ = *str;
    *l++ = '\'';
    return ol;
}

#define NAMLEN 60

/**/
int
executenamedcommand(char *prmt)
{
    int len, cmd, t0, l = strlen(prmt);
    char *ptr, *buf = halloc(l + NAMLEN + 2);
    int *obindtab = bindtab;
    HashTable okeybindtab = keybindtab;

    strcpy(buf, prmt);
    statusline = buf;
    bindtab = mainbindtab;
    keybindtab = mainkeybindtab;
    ptr = buf += l;
    len = 0;
    for (;;) {
	*ptr = '_';
	statusll = l + len + 1;
	refresh();
	if ((cmd = getkeycmd()) < 0 || cmd == z_sendbreak) {
	    statusline = NULL;
	    bindtab = obindtab;
	    keybindtab = okeybindtab;
	    return -1;
	}
	switch (cmd) {
	case z_sendstring:
	    sendstring();
	    break;
	case z_clearscreen:
	    clearscreen();
	    break;
	case z_redisplay:
	    redisplay();
	    break;
	case z_viquotedinsert:
	    *ptr = '^';
	    refresh();
	    c = getkey(0);
	    if(c == EOF || !c || len == NAMLEN)
		feep();
	    else
		*ptr++ = c, len++;
	    break;
	case z_quotedinsert:
	    if((c = getkey(0)) == EOF || !c || len == NAMLEN)
		feep();
	    else
		*ptr++ = c, len++;
	    break;
	case z_backwarddeletechar:
	case z_vibackwarddeletechar:
	    if (len)
		len--, ptr--;
	    break;
	case z_killregion:
	case z_backwardkillword:
	case z_vibackwardkillword:
	    while (len && (len--, *--ptr != '-'));
	    break;
	case z_killwholeline:
	case z_vikillline:
	case z_backwardkillline:
	    len = 0;
	    ptr = buf;
	    break;
	case z_acceptline:
	case z_vicmdmode:
	unambiguous:
	    *ptr = 0;
	    t0 = zlefindfunc(buf);
	    if (t0 != zlecmdtot) {
		statusline = NULL;
		bindtab = obindtab;
		keybindtab = okeybindtab;
		return t0;
	    }
	    /* fall through */
	default:
	    if(cmd == z_selfinsertunmeta) {
		c &= 0x7f;
		if(c == '\r')
		    c = '\n';
		cmd = z_selfinsert;
	    }
	    if (cmd == z_listchoices || cmd == z_deletecharorlist ||
		cmd == z_expandorcomplete || cmd == z_expandorcompleteprefix ||
		cmd == z_completeword || cmd == z_vicmdmode ||
		cmd == z_acceptline || c == ' ' || c == '\t') {
		LinkList cmdll;
		int ambig = 100;

		HEAPALLOC {
		    cmdll = newlinklist();
		    *ptr = 0;
		    for (t0 = 0; t0 != zlecmdtot; t0++) {
			struct zlecmd *zc = ZLEGETCMD(t0);
			if (!(zc->flags & ZLE_DELETED) && zc->name
			    && strpfx(buf, zc->name)) {
			    int xx;

			    addlinknode(cmdll, zc->name);
			    xx = pfxlen(peekfirst(cmdll), zc->name);
			    if (xx < ambig)
				ambig = xx;
			}
		    }
		} LASTALLOC;
		if (empty(cmdll))
		    feep();
		else if (cmd == z_listchoices || cmd == z_deletecharorlist) {
		    int zmultsav = zmult;
		    *ptr = '_';
		    statusll = l + len + 1;
		    zmult = 1;
		    listlist(cmdll);
		    zmult = zmultsav;
		} else if (!nextnode(firstnode(cmdll))) {
		    strcpy(ptr = buf, peekfirst(cmdll));
		    ptr += (len = strlen(ptr));
		    if(cmd == z_acceptline || cmd == z_vicmdmode)
			goto unambiguous;
		} else {
		    strcpy(buf, peekfirst(cmdll));
		    ptr = buf + ambig;
		    *ptr = '_';
		    if (isset(AUTOLIST) &&
			!(isset(LISTAMBIGUOUS) && ambig > len)) {
			int zmultsav = zmult;
			if (isset(LISTBEEP))
			    feep();
			statusll = l + ambig + 1;
			zmult = 1;
			listlist(cmdll);
			zmult = zmultsav;
		    }
		    len = ambig;
		}
	    } else {
		if (len == NAMLEN || icntrl(c) || cmd != z_selfinsert)
		    feep();
		else
		    *ptr++ = c, len++;
	    }
	}
    }
}

/**/
int
addzlefunction(char *name, bindfunc *func, int flags)
{
    struct zlecmd *zc = NULL;
    int slot, addsize = zlecmdtot - ZLECMDCOUNT;

    /* Check for a clash */
    if (zlefindfunc(name) != zlecmdtot)
	return -1;

    /* First try and find a free slot. */
    for (slot = 0; slot < addsize; slot++)
	if (zlecmdadd[slot].flags & ZLE_DELETED) {
	    zc = zlecmdadd + slot;
	    break;
	}

    if (zc) {
	slot += ZLECMDCOUNT;
    } else {
	if (zlecmdtot == ZLECMDCOUNT + zlecmdaddalloc) {
	    /* this includes the case where zlecmdaddalloc is 0 */
	    zlecmdaddalloc += ZLEADDTOT;
	    zlecmdadd = (struct zlecmd *)
		zrealloc(zlecmdadd, zlecmdaddalloc*sizeof(struct zlecmd));
	}
	slot = zlecmdtot++;
	zc = zlecmdadd + (slot-ZLECMDCOUNT);
    }
    zc->name = name;
    zc->func = func;
    zc->flags = flags;

    return slot;
}

#ifdef DYNAMIC

/**/
void
deletezlefunction(int zcn)
{
    struct zlecmd *zc;

    if (zcn < ZLECMDCOUNT || zcn >= zlecmdtot)
	return;
    /* Remove all key bindings to zcn, wherever */
    unbindzlefunc(zcn, 0);
    zcn -= ZLECMDCOUNT;

    zc = zlecmdadd + zcn;
    zc->name = "";
    zc->func = feep;
    zc->flags = ZLE_DELETED;
}
#endif

/**/
void
freecompcond(void *a)
{
    Compcond cc = (Compcond) a;
    Compcond and, or, c;
    int n;

    for (c = cc; c; c = or) {
	or = c->or;
	for (; c; c = and) {
	    and = c->and;
	    if (c->type == CCT_POS ||
		c->type == CCT_NUMWORDS) {
		free(c->u.r.a);
		free(c->u.r.b);
	    } else if (c->type == CCT_CURSUF ||
		       c->type == CCT_CURPRE) {
		for (n = 0; n < c->n; n++)
		    if (c->u.s.s[n])
			zsfree(c->u.s.s[n]);
		free(c->u.s.s);
	    } else if (c->type == CCT_RANGESTR ||
		       c->type == CCT_RANGEPAT) {
		for (n = 0; n < c->n; n++)
		    if (c->u.l.a[n])
			zsfree(c->u.l.a[n]);
		free(c->u.l.a);
		for (n = 0; n < c->n; n++)
		    if (c->u.l.b[n])
			zsfree(c->u.l.b[n]);
		free(c->u.l.b);
	    } else {
		for (n = 0; n < c->n; n++)
		    if (c->u.s.s[n])
			zsfree(c->u.s.s[n]);
		free(c->u.s.p);
		free(c->u.s.s);
	    }
	    zfree(c, sizeof(struct compcond));
	}
    }
}
