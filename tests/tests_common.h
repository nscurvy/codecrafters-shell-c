//
// Created by nkinder on 9/13/26.
//

#pragma once
#include <check.h>
#define assert(val) ck_assert((val))
#define streq(a, b) ck_assert_str_eq((a), (b))
#define uinteq(a, b) ck_assert_uint_eq((a), (b))
#define inteq(a, b) ck_assert_int_eq((a), (b))
