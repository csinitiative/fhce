/*
 * Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
 *
 * This file is part of FeedHandlers (FH).
 *
 * FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FH_TEST_ASSERT_H__
#define __FH_TEST_ASSERT_H__

/* system headers */
#include <stdio.h>
#include <string.h>

/* common FH headers */
#include "fh_errors.h"

/* FH test headers */
#include "fh_test.h"

/*! \brief Basic assertion function used by all others
 *
 *  \param expression expression being tested
 *  \param file name of the file where this assertion is made
 *  \param line line number on which this assertion is made
 */
void fh_test_assert(int expression, const char *file, int line);

/*! \brief Assert that the provided expression is true
 *  \param expression expression that is asserted to be true
 */
#define FH_TEST_ASSERT_TRUE(expression)                                                     \
do {                                                                                        \
    /* in case expression is a call that makes some change in state */                      \
    int expr_calc = expression;                                                             \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_TRUE(%d)", expr_calc);                             \
    fh_test_assert(expr_calc, __FILE__, __LINE__);                                          \
} while (0)

/*! \brief Assert that the provided expression is false
 *  \param expression expression that is asserted to be false
 */
// void fh_test_assert_false(int expression);
#define FH_TEST_ASSERT_FALSE(expression)                                                    \
do {                                                                                        \
    /* in case expression is a call that makes some change in state */                      \
    int expr_calc = expression;                                                             \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_FALSE(%d)", expr_calc);                            \
    fh_test_assert(!expr_calc, __FILE__, __LINE__);                                         \
} while (0)

/*! \brief Assert that the provided integer values are equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_EQUAL(val1,val2)                                                     \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    int val1_calc = val1;                                                                   \
    int val2_calc = val2;                                                                   \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_EQUAL(%d, %d)", val1_calc, val2_calc);             \
    fh_test_assert(val1_calc == val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided integer values are not equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_UNEQUAL(val1,val2)                                                   \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    int val1_calc = val1;                                                                   \
    int val2_calc = val2;                                                                   \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_UNEQUAL(%d, %d)", val1_calc, val2_calc);           \
    fh_test_assert(val1_calc != val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided long values are equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_LEQUAL(val1,val2)                                                    \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    long val1_calc = val1;                                                                  \
    long val2_calc = val2;                                                                  \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_LEQUAL(%ld, %ld)", val1_calc, val2_calc);          \
    fh_test_assert(val1_calc == val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided long values are not equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_LUNEQUAL(val1,val2)                                                  \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    long val1_calc = val1;                                                                  \
    long val2_calc = val2;                                                                  \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_LUNEQUAL(%ld, %ld)", val1_calc, val2_calc);        \
    fh_test_assert(val1_calc != val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided strings are equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_STREQUAL(val1,val2)                                                  \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    const char *val1_calc = val1;                                                           \
    const char *val2_calc = val2;                                                           \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_STREQUAL('%s', '%s')", val1_calc, val2_calc);      \
    fh_test_assert(!strcmp(val1_calc, val2_calc), __FILE__, __LINE__);                      \
} while (0)

/*! \brief Assert that the provided strings are not equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_STRUNEQUAL(val1,val2)                                                \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    const char *val1_calc = val1;                                                           \
    const char *val2_calc = val2;                                                           \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_STRUNEQUAL('%s', '%s')", val1_calc, val2_calc);    \
    fh_test_assert(strcmp(val1_calc, val2_calc), __FILE__, __LINE__);                       \
} while (0)

/*! \brief Assert that the provided statuses are equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_STATEQUAL(val1,val2)                                                 \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    FH_STATUS val1_calc = val1;                                                             \
    FH_STATUS val2_calc = val2;                                                             \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_STATEQUAL(%d, %d)", val1_calc, val2_calc);         \
    fh_test_assert(val1_calc == val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided statuses are not equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_STATUNEQUAL(val1,val2)                                               \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    FH_STATUS val1_calc = val1;                                                             \
    FH_STATUS val2_calc = val2;                                                             \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_STATUNEQUAL(%d, %d)", val1_calc, val2_calc);       \
    fh_test_assert(val1_calc != val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided pointers are equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_PTREQUAL(val1,val2)                                                  \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    void *val1_calc = (void *)val1;                                                         \
    void *val2_calc = (void *)val2;                                                         \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_PTREQUAL(%p, %p)", val1_calc, val2_calc);          \
    fh_test_assert(val1_calc == val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided pointers are not equal
 *  \param val1 first value in comparison
 *  \param val2 second value in comparison
 */
#define FH_TEST_ASSERT_PTRUNEQUAL(val1,val2)                                                \
do {                                                                                        \
    /* in case either value is a call that makes some change in state */                    \
    void *val1_calc = (void *)val1;                                                         \
    void *val2_calc = (void *)val2;                                                         \
                                                                                            \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_PTRUNEQUAL(%p, %p)", val1_calc, val2_calc);        \
    fh_test_assert(val1_calc != val2_calc, __FILE__, __LINE__);                             \
} while (0)

/*! \brief Assert that the provided pointer is null
 *  \param ptr pointer being tested
 */
#define FH_TEST_ASSERT_NULL(ptr)                                                            \
do {                                                                                        \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_NULL(%s)", #ptr);                                  \
    fh_test_assert(ptr == NULL, __FILE__, __LINE__);                                        \
} while (0)

/*! \brief Assert that the provided pointer is not null
 *  \param ptr pointer being tested
 */
#define FH_TEST_ASSERT_NOTNULL(ptr)                                                         \
do {                                                                                        \
    sprintf(fh_test_msg, "FH_TEST_ASSERT_NOTNULL(%s)", #ptr);                               \
    fh_test_assert(ptr != NULL, __FILE__, __LINE__);                                        \
} while (0)

#endif  /* __FH_TEST_ASSERT__ */
