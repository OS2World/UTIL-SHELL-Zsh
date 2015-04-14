/*
 * $Id: example.c,v 3.1.0.7 1996/12/05 03:59:45 hzoli Exp $
 *
 * example.c - an example module for zsh
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 1996 Zoltán Hidvégi
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * In no event shall Zoltán Hidvégi or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Zoltán Hidvégi and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Zoltán Hidvégi and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Zoltán Hidvégi and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */

#ifndef MODULE
#define mod_boot mod_boot_example
#endif

#include "zsh.h"
#include "example.pro"

/**/
int
bin_example(char *nam, char **args, char *ops, int func)
{
    unsigned char c;

    printf("Options: ");
    for (c = 32; ++c < 128;)
	if (ops[c])
	    putchar(c);
    printf("\nArguments:");
    for (; *args; args++) {
	putchar(' ');
	fputs(*args, stdout);
    }
    printf("\nName: %s\n", nam);
    return 0;
}

/*
 * boot_example is executed when the module is loaded.
 * addbuiltin() can be used to add a new builtin.  It has six arguments:
 *  - the name of the new builtin
 *  - BINF_* flags (see zsh.h).  Normally it is 0.
 *  - the handler function of the builtin
 *  - minimum number of arguments
 *  - maximum number of argument (-1 means unlimited)
 *  - possible option leters
 */

/**/
int
boot_example(Module m)
{
    if (addbuiltin("example", 0, bin_example, 0, -1, "flags")) {
	zwarnnam(m->nam, "name clash when adding builtin `example'", NULL, 0);
	return -1;
    }
    return 0;
}

#ifdef MODULE

/**/
int
cleanup_example(Module m)
{
    return deletebuiltin("example");
}
#endif
