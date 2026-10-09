// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>

typedef void (*TestFunction)(void);

void test_run(const char *name, TestFunction function);
void test_assert_int_eq(
	int expected,
	int actual,
	const char *expression,
	const char *file,
	int line
);
void test_assert_bool_eq(
	bool expected,
	bool actual,
	const char *expression,
	const char *file,
	int line
);
int test_finish(void);

#define TEST(name) static void test_##name(void)
#define ASSERT_INT_EQ(expected, actual)                                                            \
	test_assert_int_eq((expected), (actual), #actual, __FILE__, __LINE__)
#define ASSERT_BOOL_EQ(expected, actual)                                                           \
	test_assert_bool_eq((expected), (actual), #actual, __FILE__, __LINE__)
