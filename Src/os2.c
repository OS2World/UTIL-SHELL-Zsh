#include <process.h>
#include "zsh.h"
#ifdef NOT_WORKING
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <sys/rmutex.h>
#endif
#undef execve
#undef read
#undef fork
#undef init_io
#undef zexit

static pid_t zpid;
static int fork_depth;
static pid_t lpid;
#ifdef NOT_WORKING
static _rmutex read_sem;
#endif

				/* access      */

/**/
int
os2_execve(const char *name, char * const argv[], char *const envp[])
{
  /* Object is to spawn process and kill it and all descendants if signaled */
  int signo,lstat,options;
  struct stat dummy;
  char b[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT];
  char *ext,*base;
  
  strcpy(b,name);
  ext = _getext2(name);
  if (stricmp(ext,".exe")) strcat(b,".exe");
  if (_stat(b,&dummy)) return -1;
  base = _getname(name);
  options = islower(*base)?P_NOWAIT:P_UNRELATED|P_SESSION;
  options |= isupper(*(ext+1))?P_NOCLOSE:0;
  if ((lpid =spawnve(options,name,argv,envp)) == -1) return -1;
  /* Assume that any signal sent to us is intended for descendant */
  for (signo = 1;signo <=21;signo++)
    switch(signo)
      {
      case 19:
      case 20:
	break;
      default: signal(signo,os2_sighandler);
	break;
      }
  if (isupper(*base)) exit(0); /* Assume I'll kill it off later */
  do {
    wait(&lstat);
  } while (!WIFEXITED(lstat));
  
  exit(WEXITSTATUS(lstat));
}

/**/
void
os2_sighandler(int signo)
{
  kill(lpid,signo);
  switch(signo)
    {
    case SIGINT:
    case SIGQUIT:
    case SIGABRT:
    case SIGKILL:
    case SIGTERM:
    case SIGBREAK: 
    default: break;
    }
  signal(signo,os2_sighandler);
}

  
/**/
int
os2_read(int handle, void *buf, size_t nbyte)
#ifdef NOT_WORKING
{
  if(!isatty(handle))
    return(read(handle,buf,nbyte));
  while (1)
    {
      if(_rmutex_available(&read_sem))
	return(read(handle,buf,nbyte));
      _sleep2(10);
    }
}
#else
{
  return(read(handle,buf,nbyte));
}
#endif


/**/
int
os2_fork(void)
{
  int rc;
  rc = fork();
  if (rc) return rc;
  fork_depth++;
  return rc;
}

/**/
pid_t
os2_setpgrp(pid_t pid, pid_t pgrp)
{
#ifdef NOT_WORKING
  if (pid == SHTTY) pid = zpid;
  if (!pid) pid = getpid();
  if (pid == zpid) _rmutex_release(&read_sem);
  else _rmutex_request(&read_sem,_FMR_NOWAIT);
#endif
  return 0;
}


/**/
pid_t
setpgrp(pid_t pid, pid_t pgrp)
{
  return 0;
}
/**/
int
os2_tcsetpgrp(int handle,pid_t pgrp)
{
#ifdef NOT_WORKING
  if (pgrp == zpid) _rmutex_release(&read_sem);
  else _rmutex_request(&read_sem,_FMR_NOWAIT);
#endif
  return 0;
}

/**/
void
os2_init_io(void)
{
  zpid = getpid();
#ifdef NOT_WORKING
  _rmutex_create(&read_sem,0);
#endif
  init_io();
}

/**/
void
os2_zexit(int x, int y)
{
#ifdef NOT_WORKING
  _rmutex_close(&read_sem);
#endif
  zexit(x,y);
#ifdef NOT_WORKING
  _rmutex_create(&read_sem,0);/* if we get here we didn't really leave */
                              /* This may not work!!! */
#endif
}

