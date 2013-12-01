/*
 *	Testing for compile error in the cast helpers
 *	written by Jan Engelhardt
 *	this program is released in the Public Domain
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <libHX/defs.h>
#include <libHX/init.h>
#include "internal.h"
#define UNUSED __attribute__((unused))

static void c_signed(void)
{
	const char *si_00 = "foo";
	char *si_01 = const_cast1(char *, si_00);
	signed char *si_02 UNUSED = signed_cast(signed char *, si_01);
	unsigned char *si_03 UNUSED = signed_cast(unsigned char *, si_01);
	const signed char *si_04 UNUSED = signed_cast(const signed char *, si_00);
	const unsigned char *si_05 UNUSED = signed_cast(const unsigned char *, si_00);
	si_00 = signed_cast(const char *, si_05);
}

static void c_reinterpret(void)
{
	const char *si_00 = "foo";
	void *sr_00 = reinterpret_cast(void *, static_cast(long, 8));
	int sr_01 = reinterpret_cast(long, sr_00);
	void *sr_02 = reinterpret_cast(void *, static_cast(long, static_cast(int, reinterpret_cast(long, &si_00))));
	printf("sr: %p %u; %p[%p]\n", sr_00, sr_01, sr_02, &si_00);
}

static void c_static(void)
{
	double st_00 = sqrt(static_cast(int,
		10 * sqrt(static_cast(double, 3) / 4)));
	printf("st: %f\n", st_00);
}

static void c_const1(void)
{
	const int *co_00 = NULL;
	int *co_01 = const_cast1(int *, co_00);
	co_00 = co_01;
}

static void c_const2(void)
{
	const int **co_02 = NULL;
	int **co_03 UNUSED = const_cast2(int **, co_02);
	int *const *co_04 = const_cast2(int *const *, co_02);
	const int *const *co_05 = const_cast2(const int *const *, co_02);
	co_02 = const_cast2(const int **, co_05);
	co_04 = const_cast2(int *const *, co_05);
}

static void c_const3(void)
{
	const int *const *const *co_06 = NULL;
	int ***co_07 UNUSED = const_cast3(int ***,
		(printf("If this line is only printed once when the program "
		"is run, __builtin_choose_expr works as desired.\n"), co_06));
}

static void c_constA(void)
{
	static const char r1[] = "static";
	char *w1 UNUSED = const_cast1(char *, r1);

	static const char *const r2[] = {"static"};
	char **w2 UNUSED = const_cast2(char **, r2);

	static const char *const *const r3[] = {NULL};
	char ***w3 UNUSED = const_cast3(char ***, r3);
}

int main(void)
{
	if (HX_init() <= 0)
		abort();
	c_signed();
	c_reinterpret();
	c_static();
	c_const1();
	c_const2();
	c_const3();
	c_constA();
	HX_exit();
	return 0;
}
