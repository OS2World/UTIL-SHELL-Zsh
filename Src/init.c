/*
 * $Id: init.c,v 3.1.0.10 1996/12/19 21:26:53 hzoli Exp $
 *
 * init.c - main loop and initialization routines
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

#define GLOBALS
#include "zsh.h"

#include "zshpaths.h"

int noexitct = 0;

/* keep executing lists until EOF found */

/**/
void
loop(int toplevel)
{
    List list;
#ifdef DEBUG
    int oasp = toplevel ? 0 : alloc_stackp;
#endif

    pushheap();
    for (;;) {
	freeheap();
	errflag = 0;
	if (interact && isset(SHINSTDIN))
	    preprompt();
	hbegin();		/* init history mech        */
	intr();			/* interrupts on            */
	lexinit();              /* initialize lexical state */
	if (!(list = parse_event())) {	/* if we couldn't parse a list */
	    hend();
	    if (tok == ENDINPUT && !errflag)
		break;
	    continue;
	}
	if (hend()) {
	    int toksav = tok;

	    if (stopmsg)	/* unset 'you have stopped jobs' flag */
		stopmsg--;
	    execlist(list, 0, 0);
	    tok = toksav;
	    if (toplevel)
		noexitct = 0;
	}
	DPUTS(alloc_stackp != oasp, "BUG: alloc_stackp changed in loop()");
	if (ferror(stderr)) {
	    zerr("write error", NULL, 0);
	    clearerr(stderr);
	}
	if (subsh)		/* how'd we get this far in a subshell? */
	    exit(lastval);
	if (((!interact || sourcelevel) && errflag) || retflag)
	    break;
	if (trapreturn) {
	    lastval = trapreturn;
	    trapreturn = 0;
	}
	if (isset(SINGLECOMMAND) && toplevel) {
	    if (sigtrapped[SIGEXIT])
		dotrap(SIGEXIT);
	    exit(lastval);
	}
    }
    popheap();
}

/**/
void
emulate(const char *zsh_name, int fully)
{
    int optno;

    /* Work out the new emulation mode */
    if(!strcmp(zsh_name, "csh"))
	emulation = EMULATE_CSH;
    else if(!strcmp(zsh_name, "ksh"))
	emulation = EMULATE_KSH;
    else if(!strcmp(zsh_name, "sh"))
	emulation = EMULATE_SH;
    else
	emulation = EMULATE_ZSH;

    /* Set options: each non-special option is set according to the *
     * current emulation mode if either it is considered relevant   *
     * to emulation or we are doing a full emulation (as indicated  *
     * by the `fully' parameter).                                   */
    for(optno = OPT_SIZE; --optno; )
	if((fully && !(optns[optno].flags & OPT_SPECIAL)) ||
	    	(optns[optno].flags & OPT_EMULATE))
	    opts[optno] = defset(optno);
}

static char *cmd;

/**/
void
parseargs(char **argv)
{
    char **x;
    int action, optno;
    LinkList paramlist;
    int bourne = (emulation == EMULATE_KSH || emulation == EMULATE_SH);

    hackzero = argzero = *argv++;
    SHIN = 0;

    /* There's a bit of trickery with opts[INTERACTIVE] here.  It starts *
     * at a value of 2 (instead of 1) or 0.  If it is explicitly set on  *
     * the command line, it goes to 1 or 0.  If input is coming from     *
     * somewhere that normally makes the shell non-interactive, we do    *
     * "opts[INTERACTIVE] &= 1", so that only a *default* on state will  *
     * be changed.  At the end of the function, a value of 2 gets        *
     * changed to 1.                                                     */
    opts[INTERACTIVE] = isatty(0) ? 2 : 0;
    opts[SHINSTDIN] = 0;
    opts[SINGLECOMMAND] = 0;

    /* loop through command line options (begins with "-" or "+") */
    while (*argv && (**argv == '-' || **argv == '+')) {
	action = (**argv == '-');
	if(!argv[0][1])
	    *argv = "--";
	while (*++*argv) {
	    /* The pseudo-option `--' signifies the end of options. *
	     * `-b' does too, csh-style, unless we're emulating a   *
	     * Bourne style shell.                                  */
	    if (**argv == '-' || (!bourne && **argv == 'b')) {
		argv++;
		goto doneoptions;
	    }

	    if (**argv == 'c') {         /* -c command */
		if (!*++argv) {
		    zerr("string expected after -c", NULL, 0);
		    exit(1);
		}
		cmd = *argv++;
		opts[INTERACTIVE] &= 1;
		opts[SHINSTDIN] = 0;
		goto doneoptions;
	    } else if (**argv == 'o') {
		if (!*++*argv)
		    argv++;
		if (!*argv) {
		    zerr("string expected after -o", NULL, 0);
		    exit(1);
		}
		if(!(optno = optlookup(*argv)))
		    zerr("no such option: %s", *argv, 0);
		else
		    dosetopt(optno, action, 1);
		break;
	    } else {
	    	if (!(optno = optlookupc(**argv))) {
		    zerr("bad option: -%c", NULL, **argv);
		    exit(1);
		} else
		    dosetopt(optno, action, 1);
	    }
	}
	argv++;
    }
    doneoptions:
    paramlist = newlinklist();
    if (*argv) {
	if (unset(SHINSTDIN)) {
	    argzero = *argv;
	    if (!cmd)
		SHIN = movefd(open(unmeta(argzero), O_RDONLY));
	    if (SHIN == -1) {
		zerr("can't open input file: %s", argzero, 0);
		exit(1);
	    }
	    opts[INTERACTIVE] &= 1;
	    argv++;
	}
	while (*argv)
	    addlinknode(paramlist, ztrdup(*argv++));
    } else
	opts[SHINSTDIN] = 1;
    if(isset(SINGLECOMMAND))
	opts[INTERACTIVE] &= 1;
    opts[INTERACTIVE] = !!opts[INTERACTIVE];
    pparams = x = (char **) zcalloc((countlinknodes(paramlist) + 1) * sizeof(char *));

    while ((*x++ = (char *)getlinknode(paramlist)));
    free(paramlist);
    argzero = ztrdup(argzero);
}

#ifdef __EMX__
#undef init_io
#endif

/**/
void
init_io(void)
{
    long ttpgrp;
    static char outbuf[BUFSIZ], errbuf[BUFSIZ];

#ifdef RSH_BUG_WORKAROUND
    int i;
#endif

/* stdout, stderr fully buffered */
#ifdef _IOFBF
    setvbuf(stdout, outbuf, _IOFBF, BUFSIZ);
    setvbuf(stderr, errbuf, _IOFBF, BUFSIZ);
#else
    setbuffer(stdout, outbuf, BUFSIZ);
    setbuffer(stderr, errbuf, BUFSIZ);
#endif

/* This works around a bug in some versions of in.rshd. *
 * Currently this is not defined by default.            */
#ifdef RSH_BUG_WORKAROUND
    if (cmd) {
	for (i = 3; i < 10; i++)
	    close(i);
    }
#endif

    if (shout) {
	fclose(shout);
	shout = 0;
    }
    if (SHTTY != -1) {
	zclose(SHTTY);
	SHTTY = -1;
    }

    /* Make sure the tty is opened read/write. */
    if (isatty(0)) {
	zsfree(ttystrname);
	if ((ttystrname = ztrdup(ttyname(0))))
	    SHTTY = movefd(open(ttystrname, O_RDWR));
    }
    if (SHTTY == -1 && (SHTTY = movefd(open("/dev/tty", O_RDWR))) != -1) {
	zsfree(ttystrname);
	ttystrname = ztrdup("/dev/tty");
    }
    if (SHTTY == -1) {
	zsfree(ttystrname);
	ttystrname = ztrdup("");
    }

    /* We will only use zle if shell is interactive, *
     * SHTTY != -1, and shout != 0                   */
    if (interact && SHTTY != -1) {
	init_shout();
	if(!shout)
	    opts[USEZLE] = 0;
    } else
	opts[USEZLE] = 0;

#ifdef JOB_CONTROL
    /* If interactive, make the shell the foreground process */
    if (opts[MONITOR] && interact && (SHTTY != -1)) {
	attachtty(GETPGRP());
	if ((mypgrp = GETPGRP()) > 0) {
	    while ((ttpgrp = gettygrp()) != -1 && ttpgrp != mypgrp) {
		sleep(1);
		mypgrp = GETPGRP();
		if (mypgrp == gettygrp())
		    break;
#ifndef __EMX__
		killpg(mypgrp, SIGTTIN);
#endif
		mypgrp = GETPGRP();
	    }
	} else
	    opts[MONITOR] = 0;
    } else
	opts[MONITOR] = 0;
#else
    opts[MONITOR] = 0;
#endif
}

/**/
void
init_shout(void)
{
    static char shoutbuf[BUFSIZ];
#if defined(JOB_CONTROL) && defined(TIOCSETD) && defined(NTTYDISC)
    int ldisc = NTTYDISC;

    ioctl(SHTTY, TIOCSETD, (char *)&ldisc);
#endif

    /* Associate terminal file descriptor with a FILE pointer */
    shout = fdopen(SHTTY, "w");
#ifdef _IOFBF
    setvbuf(shout, shoutbuf, _IOFBF, BUFSIZ);
#endif
  
    gettyinfo(&shttyinfo);	/* get tty state */
#if defined(__sgi)
    if (shttyinfo.tio.c_cc[VSWTCH] <= 0)	/* hack for irises */
	shttyinfo.tio.c_cc[VSWTCH] = CSWTCH;
#endif
}

/* Initialise termcap */

/**/
int
init_term(void)
{
#ifndef TGETENT_ACCEPTS_NULL
    static char termbuf[2048];	/* the termcap buffer */
#endif

    if (!*term)
	return termok = TERM_BAD;

    /* unset zle if using zsh under emacs */
    if (!strcmp(term, "emacs"))
	opts[USEZLE] = 0;

#ifdef TGETENT_ACCEPTS_NULL
    /* If possible, we let tgetent allocate its own termcap buffer */
    if (tgetent(NULL, term) != 1) {
#else
    if (tgetent(termbuf, term) != 1) {
#endif

	if (isset(INTERACTIVE))
	    zerr("can't find termcap info for %s", term, 0);
	errflag = 0;
	return termok = TERM_BAD;
    } else {
	char tbuf[1024], *pp;
	int t0;

	termok = TERM_OK;
	for (t0 = 0; t0 != TC_COUNT; t0++) {
	    pp = tbuf;
	    zsfree(tcstr[t0]);
	/* AIX tgetstr() ignores second argument */
	    if (!(pp = tgetstr(tccapnams[t0], &pp)))
		tcstr[t0] = NULL, tclen[t0] = 0;
	    else {
		tclen[t0] = strlen(pp);
		tcstr[t0] = (char *) zalloc(tclen[t0] + 1);
		memcpy(tcstr[t0], pp, tclen[t0] + 1);
	    }
	}

	/* check whether terminal has automargin (wraparound) capability */
	hasam = tgetflag("am");

	/* if there's no termcap entry for cursor up, use single line mode: *
	 * this is flagged by termok which is examined in zle_refresh.c     *
	 */
	if (!tccan(TCUP)) {
		tcstr[TCUP] = NULL;
		termok = TERM_NOUP;
	}

	/* if there's no termcap entry for cursor left, use \b. */
	if (!tccan(TCLEFT)) {
	    tcstr[TCLEFT] = ztrdup("\b");
	    tclen[TCLEFT] = 1;
	}

	/* if the termcap entry for down is \n, don't use it. */
	if (tccan(TCDOWN) && tcstr[TCDOWN][0] == '\n') {
	    tclen[TCDOWN] = 0;
	    zsfree(tcstr[TCDOWN]);
	    tcstr[TCDOWN] = NULL;
	}

	/* if there's no termcap entry for clear, use ^L. */
	if (!tccan(TCCLEARSCREEN)) {
	    tcstr[TCCLEARSCREEN] = ztrdup("\14");
	    tclen[TCCLEARSCREEN] = 1;
	}
    }
    return termok;
}

/* Initialize lots of global variables and hash tables */

/**/
void
setupvals(void)
{
    struct passwd *pswd;
    struct timezone dummy_tz;
    char *ptr;
#ifdef HAVE_GETRLIMIT
    int i;
#endif

    noeval = 0;
    curhist = 0;
    histsiz = DEFAULT_HISTSIZE;
    inithist();
    clwords = (char **) zcalloc((clwsize = 16) * sizeof(char *));

    cmdstack = (unsigned char *) zalloc(256);
    cmdsp = 0;

    bangchar = '!';
    hashchar = '#';
    hatchar = '^';
    termok = TERM_BAD;
    curjob = prevjob = coprocin = coprocout = -1;
    gettimeofday(&shtimer, &dummy_tz);	/* init $SECONDS */
    srand((unsigned int)(shtimer.tv_sec + shtimer.tv_usec)); /* seed $RANDOM */

    hostnam     = (char *) zalloc(256);
    gethostname(hostnam, 256);

    /* Set default path */
    path    = (char **) zalloc(sizeof(*path) * 5);
    path[0] = ztrdup("/bin");
    path[1] = ztrdup("/usr/bin");
    path[2] = ztrdup("/usr/ucb");
    path[3] = ztrdup("/usr/local/bin");
    path[4] = NULL;

    cdpath   = mkarray(NULL);
    manpath  = mkarray(NULL);
    fignore  = mkarray(NULL);
    fpath    = mkarray(NULL);
    mailpath = mkarray(NULL);
    watch    = mkarray(NULL);
    psvar    = mkarray(NULL);
#ifdef DYNAMIC
    module_path = mkarray(ztrdup(MODULE_DIR));
    modules = newlinklist();
#endif

    /* Set default prompts */
    if (opts[INTERACTIVE]) {
	prompt  = ztrdup("%m%# ");
	prompt2 = ztrdup("%_> ");
    } else {
	prompt = ztrdup("");
	prompt2 = ztrdup("");
    }
    prompt3 = ztrdup("?# ");
    prompt4 = ztrdup("+ ");
    sprompt = ztrdup("zsh: correct '%R' to '%r' [nyae]? ");

    ifs         = ztrdup(DEFAULT_IFS);
    wordchars   = ztrdup(DEFAULT_WORDCHARS);
    postedit    = ztrdup("");
    underscore  = ztrdup("");

    zoptarg = ztrdup("");
    zoptind = 1;
    schedcmds = NULL;

    ppid  = (long) getppid();
    mypid = (long) getpid();
    term  = ztrdup("");

#ifdef TIOCGWINSZ
    if (!(columns = shttyinfo.winsize.ws_col))
	columns = 80;
    if (columns < 2)
	opts[USEZLE] = 0;
    if (!(lines = shttyinfo.winsize.ws_row))
	lines = 24;
    if (lines < 2)
	opts[SINGLELINEZLE] = 1;
#else
    columns = 80;
    lines = 24;
#endif

    /* The following variable assignments cause zsh to behave more *
     * like Bourne and Korn shells when invoked as "sh" or "ksh".  *
     * NULLCMD=":" and READNULLCMD=":"                             */

    if (emulation == EMULATE_KSH || emulation == EMULATE_SH) {
	nullcmd     = ztrdup(":");
	readnullcmd = ztrdup(":");
    } else {
	nullcmd     = ztrdup("cat");
	readnullcmd = ztrdup("more");
    }

    /* We cache the uid so we know when to *
     * recheck the info for `USERNAME'     */
    cached_uid = getuid();

    /* Get password entry and set info for `HOME' and `USERNAME' */
    if ((pswd = getpwuid(cached_uid))) {
	home = metafy(pswd->pw_dir, -1, META_DUP);
	cached_username = ztrdup(pswd->pw_name);
    } else {
	home = ztrdup("/");
	cached_username = ztrdup("");
    }

    /* Try a cheap test to see if we can *
     * initialize `PWD' from `HOME'      */
    if (ispwd(home))
	pwd = ztrdup(home);
    else if ((ptr = zgetenv("PWD")) && ispwd(ptr))
	pwd = ztrdup(ptr);
    else
	pwd = metafy(zgetcwd(), -1, META_REALLOC);

    oldpwd = ztrdup(pwd);  /* initialize `OLDPWD' = `PWD' */
#ifdef __EMX__
    *cdrive = _getdrive();
    strcat(cdrive+1,":");
#endif

    inittyptab();     /* initialize the ztypes table */
    initlextabs();    /* initialize lexing tables    */

    createreswdtable();     /* create hash table for reserved words    */
    createaliastable();     /* create hash table for aliases           */
    createcmdnamtable();    /* create hash table for external commands */
    createshfunctable();    /* create hash table for shell functions   */
    createbuiltintable();   /* create hash table for builtin commands  */
    createnameddirtable();  /* create hash table for named directories */
    createparamtable();     /* create paramater hash table             */

#ifdef ZLE_MODULE
    add_dep("compctl", "zle");
    addbuiltin("bindkey", 0, NULL, 0, -1, "zle");
    addbuiltin("vared", 0, NULL, 1, 7, "zle");
    addbuiltin("compctl", 0, NULL, 0, -1, "compctl");
#endif

#ifdef HAVE_GETRLIMIT
    for (i = 0; i != RLIM_NLIMITS; i++) {
	getrlimit(i, current_limits + i);
	limits[i] = current_limits[i];
    }
#endif

    breaks = loops = 0;
    lastmailcheck = time(NULL);
    locallist = NULL;
    locallevel = sourcelevel = 0;
    trapreturn = 0;
    noerrexit = 0;
    nohistsave = 1;
    dirstack = newlinklist();
    bufstack = newlinklist();
    hsubl = hsubr = NULL;
    lastpid = 0;
    bshin = SHIN ? fdopen(SHIN, "r") : stdin;
    if (isset(SHINSTDIN) && !SHIN && unset(INTERACTIVE)) {
#ifdef _IONBF
	setvbuf(stdin, NULL, _IONBF, 0);
#else
	setlinebuf(stdin);
#endif
    }

    times(&shtms);
}

/* Initialize signal handling */

/**/
void
init_signals(void)
{
    intr();

#ifndef QDEBUG
    signal_ignore(SIGQUIT);
#endif

    install_handler(SIGHUP);
    install_handler(SIGCHLD);
    if (interact) {
	install_handler(SIGALRM);
#ifdef SIGWINCH
	install_handler(SIGWINCH);
#endif
	signal_ignore(SIGTERM);
    }
    if (jobbing) {
	long ttypgrp;

#ifndef __EMX__
	while ((ttypgrp = gettygrp()) != -1 && ttypgrp != mypgrp)
	    kill(0, SIGTTIN);
#endif
	if (ttypgrp == -1) {
	    opts[MONITOR] = 0;
	} else {
#ifndef __EMX__
	    signal_ignore(SIGTTOU);
	    signal_ignore(SIGTSTP);
	    signal_ignore(SIGTTIN);
#endif
	    signal_ignore(SIGPIPE);
	    attachtty(mypgrp);
	}
    }
    if (islogin) {
	signal_setmask(signal_mask(0));
    } else if (interact) {
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	signal_unblock(set);
    }
}

/* Source the init scripts.  If called as "ksh" or "sh"  *
 * then we source the standard sh/ksh scripts instead of *
 * the standard zsh scripts                              */

/**/
void
run_init_scripts(void)
{
#ifdef __EMX__
  char tempbuf[128];
#endif
    noerrexit = -1;

    if (emulation == EMULATE_KSH || emulation == EMULATE_SH) {
	if (islogin)
#ifdef __EMX__
	    strcpy(tempbuf,GLOBAL_DIR);
	    source(strcat(tempbuf,"/etc/profile"));
#else
	    source("/etc/profile");
#endif
	if (unset(PRIVILEGED)) {
	    char *s = getsparam("ENV");
	    if (islogin)
#ifdef __EMX__
		sourcehome("profile");
#else
		sourcehome(".profile");
#endif
	    noerrs = 1;
	    if (s && !parsestr(s)) {
		singsub(&s);
		noerrs = 0;
		source(s);
	    }
	    noerrs = 0;
	} else
#ifdef __EMX__
	    strcpy(tempbuf,GLOBAL_DIR);
	    source(strcat(tempbuf,"/etc/suid_profile"));
#else
	    source("/etc/suid_profile");
#endif
    } else {
#ifdef GLOBAL_ZSHENV
#ifdef __EMX__
        strcpy(tempbuf,GLOBAL_DIR);
	source(strcat(GLOBAL_DIR,GLOBAL_ZSHENV));
#else
	source(GLOBAL_ZSHENV);
#endif
#endif
	if (isset(RCS)) {
	    if (unset(PRIVILEGED))
#ifdef __EMX__
		sourcehome("zshenv.zsh");
#endif
		sourcehome(".zshenv");
	    if (islogin) {
#ifdef GLOBAL_ZPROFILE
#ifdef __EMX__
	        strcpy(tempbuf,GLOBAL_DIR);
		source(strcat(tempbuf,GLOBAL_ZPROFILE));
#else
		source(GLOBAL_ZPROFILE);
#endif
#endif
		if (unset(PRIVILEGED))
#ifdef __EMX__
		    sourcehome("zprofile.zsh");
#endif
		    sourcehome(".zprofile");
	    }
	    if (interact) {
#ifdef GLOBAL_ZSHRC
#ifdef __EMX__
	        strcpy(tempbuf,GLOBAL_DIR);
		source(strcat(tempbuf,GLOBAL_ZSHRC));
#else
		source(GLOBAL_ZSHRC);
#endif
#endif
		if (unset(PRIVILEGED))
#ifdef __EMX__
		    sourcehome("zshrc.zsh");
#endif
		    sourcehome(".zshrc");
	    }
	    if (islogin) {
#ifdef GLOBAL_ZLOGIN
#ifdef __EMX__
	        strcpy(tempbuf,GLOBAL_DIR);
		source(strcat(tempbuf,GLOBAL_ZLOGIN));
#else
		source(GLOBAL_ZLOGIN);
#endif
#endif
		if (unset(PRIVILEGED))
#ifdef __EMX__
		    sourcehome("zlogin.zsh");
#endif
		    sourcehome(".zlogin");
	    }
	}
    }
    noerrexit = 0;
    nohistsave = 0;
}

/* Miscellaneous initializations that happen after init scripts are run */

/**/
void
init_misc(void)
{
    if (cmd) {
	if (SHIN >= 10)
	    fclose(bshin);
	SHIN = movefd(open("/dev/null", O_RDONLY));
	bshin = fdopen(SHIN, "r");
	execstring(cmd, 0, 1);
	stopmsg = 1;
	zexit(lastval, 0);
    }

    if (interact && isset(RCS))
	readhistfile(getsparam("HISTFILE"), 0);
    adjustwinsize();   /* check window size and adjust if necessary */
}

/* source a file */

/**/
int
source(char *s)
{
    int tempfd, fd, cj, oldlineno;
    int oldshst, osubsh, oloops;
    FILE *obshin;
    char *old_scriptname = scriptname;

    if (!s || (tempfd = movefd(open(unmeta(s), O_RDONLY))) == -1) {
	return 1;
    }

    /* save the current shell state */
    fd        = SHIN;            /* store the shell input fd                  */
    obshin    = bshin;          /* store file handle for buffered shell input */
    osubsh    = subsh;           /* store whether we are in a subshell        */
    cj        = thisjob;         /* store our current job number              */
    oldlineno = lineno;          /* store our current lineno                  */
    oloops    = loops;           /* stored the # of nested loops we are in    */
    oldshst   = opts[SHINSTDIN]; /* store current value of this option        */

    SHIN = tempfd;
    bshin = fdopen(SHIN, "r");
    subsh  = 0;
    lineno = 0;
    loops  = 0;
    dosetopt(SHINSTDIN, 0, 1);
    scriptname = s;

    sourcelevel++;
    loop(0);			/* loop through the file to be sourced        */
    sourcelevel--;
    fclose(bshin);
    fdtable[SHIN] = 0;

    /* restore the current shell state */
    SHIN = fd;                       /* the shell input fd                   */
    bshin = obshin;                  /* file handle for buffered shell input */
    subsh = osubsh;                  /* whether we are in a subshell         */
    thisjob = cj;                    /* current job number                   */
    lineno = oldlineno;              /* our current lineno                   */
    loops = oloops;                  /* the # of nested loops we are in      */
    dosetopt(SHINSTDIN, oldshst, 1); /* SHINSTDIN option                     */
    errflag = 0;
    retflag = 0;
    scriptname = old_scriptname;

    return 0;
}

/* Try to source a file in the home directory */

/**/
void
sourcehome(char *s)
{
    char buf[PATH_MAX];
    char *h;

    if (emulation == EMULATE_SH || emulation == EMULATE_KSH ||
	!(h = getsparam("ZDOTDIR")))
	h = home;
    if (strlen(h) + strlen(s) + 1 >= PATH_MAX) {
	zerr("path too long: %s", s, 0);
	return;
    }
    sprintf(buf, "%s/%s", h, s);
    source(buf);
}

#ifndef DYNAMIC
#define DOMOD(boot) int boot(Module);
#include "bltinmods.list"
#undef DOMOD

/**/
void
init_bltinmods(void)
{
#define DOMOD(boot) boot(NULL);
#include "bltinmods.list"
#undef DOMOD
}
#endif /*!DYNAMIC*/
