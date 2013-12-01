/*
 *	this program is released in the Public Domain
 */
#ifdef __cplusplus
#	include <cerrno>
#	include <cstdlib>
#	include <cstring>
#else
#	include <errno.h>
#	include <stdlib.h>
#	include <string.h>
#endif
#include <unistd.h>
#include <libHX/init.h>
#include <libHX/proc.h>
#include <libHX/string.h>

static const char *const t_args1[] = {"ls", "ls", "-dl", ".", NULL};
static const char *const t_args2[] = {"ls", "ls", "-l", NULL};
static const char *const t_args3[] = {"ls", "-l", "/proc/self/fd/", NULL};

static void t_async1(void)
{
	FILE *fp;
	int ret;
	unsigned int i = 0;
	hxmc_t *line = NULL;
	struct HXproc proc;

	memset(&proc, 0, sizeof(proc));
	proc.p_flags = HXPROC_A0 | HXPROC_STDOUT;

	if ((ret = HXproc_run_async(t_args2, &proc)) <= 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		return;
	}
	if ((fp = fdopen(proc.p_stdout, "r")) == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		goto out;
	}
	while (HX_getl(&line, fp) != NULL)
		printf("\t#%u\t%s", ++i, line);

	fclose(fp);
 out:
	close(proc.p_stdout);
	HXproc_wait(&proc);
	return;
}

int main(void)
{
	if (HX_init() <= 0)
		abort();

	/* let it fail - test verbosity */
	HXproc_run_sync(t_args1 + 2, HXPROC_VERBOSE);
	HXproc_run_sync(t_args1 + 3, HXPROC_VERBOSE);

	HXproc_run_sync(t_args1 + 1, 0);

	t_async1();
	HXproc_run_sync(t_args3, HXPROC_NULL_STDIN);
	HX_exit();
	return EXIT_SUCCESS;
}
