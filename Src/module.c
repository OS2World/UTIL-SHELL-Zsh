/*
 * $Id: module.c,v 3.1.0.15 1996/12/21 01:32:44 hzoli Exp $
 *
 * module.c - deal with dynamic modules
 *
 * This file is part of zsh, the Z shell.
 *
 * Copyright (c) 1996 Zoltán Hidvégi
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
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

#include "zsh.h"

/* addbuiltin() can be used to add a new builtin.  It has six arguments: *
 *  - the name of the new builtin                                        *
 *  - BINF_* flags (see zsh.h).  Normally it is 0.                       *
 *  - the handler function of the builtin                                *
 *  - minimum number of arguments                                        *
 *  - maximum number of argument (-1 means unlimited)                    *
 *  - possible option leters                                             *
 * It returns zero on succes -1 on failure                               */

/**/
int
addbuiltin(char *nam, int flags, HandlerFunc hfunc, int minargs, int maxargs, char *optstr)
{
    Builtin bn;

    bn = (Builtin) builtintab->getnode2(builtintab, nam);
    if (bn && bn->handlerfunc)
	return -1;
    if (!bn) {
	bn = zcalloc(sizeof(*bn));
	bn->nam = ztrdup(nam);
	PERMALLOC {
	    builtintab->addnode(builtintab, bn->nam, bn);
	} LASTALLOC;
    }
    bn->flags = flags;
    bn->handlerfunc = hfunc;
    bn->minargs = minargs;
    bn->maxargs = maxargs;
    zsfree(bn->optstr);
    bn->optstr = ztrdup(optstr);
    return 0;
}

#ifdef DYNAMIC

/* Remove the builtin added previously by addbuiltin().  Returns *
 * zero on succes and -1 if there is no builtin with that name.  *
 * Attempt to delete a builtin which is not defined by           *
 * addbuiltin() will fail badly with unpredictable results.      */

/**/
int
deletebuiltin(char *nam)
{
    Builtin bn;

    bn = (Builtin) builtintab->removenode(builtintab, nam);
    if (!bn)
	return -1;
    zsfree(bn->nam);
    zsfree(bn->optstr);
    zfree(bn, sizeof(struct builtin));
    return 0;
}

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#else
# include <sys/types.h>
# include <nlist.h>
# include <link.h>
#endif
#ifndef RTLD_LAZY
# define RTLD_LAZY 1
#endif
#ifndef RTLD_GLOBAL
# define RTLD_GLOBAL 0
#endif
#ifndef HAVE_DLCLOSE
# define dlclose(X) ((X), 0)
#endif

typedef int (*Module_func) _((Module));

/**/
void *
try_load_module(char const *name)
{
    char buf[PATH_MAX + 1];
    char **pp;
    void *ret = NULL;
    int l;

    if (strchr(name, '/')) {
	ret = dlopen(unmeta(name), RTLD_LAZY | RTLD_GLOBAL);
	if (ret || 
	    unset(PATHDIRS) ||
	    (*name == '/') ||
	    (*name == '.' && name[1] == '/') ||
	    (*name == '.' && name[1] == '.' && name[2] == '/'))
	    return ret;
    }

    l = strlen(name) + 1;
    for (pp = module_path; !ret && *pp; pp++) {
	if (l + (**pp ? strlen(*pp) : 1) > PATH_MAX)
	    continue;
	sprintf(buf, "%s/%s", **pp ? *pp : ".", name);
	ret = dlopen(unmeta(buf), RTLD_LAZY | RTLD_GLOBAL);
    }

    return ret;
}

/**/
void *
do_load_module(char const *name)
{
    void *ret = NULL;
    char buf[PATH_MAX + 1];

    if (strlen(name) + strlen(DL_EXT) < PATH_MAX) {
	sprintf(buf, "%s.%s", name, DL_EXT);
	ret = try_load_module(buf);
    }
    if (!ret)
	ret = try_load_module(name);
    if (!ret) {
	int waserr = errflag;
	zerr("failed to load module: %s", name, 0);
	errflag = waserr;
    }
    return ret;
}

/**/
LinkNode
find_module(const char *name)
{
    Module m;
    LinkNode node;

    for (node = firstnode(modules); node; incnode(node)) {
	m = (Module) getdata(node);
	if (!strcmp(m->nam, name))
	    return node;
    }
    return NULL;
}

/**/
int
init_module(Module m)
{
    char *s, *t;
    char buf[PATH_MAX + 1];
    Module_func fn;

    s = strrchr(m->nam, '/');
    if (s)
	s = dupstring(++s);
    else
	s = m->nam;
    if ((t = strrchr(s, '.')))
	*t = '\0';
    if (strlen(s) + 6 > PATH_MAX)
	return 1;
#ifdef DLSYM_NEEDS_UNDERSCORE
    sprintf(buf, "_boot_%s", s);
#else
    sprintf(buf, "boot_%s", s);
#endif
    fn = (Module_func) dlsym(m->handle, buf);
    return fn ? fn(m) : 1;
}

/**/
Module
load_module(char const *name)
{
    Module m;
    void *handle;
    LinkNode node, n;

    if (!(node = find_module(name))) {
	if (!(handle = do_load_module(name)))
	    return NULL;
	m = zcalloc(sizeof(*m));
	m->nam = ztrdup(name);
	m->handle = handle;
	if (init_module(m)) {
	    dlclose(handle);
	    zsfree(m->nam);
	    zfree(m, sizeof(*m));
	    return NULL;
	}
	PERMALLOC {
	    addlinknode(modules, m);
	} LASTALLOC;
	return m;
    }
    m = (Module) getdata(node);
    if (m->handle)
	return m;
    if (m->flags & MOD_BUSY) {
	zerr("circular dependencies for module %s", name, 0);
	return NULL;
    }
    m->flags |= MOD_BUSY;
    for (n = firstnode(m->deps); n; incnode(n))
	if (!load_module((char *) getdata(n))) {
	    m->flags &= ~MOD_BUSY;
	    return NULL;
	}
    m->flags &= ~MOD_BUSY;
    if (!(m->handle = do_load_module(name)))
	return NULL;
    if (init_module(m)) {
	dlclose(m->handle);
	m->handle = NULL;
	return NULL;
    }
    return m;
}

/**/
int
cleanup_module(Module m)
{
    char *s, *t;
    char buf[PATH_MAX + 1];
    Module_func fn;

    s = strrchr(m->nam, '/');
    if (s)
	s = dupstring(++s);
    else
	s = m->nam;
    if ((t = strrchr(s, '.')))
	*t = '\0';
    if (strlen(s) + 9 > PATH_MAX)
	return 1;
#ifdef DLSYM_NEEDS_UNDERSCORE
    sprintf(buf, "_cleanup_%s", s);
#else
    sprintf(buf, "cleanup_%s", s);
#endif
    fn = (Module_func) dlsym(m->handle, buf);
    return fn ? fn(m) : 1;
}

/**/
void
add_dep(char *name, char *from)
{
    LinkNode node;
    Module m;

    PERMALLOC {
	if (!(node = find_module(name))) {
	    m = zcalloc(sizeof(*m));
	    m->nam = ztrdup(name);
	    addlinknode(modules, m);
	} else
	    m = (Module) getdata(node);
	if (!m->deps)
	    m->deps = newlinklist();
	for (node = firstnode(m->deps);
	     node && strcmp((char *) getdata(node), from);
	     incnode(node));
	if (!node)
	    addlinknode(m->deps, ztrdup(from));
    } LASTALLOC;
}

/**/
int
bin_zmodload(char *nam, char **args, char *ops, int func)
{
    LinkNode node;
    Module m = NULL;
    int ret = 0;

    if (!*args) {
	if (ops['u']) {
	    zwarnnam(nam, "which module do you want to unload?", NULL, 0);
	    return 1;
	}
	for (node = firstnode(modules); node; incnode(node)) {
	    m = (Module) getdata(node);
	    if (ops['d']) {
		if (m->deps) {
		    LinkNode n;

		    zputs(m->nam, stdout);
		    putchar(':');
		    for (n = firstnode(m->deps); n; incnode(n)) {
			putchar(' ');
			zputs((char *) getdata(n), stdout);
		    }
		    putchar('\n');
		}
	    } else if (m->handle) {
		zputs(m->nam, stdout);
		putchar('\n');
	    }
	}
	return 0;
    }
    if (ops['d']) {
	char *tnam = *args++;

	if (!*args) {
	    node = find_module(tnam);
	    if (!node)
		return 0;
	    m = (Module) getdata(node);
	    if (m->deps) {
		freelinklist(m->deps, freestr);
		m->deps = NULL;
	    }
	    if (!m->handle) {
		remnode(modules, node);
		zsfree(m->nam);
		zfree(m, sizeof(*m));
	    }
	    return 0;
	}
	while (*args)
	    add_dep(tnam, *args++);
    }
    if (ops['a']) {
	if (args[1] && args[2]) {
	    zwarnnam(nam, "too many arguments for `zmodload -a'", NULL, 0);
	    return 1;
	}
	if (addbuiltin(args[0], 0, NULL, 0, -1, args[1] ? args[1] : args[0])) {
	    zwarnnam(nam, "failed to add builtin %s", args[0], 0);
	    return 1;
	}
	return 0;
    }
    for (; *args; args++) {
	node = find_module(*args);
	if (ops['u']) {
	    if (node) {
		LinkNode mn, dn;

		for (mn = firstnode(modules); mn; incnode(mn)) {
		    m = (Module) getdata(mn);
		    if (m->deps && m->handle)
			for (dn = firstnode(m->deps); dn; incnode(dn))
			    if (!strcmp((char *) getdata(dn), *args)) {
				zwarnnam(nam, "module %s is used by an other module, I cannot remove it.", *args, 0);
				ret++;
				goto cont;
			    }
		}

		m = (Module) getdata(node);
		if (m->handle && cleanup_module(m))
		    ret++;
		else {
		    if (m->handle)
			dlclose(m->handle);
		    remnode(modules, node);
		    zsfree(m->nam);
		    if (m->deps)
			freelinklist(m->deps, freestr);
		    zfree(m, sizeof(*m));
		}
	    } else if (!ops['i']) {
		zwarnnam(nam, "no such module %s", *args, 0);
		ret++;
	    }
	} else if (node && ((Module) getdata(node))->handle) {
	    if (!ops['i']) {
		zwarnnam(nam, "module %s already loaded.", *args, 0);
		ret++;
	    }
	} else if (!load_module(*args))
	    ret++;
      cont:
	;
    }
    return ret;
}

#ifdef ZLE_MODULE
typedef void (*Voidfn) _((void));

static struct symbols {
    char *nam;
    Voidfn *fn;
} zle_syms[] = {
#ifdef DLSYM_NEEDS_UNDERSCORE
    { "_trashzle", &trashzleptr },
    { "_zleread", (Voidfn *) &zlereadptr },
    { "_spaceinline", (Voidfn *) &spaceinlineptr },
    { "_gotword", &gotwordptr },
    { "_refresh", &refreshptr },
    { "_zle_init", NULL }
#else
    { "trashzle", &trashzleptr },
    { "zleread", (Voidfn *) &zlereadptr },
    { "spaceinline", (Voidfn *) &spaceinlineptr },
    { "gotword", &gotwordptr },
    { "refresh", &refreshptr },
    { NULL, NULL }
#endif
};

/**/
int
load_zle_syms(void *handle)
{
    struct symbols *sym;
    Voidfn fn;

    for (sym = zle_syms; sym->nam; sym++) {
	if (!handle || !(fn = (Voidfn) dlsym(handle, sym->nam))) {
	    zerr("unable to load zle: %s", dlerror(), 0);
	    return -1;
	}
	*sym->fn = fn;
    }
    return 0;
}

/**/
void
noop_function(void)
{
    /* do nothing */
}

/**/
void
noop_function_int(int nothing)
{
    /* do nothing */
}

/**/
unsigned char *
load_zleread(char *lp, char *rp)
{
    if (load_module("zle"))
	return zleread(lp, rp);
    else {
	char *pptbuf;
	int pptlen;

	pptbuf = putprompt(lp, &pptlen, NULL, 1);
	write(2, (WRITE_ARG_2_T)pptbuf, pptlen);
	free(pptbuf);
	return (unsigned char *)shingetline();
    }
}
#endif /* ZLE_MODULE */
#endif /* DYNAMIC */
