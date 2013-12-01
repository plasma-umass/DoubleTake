#ifndef _LIBHX_PROC_H
#define _LIBHX_PROC_H

#ifndef __cplusplus
#	include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
	HXPROC_VERBOSE     = 1 << 0,
	HXPROC_EXECV       = 1 << 1,
	HXPROC_A0          = 1 << 2,
	HXPROC_STDIN       = 1 << 3,
	HXPROC_STDOUT      = 1 << 4,
	HXPROC_STDERR      = 1 << 5,
	HXPROC_NULL_STDIN  = 1 << 6,
	HXPROC_NULL_STDOUT = 1 << 7,
	HXPROC_NULL_STDERR = 1 << 8,
};

struct HXproc_ops {
	void (*p_prefork)(void *);
	void (*p_postfork)(void *);
	void (*p_complete)(void *);
};

struct HXproc {
	const struct HXproc_ops *p_ops;
	void *p_data;
	unsigned int p_flags;

	int p_stdin, p_stdout, p_stderr;
	int p_pid;
	char p_status;
	bool p_exited, p_terminated;
};

extern int HXproc_run_async(const char *const *, struct HXproc *);
extern int HXproc_run_sync(const char *const *, unsigned int);
extern int HXproc_wait(struct HXproc *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBHX_PROC_H */
