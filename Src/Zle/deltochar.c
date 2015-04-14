/*
 * $Id: deltochar.c,v 3.1.0.6 1996/12/05 03:59:45 hzoli Exp $
 *
 * deltochar.c - ZLE module implementing Emacs' zap-to-char
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 1996 Peter Stephenson
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Peter Stephenson or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Peter Stephenson and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Peter Stephenson and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Peter Stephenson and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */

#include "zle.h"
#include "deltochar.pro"

static int z_deltochar;

/**/
void
deltochar(void)
{
    int c = getkey(0), dest = cs, ok = 0;;

    if (zmult > 0) {
	while (zmult-- && dest != ll) {
	    while (dest != ll && line[dest] != c)
		dest++;
	    if (dest != ll) {
		dest++;
		if (!zmult) {
		    foredel(dest - cs);
		    ok++;
		}
	    }
	}
    } else {
	/* ignore character cursor is on when scanning backwards */
	if (dest)
	    dest--;
	while (zmult++ && dest != 0) {
	    while (dest != 0 && line[dest] != c)
		dest--;
	    if (line[dest] == c && !zmult) {
		backdel(cs - dest);
		ok++;
	    }
	}
    }
    if (!ok)
	feep();
}

/**/
int
boot_deltochar(Module m)
{
    int newfunc = addzlefunction("delete-to-char", deltochar, ZLE_DELETE);
    if (newfunc > 0) {
	z_deltochar = newfunc;
	return 0;
    }
    zwarnnam(m->nam, "name clash when adding ZLE function `delete-to-char'",
	     NULL, 0);
    return -1;
}

#ifdef MODULE

/**/
int
cleanup_deltochar(Module m)
{
    deletezlefunction(z_deltochar);
    return 0;
}
#endif
