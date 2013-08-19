int os2_execve _((const char *name, char * const argv[], char *const envp[]));
void os2_sighandler _((int signo));
int os2_read _((int handle, void *buf, size_t nbyte));
int os2_fork _((void));
pid_t os2_setpgrp _((pid_t pid, pid_t pgrp));
pid_t setpgrp _((pid_t pid, pid_t pgrp));
int os2_tcsetpgrp _((int handle,pid_t pgrp));
void os2_init_io _((void));
void os2_zexit _((int x, int y));
