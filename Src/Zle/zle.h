/*
 * $Id: zle.h,v 3.1.0.4 1996/12/05 03:59:45 hzoli Exp $
 *
 * zle.h - header file for line editor
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

#ifndef _ZLE_H
#define _ZLE_H

#include "zsh.h"

#ifdef ZLEGLOBALS
#define ZLEXTERN
#else
#define ZLEXTERN extern
#endif

/* size of line buffer */
ZLEXTERN int linesz;

/* location of mark */
ZLEXTERN int mark;

/* last character pressed */
ZLEXTERN int c;

/* the z_ binding id for this key */
ZLEXTERN int bindk;

/* command argument */
ZLEXTERN int zmult;

/* buffer specified with "x */
ZLEXTERN int vibufspec;
/* is the current vi buffer specification overwriting or appending? */
ZLEXTERN int vibufappend;
/* insert mode/overwrite mode flag */
ZLEXTERN int insmode;

#ifdef HAVE_SELECT
/* cost of last update */
ZLEXTERN int cost;

/* Terminal baud rate (from the BAUD parameter) */
ZLEXTERN int baud;
#endif

/* number of lines displayed */
ZLEXTERN int nlnct;

/* Most lines of the buffer we've shown at once with the current list *
 * showing.  == 0 if there is no list.  == -1 if a new list has just  *
 * been put on the screen.  == -2 if refresh() needs to put up a new  *
 * list.                                                              */
ZLEXTERN int showinglist;

/* flags associated with last command */
ZLEXTERN int lastcmd;

/* column position before last LINEMOVE movement */
ZLEXTERN int lastcol;

/* != 0 if we're getting a vi range */
ZLEXTERN int virangeflag;

/* kludge to get cw and dw to work right */
ZLEXTERN int wordflag;

/* != 0 if we're killing lines into a buffer, vi-style */
ZLEXTERN int vilinerange;

/* last named command done */
ZLEXTERN int lastnamed;

/* != 0 if we're done editing */
ZLEXTERN int done;

/* current history line number */
ZLEXTERN int histline;

/* the last line in the history (the current one), metafied */
ZLEXTERN char *curhistline;

/* the status line, and its length */
ZLEXTERN char *statusline;
ZLEXTERN int statusll;

/* !=0 if a complete added a suffix at the end of a completion */
ZLEXTERN int addedsuffix;

/* 1 if we expect special keys after completing a parameter name */
ZLEXTERN int complexpect;

/* The current history line and cursor position for the top line *
 * on the buffer stack.                                          */
ZLEXTERN int stackhist, stackcs;

/* != 0 if we are in the middle of a menu completion */
ZLEXTERN int menucmp;

/* != 0 if we are making undo records */
ZLEXTERN int undoing;

/* last vi change buffer, for vi change repetition */
ZLEXTERN int vichgbufsz, vichgbufptr, vichgflag;
ZLEXTERN char *vichgbuf;

/* point where vi insert mode was last entered */
ZLEXTERN int viinsbegin;

/* != 0 if an argument has been given for this command */
ZLEXTERN int gotmult;

/* != 0 if a kill buffer has been given for this command */
ZLEXTERN int gotvibufspec;

struct compcond {
    struct compcond *and, *or;	/* the next or'ed/and'ed conditions    */
    int type;			/* the type (CCT_*)                    */
    int n;			/* the array length                    */
    union {			/* these structs hold the data used to */
	struct {		/* test this condition                 */
	    int *a, *b;		/* CCT_POS, CCT_NUMWORDS               */
	}
	r;
	struct {		/* CCT_CURSTR, CCT_CURPAT,... */
	    int *p;
	    char **s;
	}
	s;
	struct {		/* CCT_RANGESTR,... */
	    char **a, **b;
	}
	l;
    }
    u;
};

#define CCT_UNUSED     0
#define CCT_POS        1
#define CCT_CURSTR     2
#define CCT_CURPAT     3
#define CCT_WORDSTR    4
#define CCT_WORDPAT    5
#define CCT_CURSUF     6
#define CCT_CURPRE     7
#define CCT_CURSUB     8
#define CCT_CURSUBC    9
#define CCT_NUMWORDS  10
#define CCT_RANGESTR  11
#define CCT_RANGEPAT  12

/* Contains the real description for compctls */

struct compctl {
    int refc;			/* reference count                         */
    struct compctl *next;	/* next compctl for -x                     */
    unsigned long mask;		/* mask of things to complete (CC_*)       */
    char *keyvar;		/* for -k (variable)                       */
    char *glob;			/* for -g (globbing)                       */
    char *str;			/* for -s (expansion)                      */
    char *func;			/* for -K (function)                       */
    char *explain;		/* for -X (explanation)                    */
    char *prefix, *suffix;	/* for -P and -S (prefix, suffix)          */
    char *subcmd;		/* for -l (command name to use)            */
    char *hpat;			/* for -H (history pattern)                */
    int hnum;			/* for -H (number of events to search)     */
    struct compctl *ext;	/* for -x (first of the compctls after -x) */
    struct compcond *cond;	/* for -x (condition for this compctl)     */
    struct compctl *xor;	/* for + (next of the xor'ed compctls)     */
};

/* objects to complete */
#define CC_FILES	(1<<0)
#define CC_COMMPATH	(1<<1)
#define CC_REMOVE	(1<<2)
#define CC_OPTIONS	(1<<3)
#define CC_VARS		(1<<4)
#define CC_BINDINGS	(1<<5)
#define CC_ARRAYS	(1<<6)
#define CC_INTVARS	(1<<7)
#define CC_SHFUNCS	(1<<8)
#define CC_PARAMS	(1<<9)
#define CC_ENVVARS	(1<<10)
#define CC_JOBS		(1<<11)
#define CC_RUNNING	(1<<12)
#define CC_STOPPED	(1<<13)
#define CC_BUILTINS	(1<<14)
#define CC_ALREG	(1<<15)
#define CC_ALGLOB	(1<<16)
#define CC_USERS	(1<<17)
#define CC_DISCMDS	(1<<18)
#define CC_EXCMDS	(1<<19)
#define CC_SCALARS	(1<<20)
#define CC_READONLYS	(1<<21)
#define CC_SPECIALS	(1<<22)
#define CC_DELETE	(1<<23)
#define CC_NAMED	(1<<24)
#define CC_QUOTEFLAG	(1<<25)
#define CC_EXTCMDS	(1<<26)
#define CC_RESWDS	(1<<27)

#define CC_RESERVED	(1<<31)

/* default completion infos */
 
ZLEXTERN struct compctl cc_compos, cc_default, cc_first, cc_dummy;
 
typedef void bindfunc _((void));
typedef bindfunc *F;

struct key {
    struct hashnode *next;
    char *nam;			/* hash data */
    int flags;			/* CURRENTLY UNUSED */
    int func;			/* function code for this key */
    char *str;			/* string corresponding to this key,
			   	 * if func = z_sendstring */
    int len;			/* length of string */
    int prefixct;		/* number of strings for which this is a prefix */
};

struct zlecmd {
    char *name;			/* name of function */
    F func;			/* handler function */
    int flags;
};

/* undo event */

struct undoent {
    int pref;			/* number of initial chars unchanged */
    int suff;			/* number of trailing chars unchanged */
    int len;			/* length of changed chars */
    int cs;			/* cursor pos before change */
    char *change;		/* NOT null terminated */
};

#define UNDOCT 64

ZLEXTERN struct undoent undos[UNDOCT];

/* the line before last mod (for undo purposes) */
ZLEXTERN unsigned char *lastline;

ZLEXTERN int undoct, lastcs, lastll;

ZLEXTERN char *visrchstr;
ZLEXTERN int visrchsense;

#define ZLE_MOVEMENT	(1<<0)
#define ZLE_MENUCMP	(1<<1)
#define ZLE_UNDO	(1<<2)
#define ZLE_YANK	(1<<3)
#define ZLE_LINEMOVE	(1<<4)
#define ZLE_ARG		(1<<5)
#define ZLE_KILL	(1<<6)
#define ZLE_HISTSEARCH	(1<<7)
#define ZLE_NEGARG	(1<<8)
#define ZLE_INSERT	(1<<9)
#define ZLE_DELETE	(1<<10)
#define ZLE_DIGIT	(1<<11)
#define ZLE_DELETED	(1<<12)

typedef struct key *Key;

extern int emacsbind[], metabind[], viinsbind[], vicmdbind[];
ZLEXTERN int *bindtab, *mainbindtab;
ZLEXTERN int altbindtab[256];

/* Cut/kill buffer type.  The buffer itself is purely binary data, *
 * not NUL-terminated.  len is a length count.  flags uses the     *
 * CUTBUFFER_* constants defined below.                            */

struct cutbuffer {
    char *buf;
    size_t len;
    char flags;
};

typedef struct cutbuffer *Cutbuffer;

#define CUTBUFFER_LINE 1   /* for vi: buffer contains whole lines of data */

/* Primary cut buffer */

ZLEXTERN struct cutbuffer cutbuf;

/* Emacs-style kill buffer ring */

#define KRINGCT 8
ZLEXTERN struct cutbuffer kring[KRINGCT];
ZLEXTERN int kringnum;

/* Vi named cut buffers.  0-25 are the named buffers "a to "z, and *
 * 26-34 are the numbered buffer stack "1 to "9.                   */

ZLEXTERN struct cutbuffer vibuf[35];

/* ZLE command table indices */

enum {
    z_acceptandhold,
    z_acceptandinfernexthistory,
    z_acceptandmenucomplete,
    z_acceptline,
    z_acceptlineanddownhistory,
    z_backwardchar,
    z_backwarddeletechar,
    z_backwarddeleteword,
    z_backwardkillline,
    z_backwardkillword,
    z_backwardword,
    z_beginningofbufferorhistory,
    z_beginningofhistory,
    z_beginningofline,
    z_beginningoflinehist,
    z_capitalizeword,
    z_clearscreen,
    z_completeword,
    z_copyprevword,
    z_copyregionaskill,
    z_deletechar,
    z_deletecharorlist,
    z_deleteword,
    z_describekeybriefly,
    z_digitargument,
    z_downcaseword,
    z_downhistory,
    z_downlineorhistory,
    z_downlineorsearch,
    z_emacsbackwardword,
    z_emacsforwardword,
    z_endofbufferorhistory,
    z_endofhistory,
    z_endofline,
    z_endoflinehist,
    z_exchangepointandmark,
    z_executelastnamedcmd,
    z_executenamedcmd,
    z_expandcmdpath,
    z_expandhistory,
    z_expandorcomplete,
    z_expandorcompleteprefix,
    z_expandword,
    z_forwardchar,
    z_forwardword,
    z_getline,
    z_gosmacstransposechars,
    z_historybeginningsearchbackward,
    z_historybeginningsearchforward,
    z_historyincrementalsearchbackward,
    z_historyincrementalsearchforward,
    z_historysearchbackward,
    z_historysearchforward,
    z_infernexthistory,
    z_insertlastword,
    z_killbuffer,
    z_killline,
    z_killregion,
    z_killwholeline,
    z_killword,
    z_listchoices,
    z_listexpand,
    z_magicspace,
    z_menucomplete,
    z_menuexpandorcomplete,
    z_negargument,
    z_overwritemode,
    z_poundinsert,
    z_prefix,
    z_pushinput,
    z_pushline,
    z_pushlineoredit,
    z_quotedinsert,
    z_quoteline,
    z_quoteregion,
    z_redisplay,
    z_reversemenucomplete,
    z_runhelp,
    z_selfinsert,
    z_selfinsertunmeta,
    z_sendbreak,
    z_sendstring,
    z_setmarkcommand,
    z_spellword,
    z_transposechars,
    z_transposewords,
    z_undefinedkey,
    z_undo,
    z_universalargument,
    z_upcaseword,
    z_uphistory,
    z_uplineorhistory,
    z_uplineorsearch,
    z_viaddeol,
    z_viaddnext,
    z_vibackwardblankword,
    z_vibackwardchar,
    z_vibackwarddeletechar,
    z_vibackwardkillword,
    z_vibackwardword,
    z_vibeginningofline,
    z_vicapslockpanic,
    z_vichange,
    z_vichangeeol,
    z_vichangewholeline,
    z_vicmdmode,
    z_videlete,
    z_videletechar,
    z_vidigitorbeginningofline,
    z_vidownlineorhistory,
    z_viendofline,
    z_vifetchhistory,
    z_vifindnextchar,
    z_vifindnextcharskip,
    z_vifindprevchar,
    z_vifindprevcharskip,
    z_vifirstnonblank,
    z_viforwardblankword,
    z_viforwardblankwordend,
    z_viforwardchar,
    z_viforwardword,
    z_viforwardwordend,
    z_vigotocolumn,
    z_vigotomark,
    z_vigotomarkline,
    z_vihistorysearchbackward,
    z_vihistorysearchforward,
    z_viindent,
    z_viinsert,
    z_viinsertbol,
    z_vijoin,
    z_vikilleol,
    z_vikillline,
    z_vimatchbracket,
    z_viopenlineabove,
    z_viopenlinebelow,
    z_vioperswapcases,
    z_vipoundinsert,
    z_viputafter,
    z_viputbefore,
    z_viquotedinsert,
    z_virepeatchange,
    z_virepeatfind,
    z_virepeatsearch,
    z_vireplace,
    z_vireplacechars,
    z_virevrepeatfind,
    z_virevrepeatsearch,
    z_visetbuffer,
    z_visetmark,
    z_visubstitute,
    z_viswapcase,
    z_viundochange,
    z_viunindent,
    z_viuplineorhistory,
    z_viyank,
    z_viyankeol,
    z_viyankwholeline,
    z_whereis,
    z_whichcommand,
    z_yank,
    z_yankpop,
    ZLECMDCOUNT
};

extern struct zlecmd zlecmds[];

/* Array of commands added from modules */

ZLEXTERN struct zlecmd *zlecmdadd;

/* Size allocated for zlecmdadd */

ZLEXTERN int zlecmdaddalloc;

/* Total command count, predefined and added */

ZLEXTERN int zlecmdtot;

/* Access the appropriate command:  in the zlecmds array, or in zlecmdadd.  *
 * Commands up to, but not including, ZLECMDCOUNT are in the compiled array *
 * zlecmds.  Anything higher is in the malloc'd array zlecmdadd, so we need *
 * to subtract ZLECMDCOUNT to get the offset.                               *
 * ZLEGETCMD expands into an expression of the type (struct zlecmd *).      */ 

#define ZLEGETCMD(X)  ((X) >= ZLECMDCOUNT ? &zlecmdadd[(X)-ZLECMDCOUNT] \
		       : &zlecmds[X])

/* How many new commands to reserve space for at a time */
#define ZLEADDTOT 8

/* hash table for completion info for commands */
 
ZLEXTERN HashTable compctltab;

/* hash table for emacs multi-character bindings */

ZLEXTERN HashTable emkeybindtab;

/* hash table for vi multi-character bindings */

ZLEXTERN HashTable vikeybindtab;

/* hash table for vi command mode multi-character bindings */

ZLEXTERN HashTable altkeybindtab;

/* currently selected key binding hash tables */

ZLEXTERN HashTable keybindtab, mainkeybindtab;

/* prototypes */

#include "compctl.pro"
#include "zle_hist.pro"
#include "zle_main.pro"
#include "zle_misc.pro"
#include "zle_move.pro"
#include "zle_refresh.pro"
#include "zle_tricky.pro"
#include "zle_utils.pro"
#include "zle_vi.pro"
#include "zle_word.pro"

#endif /*!_ZLE_H*/
