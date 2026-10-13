// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <stdio.h>
#include <string.h>


static unsigned int testsRun;
static unsigned int testsFailed;
static bool currentTestFailed;

void registerGameTests(void);
void registerSolverTests(void);
void registerPerformanceTests(void);

void test_run(const char *name, const TestFunction fn) {
	currentTestFailed = false;
	++testsRun;
	fn();

	if (currentTestFailed) {
		++testsFailed;
	}
	printf("[%s] %s\n", currentTestFailed ? "FAIL" : "PASS", name);
}

int test_finish(void) {
	printf("%u tests, %u failed\n", testsRun, testsFailed);
	return testsRun == 0 || testsFailed != 0;
}

void test_assertIntEq(
	const int actual,
	const int expected,
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

void test_assertBoolEq(
	const bool actual,
	const bool expected,
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

int main(const int argc, const char *argv[]) {
	registerGameTests();
	registerSolverTests();
	// The performance test takes a while, so only run it when asked
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--performance") == 0) {
			registerPerformanceTests();
			break;
		}
	}
	return test_finish();
}
