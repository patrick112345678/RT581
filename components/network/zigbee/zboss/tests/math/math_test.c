#include <stdio.h>

#include <zboss_api.h>

#include "tests_zb_uint24.h"
#include "tests_zb_uint48.h"
#include "tests_zb_int24.h"
#include "tests_zb_int48.h"

MAIN()
{
    int ret = 0;
    ARGV_UNUSED;

    ret += tests_zb_uint24();
    ret += tests_zb_uint48();
    ret += tests_zb_int24();
    ret += tests_zb_int48();

    return ret;
}
