// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <stdio.h>

static unsigned int testsRun;
static unsigned int testsFailed;
static bool currentTestFailed;

void register_game_tests(void);
void register_solver_tests(void);

void test_run(const char *name, const TestFunction function) {
	currentTestFailed = false;
	++testsRun;
	function();

	if (currentTestFailed) {
		++testsFailed;
	}
	printf("[%s] %s\n", currentTestFailed ? "FAIL" : "PASS", name);
}

void test_assert_int_eq(
	const int expected,
	const int actual,
	const char *expression,
	const char *file,
	const int line
) {
	if (expected == actual) {
		return;
	}

	currentTestFailed = true;
	fprintf(stderr, "%s:%d: %s expected %d, got %d\n", file, line, expression, expected, actual);
}

void test_assert_bool_eq(
	const bool expected,
	const bool actual,
	const char *expression,
	const char *file,
	const int line
) {
	if (expected == actual) {
		return;
	}

	currentTestFailed = true;
	fprintf(
		stderr,
		"%s:%d: %s expected %s, got %s\n",
		file,
		line,
		expression,
		expected ? "true" : "false",
		actual ? "true" : "false"
	);
}

int test_finish(void) {
	printf("%u tests, %u failed\n", testsRun, testsFailed);
	return testsRun == 0 || testsFailed != 0;
}

int main(void) {
	register_game_tests();
	register_solver_tests();
	return test_finish();
}
