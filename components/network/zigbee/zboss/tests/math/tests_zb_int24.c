#include <zboss_api.h>

#include "type_tests_math.h"
#include "tests_zb_int24.h"

static int test_zb_int24_add(test_param_t param);
static int test_zb_int24_sub(test_param_t param);
static int test_zb_int24_mul(test_param_t param);
static int test_zb_int24_div(test_param_t param);

/**
 * @brief Tests List
 */
static test_t test_arr[] =
{
    /* zb_int24_add tests */
    {test_zb_int24_add, 0, {0, 0, 0}},
    {test_zb_int24_add, 0, {1, 1, 2}},
    {test_zb_int24_add, 0, {MIN_SIGNED_24BIT_VAL, 1, (MIN_SIGNED_24BIT_VAL + 1)}},
    {test_zb_int24_add, 1, {MAX_SIGNED_24BIT_VAL, 1, MIN_SIGNED_24BIT_VAL}}, /* int24 out of bounds */
    /* zb_int24_sub tests */
    {test_zb_int24_sub, 0, {1, 1, 0}},
    {test_zb_int24_sub, 0, {0, 1, -1}},
    {test_zb_int24_sub, 0, {MAX_SIGNED_24BIT_VAL, 1, (MAX_SIGNED_24BIT_VAL - 1)}},
    {test_zb_int24_sub, 1, {MIN_SIGNED_24BIT_VAL, 1, MAX_SIGNED_24BIT_VAL}}, /* int24 out of bounds */
    /* zb_int24_mul tests */
    {test_zb_int24_mul, 0, {1, 1, 1}},
    {test_zb_int24_mul, 0, {MAX_SIGNED_24BIT_VAL, 0, 0}},
    {test_zb_int24_mul, 1, {MIN_SIGNED_24BIT_VAL, 2, 0}}, /* int24 out of bounds */
    {test_zb_int24_mul, 1, {MAX_SIGNED_24BIT_VAL, 2, 0}}, /* int24 out of bounds */
    /* zb_int24_div */
    {test_zb_int24_div, 0, {1, 1, 1}},
    {test_zb_int24_div, 0, {4, 2, 2}},
    {test_zb_int24_div, 0, {MAX_SIGNED_24BIT_VAL, 2, (MAX_SIGNED_24BIT_VAL / 2)}},
    {test_zb_int24_div, 0, {MIN_SIGNED_24BIT_VAL, 2, (MIN_SIGNED_24BIT_VAL / 2)}},
};

/* TESTS EXECUTION */

int tests_zb_int24(void)
{
    zb_bool_t pass = ZB_TRUE;

    for (size_t i = 0; i < (sizeof(test_arr) / sizeof(test_t)); i++)
    {
        int ret = test_arr[i].func(test_arr[i].params);
        if (ret != test_arr[i].expected_ret)
        {
            TRACE_MSG(TRACE_ERROR, "Error in tests_zb_int24 index [%u] - ret [%d]", (FMT__D_D, i, ret));
            pass = ZB_FALSE;
        }
    }

    return pass ? 0 : -1;
}

/* TESTS IMPLEMENTATION */

static int test_zb_int24_add(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_int24_t, zb_int32_to_int24, zb_int24_to_int32, zb_int24_add, ret);

    return ret;
}

static int test_zb_int24_sub(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_int24_t, zb_int32_to_int24, zb_int24_to_int32, zb_int24_sub, ret);

    return ret;
}

static int test_zb_int24_mul(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_int24_t, zb_int32_to_int24, zb_int24_to_int32, zb_int24_mul, ret);

    return ret;
}

static int test_zb_int24_div(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_int24_t, zb_int32_to_int24, zb_int24_to_int32, zb_int24_div, ret);

    return ret;
}
