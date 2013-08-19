/** rlimits.h                                 **/
/** architecture-customized limits for zsh **/

static char *recs[RLIM_NLIMITS+1] = {
	"cputime",
	"filesize",
	"datasize",
	"stacksize",
	"coredumpsize",
	"memoryuse",
	"memorylocked",
	"maxproc",
	"descriptors",
	NULL
};

