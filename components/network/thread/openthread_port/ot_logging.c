/**
 * @file ot_logging.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include <openthread_port.h>

#if defined(__CC_ARM) || defined(__CLANG_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list argp;

    va_start(argp, aFormat);
    vprintf(aFormat, argp);
    va_end(argp);
}
#else
extern void vprint(const char *fmt, va_list argp);

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list argp;

    va_start(argp, aFormat);
    vprint(aFormat, argp);
    va_end(argp);
}
#endif