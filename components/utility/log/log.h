/**
 * @file log.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <util_log.h>

#include "log_type.h"
#include "log_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
#if (CFG_COMPONENT_LOG_ENABLE == 1)
#define ATTR_LOG_CODE1(name)        __attribute__((used, section(".static_logcomponent_code." #name)))
#define ATTR_LOG_CODE2(name)        __attribute__((used, section(".static_logfile_code." #name)))
#define ATTR_LOG_CODE3(name)        __attribute__((used, section(".static_logpri_code." #name)))

/* for hard debug level */
const static uint32_t LOG_HARD_DECLARE_DISABLE __attribute__((used)) = 0;

/* component level */
#define _REFC_LEVEL(name)            _fsymc_level_##name
#define REFC_LEVEL(name)             _REFC_LEVEL(name)
#define _DEFC_LEVEL(name)            log_level_t REFC_LEVEL(name) __attribute__((weak))
#define DECLARE_C_LEVEL(name)        _DEFC_LEVEL(name)
/* component info */
#define _REFC_INFO(name)             _fsymc_info_##name
#define REFC_INFO(name)              _REFC_INFO(name)
#define _DEFC_INFO(name)             static const log_info_t REFC_INFO(name) ATTR_LOG_CODE1(name) = {\
                                         (log_level_t *)(&REFC_LEVEL(name)), (char *)(#name)}
#define DECLARE_C_INFO(name)         _DEFC_INFO(name)
DECLARE_C_LEVEL(__COMPONENT_NAME_DEQUOTED__);
DECLARE_C_INFO(__COMPONENT_NAME_DEQUOTED__);

/* file level */
#define _REFF_LEVEL(name)            _fsymf_level_##name
#define REFF_LEVEL(name)             _REFF_LEVEL(name)
#define _DEFF_LEVEL(name)            log_level_t REFF_LEVEL(name)
#define DECLARE_F_LEVEL(name)        _DEFF_LEVEL(name)
/* file info */
#define _REFF_INFO(name)             _fsymf_info_##name
#define REFF_INFO(name)              _REFF_INFO(name)
#define _DEFF_INFO(name, named)      const log_info_t REFF_INFO(name) ATTR_LOG_CODE2(name) = {\
                                        (log_level_t *)(&REFF_LEVEL(name)), (char *)(#named)}
#define DECLARE_F_INFO(name, named) _DEFF_INFO(name, named)
DECLARE_F_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__);
DECLARE_F_INFO(__COMPONENT_FILE_NAME_DEQUOTED__, __COMPONENT_FILE_NAMED__);

/* pri level */
#define _REFP_LEVEL(name)            _fsymp_level_##name
#define REFP_LEVEL(name)             _REFP_LEVEL(name)
#define _DEFP_LEVEL(name)            log_level_t REFP_LEVEL(name)
#define DECLARE_P_LEVEL(name)        _DEFP_LEVEL(name)
/* pri info */
#define _REFP_INFO(name)             _fsymp_info_##name
#define REFP_INFO(name)              _REFP_INFO(name)
#define _DEFP_INFO(name, prefix)     const log_info_t REFP_INFO(name) ATTR_LOG_CODE3(name) = {\
                                        (log_level_t *)(&REFP_LEVEL(name)), (char *)(#prefix"."#name)}
#define DECLARE_P_INFO(name, prefix) _DEFP_INFO(name, prefix)

#define LOG_DECLARE(name)           DECLARE_P_LEVEL(name);\
                                     DECLARE_P_INFO(name, __COMPONENT_FILE_NAMED__);

#define custom_cflog(lowlevel, N, M, ...) do {\
    if ( (lowlevel >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) && \
         (lowlevel >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__))\
       ) {\
            __log_printf("[%10u][%s: %s:%4d] " M"\r\n",\
            (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
            N, __FILENAME__, __LINE__,\
            ##__VA_ARGS__);\
    }\
} while(0==1)

#define custom_cflog_raw(lowlevel, N, M, ...) do {\
    if ( (lowlevel >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) && \
         (lowlevel >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__))\
       ) {\
            __log_printf(M,\
            ##__VA_ARGS__);\
    }\
} while(0==1)

#define custom_plog(priname, lowlevel, N, M, ...) do {\
    if ( (lowlevel >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) && \
         (lowlevel >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__)) && \
         (lowlevel >= REFP_LEVEL(priname)) \
       ) {\
            __log_printf("[%10u][%s: %s:%4d] " M,\
            (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
            N, __FILENAME__, __LINE__,\
            ##__VA_ARGS__);\
    }\
} while(0==1)

#define custom_plog_raw(priname, lowlevel, N, M, ...) do {\
    if ( (lowlevel >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) && \
         (lowlevel >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__)) && \
         (lowlevel >= REFP_LEVEL(priname)) \
       ) {\
            __log_printf(M,\
            ##__VA_ARGS__);\
    }\
} while(0==1)

#define custom_hexdumplog(name, lowlevel, logo, buf, size) do {\
    if ( (lowlevel >= REFC_LEVEL(__COMPONENT_NAME_DEQUOTED__)) && \
         (lowlevel >= REFF_LEVEL(__COMPONENT_FILE_NAME_DEQUOTED__))\
       ) {\
            __log_printf("[%10u][%s: %s:%4d] %s:\r\n",\
            (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
            logo, __FILENAME__, __LINE__,\
            name);\
            log_hexdump_out(name, 16, buf, size);\
    }\
} while(0==1)

#define LOG_USE_COLOR (1)

#if LOG_USE_COLOR
#define LOG_PREFIX_INFO  "\x1b[32mINFO  \x1b[0m"
#define LOG_PREFIX_WARN  "\x1b[33mWARN  \x1b[0m"
#define LOG_PREFIX_ERROR "\x1b[31mERROR \x1b[0m"
#define LOG_PREFIX_USER  "\x1b[35mASSERT\x1b[0m"
#define LOG_PREFIX_DUMP  "\x1b[35mDUMP\x1b[0m"
#else
#define LOG_PREFIX_INFO  "INFO"
#define LOG_PREFIX_WARN  "WARN"
#define LOG_PREFIX_ERROR "ERROR"
#define LOG_PREFIX_USER  "ASSERT"
#define LOG_PREFIX_DUMP  "DUMP"
#endif

#define log_debug(M, ...)              if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog(LOG_LEVEL_DEBUG,"DEBUG ", M, ##__VA_ARGS__);}                 // NULL
#define log_info(M, ...)               if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog(LOG_LEVEL_INFO,LOG_PREFIX_INFO, M, ##__VA_ARGS__);}    // F_GREEN
#define log_warn(M, ...)               if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog(LOG_LEVEL_WARN,LOG_PREFIX_WARN, M, ##__VA_ARGS__);}    // F_YELLOW
#define log_error(M, ...)              if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog(LOG_LEVEL_ERROR,LOG_PREFIX_ERROR, M, ##__VA_ARGS__);}  // F_RED
#define log_assert(assertion)          if (0 == (assertion)) {\
                                                __log_printf("assert, %s:%d\r\n", __FILENAME__, __LINE__);\
                                                while(1);\
                                            }
#define log_debug_raw(M, ...)              if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog_raw(LOG_LEVEL_DEBUG,"DEBUG ", M, ##__VA_ARGS__);}                 // NULL
#define log_info_raw(M, ...)               if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog_raw(LOG_LEVEL_INFO,LOG_PREFIX_INFO, M, ##__VA_ARGS__);}    // F_GREEN
#define log_warn_raw(M, ...)               if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog_raw(LOG_LEVEL_WARN,LOG_PREFIX_WARN, M, ##__VA_ARGS__);}    // F_YELLOW
#define log_error_raw(M, ...)              if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_cflog_raw(LOG_LEVEL_ERROR,LOG_PREFIX_ERROR, M, ##__VA_ARGS__);}  // F_RED

#define log_debug_user(name, M, ...)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog(name,LOG_LEVEL_DEBUG, "DEBUG ", M, ##__VA_ARGS__);}
#define log_info_user(name, M, ...)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog(name,LOG_LEVEL_INFO, LOG_PREFIX_INFO, M, ##__VA_ARGS__);}
#define log_warn_user(name, M, ...)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog(name,LOG_LEVEL_WARN, LOG_PREFIX_WARN, M, ##__VA_ARGS__);}
#define log_error_user(name, M, ...)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog(name,LOG_LEVEL_ERROR, LOG_PREFIX_ERROR, M, ##__VA_ARGS__);}
#define log_debug_user_raw(name, M, ...)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog_raw(name,LOG_LEVEL_DEBUG, "DEBUG ", M, ##__VA_ARGS__);}
#define log_info_user_raw(name, M, ...)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog_raw(name,LOG_LEVEL_INFO, LOG_PREFIX_INFO, M, ##__VA_ARGS__);}
#define log_warn_user_raw(name, M, ...)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog_raw(name,LOG_LEVEL_WARN, LOG_PREFIX_WARN,M, ##__VA_ARGS__);}
#define log_error_user_raw(name, M, ...)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog_raw(name,LOG_LEVEL_ERROR, LOG_PREFIX_ERROR, M, ##__VA_ARGS__);}
#define log_assert_user(name, M, ...)  if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                            custom_plog(name,LOG_LEVEL_ASSERT, LOG_PREFIX_USER, M, ##__VA_ARGS__);}

#define log_debug_hexdump(name, buf, size)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                              custom_hexdumplog(name,LOG_LEVEL_DEBUG, "DEBUG ", buf, size);}
#define log_info_hexdump(name, buf, size)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                              custom_hexdumplog(name,LOG_LEVEL_INFO, LOG_PREFIX_INFO, buf, size);}
#define log_warn_hexdump(name, buf, size)    if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                              custom_hexdumplog(name,LOG_LEVEL_WARN, LOG_PREFIX_WARN, buf, size);}
#define log_error_hexdump(name, buf, size)   if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                              custom_hexdumplog(name,LOG_LEVEL_ERROR, LOG_PREFIX_ERROR, buf, size);}
#define log_assert_hexdump(name, buf, size)  if (0 == LOG_HARD_DECLARE_DISABLE) {\
                                              custom_hexdumplog(name,LOG_LEVEL_ASSERT, LOG_PREFIX_DUMP, buf, size);}
#define log_print          __log_printf
#define log_buf            log_buf//unsupport

#else

//#define LOG_HARD_DECLARE_DISABLE 1
#if defined(__CC_ARM) || defined(__CLANG_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
#define LOG_DECLARE(name)


#define log_error(M, ...) printf(M"\r\n", ##__VA_ARGS__)
#define log_info(M, ...) printf(M"\r\n", ##__VA_ARGS__)
#define log_warn(M, ...) printf(M"\r\n", ##__VA_ARGS__)

#define log_debug_hexdump(name, buf, size) do {\
				printf("%s:\r\n",\
				name);\
				for(int i =0; i < size; i++) \
					printf("%02x ", buf[i]);\
				printf("\r\n"); \
}while(0)

#define log_info_hexdump(name, buf, size)do {\
				printf("%s:\r\n",\
				name);\
				for(int i =0; i < size; i++) \
					printf("%02x ", buf[i]);\
				printf("\r\n"); \
}while(0)

#define log_warn_hexdump(name, buf, size)do {\
				printf("%s:\r\n",\
				name);\
				for(int i =0; i < size; i++) \
					printf("%02x ", buf[i]);\
				printf("\r\n"); \
}while(0)
#else
#define LOG_DECLARE(name)

#define log_debug(M, ...)
#define log_info(M, ...)
#define log_warn(M, ...)
#define log_error(M, ...)
#define log_debug_raw(M, ...)
#define log_info_raw(M, ...)
#define log_warn_raw(M, ...)
#define log_error_raw(M, ...)
#define log_assert(M, ...)

#define log_debug_user(name, M, ...)
#define log_info_user(name, M, ...)
#define log_warn_user(name, M, ...)
#define log_error_user(name, M, ...)
#define log_debug_user_raw(name, M, ...)
#define log_info_user_raw(name, M, ...)
#define log_warn_user_raw(name, M, ...)
#define log_error_user_raw(name, M, ...)
#define log_assert_user(name, M, ...)

#define log_debug_hexdump(name, buf, size)
#define log_info_hexdump(name, buf, size)
#define log_warn_hexdump(name, buf, size)
#define log_error_hexdump(name, buf, size)
#define log_assert_hexdump(name, buf, size)

#define log_print(...)
#define log_buf(...)
#endif

#endif

void log_init(void);

void log_hexdump_out(const char *name, uint8_t width, uint8_t *buf, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif