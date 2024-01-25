#include <zboss_api.h>

#include "type_tests_math.h"
#include "tests_zb_uint24.h"

static int test_zb_uint24_add(test_param_t param);
static int test_zb_uint24_sub(test_param_t param);
static int test_zb_uint24_mul(test_param_t param);
static int test_zb_uint24_div(test_param_t param);

/**
 * @brief Tests List
 */
static test_t test_arr[] =
{
    /* zb_uint24_add tests */
    {test_zb_uint24_add, 0, {0, 0, 0}},
    {test_zb_uint24_add, 0, {1, 1, 2}},
    {test_zb_uint24_add, 1, {MAX_UNSIGNED_24BIT_VAL, 1, 1}}, /* uint24 out of bounds */
    /* zb_uint24_sub tests */
    {test_zb_uint24_sub, 0, {1, 1, 0}},
    {test_zb_uint24_sub, 0, {MAX_UNSIGNED_24BIT_VAL, 1, (MAX_UNSIGNED_24BIT_VAL - 1)}},
    {test_zb_uint24_sub, 1, {0, 1, -1}},  /* uint24 does not allow negative values */
    /* zb_uint24_mul tests */
    {test_zb_uint24_mul, 0, {1, 1, 1}},
    {test_zb_uint24_mul, 0, {MAX_UNSIGNED_24BIT_VAL, 0, 0}},
    {test_zb_uint24_mul, 1, {MAX_UNSIGNED_24BIT_VAL, 2, 0}}, /* uint24 out of bounds */
    /* zb_uint24_div */
    {test_zb_uint24_div, 0, {1, 1, 1}},
    {test_zb_uint24_div, 0, {4, 2, 2}},
    {test_zb_uint24_div, 0, {MAX_UNSIGNED_24BIT_VAL, 2, (MAX_UNSIGNED_24BIT_VAL / 2)}},
};

/* TESTS EXECUTION */

int tests_zb_uint24(void)
{
    zb_bool_t pass = ZB_TRUE;

    for (size_t i = 0; i < (sizeof(test_arr) / sizeof(test_t)); i++)
    {
        int ret = test_arr[i].func(test_arr[i].params);
        if (ret != test_arr[i].expected_ret)
        {
            TRACE_MSG(TRACE_ERROR, "Error in tests_zb_uint24 index [%u] - ret [%d]", (FMT__D_D, i, ret));
            pass = ZB_FALSE;
        }
    }

    return pass ? 0 : -1;
}

/* TESTS IMPLEMENTATION */

static int test_zb_uint24_add(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_uint24_t, zb_uint32_to_uint24, zb_uint24_to_int32, zb_uint24_add, ret);

    return ret;
}

static int test_zb_uint24_sub(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_uint24_t, zb_uint32_to_uint24, zb_uint24_to_int32, zb_uint24_sub, ret);

    return ret;
}

static int test_zb_uint24_mul(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_uint24_t, zb_uint32_to_uint24, zb_uint24_to_int32, zb_uint24_mul, ret);

    return ret;
}

static int test_zb_uint24_div(test_param_t param)
{
    int ret;

    TEST_ZB_MATH(zb_uint24_t, zb_uint32_to_uint24, zb_uint24_to_int32, zb_uint24_div, ret);

    return ret;
}
