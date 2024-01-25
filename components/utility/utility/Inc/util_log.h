/**
 * @file util_log.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __UTIL_LOG_H__
#define __UTIL_LOG_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __util_printf              rf_printk


#define BL_ASSERT(condition)    \
    do { \
        if (!(condition)) { \
            log_assert("ASSERT: %s:%d\r\n", __FILENAME__, __LINE__); \
            while (1) { \
                /*deap loop now*/ \
            } \
        } \
    } while (0)


#define ULOG_USE_COLOR  (1)
typedef enum LOG_BUF_OUT_DATA_TYPE {
    LOG_BUF_OUT_DATA_TYPE_HEX,
    LOG_BUF_OUT_DATA_TYPE_INT8,
    LOG_BUF_OUT_DATA_TYPE_UNT8,
} LOG_BUF_OUT_DATA_TYPE_T;
//#define LOG_USE_LINE_FEED
#define LOG_LOCK_LOCK    /* reserved, must diffrent with buf lock */
#define LOG_LOCK_UNLOCK  /* reserved, must diffrent with buf lock */
#define SHORT_FILE __FILENAME__

#ifdef LOG_USE_LINE_FEED
#define custom_log(N, M, ...) do {  LOG_LOCK_LOCK;\
                                    __util_printf("[%10u][%s: %s:%4d] " M "\r\n",\
                                    (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                                    N, SHORT_FILE, __LINE__,\
                                    ##__VA_ARGS__);\
                                    LOG_LOCK_UNLOCK;\
                                    } while(0==1)
#define custom_buf_pri(file, line, N, M, ...) do {  LOG_LOCK_LOCK;\
                                    __util_printf("[%10u][%s: %s:%4d] " M, "\r\n",\
                                    (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                                    N, file, line,\
                                    ##__VA_ARGS__);\
                                    LOG_LOCK_UNLOCK;\
                                    } while(0==1)
#else
#define custom_log(N, M, ...) do {  LOG_LOCK_LOCK;\
                                    __util_printf("[%10u][%s: %s:%4d] " M,\
                                    (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                                    N, SHORT_FILE, __LINE__,\
                                    ##__VA_ARGS__);\
                                    LOG_LOCK_UNLOCK;\
                                    } while(0==1)
#define custom_buf_pri(file, line, N, M, ...) do {  LOG_LOCK_LOCK;\
                                    __util_printf("[%10u][%s: %s:%4d] " M,\
                                    (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                                    N, file, line,\
                                    ##__VA_ARGS__);\
                                    LOG_LOCK_UNLOCK;\
                                    } while(0==1)
#endif

#if ULOG_USE_COLOR
#define ulog_buf_pri(file, line, M, ...)   custom_buf_pri(file, line, "\x1b[36mBUF\x1b[0m", M, ##__VA_ARGS__)
#else
#define ulog_buf_pri(file, line, M, ...)   custom_buf_pri(file, line, "BUF", M, ##__VA_ARGS__)
#endif

#if ULOG_USE_COLOR
#define ulog_trace(M, ...)   custom_log("\x1b[94mTRACE \x1b[0m", M, ##__VA_ARGS__)
#define ulog_debug(M, ...)   custom_log("\x1b[36mDEBUG \x1b[0m", M, ##__VA_ARGS__)
#define ulog_info(M, ...)    custom_log("\x1b[32mINFO  \x1b[0m", M, ##__VA_ARGS__)
#define ulog_warn(M, ...)    custom_log("\x1b[33mWARN  \x1b[0m", M, ##__VA_ARGS__)
#define ulog_error(M, ...)   custom_log("\x1b[31mERROR \x1b[0m", M, ##__VA_ARGS__)
#define ulog_assert(M, ...)  custom_log("\x1b[35mASSERT\x1b[0m", M, ##__VA_ARGS__)
#else
#define ulog_trace(M, ...)   custom_log("TRACE ", M, ##__VA_ARGS__)
#define ulog_debug(M, ...)   custom_log("DEBUG ", M, ##__VA_ARGS__)
#define ulog_info(M, ...)    custom_log("INFO  ", M, ##__VA_ARGS__)
#define ulog_warn(M, ...)    custom_log("WARN  ", M, ##__VA_ARGS__)
#define ulog_error(M, ...)   custom_log("ERROR ", M, ##__VA_ARGS__)
#define ulog_assert(M, ...)  custom_log("ASSERT", M, ##__VA_ARGS__)
#endif
#define ulog_buf(pbuf, len)  log_buf_out(SHORT_FILE, __LINE__, pbuf, len, LOG_BUF_OUT_DATA_TYPE_HEX)
#define ulog_buf_int8(pbuf, len)  log_buf_out(SHORT_FILE, __LINE__, pbuf, len, LOG_BUF_OUT_DATA_TYPE_INT8)
#define ulog_buf_unt8(pbuf, len)  log_buf_out(SHORT_FILE, __LINE__, pbuf, len, LOG_BUF_OUT_DATA_TYPE_UNT8)
#define ulog_compile_time()  do {\
                            ulog_info("compile date = %s\r\n", __DATE__);\
                            ulog_info("compile time = %s\r\n", __TIME__);\
                            } while (0 == 1);

int log_buf_out(const char *file, int line, const void *inbuf, int len, LOG_BUF_OUT_DATA_TYPE_T type);

void rf_printk(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

